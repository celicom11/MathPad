This is MathPad's self-contained x64 raster/FreeType Skia package. Application
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
GPU backends, Windows font backends, PDF, SVG, animation, shaping, ICU, and
optional Skia image codecs are disabled. The library's built-in BMP/WBMP and
Windows image-generator sources remain where required by the upstream core
target. This is a MathPad profile; it does not restrict a future FEML build.

Exact GN arguments, library hashes, and the transitive header list are in `Build/`.
Upstream copyright and license notices are retained in headers and `Licenses/`.
Only application/test header dependencies are packaged, including private
transitive dependencies. Do not substitute headers from another Skia revision.

The static libraries are stored with Git LFS. After cloning with Git LFS
installed, run `git lfs pull`; a library file containing a short text pointer
instead of a binary archive has not been downloaded yet.

To rebuild from a separate checkout of the pinned source, acquire the three
dependency revisions listed above under `third_party/externals/`, fetch GN with
`python bin/fetch-gn`. Create separate output directories in the source checkout,
copy `Build/Release.args.gn` and `Build/Debug.args.gn` from this package to their
respective `args.gn` files, and run from an x64 Developer PowerShell:

```powershell
bin/gn.exe gen out/MathPadRelease --script-executable=<python.exe> --fail-on-unused-args
ninja -C out/MathPadRelease skia
bin/gn.exe gen out/MathPadDebug --script-executable=<python.exe> --fail-on-unused-args
ninja -C out/MathPadDebug skia
```

Copy each output's `skia.lib` to this package's `Lib` or `LibD` directory and
update its hash/build manifest. The source checkout is needed only for rebuilding
the package. The application uses the packaged headers/libraries. Build arguments record this
machine's compiler/SDK locations; adjust those explicitly when changing machines.

Local checks verified glyph mappings, all 15,401 glyph design metrics, and 70
representative outline-run bounds against the frozen baseline. The internal test
and packaging tools are not distributed with this repository; validation results
are recorded in `Build/manifest.json`. Rendering comparisons still require visual
checks; passing design-metric checks does not imply identical antialiasing.
Printing remains unavailable in the raster
adapter; replacing its unsupported print methods is a separate milestone.
