cmake_minimum_required(VERSION 3.12)

include(pico_sdk_import.cmake)

project(wasm3_dino)

pico_sdk_init()

include_directories(../pimoroni-pico/libraries/pico_display/)
link_directories(../pimoroni-pico/build/libraries/pico_graphics/ ../pimoroni-pico/build/drivers/st7789/)

file(GLOB WASM3_SRC "./wasm3/source/*.c")

add_executable(
  wasm3_dino
  ${WASM3_SRC}
  dino_vm.cpp
  ../pimoroni-pico/libraries/pico_display/pico_display.cpp
)

target_include_directories(wasm3_dino PRIVATE ./wasm3/source)

target_compile_definitions(wasm3_dino PRIVATE
    d_m3LogOutput=0
    d_m3MaxFunctionStackHeight=128
    d_m3CodePageAlignSize=1024
)

target_compile_options(wasm3_dino PRIVATE
    -fomit-frame-pointer
    -fno-stack-check
    -fno-stack-protector
)

# Pull in pico libraries that we need
target_link_libraries(wasm3_dino
    pico_stdlib
    hardware_spi
    hardware_pwm
    hardware_dma
    pico_graphics
    st7789
)

# enable usb output, disable uart output
pico_enable_stdio_usb (wasm3_dino 1)
pico_enable_stdio_uart(wasm3_dino 0)

# create map/bin/hex file etc.
pico_add_extra_outputs(wasm3_dino)

