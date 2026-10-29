"""Package a successful board build without reading device/NVS data."""
import hashlib,json,pathlib,shutil,subprocess,sys
from partition_layout import read_layout
from board_catalog import BY_STEM
def validate_identity(data,name,chip,project):
 target=BY_STEM.get(name)
 if target is None or (target['chip'],target['project'])!=(chip,project):raise ValueError('Artifact name, chip and board project must match')
 if len(data)<112 or data[0]!=0xe9 or int.from_bytes(data[12:14],'little')!=target['chip_id'] or data[32:36]!=bytes.fromhex('3254cdab'):
  raise ValueError('Expected a matching ESP application image')
 field=data[80:112]
 if b'\0' not in field or field.split(b'\0',1)[0].decode('ascii')!=project:
  raise ValueError('Application project identity differs from the configured board build')

def package(build,name):
 sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[1]/'scripts'))
 from release_policy import read as release_identity
 release=release_identity(pathlib.Path(__file__).resolve().parents[1])
 build=pathlib.Path(build);out=build/'artifacts';out.mkdir(exist_ok=True)
 meta=json.loads((build/'flasher_args.json').read_text('utf-8'));chip=meta['extra_esptool_args']['chip']
 if chip=='esp32p4':
  configs=[json.loads((build/part/'sdkconfig.json').read_text('utf-8')) for part in ('config','bootloader/config')]
  if any(c.get('CACHE_L2_CACHE_LINE_SIZE')!=64 for c in configs):
   raise ValueError('P4 application and bootloader must both use validated 64-byte cache lines; regenerate sdkconfig and rebuild')
 app=build/meta['app']['file'];data=app.read_bytes()
 project=json.loads((build/'project_description.json').read_text('utf-8'))['project_name']
 validate_identity(data,name,chip,project)
 shutil.copy2(app,out/(name+'.bin'))
 command=[sys.executable,'-m','esptool','--chip',chip,'merge-bin','-o',str(out/(name+'-full.bin'))]+meta['write_flash_args']
 for offset,path in meta['flash_files'].items():command.extend([offset,str(build/path)])
 subprocess.run(command,check=True)
 version=release['version']
 if data[48:80].split(b'\0',1)[0].decode('ascii')!=version:
  raise ValueError('Application descriptor version differs from version.json; rebuild before packaging')
 full=(out/(name+'-full.bin')).read_bytes()
 app_offset=int(meta['app']['offset'],0)
 layout=read_layout((build/meta['partition-table']['file']).read_bytes())
 slot=layout['ota_0']['size'];flash_bytes=int(meta['flash_settings']['flash_size'].removesuffix('MB'))*1048576
 if max(p['offset']+p['size'] for p in layout.values())!=flash_bytes:raise ValueError('Partition table does not use configured Flash capacity')
 if len(data)>slot or full[app_offset:app_offset+len(data)]!=data or len(full)>layout['ota_1']['offset']:
  raise ValueError('Application exceeds OTA slot or merged image overlaps the second slot')
 for offset,path in meta['flash_files'].items():
  part=(build/path).read_bytes();start=int(offset,0)
  if full[start:start+len(part)]!=part:raise ValueError('Merged image is missing a flash part')
 laboratory=b'/api/test/' in data
 manifest={'laboratoryBuild':laboratory,'flashBytes':flash_bytes,'applicationSlotBytes':slot,'chip':chip,'version':version,'application':name+'.bin','fullImage':name+'-full.bin','applicationBytes':len(data),'applicationOffset':app_offset,'sha256':hashlib.sha256(data).hexdigest(),'fullSha256':hashlib.sha256(full).hexdigest()}
 manifest['project']=project
 manifest['releaseTag']=release['tag'];manifest['prerelease']=release['prerelease']
 (out/'manifest.json').write_text(json.dumps(manifest,indent=2)+'\n','utf-8')
 (out/(name+'-manifest.json')).write_text(json.dumps(manifest,indent=2)+'\n','utf-8')
 ota={'version':version,'releaseTag':release['tag'],'prerelease':release['prerelease'],'url':f'https://github.com/tbc0309/xTouch-CN/releases/download/{release["tag"]}/{name}.bin','size':len(data),'md5':hashlib.md5(data).hexdigest()}
 if laboratory:(out/'ota.json').unlink(missing_ok=True)
 else:(out/'ota.json').write_text(json.dumps(ota,indent=2)+'\n','utf-8')
 (out/'SHA256SUMS.txt').write_text(f"{manifest['sha256']}  {name}.bin\n{manifest['fullSha256']}  {name}-full.bin\n",'utf-8')
 print('Packaged firmware:',out)
 return out
if __name__=='__main__':package(sys.argv[1],sys.argv[2])
