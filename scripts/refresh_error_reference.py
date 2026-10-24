"""Refresh only BambuStudio's HMS sources, never a community-code union.

Uses the same vendor endpoints/device split as DevHMSQuery.cpp. --snapshot makes
an offline rebuild from a previously captured set of JSON responses possible.
"""
from pathlib import Path
import argparse, concurrent.futures, json, re, urllib.request
ROOT=Path(__file__).resolve().parents[1]
parser=argparse.ArgumentParser();parser.add_argument('--snapshot',type=Path);args=parser.parse_args()
def get(url):
 with urllib.request.urlopen(urllib.request.Request(url,headers={"User-Agent":"xTouch-CN-reference-updater/1.0","Accept":"application/json"}),timeout=20) as r:
  raw=r.read(8000001)
  if len(raw)>8000000:raise ValueError('Response too large')
  return json.loads(raw)
sha=get('https://api.github.com/repos/bambulab/BambuStudio/commits/master')['sha'] if not args.snapshot else '79df7ae309be398ed30c9f397c13c9911c2edc57'
if args.snapshot:devices=sorted(p.name[:-8] for p in args.snapshot.glob('*-en.json'))
else:
 tree=get('https://api.github.com/repos/bambulab/BambuStudio/git/trees/'+sha+'?recursive=1')['tree']
 devices=sorted({re.fullmatch(r'resources/hms/hms_action_(\w+)\.json',r['path'])[1] for r in tree if re.fullmatch(r'resources/hms/hms_action_(\w+)\.json',r['path'])}|{'01P','00M','03W'})
def fetch(item):
 dev,lang=item
 if args.snapshot:j=json.loads((args.snapshot/(dev+'-'+lang+'.json')).read_text('utf-8'))
 else:j=get('https://e.bambulab.com/hms/GetActionImage.php?d='+dev if lang=='action' else 'https://e.bambulab.com/query.php?lang='+lang+'&d='+dev)
 if 'data' not in j:raise ValueError('Invalid response')
 return dev,lang,j
with concurrent.futures.ThreadPoolExecutor(max_workers=6) as pool:responses=list(pool.map(fetch,[(d,l) for d in devices for l in ['en','zh-cn','action']]))
values={'hms':{},'device':{}};zh={'hms':{},'device':{}};origins={'hms':{},'device':{}};variants={'hms':{},'device':{}};actions={};versions={}
for dev,lang,j in responses:
 if lang=='action':
  rows=j['data'];assert isinstance(rows,list)
  actions[dev]=[]
  for r in rows:
   c=r['ecode'].upper();assert re.fullmatch(r'[0-9A-F]{8}|[0-9A-F]{16}',c)
   action=r.get('actions',[]);assert isinstance(action,list) and all(isinstance(x,int) for x in action)
   actions[dev].append([c,action])
  continue
 for kind,source in [('hms','device_hms'),('device','device_error')]:
  block=j['data'][source];rows=block[lang];assert rows
  versions[dev+'-'+lang]=str(block['ver'])
  for row in rows:
   code=row['ecode'].upper();text=row['intro'].strip();assert re.fullmatch(r'[0-9A-F]{16}' if kind=='hms' else r'[0-9A-F]{8}',code)
   if not text:continue
   if lang=='en':
    values[kind].setdefault(code,text);origins[kind].setdefault(code,[]).append(dev)
    variants[kind].setdefault(code,{})[dev]=text
   else:zh[kind].setdefault(code,text)
# Studio's explicit pseudo-button override; other actions are scoped per device.
if args.snapshot:
 text=(args.snapshot/'DeviceErrorDialog.cpp').read_text('utf-8')
else:
 url='https://raw.githubusercontent.com/bambulab/BambuStudio/'+sha+'/src/slic3r/GUI/DeviceErrorDialog.cpp'
 with urllib.request.urlopen(urllib.request.Request(url,headers={'User-Agent':'xTouch-CN-reference-updater/1.0'}),timeout=20) as response:text=response.read(1000000).decode('utf-8')
match=re.search(r'message_containing_retry\s*\{(.*?)\};',text,re.S)
if not match:raise ValueError('Studio retry action structure changed; review before updating')
retry=[c.replace('-','') for c in re.findall(r'"([0-9A-Fa-f]{4}-[0-9A-Fa-f]{4})"',match[1])]
assert retry

corpus={k:sorted(v.items()) for k,v in values.items()}
corpus.update({'retry':retry,'done':[], 'zh':zh,'origins':origins,'variants':{k:{c:v for c,v in variants[k].items() if len(set(v.values()))>1} for k in variants},'deviceActions':actions,'source':'BambuStudio DevHMSQuery / DeviceErrorDialog','studioCommit':sha,'referenceVersion':versions})
target=ROOT/'docs/data/error-codes.json';tmp=target.with_suffix('.tmp');tmp.write_text(json.dumps(corpus,ensure_ascii=False,indent=2)+'\n','utf-8');tmp.replace(target)
print('Studio-only corpus:',{k:len(v) for k,v in values.items()},'; device groups:',len(devices))
