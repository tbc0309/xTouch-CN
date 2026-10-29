"""Generate board-specific OTA endpoints, rejecting test endpoints on main."""
import argparse,json,pathlib,re
from release_policy import read as release_identity
root=pathlib.Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser();p.add_argument('--board',required=True);p.add_argument('--output',type=pathlib.Path,required=True);a=p.parse_args()
policy=json.loads((root/'resources/ota_channels.json').read_text());channels=policy['channels']
if not re.fullmatch(r'[a-z0-9][a-z0-9.-]*',a.board):raise SystemExit('Invalid board directory')
if channels not in (['main'],['main','test']):raise SystemExit('Unsupported OTA channels')
try:release=release_identity(root)
except ValueError as error:raise SystemExit(str(error))
repo=policy['repository']
if repo!='tbc0309/xTouch-CN':raise SystemExit('Unexpected OTA repository')
paths=['/'+repo+'/'+channel+'/boards/'+a.board+'/ota.json' for channel in channels]
catalog={}
for catalog_name in ('port_targets.json','bsp_targets.json'):
    catalog_path=root/'boards'/catalog_name
    if catalog_path.exists():
        for name,profile in json.loads(catalog_path.read_text('utf8')).items():
            if name in catalog:raise SystemExit('Duplicate board OTA identity')
            catalog[name]=profile
exclusions=root/'boards/target_exclusions.json'
if exclusions.exists():
    excluded=json.loads(exclusions.read_text(encoding='utf8'))
    if a.board in excluded or catalog.get(a.board,{}).get('candidateId') in excluded:
        raise SystemExit('Excluded board: no OTA identity may be generated for this target')
original_boards={'esp32-2432s028r','esp32-s3-touch-lcd-2.8-v2',
                 'esp32-p4-wifi6-touch-lcd-4.3','lcdwiki-es3c35p'}
if a.board not in catalog and a.board not in original_boards:
    raise SystemExit('Unknown or unimplemented board: refusing an OTA identity fallback')
header='#pragma once\n#define XTOUCH_OTA_GENERATED 1\n#define XTOUCH_OTA_MANIFEST_PATH '+json.dumps(paths[0])+'\n#define XTOUCH_OTA_MANIFEST_URL "https://raw.githubusercontent.com" XTOUCH_OTA_MANIFEST_PATH\nstatic const char *const xtouch_ota_manifest_paths[] = {'+','.join(json.dumps(v) for v in paths)+'};\n'
if a.board in catalog:
    board=catalog[a.board]
    limit=6*1024*1024 if board['flash']==16*1024*1024 else 12*1024*1024
    header+='#define XTOUCH_FIRMWARE_FILENAME_OVERRIDE '+json.dumps('/'+board['stem']+'.bin')+'\n#define XTOUCH_FIRMWARE_LIMIT_OVERRIDE '+str(limit)+'U\n'

a.output.parent.mkdir(parents=True,exist_ok=True)
if not a.output.exists() or a.output.read_text()!=header:a.output.write_text(header,encoding='utf8')
print('OTA channels:',','.join(channels),'board:',a.board)
