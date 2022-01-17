Simple minecraft world chunk serializer/deserializer for WebAssembly. Does not require any dependencies (or even a standard library). 

Build with:

clang++ src/main.cpp src/walloc.cpp -gdwarf-5 --target=wasm32 -std=c++20 -nostdlib -Wl,--no-entry -Wl,--export=malloc -Wl,--export=free -Wl,--import-memory -o mcw.wasm -DWEBASSEMBLY -Wl,-z,stack-size=4000000

This project uses the new ESM loader for WebAssembly. A light-weight JavaScript wrapper is provided that is API compatible with prismarine-chunk.

---

BUILDING NOTES

* Recommended stack size is 4MB, 2MB is required