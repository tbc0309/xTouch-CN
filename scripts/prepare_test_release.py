"""Prepare a complete prerelease asset set from audited native packages; never publishes."""
import argparse,hashlib,json,shutil,sys
from pathlib import Path
from release_policy import read
root=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--artifact-dir',type=Path,action='append',required=True)
parser.add_argument('--output',type=Path,required=True)
args=parser.parse_args();release=read(root)
if not release['prerelease']:raise SystemExit('Expected a test prerelease source tree')
registry=json.loads((root/'web/test-board-targets.json').read_text('utf8'))
by_stem={row['stem']:key for key,row in registry.items()}
records={};assets={}
for folder in args.artifact_dir:
    manifest=json.loads((folder/'manifest.json').read_text('utf8'))
    ota=json.loads((folder/'ota.json').read_text('utf8'))
    stem=str(manifest.get('application','')).removesuffix('.bin');key=by_stem.get(stem)
    if not key or key in records:raise SystemExit('Unknown or duplicate package')
    if manifest.get('version')!=release['version']:raise SystemExit('A package has not been rebuilt for '+release['version'])
    image=(folder/(stem+'.bin')).read_bytes()
    if len(image)!=manifest['applicationBytes'] or hashlib.sha256(image).hexdigest()!=manifest['sha256'] or hashlib.md5(image).hexdigest()!=ota['md5']:
        raise SystemExit('Application asset differs from its audited package: '+key)
    records[key]=dict(id=key,manifest=manifest,ota=ota)
    for filename in [stem+'.bin',stem+'-full.bin']:assets[filename]=folder/filename
if set(records)!=set(registry):raise SystemExit('Missing test packages: '+', '.join(sorted(set(registry)-set(records))))
output=args.output.resolve();output.mkdir(parents=True,exist_ok=True)
expected=set(assets)|{'test-firmware-catalog.json','SHA256SUMS.txt'}
if any(file.is_file() and file.name not in expected for file in output.iterdir()):
    raise SystemExit('Output contains unrelated files; choose a dedicated prerelease folder')
for name,source in assets.items():shutil.copy2(source,output/name)
catalog=dict(schemaVersion=1,version=release['version'],releaseTag=release['tag'],prerelease=True,
             boards=[records[key] for key in sorted(records)])
(output/'test-firmware-catalog.json').write_text(json.dumps(catalog,ensure_ascii=False,indent=2)+'\n','utf8')
# Exercise the same image, partition, version and URL checks used by the public website.
import subprocess
subprocess.run([sys.executable,str(root/'scripts/build_test_web.py'),'--catalog',str(output/'test-firmware-catalog.json'),
                '--firmware-dir',str(output),'--output',str(output/'installer-check')],check=True)
lines=[]
for file in sorted(output.iterdir()):
    if file.is_file() and file.name!='SHA256SUMS.txt':lines.append(hashlib.sha256(file.read_bytes()).hexdigest()+'  '+file.name)
(output/'SHA256SUMS.txt').write_text('\n'.join(lines)+'\n','utf8')
print('Prepared',len(records),'prerelease targets;',len(assets),'firmware files; all checked with the production installer builder.')
