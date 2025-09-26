# 图片转图标工具

把图片转换成ICO（Windows图标）和ICNS（macOS图标）格式。

## 功能
- 图片 → ICO格式
- 图片 → ICNS格式  
- 自动生成多种尺寸（16-1024像素）

## 已封装好可直接使用，包含js/ts
### test/index.js  test/index.ts

```javascript
// 初始化
initModule()

// 下载ico
getIco()

// 下载png
getPngs()

// 下载icns
getIcns()

// 获取所有
getImageBoth()
```

## 主要方法

```javascript
// 写入图片到虚拟文件系统
Module.FS_writeFile("/input.png", imageData);

// 转换ICO
Module.ccall("wasm_convert_to_ico", "number", ["string", "string"], ["/input.png", "/output.ico"]);

// 转换ICNS  
Module.ccall("wasm_convert_to_icns", "number", ["string", "string"], ["/input.png", "/output.icns"]);

// 同时转换ICO+ICNS+多尺寸PNG
Module.ccall("wasm_convert_to_both", "number", ["string", "string"], ["/input.png", "/output"]);

// 读取结果
const icoData = Module.FS_readFile("/output.ico");
```

## 快速测试
打开 `test/index.html` 即可测试。
