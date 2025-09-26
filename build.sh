emcc fun.c -o fun.js \
  -s MODULARIZE=1 \
  -s EXPORT_ES6=1 \
  -s ENVIRONMENT='web' \
  -s ALLOW_MEMORY_GROWTH=1 \
  -s INITIAL_MEMORY=64MB \
  -s EXPORTED_FUNCTIONS="['_wasm_convert_to_both','_wasm_convert_to_ico','_wasm_convert_to_icns','_wasm_convert_to_pngs']" \
  -s EXPORTED_RUNTIME_METHODS="['ccall','cwrap','FS_writeFile','FS_readFile','FS_unlink']"