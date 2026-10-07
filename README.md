# Image to ICO & ICNS Converter

A WebAssembly-powered tool to convert images to **ICO** (Windows icons) and **ICNS** (macOS icons) formats, plus multiple PNG sizes. Built with C and compiled via Emscripten.

## Features

- **ICO Generation**: 14 standard sizes (16×16 to 1024×1024)
- **ICNS Generation**: 13 standard sizes (16×16 to 1024×1024, including @2x variants)
- **Multi-size PNG Export**: All standard icon sizes as individual PNG files
- **Batch Conversion**: ICO + ICNS + PNGs in one call
- **Zero Dependencies**: Uses `stb_image`, `stb_image_resize`, `stb_image_write`
- **Web & Node.js Ready**: ES6 module output

## Usage

This library has been integrated into **[ts-lab](https://github.com/bibibala/ts-lab)**. See the [documentation](https://ts-lab.netlify.app/zh/api/wasm/image.html) for complete usage examples.

## Building from Source

Requires [Emscripten](https://emscripten.org/):

```bash
./build.sh
```

Output: `dist/fun.js` (ES6 module) and `dist/fun.wasm`

## API

| Function | Parameters | Returns | Description |
|----------|------------|---------|-------------|
| `wasm_convert_to_ico(input, output)` | `string, string` | `number` | Convert to ICO |
| `wasm_convert_to_icns(input, output)` | `string, string` | `number` | Convert to ICNS |
| `wasm_convert_to_both(input, prefix)` | `string, string` | `number` | Convert to ICO + ICNS + PNGs |
| `wasm_convert_to_pngs(input)` | `string` | `number` | Generate all PNG sizes |

## GitHub Actions

Automated builds on push to `main`. Releases created with versioned tags.

## License

MIT - see [LICENSE](LICENSE)

## Credits

- [stb_image](https://github.com/nothings/stb) - Image loading/resizing/writing
- [Emscripten](https://emscripten.org/) - C to WebAssembly compilation
