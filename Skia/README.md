This is MathPad's self-contained x64 raster/PDF/FreeType Skia package. Application
builds use only this directory; no external Skia project or installed FreeType
library is required.

Pinned source revisions:

- Skia: e8ff7a4187ecd9c98df14e7a617dd4ac5d74295a
- FreeType: 264b5fbf5b912b39f98d038bf75d39be0a73f21b
- libpng: d5515b5b8be3901aac04e5bd8bd5c89f287bcd33
- zlib: 646b7f569718921d7d4b5b8e22572ff6c76f2596

Compiler: Visual Studio 2022 v143, MSVC directory 14.44.35207, compiler
19.44.35229.0; Windows SDK 10.0.26100.0; C++20. GN revision:
b2afae122eeb6ce09c52d63f67dc53fc517dbdc8.

`Lib/skia.lib` uses /MD and size optimization. `LibD/skia.lib` uses /MDd and
debug information. Both are static libraries containing their required FreeType,
libpng, zlib, and skcms objects. FreeType's upstream target requires libpng/zlib
for its font-image support; MathPad does not need separate dependency DLLs.

CPU raster drawing and a custom empty FreeType font manager are enabled.
PDF output is enabled with zlib compression. HarfBuzz font subsetting remains
disabled; Skia can embed full TrueType fonts and emits used CFF glyphs as
Type 3 PDF fonts with Unicode maps.
GPU backends, Windows font backends, SVG, animation, shaping, ICU, and
optional Skia image codecs are disabled. The library's built-in BMP/WBMP and
Windows image-generator sources remain where required by the upstream core
target. This is a MathPad profile; it does not restrict a future FEML build.

Exact GN arguments, library hashes, and the transitive header list are in `Build/`.
Upstream copyright and license notices are retained in headers and `Licenses/`.
Only application/test header dependencies are packaged, including private
transitive dependencies. Do not substitute headers from another Skia revision.

Only the Release static library (`Lib/skia.lib`) is stored with Git LFS.
The Debug library (`LibD/skia.lib`) is deliberately ignored and must be rebuilt
locally before building MathPad Debug. Untracking it does not remove older LFS
objects from Git history. After cloning with Git LFS
installed, run `git lfs pull`; a library file containing a short text pointer
instead of a binary archive has not been downloaded yet.

To rebuild, use a separate checkout of Skia at the pinned revision above.
Acquire FreeType, libpng and zlib at the listed revisions in
`third_party/externals/freetype`, `third_party/externals/libpng`, and
`third_party/externals/zlib` (upstream repositories are recorded in Skia's `DEPS`).
Run `python bin/fetch-gn` in that checkout. Python, GN, Ninja and Visual Studio
2022 C++ tools are required only to rebuild the package.

From an x64 Developer PowerShell at the MathPad solution directory:

```powershell
# Recreate the local Debug library after cloning:
./Skia/Build/Rebuild.ps1 -SkiaSource D:/Dev/Projects/Libs/Skia -Configuration Debug
# Rebuild both configurations when updating the package:
./Skia/Build/Rebuild.ps1 -SkiaSource D:/Dev/Projects/Libs/Skia -Configuration Both
```

The script checks the pinned source/dependency revisions, uses the checked-in
`Build/*.args.gn`, builds `out/mathpad-x64-Release` or `out/mathpad-x64-Debug`
in that external source checkout, then packages the static libraries, only the
required transitive headers, licenses and SHA-256 hashes. `Package.py` requires
Python 3.9 or later. Arguments contain this machine's compiler/SDK locations;
adjust those explicitly when changing machines. Keep C++20, x64 and `/MD` Release
versus `/MDd` Debug compatible with MathPad. No dependency source is downloaded
or changed by the rebuild script.

PDF export draws directly to a Skia document canvas. Page sizes/margins use the
same layout as preview, converted from 96 DIPs/inch to 72 PDF points/inch; screen
zoom, scrolling, preview borders and shadows are excluded. Windows supplies only
the Save dialog and checked temporary-file output. The renderer borrows each PDF
page canvas, and the view owns/releases the PDF document. Search/copy depends on
the fonts' glyph-to-Unicode mappings; some size variants and assembled symbols
have no recoverable Unicode text with the current glyph-only MathBox callbacks.
Mathematical reading order is not semantic LaTeX or tagged accessible mathematics.
PDF export does not require a printer.

Local checks verified glyph mappings, all 15,401 glyph design metrics, and 70
representative outline-run bounds against the frozen baseline. The internal test
tools remain local; rebuild and packaging scripts are distributed in `Build/`.
Validation results are recorded in `Build/manifest.json`. Rendering comparisons still require visual
checks; passing design-metric checks does not imply identical antialiasing.
The toolbar/menu command saves PDF directly; it does not submit Windows print jobs.
