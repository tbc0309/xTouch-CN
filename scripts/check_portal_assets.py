"""Reject stale embedded portal resources before compiling firmware."""
from pathlib import Path
import argparse
import hashlib
import json

ROOT = Path(__file__).resolve().parents[1]


def pixel_source_bytes():
    words = json.loads((ROOT / 'resources/pixel-screen-locales.json').read_text('utf-8'))
    if any(len(row) != 8 or any(not isinstance(text, str) or not text for text in row)
           for row in words.values()):
        raise ValueError('Pixel-screen translations require eight nonempty language strings')
    prefix = ('const pixelLocaleWords=' + json.dumps(words, ensure_ascii=False, separators=(',', ':')) + ';\n').encode('utf-8')
    return prefix + (ROOT / 'portal/pixel-art.js').read_bytes() + b'\n' + (ROOT / 'portal/pixel-screen.js').read_bytes()


def source_digest(limited=False):
    paths = list((ROOT / 'portal').glob('*')) + [
        ROOT / 'scripts/embed_portal.py', ROOT / 'scripts/generate_locales.py',
        ROOT / 'scripts/minify-portal.cjs', ROOT / 'resources/languages.json',
        ROOT / 'resources/ui-locales.json', ROOT / 'resources/portal-locales.json',
        ROOT / 'resources/pixel-screen-locales.json', ROOT / 'resources/pixel-layout.json',
        ROOT / 'scripts/generate_pixel_layout.py',
        ROOT / 'resources/timezones/iana-posix.json', ROOT / 'version.json',
    ]
    digest = hashlib.sha256(b'zh-en' if limited else b'all-languages')
    for path in sorted(p for p in paths if p.is_file()):
        digest.update(path.relative_to(ROOT).as_posix().encode())
        digest.update(b'\0')
        digest.update(path.read_bytes().replace(b'\r\n', b'\n'))
        digest.update(b'\0')
    return digest.hexdigest()


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--zh-en', action='store_true')
    args = parser.parse_args()
    target = ROOT / 'src/xtouch' / ('portal_assets_zh_en.h' if args.zh_en else 'portal_assets.h')
    expected = '// Portal source SHA256: ' + source_digest(args.zh_en)
    if not target.exists() or target.open(encoding='utf-8').readline().strip() != expected:
        parser.exit(1, 'Embedded portal is stale. Run scripts/embed_portal.py' +
                    (' --zh-en' if args.zh_en else '') + ' before compiling.\n')
    pixel=ROOT/'src/features/pixel_screen/portal_assets.h'
    expected_pixel='// Pixel portal SHA256: '+hashlib.sha256(pixel_source_bytes().replace(b'\r\n',b'\n')).hexdigest()
    if not pixel.exists() or pixel.open(encoding='utf-8').readline().strip()!=expected_pixel:
        parser.exit(1,'Pixel-screen portal is stale. Run scripts/embed_portal.py before compiling.\n')
    print('PASS: embedded portal matches current sources')
