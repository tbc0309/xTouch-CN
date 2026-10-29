"""Reject stale embedded portal resources before compiling firmware."""
from pathlib import Path
import argparse
import hashlib

ROOT = Path(__file__).resolve().parents[1]


def source_digest(limited=False):
    paths = list((ROOT / 'portal').glob('*')) + [
        ROOT / 'scripts/embed_portal.py', ROOT / 'scripts/generate_locales.py',
        ROOT / 'scripts/minify-portal.cjs', ROOT / 'resources/languages.json',
        ROOT / 'resources/ui-locales.json', ROOT / 'resources/portal-locales.json',
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
    print('PASS: embedded portal matches current sources')
