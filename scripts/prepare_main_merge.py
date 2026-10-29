"""Run on test before promotion; remove the test OTA channel from source policy."""
import argparse,json,pathlib,subprocess
root=pathlib.Path(__file__).resolve().parents[1];p=argparse.ArgumentParser();p.add_argument('--apply',action='store_true');a=p.parse_args()
branch=subprocess.check_output(['git','-c','safe.directory='+root.as_posix(),'-C',str(root),'branch','--show-current'],text=True).strip()
if branch!='test':raise SystemExit('Run from the test working tree before merging into main')
path=root/'resources/ota_channels.json';d=json.loads(path.read_text())
if a.apply:
 d['channels']=['main'];path.write_text(json.dumps(d,indent=2)+'\n',encoding='utf8')
 version_path=root/'version.json';version=json.loads(version_path.read_text('utf8'))
 version['version']=version['version'].removesuffix('-test')
 version_path.write_text(json.dumps(version,indent=2)+'\n','utf8')
 for ota_path in (root/'boards').glob('*/ota.json'):
  record=json.loads(ota_path.read_text('utf8'))
  if str(record.get('version','')).endswith('-test'):
   record.update(version=version['version'],releaseTag=version['version'],prerelease=False,
                 status='awaiting-main-rebuild',url='',size=0,md5='')
   ota_path.write_text(json.dumps(record,indent=2)+'\n','utf8')
 print('测试 OTA 地址已移除，仅保留 main；-test 版本后缀已移除，旧测试 OTA 清单已失效。请重新编译生成正式清单。')
else:print('Promotion requires --apply: removes test OTA, retains main. Rebuild/audit before merging; never pushes.')
