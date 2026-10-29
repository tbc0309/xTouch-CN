"""Verify a completed native board build excludes the Arduino framework and PubSubClient."""
import argparse,json,pathlib,re
from collections import Counter
import hashlib
import struct
import subprocess
import os
import sys
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('build_dir',type=pathlib.Path)
parser.add_argument('--release',action='store_true',help='Reject laboratory test APIs')
parser.add_argument('--report',type=pathlib.Path)
args=parser.parse_args();build=args.build_dir.resolve()
description=build/'project_description.json';data=json.loads(description.read_text(encoding='utf-8'))
components=data['build_components'];forbidden=[c for c in components if re.search(r'arduino|pubsubclient',c,re.I)]
if forbidden:raise SystemExit('Forbidden framework components: '+', '.join(forbidden))
commands=json.loads((build/'compile_commands.json').read_text(encoding='utf-8'))
if args.release:
 flag_text='\n'.join(entry.get('command','') for entry in commands)
 for name in ('cflags','cxxflags','asmflags'):
  response=build/'toolchain'/name
  if response.exists():flag_text+='\n'+response.read_text(encoding='utf-8')
 if re.search(r'-DXTOUCH_TEST_(?:AUTOMATION|DEFER_BLE|PROVISIONING_ONLY)(?:=1\b|(?=\s|$))',flag_text):
  raise SystemExit('Laboratory compile flag must not enter a release configuration')
for entry in commands:
 command=entry['command'].replace('\\','/').lower()
 if pathlib.Path(entry['file']).suffix.lower()=='.s' and 'lvgl_compat.h' in command:raise SystemExit('C/C++ LVGL adapter must not be injected into assembly: '+entry['file'])
 if re.search(r'/(?:espressif__)?arduino-esp32/|/pubsubclient/src/',command):raise SystemExit('Forbidden framework source/header: '+entry['file'])
application=[e for e in commands if '/src/main.cpp' in e['file'].replace('\\','/')]
if len(application)!=1 or '-DXTOUCH_NATIVE_IDF=1' not in application[0]['command']:raise SystemExit('Native application entry point not selected')
graphics_memory=[e for e in commands if e['file'].replace('\\','/').endswith('/lv_mem_core_builtin.c')]
if len(graphics_memory)!=1:raise SystemExit('Expected one LVGL memory implementation')
def profile_definitions(command):
 return dict((name,value or '1') for name,value in re.findall(r'(?:^|\s)-D(\w+)(?:=([^\s]+))?',command))
application_profile=profile_definitions(application[0]['command'])
sdk_config=json.loads((build/'config/sdkconfig.json').read_text(encoding='utf-8'))
main_task_core=1 if sdk_config.get('ESP_MAIN_TASK_AFFINITY_CPU1') else 0 if sdk_config.get('ESP_MAIN_TASK_AFFINITY_CPU0') else None
expected_main_core=0 if sdk_config.get('FREERTOS_UNICORE') else 1
if args.release and main_task_core!=expected_main_core:
 raise SystemExit('UI/main task affinity differs from the SDK core topology; inspect the generated sdkconfig')
graphics_profile=profile_definitions(graphics_memory[0]['command'])
for name in ('XTOUCH_WAVESHARE','XTOUCH_BOARD_CYD','XTOUCH_LANGUAGES_ZH_EN','XTOUCH_SINGLE_PRINTER_LOGO','XTOUCH_UI_PICTURE_PICKER','XTOUCH_UI_HIDPI_ASSETS','XTOUCH_UI_LVGL_BYTES'):
 if application_profile.get(name)!=graphics_profile.get(name):
  raise SystemExit('Application/LVGL profile mismatch: '+name)
passive_sensor_budget=None
if application_profile.get('XTOUCH_COMPACT_DEVICE_CAPACITY')=='1' and application_profile.get('XTOUCH_FEATURE_XIAOMI_BLE')=='1':
 if not sdk_config.get('BT_NIMBLE_ROLE_OBSERVER') or any(sdk_config.get('BT_NIMBLE_'+role) for role in ('ROLE_CENTRAL','ROLE_PERIPHERAL','ROLE_BROADCASTER','GATT_CLIENT','GATT_SERVER')):
  raise SystemExit('Compact passive receiver must use observer-only roles')
 if not sdk_config.get('BT_NIMBLE_MEM_ALLOC_MODE_INTERNAL'):
  raise SystemExit('A board without PSRAM must use internal NimBLE memory')
 passive_sensor_budget={}
 for key in ('BT_NIMBLE_MSYS_1_BLOCK_COUNT','BT_NIMBLE_MSYS_2_BLOCK_COUNT'):
  if sdk_config.get(key)!=4:raise SystemExit('Compact passive buffer budget differs from its board configuration: '+key)
  passive_sensor_budget[key]=4
if application_profile.get('XTOUCH_BOARD_CYD')=='1' and application_profile.get('XTOUCH_FEATURE_XIAOMI_BLE')=='1':
 header=build/'config/sdkconfig.h'
 if not header.is_file():raise SystemExit('CYD passive sensor budget: missing compiled SDK configuration')
 selected=dict(re.findall(r'^#define (CONFIG_BT_NIMBLE_\w+) (\d+)$',header.read_text(encoding='utf-8'),re.M))
 if selected.get('CONFIG_BT_NIMBLE_ROLE_OBSERVER')!='1' or any(selected.get('CONFIG_BT_NIMBLE_'+name)=='1' for name in ('ROLE_CENTRAL','ROLE_PERIPHERAL','ROLE_BROADCASTER','GATT_CLIENT','GATT_SERVER')):
  raise SystemExit('CYD passive sensor budget: observer-only roles required')
 defaults=(pathlib.Path(__file__).resolve().parents[1]/'boards/esp32-2432s028r/sdkconfig.defaults').read_text(encoding='utf-8')
 passive_sensor_budget={}
 for name in ('CONFIG_BT_NIMBLE_MSYS_1_BLOCK_COUNT','CONFIG_BT_NIMBLE_MSYS_2_BLOCK_COUNT'):
  expected=re.search(r'^'+name+r'=(\d+)$',defaults,re.M)
  if not expected or selected.get(name)!=expected[1]:raise SystemExit('CYD passive sensor budget mismatch: '+name)
  passive_sensor_budget[name]=int(expected[1])
project=data.get('project_name','xtouch_waveshare')
image=build/(project+'.bin');linkmap=build/(project+'.map')
if not image.is_file() or image.stat().st_mtime<description.stat().st_mtime:raise SystemExit('Native binary has not been linked after configuration')
from check_cyd_assets import check as check_linked_assets
main_command=application[0]['command']
source_paths=[e['file'].replace('\\','/') for e in commands]
source_names=[path.rsplit('/',1)[-1] for path in source_paths]
duplicate_fonts=sorted(name for name,count in Counter(source_names).items() if count>1 and re.fullmatch(r'ui_font_.*\.c',name))
if duplicate_fonts:raise SystemExit('Duplicate UI font source names: '+', '.join(duplicate_fonts))
if '-DXTOUCH_SINGLE_PRINTER_LOGO=1' in main_command and any(re.search(r'/(images|generated/large)/printer_models\.c$',path) for path in source_paths):
 raise SystemExit('Model artwork compiled in single-logo firmware')
if '-DXTOUCH_FEATURE_CAMERA=0' in main_command and '-DXTOUCH_FEATURE_PRINT_PREVIEW=0' in main_command:
 if any(e['file'].replace('\\','/').endswith('/JPEGDEC/src/JPEGDEC.cpp') for e in commands):
  raise SystemExit('Unused JPEG decoder compiled with both camera and preview disabled')
partition_bytes=(build/'partition_table/partition-table.bin').read_bytes()
ota_slots={}
for offset in range(0,len(partition_bytes)-31,32):
 magic,kind,subtype,address,length,label,flags=struct.unpack_from('<HBBII16sI',partition_bytes,offset)
 if magic==0x50aa and kind==0 and subtype in (0x10,0x11):ota_slots[subtype]=length
if set(ota_slots)!={0x10,0x11}:raise SystemExit('Expected both OTA application slots')
slot_bytes=min(ota_slots.values())
if image.stat().st_size>slot_bytes:raise SystemExit('Application exceeds OTA slot capacity')
elf_path=build/(project+'.elf')
elf_bytes=elf_path.read_bytes();image_bytes=image.read_bytes()
if len(image_bytes)<208 or image_bytes[0]!=0xe9 or image_bytes[32:36]!=bytes.fromhex('3254cdab'):
 raise SystemExit('Invalid native application image descriptor')
if image_bytes[176:208]!=hashlib.sha256(elf_bytes).digest():
 raise SystemExit('Application image does not match audited ELF SHA256')
indicator_drivers=[path for path in source_paths if '/boards/' in path and path.endswith('/indicator.cpp')]
if indicator_drivers:
 section_offset=struct.unpack_from('<I',elf_bytes,32)[0]
 section_size,section_count=struct.unpack_from('<HH',elf_bytes,46)
 sections=[struct.unpack_from('<10I',elf_bytes,section_offset+i*section_size) for i in range(section_count)]
 bindings={}
 for section in sections:
  if section[1]!=2:continue
  strings=sections[section[6]]
  names=elf_bytes[strings[4]:strings[4]+strings[5]]
  for at in range(section[4],section[4]+section[5],section[9]):
   name,address,length,info,other,index=struct.unpack_from('<IIIBBH',elf_bytes,at)
   label=names[name:names.find(b'\0',name)]
   if label in (b'xtouch_board_indicator_begin',b'xtouch_board_indicator_rgb'):bindings[label]=info>>4
 if any(bindings.get(name)!=1 for name in (b'xtouch_board_indicator_begin',b'xtouch_board_indicator_rgb')):
  raise SystemExit('Hardware indicator must link its board driver, not weak defaults')
asset_size='large' if '-DXTOUCH_UI_HIDPI_ASSETS=1' in main_command else 'small'
icon_set='mdi' if '-DXTOUCH_ICON_STYLE=1' in main_command else 'original'
check_linked_assets(build/(project+'.elf'),asset_size,icon_set)
text=linkmap.read_text(encoding='utf-8',errors='replace')
if re.search(r'lib(?:espressif__)?arduino(?:-esp32)?\.a|libPubSubClient\.a|esp32-hal-[\w-]+\.[co]',text,re.I):raise SystemExit('Arduino object/archive found in link map')
laboratory=any(marker in image.read_bytes() for marker in (b'/api/test/', b'[PROV_TEST]'))
if args.release and laboratory:raise SystemExit('Laboratory test API must not enter a release image')

# Validate the bytes actually linked into the image, not just generated headers.
root=pathlib.Path(__file__).resolve().parents[1]
portal_header='portal_assets_zh_en.h' if application_profile.get('XTOUCH_LANGUAGES_ZH_EN')=='1' else 'portal_assets.h'
portal_flags=('XTOUCH_WAVESHARE','XTOUCH_BOARD_CYD','__XTOUCH_SCREEN_28__','XTOUCH_BOARD_S3',
              'XTOUCH_BOARD_P4','XTOUCH_BOARD_ES3C35P','XTOUCH_FEATURE_CLOUD_TOKEN',
              'XTOUCH_FEATURE_CLOUD_LOGIN','XTOUCH_FEATURE_XIAOMI_BLE','XTOUCH_FEATURE_AUDIO',
              'XTOUCH_FEATURE_INDICATOR','XTOUCH_HAS_RGB_LED','XTOUCH_FEATURE_DEEP_SLEEP')
compiler=data.get('c_compiler')
if not compiler:raise SystemExit('Target compiler missing for linked portal verification')
selected=subprocess.run([compiler,'-E','-P','-x','c++','-I'+str(root/'src'),
                         '-DXTOUCH_NATIVE_IDF_TEXT_H=1','-DPROGMEM=',
                         *['-D'+name+'='+application_profile[name] for name in portal_flags if name in application_profile],'-'],
                        input='#include "xtouch/'+portal_header+'"\n',text=True,capture_output=True,check=True)
portal_arrays=re.findall(r'portal_(index|script|style)\[\]\s*=\s*\{([^}]+)\}',selected.stdout)
if len(portal_arrays)!=3:raise SystemExit('Expected three selected portal resource arrays')
for name,values in portal_arrays:
 payload=bytes(int(value,16) for value in re.findall(r'0x[0-9a-fA-F]+',values))
 if not payload or payload not in image_bytes:raise SystemExit('Linked portal resource does not match source/module profile: '+name)
dependencies=json.loads((pathlib.Path(__file__).resolve().parents[1]/'boards/dependencies.json').read_text(encoding='utf-8'))
policy_path=root/'resources/ota_channels.json'
compiled_ota=None
if policy_path.exists():
 policy=json.loads(policy_path.read_text(encoding='utf-8'))
 channels=policy['channels']
 if channels not in (['main'],['main','test']) or policy['repository']!='tbc0309/xTouch-CN':
  raise SystemExit('Unexpected OTA channel policy')
 from release_policy import read as release_identity
 try:release=release_identity(root)
 except ValueError as error:raise SystemExit(str(error))
 if image_bytes[48:80].split(b'\0',1)[0].decode('ascii')!=release['version']:
  raise SystemExit('Application descriptor version differs from the release identity; rebuild required')
 sys.path.insert(0,str(root/'boards'))
 from board_catalog import BOARDS
 matches=[board for board in BOARDS.values() if board['project']==project and board['chip']==data.get('target')]
 if len(matches)!=1:raise SystemExit('Cannot identify exact OTA board from native application')
 board=matches[0]
 if board.get('compactMemory'):
  if application_profile.get('XTOUCH_COMPACT_DEVICE_CAPACITY')!='1' or application_profile.get('XTOUCH_UI_LVGL_BYTES')!='49152':
   raise SystemExit('Compact board memory/capacity flags are missing; reconfigure and rebuild')
 for feature in board.get('disabledFeatures',[]):
  if application_profile.get('XTOUCH_FEATURE_'+feature.upper())!='0':
   raise SystemExit('Unsupported board feature compiled: '+feature)
 if board.get('source'):
  for macro,key in [('XTOUCH_DEEP_WAKE_PIN','deepSleepWakeGpio'),('XTOUCH_SLEEP_BUTTON_PIN','sleepButtonGpio')]:
   if application_profile.get(macro)!=str(board[key]):raise SystemExit('Compiled board capability changed; reconfigure/rebuild: '+macro)
  for macro,supported in [('XTOUCH_FEATURE_INDICATOR',board['statusIndicatorSupported']),('XTOUCH_FEATURE_DEEP_SLEEP',board['deepSleepWakeGpio']>=0)]:
   if not supported and application_profile.get(macro)!='0':raise SystemExit('Unsupported hardware module compiled: '+macro)
 prefix='/'+policy['repository']+'/'
 for channel in ['main','test']:
  path=(prefix+channel+'/boards/'+board['directory']+'/ota.json').encode()
  if (path in image_bytes)!=(channel in channels):raise SystemExit('Linked OTA endpoints differ from source policy; reconfigure and rebuild')
 if ('/'+board['stem']+'.bin').encode() not in image_bytes:raise SystemExit('Linked OTA firmware filename differs from exact board identity')
 compiled_ota={'board':board['directory'],'channels':channels}
report={'lvgl':dependencies['libraries']['lvgl']['ref'],'laboratoryBuild':laboratory,'passed':True,'target':data.get('target'),'components':len(components),'framework':'ESP-IDF','arduinoFramework':False,'pubSubClient':False,'imageBytes':image.stat().st_size,'otaSlotBytes':slot_bytes,'imageMatchesElf':True,'scope':'build dependency, binary/ELF identity, capacity and final link audit; not hardware validation'}
report['applicationLvglProfileMatches']=True
report['indicatorDriverLinked']=bool(indicator_drivers)
report['compiledPortalMatches']=True
report['passiveSensorBudget']=passive_sensor_budget
report['mainTaskCore']=main_task_core
if compiled_ota:report['compiledOtaPolicy']=compiled_ota
if args.report:args.report.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print(json.dumps(report))
