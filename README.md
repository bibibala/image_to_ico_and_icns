# Image to ICO & ICNS Converter

A WebAssembly-powered tool to convert images to **ICO** (Windows icons) and **ICNS** (macOS icons) formats, plus multiple PNG sizes. Built with C and compiled via Emscripten.

## Features

- **ICO Generation**: 14 standard sizes (16×16 to 1024×1024)
- **ICNS Generation**: 13 standard sizes (16×16 to 1024×1024, including @2x variants)
- **Multi-size PNG Export**: All standard icon sizes as individual PNG files
- **Batch Conversion**: ICO + ICNS + PNGs in one call
- **SVG → Image**: Rasterize SVG (via NanoSVG) to PNG / JPG / BMP / TGA with custom width/height
- **Image Compression**: Re-encode with JPEG quality / PNG compression level, plus optional max-size downscaling
- **Format Conversion**: Convert between PNG / JPG / BMP / TGA / HDR / ICO / ICNS (input accepts all stb_image formats plus SVG)
- **Zero Dependencies**: Uses `stb_image`, `stb_image_resize`, `stb_image_write`, `nanosvg`
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
| `wasm_svg_to_image(input, output, width, height, quality)` | `string, string, number, number, number` | `number` | SVG → PNG/JPG/BMP/TGA (width/height ≤ 0 = auto) |
| `wasm_convert_format(input, output, quality)` | `string, string, number` | `number` | Convert between formats, output format from extension |
| `wasm_compress_image(input, output, quality, max_size)` | `string, string, number, number` | `number` | Compress / re-encode; `max_size` = longest side (0 = keep) |

### Parameters

- `quality` (1–100): JPEG quality for `.jpg`; PNG/BMP/TGA compression strength (higher = smaller) for others; PNG compression level inside .ico/.icns.
- `max_size`: maximum longest edge while keeping aspect ratio (0 = original size).

### Return codes

| Code | Meaning |
|------|---------|
| `0` | Success |
| `-1` | Failed to read input image |
| `-2` | Failed to open output file |
| `-3` | Unsupported output format |
| `-4` | Out of memory / resize failed |
| `-5` | Failed to write output (incl. SVG parse failure) |

> Note: SVG rasterization uses [NanoSVG](https://github.com/memononen/nanosvg), which does not support all SVG features (e.g. filters, `<text>` with external fonts).

## GitHub Actions

Automated builds on push to `main`. Releases created with versioned tags.

## License

MIT - see [LICENSE](LICENSE)

## Credits

- [stb_image](https://github.com/nothings/stb) - Image loading/resizing/writing
- [NanoSVG](https://github.com/memononen/nanosvg) - SVG parsing/rasterization
- [Emscripten](https://emscripten.org/) - C to WebAssembly compilation
