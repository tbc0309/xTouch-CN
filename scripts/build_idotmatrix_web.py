"""Prepare the independent pixel-controller installer without touching xTouch images."""
import hashlib
import json
import re
import struct
from pathlib import Path

PROJECT='idotmatrix-bambulab'
BOARD='lckfb-esp32s3r8n8'
BOARDS={
    BOARD: (None,'esp32s3','ESP32-S3',9,'8MB',0x370000,0x90000,0x89000,0x80000,0x400000),
    'clockwise-esp32-hub75': (None,'esp32','ESP32',0,'4MB',0x1e0000,0x20000,0x19000,0x10000,0x200000),
    'esp32c3-oled-042': (None,'esp32c3','ESP32-C3',5,'4MB',0x1e0000,0x20000,0x19000,0x10000,0x200000),
    'esp32c3-rgb24': (None,'esp32c3','ESP32-C3',5,'4MB',0x1e0000,0x20000,0x19000,0x10000,0x200000),
    'seeed-xiao-esp32c6': (None,'esp32c6','ESP32-C6',13,'4MB',0x1e0000,0x20000,0x19000,0x10000,0x200000),
}
REPOSITORY='https://github.com/tbc0309/'+PROJECT
ROOT=Path(__file__).resolve().parents[1]

def write_json(path,value):
    path.write_text(json.dumps(value,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')

def build_page(output,css_name,language_name,installer_name):
    output=Path(output)
    source=ROOT/'web'
    js=(source/'idotmatrix.js').read_text('utf-8').replace('./language.js','./'+language_name).replace('./vendor/install-button.js','./vendor/'+installer_name)
    js_name='idotmatrix-'+hashlib.sha256(js.encode()).hexdigest()[:12]+'.js'
    (output/js_name).write_text(js,encoding='utf-8')
    css=(source/'idotmatrix.css').read_bytes()
    pixel_css='idotmatrix-'+hashlib.sha256(css).hexdigest()[:12]+'.css'
    (output/pixel_css).write_bytes(css)
    page=(source/'idotmatrix.html').read_text('utf-8').replace('href="style.css"',f'href="{css_name}"').replace('href="idotmatrix.css"',f'href="{pixel_css}"').replace('src="idotmatrix.js"',f'src="{js_name}"')
    (output/'idotmatrix.html').write_text(page,encoding='utf-8')
    # Every build starts unavailable. Only a verified release can enable flashing.
    write_json(output/'idotmatrix-firmware.json',{'project':PROJECT,'published':False,'version':None,'boards':[]})

def verified_package(folder):
    folder=Path(folder)
    record=json.loads((folder/'manifest.json').read_text('utf-8'))
    version=record.get('version','')
    board=record.get('board')
    if board not in BOARDS:raise ValueError('Unknown pixel-controller board')
    name,target,chip,chip_id,flash,slot,app_offset,ota_offset,nvs_size,second=BOARDS[board]
    if not re.fullmatch(r'\d+\.\d+\.\d+',version) or record.get('project')!=PROJECT or record.get('projectName')!='idotmatrix_bambulab' or record.get('target')!=target or record.get('flash')!=flash or record.get('appSlotBytes')!=slot:
        raise ValueError('Invalid pixel-controller package identity')
    stem=board
    expected={stem+'.bin',stem+'-full.bin'}
    entries=record.get('files',[])
    if len(entries)!=2 or {v.get('file') for v in entries}!=expected:
        raise ValueError('Unexpected pixel firmware filenames')
    data={}
    for entry in entries:
        blob=(folder/entry['file']).read_bytes()
        if len(blob)!=entry.get('size') or hashlib.sha256(blob).hexdigest()!=entry.get('sha256'):
            raise ValueError('Pixel package checksum mismatch')
        data[entry['file']]=blob
    app=data[stem+'.bin'];full=data[stem+'-full.bin']
    if len(app)<288 or len(app)>slot or app[0]!=0xe9 or int.from_bytes(app[12:14],'little')!=chip_id or app[32:36]!=bytes.fromhex('3254cdab') or app[48:80].split(b'\0',1)[0]!=version.encode() or app[80:112].split(b'\0',1)[0]!=b'idotmatrix_bambulab':
        raise ValueError('Pixel application identity or capacity mismatch')
    boot_offset=0x1000 if target=='esp32' else 0
    if (len(full)!=app_offset+len(app) or full[:boot_offset]!=b'\xff'*boot_offset
        or full[boot_offset]!=0xe9 or int.from_bytes(full[boot_offset+12:boot_offset+14],'little')!=chip_id
        or full[app_offset:]!=app):
        raise ValueError('Pixel merged image does not match application')
    expected_layout={
        'nvs':(1,2,0x9000,nvs_size),
        'otadata':(1,0,ota_offset,0x2000),
        'phy_init':(1,1,ota_offset+0x2000,0x1000),
        'ota_0':(0,16,app_offset,slot),
        'ota_1':(0,17,second,slot),
    }
    layout={}
    for pos in range(0x8000,0x9000-31,32):
        magic,kind,subtype,offset,size,label,flags=struct.unpack_from('<HBBII16sI',full,pos)
        if magic!=0x50aa:break
        name=label.split(b'\0',1)[0].decode('ascii')
        if name in layout or flags:raise ValueError('Invalid pixel partitions')
        layout[name]=(kind,subtype,offset,size)
    if layout!=expected_layout or full[0x9000:ota_offset]!=b'\xff'*nvs_size:
        raise ValueError('Unexpected pixel layout or populated NVS')
    return version,stem,full

def add_firmware(output,folder,published=False):
    version,board,full=verified_package(folder)
    name,target_name,chip,_,_,_,_,ota_offset,_,_=BOARDS[board]
    # Names come from boards metadata, not a second display-name table.
    record=json.loads((Path(folder)/'manifest.json').read_text('utf-8'))
    name=record.get('boardName')
    if not isinstance(name,str) or not name.strip() or len(name)>100:raise ValueError('Missing board name')
    stem=f'{PROJECT}-{version}-{board}'
    output=Path(output)
    target=output/'idotmatrix';(target/'firmware').mkdir(parents=True,exist_ok=True)
    parts=[]
    boot_offset=0x1000 if target_name=='esp32' else 0
    for i,(offset,data) in enumerate([(boot_offset,full[boot_offset:0x9000]),(ota_offset,full[ota_offset:])]):
        path=f'idotmatrix/firmware/{stem}-install-{i}.bin'
        (output/path).write_bytes(data)
        parts.append({'path':path,'offset':offset,'size':len(data),'sha256':hashlib.sha256(data).hexdigest()})
    title='3D Print Pixel Monitor' if board=='clockwise-esp32-hub75' else 'iDotMatrix × Bambu Lab'
    manifest={'name':title+' · '+name,'version':version,'new_install_prompt_erase':True,'new_install_improv_wait_time':0,
              'builds':[{'chipFamily':chip,'improv':False,'parts':[{'path':p['path'].removeprefix('idotmatrix/'),'offset':p['offset']} for p in parts]}]}
    manifest_name='manifest.json' if board==BOARD else f'{board}.json'
    write_json(target/manifest_name,manifest)
    catalog={'project':PROJECT,'version':version,'published':bool(published),'releaseUrl':REPOSITORY+'/releases/tag/'+version,
             'boards':[{'id':board,'name':name,'chip':chip,'manifest':'idotmatrix/'+manifest_name,'parts':parts}]}
    path=output/'idotmatrix-firmware.json'
    if path.exists():
        previous=json.loads(path.read_text('utf-8'))
        if previous.get('version')==version and previous.get('published')==bool(published):
            catalog['boards']=[b for b in previous['boards'] if b['id']!=board]+catalog['boards']
    write_json(path,catalog)
    return catalog

if __name__=='__main__':
    import argparse
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output',required=True,type=Path)
    parser.add_argument('--firmware-dir',required=True,type=Path)
    args=parser.parse_args()
    add_firmware(args.output,args.firmware_dir,published=False)
    print('Pixel candidate verified; NVS excluded; public installation remains disabled.')
