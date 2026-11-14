"""Package the pinned raster/PDF build and its transitive headers for MathPad."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import shutil

parser = argparse.ArgumentParser()
parser.add_argument('source', type=Path)
parser.add_argument('--configuration', choices=['Release', 'Debug', 'Both'], default='Both')
args = parser.parse_args()
project = Path(__file__).resolve().parents[2]
source = args.source.resolve(strict=True)
destination = (project / 'Skia').resolve()
if destination != project / 'Skia':
    raise SystemExit('Unexpected package destination')

includes = re.compile(r'^\s*#\s*include\s*["<]([^">]+)[">]', re.M)
pending = []
for folder in ['Code', 'Tests']:
    for path in (project / folder).rglob('*'):
        if path.suffix in ['.h', '.cpp']:
            pending.extend(source / name for name in includes.findall(path.read_text(encoding='utf-8-sig'))
                           if name.startswith(('include/', 'modules/', 'src/')))
needed = set()
while pending:
    path = pending.pop().resolve()
    if path in needed:
        continue
    if not path.is_file() or not path.is_relative_to(source):
        raise SystemExit(f'Missing source header: {path}')
    needed.add(path)
    for name in includes.findall(path.read_text(encoding='utf-8', errors='replace')):
        for candidate in [source / name, path.parent / name]:
            if candidate.is_file():
                pending.append(candidate)
                break

relative = {path.relative_to(source) for path in needed}
for path in needed:
    target = destination / path.relative_to(source)
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(path, target)
removed = 0
for folder in ['include', 'modules', 'src']:
    root = destination / folder
    if root.is_dir():
        for path in root.rglob('*'):
            if path.is_file() and path.relative_to(destination) not in relative:
                if not path.resolve().is_relative_to(destination):
                    raise SystemExit(f'Refusing removal outside package: {path}')
                path.unlink()
                removed += 1
        for path in sorted(root.rglob('*'), key=lambda item: len(item.parts), reverse=True):
            if path.is_dir() and not any(path.iterdir()):
                path.rmdir()

configurations = ['Release', 'Debug'] if args.configuration == 'Both' else [args.configuration]
build = destination / 'Build'
build.mkdir(exist_ok=True)
for configuration in configurations:
    output = source / 'out' / f'mathpad-x64-{configuration}'
    target = destination / ('Lib' if configuration == 'Release' else 'LibD') / 'skia.lib'
    target.parent.mkdir(exist_ok=True)
    shutil.copy2(output / 'skia.lib', target)
    shutil.copy2(output / 'args.gn', build / f'{configuration}.args.gn')
    (build / f'{configuration}.sha256').write_text(hashlib.sha256(target.read_bytes()).hexdigest() + '\n')

licenses = destination / 'Licenses'
licenses.mkdir(exist_ok=True)
for name, path in [('Skia.txt', source / 'LICENSE'),
                   ('FreeType.txt', source / 'third_party/externals/freetype/docs/FTL.TXT'),
                   ('FreeType-GPL.txt', source / 'third_party/externals/freetype/docs/GPLv2.TXT'),
                   ('FreeType-license.txt', source / 'third_party/externals/freetype/LICENSE.TXT'),
                   ('libpng.txt', source / 'third_party/externals/libpng/LICENSE'),
                   ('zlib.txt', source / 'third_party/externals/zlib/LICENSE'),
                   ('skcms.txt', source / 'modules/skcms/LICENSE')]:
    if not path.is_file():
        raise SystemExit(f'Missing dependency license: {path}')
    shutil.copy2(path, licenses / name)
(build / 'headers.json').write_text(json.dumps(sorted(path.as_posix() for path in relative), indent=2) + '\n')
print(f'Packaged {len(relative)} transitive headers; removed {removed} unused files; configurations: {configurations}')

manifest_path = build / 'manifest.json'
manifest = json.loads(manifest_path.read_text())
manifest['features'] = ['raster', 'FreeType custom empty font manager', 'PDF with zlib compression']
manifest['headers'] = len(relative)
manifest['artifacts'] = [
    dict(configuration=configuration,
         bytes=(destination / ('Lib' if configuration == 'Release' else 'LibD') / 'skia.lib').stat().st_size,
         sha256=(build / f'{configuration}.sha256').read_text().strip(),
         tracked=(configuration == 'Release'))
    for configuration in ['Release', 'Debug']
    if (destination / ('Lib' if configuration == 'Release' else 'LibD') / 'skia.lib').is_file()
]
manifest['validation'] = {'status': 'Rebuilt; rerun local validation before publishing.'}
manifest_path.write_text(json.dumps(manifest, indent=2) + '\n')
