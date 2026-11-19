"""Compare upstream Bambu Studio thumbnail/ZIP writer code before firmware updates."""
from __future__ import annotations
import argparse
import concurrent.futures
import hashlib
import json
import os
import re
import subprocess
import urllib.request
from pathlib import Path

ROOT=Path(__file__).resolve().parents[1]
FILES={
 'src/libslic3r/Format/bbs_3mf.cpp':['_add_thumbnail_file_to_archive','_add_relationships_file_to_archive','_add_file_to_archive'],
 'src/libslic3r/miniz_extension.cpp':[],
 'src/miniz/miniz.c':['mz_zip_writer_add_mem_ex_v2','mz_zip_writer_add_file','mz_zip_writer_finalize_archive','mz_zip_writer_add_cfile'],
}

def github(path):
    token=os.environ.get('GH_TOKEN') or os.environ.get('GITHUB_TOKEN')
    if token:
        req=urllib.request.Request('https://api.github.com/repos/bambulab/BambuStudio/'+path,headers={'Authorization':'Bearer '+token,'Accept':'application/vnd.github+json'})
        with urllib.request.urlopen(req,timeout=30) as r:return json.load(r)
    result=subprocess.run(['gh','api','repos/bambulab/BambuStudio/'+path],capture_output=True,text=True,encoding="utf-8",check=True,timeout=45)
    return json.loads(result.stdout)

def bodies(source,names):
    mask=re.sub(r'//[^\n]*|/\*.*?\*/|"(?:\\.|[^"\\])*"|\x27(?:\\.|[^\x27\\])*\x27',lambda m:' '*len(m.group()),source,flags=re.S)
    result={}
    for name in names:
        for match in re.finditer(r'\b'+re.escape(name)+r'\s*\(',mask):
            pos=match.end();depth=1
            while pos<len(mask) and depth:
                depth+=(mask[pos]=='(')-(mask[pos]==')');pos+=1
            while pos<len(mask) and mask[pos].isspace():pos+=1
            qualifier=re.match(r'(?:const\b\s*)?(?:noexcept\b\s*)?',mask[pos:])
            pos+=qualifier.end()
            if pos>=len(mask) or mask[pos]!='{':continue
            start=pos;depth=1;pos+=1
            while pos<len(mask) and depth:
                depth+=(mask[pos]=='{')-(mask[pos]=='}');pos+=1
            if depth:raise ValueError('unterminated function '+name)
            text=source[start:pos]
            result[name]={'sha256':hashlib.sha256(re.sub(r'\s+',' ',text).encode()).hexdigest(),'line':source.count('\n',0,start)+1}
            break
        if name not in result:result[name]=None
    return result

def inspect(ref,cache):
    commit=github('commits/'+ref)['sha'];rows={}
    for file,names in FILES.items():
        url=f'https://raw.githubusercontent.com/bambulab/BambuStudio/{commit}/{file}'
        try:
            with urllib.request.urlopen(url,timeout=30) as r:data=r.read(3*1024*1024+1)
        except Exception as e:
            if getattr(e,'code',None)==404:rows[file]={'absent':True};continue
            raise
        if len(data)>3*1024*1024:raise ValueError('source file exceeds budget')
        source=data.decode('utf-8');target=cache/commit/file;target.parent.mkdir(parents=True,exist_ok=True);target.write_bytes(data)
        important=sorted(set(line.strip() for line in source.splitlines() if any(s in line for s in ['Metadata/plate_','cover-thumbnail-small','cover-thumbnail-middle','THUMBNAIL_FILE =','gcode.md5'])))
        rows[file]={'sha256':hashlib.sha256(data).hexdigest(),'functions':bodies(source,names),'format_lines':important,'url':url}
    return {'ref':ref,'commit':commit,'files':rows}

def compare(reference,target):
    changes=[]
    for path in FILES:
        old,new=reference['files'].get(path,{}),target['files'].get(path,{})
        for key in ('functions','format_lines','absent'):
            if key=='functions':
                left={k:v and v['sha256'] for k,v in old.get(key,{}).items()};right={k:v and v['sha256'] for k,v in new.get(key,{}).items()}
            else:left,right=old.get(key),new.get(key)
            if left!=right:changes.append({'file':path,'area':key})
        if not FILES[path] and old.get('sha256')!=new.get('sha256'):changes.append({'file':path,'area':'ZIP wrapper'})
    return changes

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--ref',action='append');p.add_argument('--latest-release',action='store_true');p.add_argument('--baseline',type=Path,default=ROOT/'resources/thumbnail-upstream.json');p.add_argument('--report',type=Path,required=True);p.add_argument('--cache',type=Path,default=ROOT.parent/'outputs/thumbnail-upstream');p.add_argument('--update-baseline',action='store_true');p.add_argument('--strict',action='store_true');a=p.parse_args()
    refs=a.ref or ['master']
    if a.latest_release:refs=list(dict.fromkeys(refs+[github('releases/latest')['tag_name']]))
    with concurrent.futures.ThreadPoolExecutor(max_workers=3) as pool:records=list(pool.map(lambda ref:inspect(ref,a.cache),refs))
    baseline=json.loads(a.baseline.read_text('utf-8')) if a.baseline.exists() else None
    reports=[dict(record,changes=compare(baseline['reference'],record) if baseline else [{'area':'baseline_missing'}]) for record in records]
    report={'repository':'https://github.com/bambulab/BambuStudio','targets':reports,'review_required':any(x['changes'] for x in reports)}
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2), 'utf-8')
    if a.update_baseline:
        a.baseline.parent.mkdir(parents=True,exist_ok=True);a.baseline.write_text(json.dumps({'reference':records[-1],'reviewed_history':records},indent=2),'utf-8')
    for r in reports:print(json.dumps({'ref':r['ref'],'commit':r['commit'],'changed_areas':len(r['changes'])}),flush=True)
    if a.strict and report['review_required']:raise SystemExit(2)
if __name__=='__main__':main()
