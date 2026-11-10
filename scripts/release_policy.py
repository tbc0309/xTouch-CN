"""Shared version, release-tag and OTA-channel identity checks."""
import json
import os
import re
import subprocess
from pathlib import Path

VERSION = re.compile(r'(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)\.(0|[1-9][0-9]*)(-test)?\Z')

def validate(version, channels, branch='', ref='', ref_name=''):
    if not isinstance(version, str) or not VERSION.fullmatch(version):
        raise ValueError('Expected a version such as 1.0.1 or 1.0.1-test')
    if channels not in (['main'], ['main', 'test']):
        raise ValueError('Unexpected OTA channels')
    prerelease = version.endswith('-test')
    if prerelease != ('test' in channels):
        raise ValueError('Test firmware needs a -test version and main/test OTA; stable needs main only')
    if (branch == 'main' or ref == 'refs/heads/main' or ref_name == 'main') and prerelease:
        raise ValueError('Main cannot contain test OTA/version: run prepare_main_merge.py first')
    if ref.startswith('refs/tags/') and ref.removeprefix('refs/tags/') != version:
        raise ValueError('Git tag must match the firmware version exactly')
    return {'version': version, 'tag': version, 'prerelease': prerelease, 'channels': channels}

def read(root):
    root = Path(root)
    version = json.loads((root / 'version.json').read_text('utf8'))['version']
    policy = json.loads((root / 'resources/ota_channels.json').read_text('utf8'))
    git = subprocess.run(['git', '-c', 'safe.directory=' + root.as_posix(), '-C', str(root),
                          'branch', '--show-current'], text=True, capture_output=True)
    return validate(version, policy['channels'], git.stdout.strip() if git.returncode == 0 else '',
                    os.environ.get('GITHUB_REF', ''), os.environ.get('GITHUB_REF_NAME', ''))

if __name__ == '__main__':
    print(json.dumps(read(Path(__file__).resolve().parents[1])))
