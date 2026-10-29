"""Update committed OTA/installer records from four verified native packages."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
from board_catalog import BOARDS
from package import validate_identity
from partition_layout import read_layout


def sync(root, builds, check=False):
    version=json.loads((root.parent/'version.json').read_text(encoding='utf-8'))['version']
    records=[]
    for key,target in BOARDS.items():
        folder=builds/('build-'+key)/'artifacts'
        record=json.loads((folder/'manifest.json').read_text(encoding='utf-8'))
        ota=json.loads((folder/'ota.json').read_text(encoding='utf-8'))
        app=(folder/(target['stem']+'.bin')).read_bytes()
        full=(folder/(target['stem']+'-full.bin')).read_bytes()
        validate_identity(app,target['stem'],record['chip'],record['project'])
        layout=read_layout(full[0x8000:0x9000])
        expected_url=f"https://github.com/tbc0309/xTouch-CN/releases/download/{version}/{target['stem']}.bin"
        if (record.get('laboratoryBuild') is not False or record['version']!=version or ota['version']!=version
            or record['sha256']!=hashlib.sha256(app).hexdigest() or record['fullSha256']!=hashlib.sha256(full).hexdigest()
            or record['applicationBytes']!=len(app) or ota['size']!=len(app) or ota['md5']!=hashlib.md5(app).hexdigest()
            or ota['url']!=expected_url or record['flashBytes']!=target['flash']
            or app[48:80].split(b'\0',1)[0]!=version.encode('ascii')
            or layout['ota_0']['offset']!=target['app_offset']
            or record['applicationSlotBytes']!=layout['ota_0']['size']
            or len(app)>layout['ota_0']['size']
            or max(part['offset']+part['size'] for part in layout.values())!=target['flash']
            or record['applicationOffset']!=target['app_offset'] or full[target['app_offset']:]!=app):
            raise ValueError('Invalid release package: '+key)
        records.extend((folder/name,root/target['directory']/name) for name in ('manifest.json','ota.json'))
    for source,destination in records:
        if check:
            if not destination.is_file() or json.loads(source.read_text())!=json.loads(destination.read_text()):
                raise ValueError('Committed release metadata differs: '+str(destination))
        else:
            shutil.copy2(source,destination)
    print(('Checked' if check else 'Updated')+'4-board release metadata: '+version)


if __name__=='__main__':
    root=Path(__file__).resolve().parent
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--builds',type=Path,default=root)
    parser.add_argument('--check',action='store_true')
    args=parser.parse_args()
    sync(root,args.builds,args.check)
