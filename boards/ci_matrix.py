"""Select exact board targets for stable or test firmware validation."""
import json
from board_catalog import BOARDS
from pathlib import Path
import sys
import os

root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'scripts'))
from release_policy import read
release=read(root)
original=['cyd','s3','p4','es3c35p']
names=[key for key in BOARDS if key not in original] if release['prerelease'] else original
matrix=json.dumps({'include':[{'board':key} for key in names]},separators=(',',':'))
if '--github-output' in sys.argv:
    with open(os.environ['GITHUB_OUTPUT'],'a',encoding='utf8') as output:
        output.write('matrix='+matrix+'\nprerelease='+str(release['prerelease']).lower()+'\n')
else:print(matrix)
