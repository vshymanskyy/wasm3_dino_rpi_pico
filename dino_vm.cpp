#include <string.h>
#include <cstdlib>

#include "pico_display.hpp"
#include "pico/stdlib.h"

#include <wasm3.h>
#include <m3_api_defs.h>

/*
 * Dino game by by Ben Smith (binji)
 *   https://github.com/binji/raw-wasm/tree/master/dino
 * To build:
 *   export PATH=/opt/wasp/build/src/tools:$PATH
 *   wasp wat2wasm --enable-numeric-values -o dino.wasm dino.wat
 *   xxd -iC dino.wasm > dino.wasm.h
 */
#include "dino.wasm.h"

using namespace pimoroni;


#define FATAL(func, msg) { printf("Fatal %s: %s\n", func, msg); while(1) { sleep_ms(100); } }

// The Math.random() function returns a floating-point,
// pseudo-random number in the range 0 to less than 1
m3ApiRawFunction(Math_random)
{
    m3ApiReturnType (float)

    float r = (float)rand()/(float)(RAND_MAX);

    m3ApiReturn(r);
}

// Memcpy is generic, and much faster in native code
m3ApiRawFunction(Dino_memcpy)
{
    m3ApiGetArgMem  (uint8_t *, dst)
    m3ApiGetArgMem  (uint8_t *, src)
    m3ApiGetArgMem  (uint8_t *, dstend)

    do {
        *dst++ = *src++;
    } while (dst < dstend);

    m3ApiSuccess();
}

IM3Environment  env;
IM3Runtime      runtime;
IM3Module       module;
IM3Function     func_run;
uint8_t*        mem;

void load_wasm()
{
    M3Result result = m3Err_none;
    
    if (!env) {
      env = m3_NewEnvironment ();
      if (!env) FATAL("NewEnvironment", "failed");
    }

    m3_FreeRuntime(runtime);

    runtime = m3_NewRuntime (env, 1024, NULL);
    if (!runtime) FATAL("NewRuntime", "failed");

    result = m3_ParseModule (env, &module, dino_wasm, sizeof(dino_wasm));
    if (result) FATAL("ParseModule", result);

    result = m3_LoadModule (runtime, module);
    if (result) FATAL("LoadModule", result);

    m3_LinkRawFunction (module, "Math",   "random",     "f()",      &Math_random);
    m3_LinkRawFunction (module, "Dino",   "memcpy",     "v(iii)",   &Dino_memcpy);

    mem = m3_GetMemory (runtime, NULL, 0);
    if (!mem) FATAL("GetMemory", "failed");

    result = m3_FindFunction (&func_run, runtime, "run");
    if (result) FATAL("FindFunction", result);
}


uint16_t buffer[PicoDisplay::WIDTH * PicoDisplay::HEIGHT];
PicoDisplay pico_display(buffer);

void drawImage(uint16_t *src, int dst_x, int dst_y, int src_w, int src_h)
{
    uint16_t* dst = buffer;
    dst += dst_y * PicoDisplay::WIDTH + dst_x;
    int line_inc = PicoDisplay::WIDTH - src_w;
    for (int y = 0; y < src_h; y++) {
        for (int x = 0; x < src_w; x++) {
            *dst++ = __builtin_bswap16(*src++);
        }
        dst += line_inc;
    }
}

int main()
{
    stdio_init_all();
    
    printf("\nWasm3 v" M3_VERSION " (" M3_ARCH "), build " __DATE__ " " __TIME__ "\n");

    pico_display.init();
    pico_display.set_backlight(192);

    pico_display.set_pen(255, 255, 255);
    pico_display.clear();

    pico_display.set_pen(20, 20, 20);
    pico_display.text("Wasm3 " M3_VERSION "  (" M3_ARCH " M0)",     Point(5, 5+7*2*0), 230);
    pico_display.text("Dino game  by binji",                        Point(5, 5+7*2*1), 230);

    pico_display.update();

    M3Result result;
    
    load_wasm();
    
    uint64_t last_fps_print = 0;

    while (true) {
      const uint64_t framestart = time_us_64();

      // Process inputs
      uint32_t* input = (uint32_t*)(mem + 0x0000);
      *input = 0;
      if (pico_display.is_pressed(pico_display.X)) { // Up
        *input |= 0x1;
      }
      if (pico_display.is_pressed(pico_display.Y)) { // Down
        *input |= 0x2;
      }

      // Render frame
      result = m3_CallV (func_run);
      if (result) break;

      drawImage((uint16_t*)(mem+0x5000), 0, 135-10-75, 240, 75);

      pico_display.update();

      const uint64_t frametime = time_us_64() - framestart;
      const uint32_t target_frametime = 1000000/40;
      if (target_frametime > frametime) {
        sleep_us(target_frametime - frametime);
      }
      if (framestart - last_fps_print > 1000000) {
        printf("FPS: %3.2f\n", 1000000.0f/frametime);
        last_fps_print = framestart;
      }
    }

    if (result != m3Err_none) {
        M3ErrorInfo info;
        m3_GetErrorInfo (runtime, &info);
        printf("Error: %s (%s) ", result, info.message);
        if (info.file && strlen(info.file) && info.line) {
            printf("at %s:%d", info.file, info.line);
        }
        printf("\n");
    }

    return 0;
}
