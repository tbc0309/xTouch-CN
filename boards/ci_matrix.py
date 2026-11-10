"""Print release-channel build targets from board metadata."""
import json
import os
import sys
from pathlib import Path
from board_catalog import BOARDS
root = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(root / 'scripts'))
from release_policy import read
release = read(root)
channel = 'test' if release['prerelease'] else 'main'
names = [key for key, board in BOARDS.items() if board.get('releaseChannel') == channel and
         not board.get('localOnly', False) and (channel == 'test' or board.get('releaseSupported', True))]
if not names:
    raise SystemExit('No enabled build targets in the selected release channel')
matrix = json.dumps({'include': [{'board': key} for key in names]}, separators=(',', ':'))
if '--github-output' in sys.argv:
    with open(os.environ['GITHUB_OUTPUT'], 'a', encoding='utf8') as out:
        out.write('matrix=' + matrix + '\nprerelease=' + str(release['prerelease']).lower() + '\n')
else:
    print(matrix)
