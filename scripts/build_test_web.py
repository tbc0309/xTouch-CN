"""Build a test installer channel from verified prerelease artifacts, preserving NVS."""
import argparse
import hashlib
import json
import re
from pathlib import Path
import sys

root=Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'boards'))
from partition_layout import read_layout,installer_segments
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--catalog',type=Path)
parser.add_argument('--firmware-dir',type=Path)
parser.add_argument('--output',type=Path,required=True)
parser.add_argument('--unavailable',action='store_true')
parser.add_argument('--allow-partial',action='store_true')
args=parser.parse_args()
tag=json.loads((root/'web/test-release.json').read_text('utf8'))['tag']
if not re.fullmatch(r'\d+\.\d+\.\d+-test',tag):raise SystemExit('Invalid test release tag')
registry=json.loads((root/'web/test-board-targets.json').read_text('utf8'))
output=args.output/'test';output.mkdir(parents=True,exist_ok=True)
index=dict(version=tag,releaseTag=tag,prerelease=True,channel='test',boards=[])
if args.unavailable:
    (output/'firmware.json').write_text(json.dumps(index,indent=2)+'\n','utf8')
    print('Test channel unavailable; release installer remains available.')
    raise SystemExit(0)
if not args.catalog or not args.firmware_dir:parser.error('Provide --catalog and --firmware-dir')
catalog=json.loads(args.catalog.read_text('utf8'))
if catalog.get('version')!=tag or catalog.get('releaseTag')!=tag or catalog.get('prerelease') is not True:
    raise SystemExit('Catalog must identify the configured prerelease exactly')
seen=set();(output/'firmware').mkdir(exist_ok=True)
for item in catalog['boards']:
    key=item['id'];target=registry.get(key);record=item['manifest'];ota=item['ota']
    if not target or key in seen:raise SystemExit('Unknown/duplicate test board')
    seen.add(key);stem=target['stem'];filename=stem+'-full.bin'
    if (record.get('fullImage')!=filename or record.get('application')!=stem+'.bin' or
        record.get('version')!=tag or record.get('releaseTag')!=tag or not record.get('prerelease') or
        record.get('project')!=target['project'] or record.get('chip')!=target['chip'] or
        record.get('flashBytes')!=target['flash'] or record.get('applicationOffset')!=target['app_offset']):
        raise SystemExit('Test board/version/image identity mismatch: '+key)
    full=(args.firmware_dir/filename).read_bytes();offset=target['app_offset'];app=full[offset:]
    if (hashlib.sha256(full).hexdigest()!=record['fullSha256'] or
        hashlib.sha256(app).hexdigest()!=record['sha256'] or len(app)!=record['applicationBytes'] or
        len(app)<208 or app[0]!=0xe9 or int.from_bytes(app[12:14],'little')!=target['chip_id'] or
        app[32:36]!=bytes.fromhex('3254cdab') or app[48:80].split(b'\0',1)[0].decode('ascii')!=tag or
        app[80:112].split(b'\0',1)[0].decode('ascii')!=target['project'] or
        full[target['boot_offset']]!=0xe9):
        raise SystemExit('Test image content/checksum mismatch: '+key)
    layout=read_layout(full[0x8000:0x9000]);slot=layout['ota_0']
    if (slot['offset']!=offset or slot['size']!=record['applicationSlotBytes'] or
        len(app)>slot['size'] or len(full)>layout['ota_1']['offset'] or
        max(p['offset']+p['size'] for p in layout.values())!=target['flash']):
        raise SystemExit('Invalid test Flash/OTA layout: '+key)
    url='https://github.com/tbc0309/xTouch-CN/releases/download/'+tag+'/'+stem+'.bin'
    if (ota.get('version')!=tag or ota.get('releaseTag')!=tag or not ota.get('prerelease') or
        ota.get('url')!=url or ota.get('size')!=len(app) or ota.get('md5')!=hashlib.md5(app).hexdigest()):
        raise SystemExit('Test OTA record mismatch: '+key)
    parts=[]
    (output/'firmware'/filename).write_bytes(full)
    for number,(part_offset,data) in enumerate(installer_segments(full)):
        path='firmware/'+stem+'-install-'+str(number)+'.bin'
        (output/path).write_bytes(data);parts.append(dict(path=path,offset=part_offset))
    manifest=dict(name='xTouch-CN · '+key,version=tag,new_install_prompt_erase=True,new_install_improv_wait_time=0,
                  builds=[dict(chipFamily=target['family'],improv=False,parts=parts)])
    manifest_name='manifest-'+key+'.json'
    (output/manifest_name).write_text(json.dumps(manifest,indent=2)+'\n','utf8')
    index['boards'].append(dict(id=key,name=target['name'],version=tag,manifest='test/'+manifest_name,
                               fullPath='test/firmware/'+filename,fullSha256=record['fullSha256'],appUrl=url))
if not seen or (not args.allow_partial and seen!=set(registry)):
    raise SystemExit('Missing test firmware targets: '+', '.join(sorted(set(registry)-seen)))
(output/'firmware.json').write_text(json.dumps(index,ensure_ascii=False,indent=2)+'\n','utf8')
print('Built verified prerelease installer:',tag,len(seen),'boards; NVS excluded from flash ranges.')
