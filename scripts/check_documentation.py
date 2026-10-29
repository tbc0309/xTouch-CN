"""Check user-guide links, anchors, language peers and the README image policy."""
from pathlib import Path
import argparse
import fnmatch
import json
import re
import shutil
import subprocess
from urllib.parse import unquote, urlsplit

ROOT = Path(__file__).resolve().parents[1]


def publication_errors(text, phrases):
    """Public guides describe behavior; private bench progress stays local."""
    return [f'internal wording or bench detail: {phrase}' for phrase in phrases
            if phrase.casefold() in text.casefold()]


def anchors(text):
    found = set(re.findall(r'<a\s+id=["\']([^"\']+)', text))
    counts = {}
    for heading in re.findall(r'^#{1,6}\s+(.+)$', text, re.M):
        slug = re.sub(r'[^\w\- ]', '', heading.lower()).replace(' ', '-')
        count = counts.get(slug, 0)
        counts[slug] = count + 1
        found.add(slug if not count else f'{slug}-{count}')
    return found


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.parse_args()
    files = [ROOT / 'README.md', ROOT / 'README.en.md', *sorted(ROOT.glob('LICENSE*.md')),
             *sorted((ROOT / 'docs').glob('*.md')),
             *sorted((ROOT / 'boards').glob('*.md')),
             *sorted((ROOT / 'boards').glob('*/README*.md')),
             *sorted((ROOT / '.github/ISSUE_TEMPLATE').glob('*.md'))]
    errors = []
    policy = json.loads((ROOT / 'resources/documentation-policy.json').read_text('utf-8'))
    git_available = shutil.which('git') and (ROOT / '.git').exists()

    def ignored_targets(targets):
        if not git_available or not targets:
            return set()
        result = subprocess.run(
            ['git', '-c', f'safe.directory={ROOT.as_posix()}', '-C', str(ROOT),
             'check-ignore', '--stdin', '-z'],
            input='\0'.join(sorted(targets)) + '\0',
            text=True, capture_output=True, encoding='utf-8', check=False,
        )
        if result.returncode not in (0, 1):
            errors.append('could not check documentation references against Git ignore rules')
            return set()
        return set(filter(None, result.stdout.split('\0')))

    # Private worktree notes are not published documentation or CI inputs.
    private_guides = ignored_targets({p.relative_to(ROOT).as_posix() for p in files})
    files = [p for p in files if p.relative_to(ROOT).as_posix() not in private_guides]
    checked = 0
    local_targets = set()
    for path in files:
        text = path.read_text(encoding='utf-8')
        errors.extend(f'{path.relative_to(ROOT)}: {message}'
                      for message in publication_errors(text, policy['internalPhrases']))
        if '\ufffd' in text:
            errors.append(f'{path.relative_to(ROOT)}: replacement character')
        plain = re.sub(r'```.*?```', '', text, flags=re.S)
        targets = re.findall(r'!?\[[^\]]*\]\(([^)]+)\)', plain)
        targets += re.findall(r'<img\b[^>]*\bsrc=["\']([^"\']+)', plain, re.I)
        for target in targets:
            target = target.strip().strip('<>')
            parsed = urlsplit(target)
            if parsed.scheme or parsed.netloc:
                continue
            checked += 1
            dest = (path.parent / unquote(parsed.path)).resolve() if parsed.path else path
            if not dest.is_relative_to(ROOT):
                errors.append(f'{path.relative_to(ROOT)}: reference outside repository {target}')
                continue
            local_targets.add(dest.relative_to(ROOT).as_posix())
            if not dest.exists():
                errors.append(f'{path.relative_to(ROOT)}: missing {target}')
            elif parsed.fragment and dest.is_file() and dest.suffix == '.md':
                if unquote(parsed.fragment) not in anchors(dest.read_text(encoding='utf-8')):
                    errors.append(f'{path.relative_to(ROOT)}: missing anchor {target}')
        if path.name in ('README.md', 'README.en.md') and path.parent == ROOT:
            images = re.findall(r'!\[[^\]]*\]\(([^)]+)\)', plain)
            image_base = 'docs/images/en/boards/' if path.name == 'README.en.md' else 'docs/images/boards/'
            if images != [image_base+'p4-'+name+'.png' for name in ('home','printing','printer-picker','temperature','filament','camera')]:
                errors.append(f'{path.name}: expected six separate P4 screenshots, got {images}')
    for stem in ['INSTALLATION', 'USER-GUIDE', 'PRINTERS', 'THERMOMETERS', 'HARDWARE',
                 'MAINTENANCE', 'DEVELOPMENT', 'SCREENSHOTS']:
        for suffix in ('', '.en'):
            if not (ROOT / 'docs' / f'{stem}{suffix}.md').exists():
                errors.append(f'missing language peer {stem}{suffix}.md')
    # The portal keeps these stable entry points even when guide prose is rewritten.
    for suffix in ('', '.en'):
        path = ROOT / 'docs' / f'INSTALLATION{suffix}.md'
        if 'token' not in anchors(path.read_text(encoding='utf-8')):
            errors.append(f'{path.name}: portal Token tutorial anchor missing')
        path = ROOT / 'docs' / f'THERMOMETERS{suffix}.md'
        if 'ble-key' not in anchors(path.read_text(encoding='utf-8')):
            errors.append(f'{path.name}: portal BLE Key tutorial anchor missing')
    portal = (ROOT / 'portal/account-token.js').read_text(encoding='utf-8')
    if not all(name in portal for name in ('INSTALLATION.md', 'INSTALLATION.en.md', '#token')):
        errors.append('portal/account-token.js: bilingual canonical Token guide links missing')
    # A local file can exist while being excluded from a clean checkout.
    for target in sorted(ignored_targets(local_targets)):
        errors.append(f'local reference excluded by Git ignore rules: {target}')
    if git_available:
        tracked = subprocess.run(
            ['git', '-c', f'safe.directory={ROOT.as_posix()}', '-C', str(ROOT), 'ls-files', '-z'],
            capture_output=True, text=True, encoding='utf-8', check=True,
        )
        for name in filter(None, tracked.stdout.split('\0')):
            if any(fnmatch.fnmatchcase(name, pattern) for pattern in policy['localOnly']):
                errors.append('local-only material in Git index: ' + name)
    for error in errors:
        print('FAIL', error)
    print(f'{"FAIL" if errors else "PASS"} {len(files)} documents; {checked} local references; README and Token link policy')
    return bool(errors)


if __name__ == '__main__':
    raise SystemExit(main())
