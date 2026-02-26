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

### `[MRU List]`
Contains recently used files.

## Limitations
- **ASCII input only** — extended Unicode characters in the source text are not supported
- Windows 10/11 x64 only
- No installer — xcopy deployment
