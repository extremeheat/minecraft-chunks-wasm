Simple minecraft world chunk serializer/deserializer for WebAssembly. Does not require any dependencies (or even a standard library). 

Build with:

clang -O3 -std=c++20 --target=wasm32 -nostdlib -Wl,--no-entry -Wl,--export-all -o minecraft-chunks.wasm main.cpp

This project uses the new ESM loader for WebAssembly. A light-weight JavaScript wrapper is provided that is API compatible with prismarine-chunk.
