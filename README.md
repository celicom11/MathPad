# MathPad

A lightweight LaTeX math document editor/viewer for Windows, built with WTL/ATL and Direct2D.

## Features
- Rich text LaTeX editor with error highlighting
- Page-layout math renderer (MathBoxLib)
- Per-document settings via `%% key = value` header comments
- Print (to PDF) support

## Requirements
- Windows 10/11 x64
- Visual Studio 2022

## Building
Open `Code\MathPad.vcxproj` in Visual Studio 2022 and build **x64 Release** or **x64 Debug**.
The executable and all runtime assets are output to `Bin\`.

## Runtime files (pre-placed in `Bin\`)
- `MathBoxLib.dll` — math renderer, v1.1-beta (ABI v1)
- `LatinModernFonts\` — required fonts
- `Macros.mth` — LaTeX macro definitions
- `MathPad.cfg` — default configuration
- `TestDoc.mth` — sample document

## Document format
`.mth` files are plain UTF-8 LaTeX text. Optional layout directives at the top: