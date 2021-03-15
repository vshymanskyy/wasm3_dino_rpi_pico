
# WebAssembly Dino game

Dino game for Raspberry Pi Pico with Pimoroni Pico Display pack

Prebuilt binary (`wasm3_dino.uf2`) is [here](https://github.com/vshymanskyy/wasm3_dino_rpi_pico/releases).

## Controls

```log
X - jump
Y - duck
```

## Build

You'll need the [pimoroni-pico](https://github.com/pimoroni/pimoroni-pico) library.

```sh
# Clone this repo to pico home directory
cd $PICO_SDK_PATH/..
git clone https://github.com/vshymanskyy/wasm3_dino_rpi_pico.git
cd wasm3_dino_rpi_pico

# Clone Wasm3 engine
git clone --depth=1 https://github.com/wasm3/wasm3.git ./wasm3

# Generate wasm from wat
export PATH=/opt/wasp/build/src/tools:$PATH
wasp wat2wasm --enable-numeric-values -o dino.wasm dino.wat
wasm-opt -Oz -o dino.wasm dino.wasm
xxd -iC dino.wasm > dino.wasm.h

# Build 
mkdir build
cd build
cmake ..
make -j8

# Install: copy wasm3_dino.uf2 to your RPi Pico
```
