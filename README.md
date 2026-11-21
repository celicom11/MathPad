# MathPad

A lightweight LaTeX math document editor/viewer for Windows, built with WTL/ATL,
Skia raster rendering, and FreeType fonts.
LaTeX engine is provided by cross-platform MathBox library (https://github.com/celicom11/MathBox).

## Features
- Rich text LaTeX editor with error highlighting
- Page-layout math renderer (MathBoxLib)
- Per-document settings via `%% key = value` header comments
- Direct PDF export through Skia

## Requirements
- Windows 10/11 x64
- Visual Studio 2022, v143 toolset, C++20
- Git LFS to download the bundled static Skia libraries

## Building
After cloning, run `git lfs install` and `git lfs pull` to obtain the libraries.
Open `MathPad.sln` in Visual Studio 2022 and build **x64 Release** or **x64 Debug**.
The self-contained Skia headers and libraries are under `Skia/`; no installed
FreeType or external Skia project is needed. See [Skia package details](Skia/README.md).
The executable and all runtime assets are output to `Bin\`.

## Runtime files (pre-placed in `Bin\`)
- `MathBoxLib.dll` — math renderer, local v2.0.0-beta (ABI 2)
- `LatinModernFonts\` — required fonts
- `Macros.mth` — LaTeX macro definitions
- `MathPad.cfg` — default configuration
- `TestDoc.mth` — sample document

## Document format
`.mth` files are plain UTF-8 LaTeX text with a LaTeX-compatible subset supported by MathBoxLib.

### Per-document header directives
Optional `%% key = value` lines at the very top of the file override the corresponding
`MathPad.cfg` settings for that document only. Parsing stops at the first non-`%%` line.
Inline comments after the value are supported with `%`.

| Directive | Overrides cfg key | Accepted values |
|---|---|---|
| `paper` | `PageSizePts` | Named: `A4` `A5` `Letter` `Legal`, or `width,height` in pts |
| `margins` | `PageMarginsPts` | `hMargin,vMargin` in typographic points |
| `fontsize` | `MB_FontSizePts` | Number with optional unit: `pt` (default), `in`, `mm`. Range 8–72 |
| `pagecolor` | `MB_ColorBkg` | `#RRGGBB` or color name (see below) |
| `textcolor` | `MB_ColorText` | `#RRGGBB` or color name (see below) |

Supported color names: `black` `white` `red` `green` `blue` `cyan` `magenta` `yellow`
`gray` `darkgray` `lightgray` `brown` `lime` `olive` `orange` `pink` `purple` `teal` `violet`

Example header:
%% paper = A4 %% margins = 72,54        % 1 inch horizontal, 0.75 inch vertical 
%% fontsize = 11.0pt 
%% textcolor = #000060    % dark navy


## MathPad.cfg reference
The configuration file uses a simple `key = value` format. Lines starting with `;` are comments.

### `[EditView]` — LaTeX source editor
| Key | Default | Description |
|---|---|---|
| `EditColorBkg` | `0xFFFFFF` | Editor background color (RGB hex) |
| `EditColorText` | `0x0` | Editor text color (RGB hex) |
| `EditFontName` | `Tahoma` | Editor font face name |
| `EditFontSize` | `12` | Editor font size in points |
| `EditWordWrap` | `1` | Word wrap: `1` = on, `0` = off |
| `EditSyntaxHighlight` | `0` | Syntax highlighting: `1` = on, `0` = off (reserved) |
| `EditOpenLastDoc` | `1` | Reopen last document on startup: `1` = yes, `0` = no |

### `[MathBoxView]` — rendered page view
| Key | Default | Description |
|---|---|---|
| `ColorBkg` | `0x3E00` | View background color surrounding pages (RGB hex) |
| `PageSizePts` | `595,842` | Page size in typographic points: `width,height` (A4 default) |
| `PageMarginsPts` | `72,72` | Page margins in points: `horizontal,vertical` |
| `PageVertGapPts` | `10` | Vertical gap between pages in points |

### `[MathBox]` — renderer settings
| Key | Default | Description |
|---|---|---|
| `MB_Macros` | `Macros.mth` | Path to LaTeX macro definitions file (relative to exe) |
| `MB_FontSizePts` | `10.0` | Base math font size in points |
| `MB_ColorBkg` | `0xFFFFFF` | Page/math background color (RGB hex) |
| `MB_ColorText` | `0x0` | Math text color (RGB hex) |
| `Utf8Font` | `Arial` | Installed Unicode text family; regular/bold/italic/bold-italic slots 14–17 |
| `Utf8FontScale` | `0.9` | Inner scale applied to Unicode metrics and drawing; range 0.1–2.0 |
| `SymbolFont` | `Segoe UI Symbol` | Installed monochrome symbol family, slot 18 |
| `SymbolFontScale` | `0.8` | Inner scale applied to symbol metrics and drawing; range 0.1–2.0 |

### `[MRU List]`
Contains recently used files.

## Limitations
- UTF-8 input is supported; complex-script shaping, bidirectional layout and color emoji are not supported
- Windows 10/11 x64 only
- No installer — xcopy deployment

## Local MathBox v2 integration

The ABI-2 header and Release DLL/PDB come from the local package
`D:/Dev/Projects/MyCode/MathBoxDemo/Releases/MathBox-v2.0.0-beta`.
This integration is local and unpublished pending MathPad and FEML testing.
Both MathPad configurations use the packaged Release DLL; the opaque C ABI
keeps DLL allocation and destruction together. No MathBox rebuild is required.
The host checks exact ABI major 2, sufficient API-table size and required entries.
Font and renderer contexts borrow per-host resources owned by the application
frame; items and engines are released before those resources and the DLL.

The four font settings above can be edited on the MathBox page of the Settings
dialog and are saved in the `;MathBox` section. Scales must be finite numbers
from 0.1 through 2.0, inclusive. Font changes take effect after restarting MathPad.
An empty font family disables that optional group. Missing
configured families or invalid scales cause startup to report an error.
Editor load/save and parser input use UTF-8. Parser byte positions are mapped
to RichEdit UTF-16 positions, including CRLF and supplementary characters.
The standard macro updates include `tabcolsep`, multiline `makecell` and logos.
The original Latin Modern font order and font files are retained.

Skia retains the packaged FreeType backend. Installed font files are located
through the Windows machine/user font registry and their family/style names
are verified through FreeType. Family names should match the installed font
registry display names (including spaces). Available faces are chosen by
slant, width and weight; absent styles use the closest real face. No synthetic
styles, DirectWrite dependency or Skia rebuild is introduced. Slots 0–13 keep
the frozen metric path; optional faces normalize their metrics to 1000 units/em.

Local validation (2026-11-21): Debug and Release x64 builds pass. The focused
`Tests/MathBoxV2Check.cpp` checks pass in both configurations: ABI-table rejection,
real ABI-1 DLL rejection/unloading, UTF-8 positions, all 19 font slots and scales,
two-host context isolation, real raster drawing, Unicode/symbol/table parsing,
legacy samples and repeated cleanup. Run `Tools/CheckMathBoxV2.ps1 -Skia
-Configuration Release` (or `Debug`) from an x64 Developer PowerShell.
These test tools remain local-only.

The existing `Tools/CheckSkia.ps1 -Configuration Both` checks also pass: all
15,401 frozen glyph metrics and stored outline-run bounds have zero differences;
font reload, raster/zoom/resize, PDF export/recovery and three Debug shutdown
cycles pass (zero retained normal-heap bytes/blocks). The final Release executable
imports no DirectWrite, D2D, D3D or DXGI DLLs. Interactive rendering comparisons,
editor file round trips and visible error highlighting still need manual checks.

The MathBox settings controls, installed editor-font family list, and immediate
editor font updates (preserving undo history and the modified flag) match
master commit `5335727`.
