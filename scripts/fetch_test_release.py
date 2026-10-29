"""Fetch the configured GitHub prerelease and build its optional installer channel."""
import argparse,json,os,re,subprocess,sys,urllib.error,urllib.request
from pathlib import Path
root=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--output',type=Path,required=True)
parser.add_argument('--download-dir',type=Path,required=True)
args=parser.parse_args()
tag=json.loads((root/'web/test-release.json').read_text('utf8'))['tag']
if not re.fullmatch(r'\d+\.\d+\.\d+-test',tag):raise SystemExit('Invalid configured test tag')
repo='tbc0309/xTouch-CN'
headers={'Accept':'application/vnd.github+json','User-Agent':'xTouch-CN-installer-builder'}
token=os.environ.get('GH_TOKEN') or os.environ.get('GITHUB_TOKEN')
if token:headers['Authorization']='Bearer '+token
request=urllib.request.Request('https://api.github.com/repos/'+repo+'/releases/tags/'+tag,headers=headers)
try:
    with urllib.request.urlopen(request,timeout=30) as response:release=json.load(response)
except urllib.error.HTTPError as error:
    if error.code!=404:raise SystemExit('Could not query test release: HTTP '+str(error.code))
    subprocess.run([sys.executable,str(root/'scripts/build_test_web.py'),'--output',str(args.output),'--unavailable'],check=True)
    raise SystemExit(0)
if release['draft'] or not release['prerelease'] or release['tag_name']!=tag:
    raise SystemExit('Configured test release must be a published prerelease with the exact tag')
assets={item['name'] for item in release['assets']}
if 'test-firmware-catalog.json' not in assets:raise SystemExit('Prerelease catalog is missing')
args.download_dir.mkdir(parents=True,exist_ok=True)
subprocess.run(['gh','release','download',tag,'--repo',repo,'--pattern','*-full.bin','--pattern','test-firmware-catalog.json','--dir',str(args.download_dir),'--clobber'],check=True)
subprocess.run([sys.executable,str(root/'scripts/build_test_web.py'),'--output',str(args.output),'--catalog',str(args.download_dir/'test-firmware-catalog.json'),'--firmware-dir',str(args.download_dir)],check=True)
