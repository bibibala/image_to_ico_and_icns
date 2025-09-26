import JSZip from "jszip";
import createModule from "../output/fun.js";

// 定义WASM模块的类型
interface WasmModule {
    FS_writeFile: (path: string, data: Uint8Array) => void;
    FS_readFile: (path: string) => Uint8Array;
    ccall: (
        funcName: string,
        returnType: string,
        argTypes: string[],
        args: any[]
    ) => number;
}


function getBaseFileName(name: string) {
    return name.replace(/\.[^/.]+$/, "") || "output";
}

const downloadFile = (data: Uint8Array | Blob, filename: string, mimeType: string) => {
    const blob = data instanceof Blob ? data : new Blob([data], {type: mimeType});
    const url = URL.createObjectURL(blob);
    const a = document.createElement("a");
    a.href = url;
    a.download = filename;
    document.body.appendChild(a);
    a.click();
    document.body.removeChild(a);
    URL.revokeObjectURL(url);
};

let wasmModule: WasmModule | null = null;

/**
 * 初始化WASM模块
 * @return {Promise<void>}
 */
export async function initModule(): Promise<void> {
    wasmModule = await createModule();
}

/**
 *
 * @description ico
 * @param file {File}
 *
 */
export async function getIco(file: File): Promise<void> {
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
        downloadFile(data, `${getBaseFileName(file.name)}.ico`, "image/x-icon");
    } else {
        console.error("ICO生成失败");
    }
}

const ImageSizes: number[] = [
    16, 24, 30, 32, 40, 48, 64, 72, 80, 96, 128, 256, 512, 1024,
];

/**
 *
 * @description png
 * @param file {File}
 *
 */
export async function getPngs(file: File): Promise<void> {
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
            try {
                const pngData = wasmModule.FS_readFile(`/${size}.png`);
                zip.file(`${size}.png`, pngData);
                pngCount++;
            } catch (error) {
                console.warn(`无法读取尺寸为${size}的PNG文件:`, error);
            }
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
 * @param file {File}
 *
 */
export async function getIcns(file: File): Promise<void> {
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
        downloadFile(data, `${getBaseFileName(file.name)}.icns`, "image/icns");
    } else {
        console.error("ICNS生成失败");
    }
}


/**
 *
 * @description ico，png，icns
 * @param file
 */
export async function getImageBoth(file): Promise<void> {
    const buffer = new Uint8Array(await file.arrayBuffer());
    wasmModule.FS_writeFile("input.png", buffer);


    const result = wasmModule.ccall(
        "wasm_convert_to_both",
        "number",
        ["string", "string"],
        ["input.png", "output_both"],
    );
    if (result === 0) {
        // 创建ZIP文件包含所有输出
        const zip = new JSZip();
        let fileCount = 0;

        // 添加ICNS文件
        try {
            const icnsData =
                wasmModule.FS_readFile("/output_both.icns");
            zip.file(`${getBaseFileName(file.name)}.icns`, icnsData);
            fileCount++;
        } catch (e) {
            console.error(e)
        }

        // 添加ICO文件
        try {
            const icoData =
                wasmModule.FS_readFile("/output_both.ico");
            zip.file(`${getBaseFileName(file.name)}.ico`, icoData);
            fileCount++;
        } catch (e) {
            console.error(e)
        }

        for (const size of ImageSizes) {
            try {
                const pngData = wasmModule.FS_readFile(
                    `/${size}.png`,
                );
                zip.file(`${size}.png`, pngData);
                fileCount++;
            } catch (e) {
                console.error(e)
            }
        }

        if (fileCount > 0) {
            const zipBlob = await zip.generateAsync({type: "blob"});
            downloadFile(
                zipBlob,
                `${getBaseFileName(file.name)}_all_formats.zip`,
                "application/zip",
            );
        } else {
            console.error("生成失败")
        }
    }
}
