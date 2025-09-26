import JSZip from "jszip";
import createModule from "../output/fun.js";

function getBaseFileName(name) {
    return name.replace(/\.[^/.]+$/, "") || "output";
};

function downloadFile(data, filename, mimeType) {
    const blob = new Blob([data], {type: mimeType});
    const url = URL.createObjectURL(blob);
    const a = document.createElement("a");
    a.href = url;
    a.download = filename;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
};

let wasmModule = null;

/**
 *
 * @description init
 * @return {Promise<void>}
 *
 */
export async function initModule() {
    wasmModule = await createModule();
}

/**
 *
 * @description ico
 * @param file{File}
 *
 */
export async function getIco(file) {
    if (!wasmModule) {
        throw new Error("WASM模块尚未初始化，请先调用initModule");
    }

    const buffer = new Uint8Array(await file.arrayBuffer());
    wasmModule.FS_writeFile("input.png", buffer);
    const result = wasmModule.ccall(
        "wasm_convert_to_ico",
        "number",
        ["string", "string"],
        ["input.png", "output.ico"],
    );
    if (result === 0) {
        const data = wasmModule.FS_readFile("output.ico");
        downloadFile(data, `${getBaseFileName(file.name)}.icns`, "image/icns");
    } else {
        console.error("ICO生成失败");
    }
}

const ImageSizes = [
    16, 24, 30, 32, 40, 48, 64, 72, 80, 96, 128, 256, 512, 1024,
];

/**
 *
 * @description png
 * @param file {File}
 *
 */
export async function getPngs(file) {
    if (!wasmModule) {
        throw new Error("WASM模块尚未初始化，请先调用initModule");
    }

    const buffer = new Uint8Array(await file.arrayBuffer());
    wasmModule.FS_writeFile("input.png", buffer);
    const result = wasmModule.ccall(
        "wasm_convert_to_pngs",
        "number",
        ["string"],
        ["input.png"],
    );
    if (result === 0) {
        const zip = new JSZip();
        let pngCount = 0;

        for (const size of ImageSizes) {
            const pngData = wasmModule.FS_readFile(`/${size}.png`);
            zip.file(`${size}.png`, pngData);
            pngCount++;
        }

        if (pngCount > 0) {
            const data = await zip.generateAsync({type: "blob"});
            downloadFile(data, `${getBaseFileName(file.name)}_pngs.zip`, "application/zip");
        } else {
            console.error("PNG生成失败，未生成任何文件");
        }
    } else {
        console.error("PNG转换函数调用失败");

    }
}

/**
 *
 * @description icns
 * @param file{File}
 *
 */

export async function getIcns(file) {
    if (!wasmModule) {
        throw new Error("WASM模块尚未初始化，请先调用initModule");
    }

    const buffer = new Uint8Array(await file.arrayBuffer());
    wasmModule.FS_writeFile("input.png", buffer);
    const result = wasmModule.ccall(
        "wasm_convert_to_icns",
        "number",
        ["string", "string"],
        ["input.png", "output.icns"],
    );
    if (result === 0) {
        const data = wasmModule.FS_readFile("output.icns");
        downloadFile(data, `${getBaseFileName(file.name)}.ico`, "image/icns");
    } else {
        console.error("ICNS生成失败");
    }
}
