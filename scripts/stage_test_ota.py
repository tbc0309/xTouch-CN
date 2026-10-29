"""Stage board OTA records from verified test artifacts; never publishes anything."""
import argparse
import hashlib
import json
from pathlib import Path
import sys
from release_policy import read

root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'boards'))
from board_catalog import BOARDS,BY_STEM
from package import validate_identity

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--reset-pending',action='store_true',help='Invalidate stale board OTA metadata before a version rebuild')
parser.add_argument('--artifacts',type=Path,action='append',default=[])
args=parser.parse_args()
release=read(root)
if not release['prerelease']:raise SystemExit('This staging tool is for -test prereleases only')
if args.reset_pending:
    for key,board in BOARDS.items():
        path=root/'boards'/board['directory']/'ota.json'
        record=dict(version=release['version'],releaseTag=release['tag'],prerelease=True,board=key,
                    status='awaiting-native-build',url='https://github.com/tbc0309/xTouch-CN/releases/download/'+release['tag']+'/'+board['stem']+'.bin',size=0,md5='')
        path.write_text(json.dumps(record,indent=2)+'\n','utf8')
    print('Stale test metadata invalidated:',len(BOARDS),'boards; zero size/empty digest prevents installation.')
for folder in args.artifacts:
    record=json.loads((folder/'manifest.json').read_text('utf8'))
    ota=json.loads((folder/'ota.json').read_text('utf8'))
    board=BY_STEM.get(record.get('application','').removesuffix('.bin'))
    if not board:raise SystemExit('Unknown board artifact')
    image=(folder/record['application']).read_bytes()
    validate_identity(image,board['stem'],board['chip'],board['project'])
    expected='https://github.com/tbc0309/xTouch-CN/releases/download/'+release['tag']+'/'+board['stem']+'.bin'
    if (record['version']!=release['version'] or ota['version']!=release['version'] or
        record.get('releaseTag')!=release['tag'] or ota.get('releaseTag')!=release['tag'] or
        not record.get('prerelease') or not ota.get('prerelease') or ota['url']!=expected or
        image[48:80].split(b'\0',1)[0].decode('ascii')!=release['version'] or
        ota['size']!=len(image) or hashlib.md5(image).hexdigest()!=ota['md5'] or
        hashlib.sha256(image).hexdigest()!=record['sha256']):
        raise SystemExit('OTA metadata/image/version mismatch; rebuild instead of relabeling')
    dest=root/'boards'/board['directory']
    (dest/'ota.json').write_text(json.dumps(ota,indent=2)+'\n','utf8')
    (dest/'manifest.json').write_text(json.dumps(record,indent=2)+'\n','utf8')
    print('Staged verified test OTA:',board['directory'],release['version'])
