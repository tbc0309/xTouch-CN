"""Add a verified public iDotMatrix release to an already-built installer site."""
import argparse
import hashlib
import json
import os
import re
import tempfile
import urllib.error
import urllib.request
from pathlib import Path
from build_idotmatrix_web import PROJECT, BOARD, BOARDS, REPOSITORY, add_firmware, write_json

def download(url,limit,headers=None):
    request=urllib.request.Request(url,headers=headers or {'User-Agent':'xTouch-CN-installer'})
    with urllib.request.urlopen(request,timeout=45) as response:
        data=response.read(limit+1)
    if len(data)>limit:raise ValueError('Pixel release download exceeds size limit')
    return data

def fetch(output):
    output=Path(output)
    if not (output/'idotmatrix.html').is_file():raise ValueError('Build the independent installer page first')
    write_json(output/'idotmatrix-firmware.json',{'project':PROJECT,'published':False,'version':None,'boards':[]})
    headers={'User-Agent':'xTouch-CN-installer','Accept':'application/vnd.github+json'}
    if os.environ.get('GH_TOKEN'):headers['Authorization']='Bearer '+os.environ['GH_TOKEN']
    try:
        release=json.loads(download('https://api.github.com/repos/tbc0309/'+PROJECT+'/releases/latest',1024*1024,headers))
    except urllib.error.HTTPError as error:
        if error.code==404:
            print('No public iDotMatrix release; independent installer stays unavailable.')
            return False
        raise
    version=release.get('tag_name','')
    if release.get('draft') or release.get('prerelease') or not re.fullmatch(r'\d+\.\d+\.\d+',version):
        raise ValueError('Expected a public stable pixel-controller release')
    board_catalog=json.loads(download('https://raw.githubusercontent.com/tbc0309/'+PROJECT+'/'+version+'/boards/catalog.json',65536))
    for stem,(_,_,_,_,flash,slot,app_offset,_,_,_) in BOARDS.items():
        expected={stem+'.bin':slot,stem+'-full.bin':app_offset+slot}
        assets={item['name']:item for item in release.get('assets',[])}
        if stem!=BOARD and not set(expected).intersection(assets):
            continue
        if not set(expected).issubset(assets):raise ValueError('Incomplete pixel release')
        with tempfile.TemporaryDirectory(prefix='idotmatrix-release-') as temporary:
            folder=Path(temporary)
            for name,limit in expected.items():
                asset=assets[name]
                url=REPOSITORY+'/releases/download/'+version+'/'+name
                if asset.get('browser_download_url')!=url or not isinstance(asset.get('size'),int) or not 0<asset['size']<=limit:
                    raise ValueError('Unexpected pixel release asset')
                digest=asset.get('digest')
                cache_key=digest.removeprefix('sha256:') if digest else str(asset.get('id',''))
                data=download(url+('?asset='+cache_key if cache_key else ''),limit)
                if len(data)!=asset['size']:raise ValueError('Incomplete pixel asset download')
                digest=asset.get('digest')
                if digest and digest!='sha256:'+hashlib.sha256(data).hexdigest():raise ValueError('Pixel GitHub asset digest mismatch')
                (folder/name).write_bytes(data)
            metadata=json.loads(download('https://raw.githubusercontent.com/tbc0309/'+PROJECT+'/'+version+'/boards/'+stem+'/ota.json?image='+hashlib.sha256((folder/(stem+'.bin')).read_bytes()).hexdigest(),65536))
            app=(folder/(stem+'.bin')).read_bytes()
            if metadata.get('version')!=version or metadata.get('board')!=stem or metadata.get('sha256')!=hashlib.sha256(app).hexdigest() or metadata.get('size')!=len(app):
                raise ValueError('Board OTA metadata mismatch')
            definition=board_catalog.get(stem,{})
            if metadata.get('boardName')!=definition.get('name') or metadata.get('target')!=definition.get('target'):
                raise ValueError('Board metadata does not match boards/catalog.json')
            record=dict(metadata,flash=flash,files=[{'file':name,'size':(folder/name).stat().st_size,'sha256':hashlib.sha256((folder/name).read_bytes()).hexdigest()} for name in expected])
            write_json(folder/'manifest.json',record)
            add_firmware(output,folder,published=True)
    print('Verified public iDotMatrix release '+version+'; independent installer ready.')
    return True

if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',required=True,type=Path)
    args=parser.parse_args()
    fetch(args.output)
