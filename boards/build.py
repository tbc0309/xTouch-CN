"""Run from an ESP-IDF 6.1 terminal: python boards/build.py s3|p4."""
import argparse,os,pathlib,subprocess,sys,shutil,re
root=pathlib.Path(__file__).resolve().parent
parser=argparse.ArgumentParser();parser.add_argument('board',choices=['s3','p4'])
parser.add_argument('--without',nargs='*',choices=['audio','deep_sleep','indicator','camera','print_preview'],default=[])
parser.add_argument('--ble-provisioning',action='store_true',help='Include optional BLE Wi-Fi provisioning')
parser.add_argument('--c6-preinitialized-bt',action='store_true',help='P4: use a C6 controller initialized by its firmware')
parser.add_argument('--icons',choices=['original','mdi'])
args=parser.parse_args()
if args.c6_preinitialized_bt and (args.board!='p4' or not args.ble_provisioning):parser.error('--c6-preinitialized-bt requires p4 --ble-provisioning')
if not os.environ.get('IDF_PATH'):raise SystemExit('Activate ESP-IDF 6.1 first (export.ps1 / export.sh)')
subprocess.run([sys.executable,str(root/'prepare.py')],check=True)
name='esp32-s3-touch-lcd-2.8-v2' if args.board=='s3' else 'esp32-p4-wifi6-touch-lcd-4.3'
target='esp32s3' if args.board=='s3' else 'esp32p4'
build_dir=root/('build-'+args.board+('-ble' if args.ble_provisioning else ''))
sdkconfig=root/name/'sdkconfig'
if args.ble_provisioning:
 build_dir.mkdir(exist_ok=True)
 # A fresh checkout has defaults, but no generated ordinary-build sdkconfig yet.
 text=(sdkconfig.read_text(encoding='utf-8') if sdkconfig.is_file() else
       (root/'sdkconfig.defaults').read_text(encoding='utf-8')+'\n'+
       (root/name/'sdkconfig.defaults').read_text(encoding='utf-8'))
 values={'BT_ENABLED':'y','BT_NIMBLE_ENABLED':'y','BT_BLUEDROID_ENABLED':'n','ESP_PROTOCOMM_SUPPORT_SECURITY_VERSION_1':'y'}
 if args.board=='p4':values.update(BT_CONTROLLER_DISABLED='y',BT_NIMBLE_TRANSPORT_UART='n',ESP_HOSTED_ENABLE_BT_NIMBLE='y',ESP_HOSTED_NIMBLE_HCI_VHCI='y')
 for key,value in values.items():
  text=re.sub(r'^(?:CONFIG_'+key+r'=.*|# CONFIG_'+key+r' is not set)\n?', '',text,flags=re.M)
  text+=('CONFIG_'+key+'='+value if value!='n' else '# CONFIG_'+key+' is not set')+'\n'
 sdkconfig=build_dir/'sdkconfig'
 sdkconfig.write_text(text,encoding='utf-8')
options=['-D','XTOUCH_FEATURE_BLE_PROVISIONING='+('ON' if args.ble_provisioning else 'OFF'),'-D','XTOUCH_P4_C6_PREINITIALIZED_BT='+('ON' if args.c6_preinitialized_bt else 'OFF')]
for feature in ['audio','deep_sleep','indicator','camera','print_preview']:
 options+=['-D','XTOUCH_FEATURE_'+feature.upper()+'='+('OFF' if feature in args.without else 'ON')]
options+=['-D','XTOUCH_ICON_SET='+(args.icons or '')]
command=[sys.executable,str(pathlib.Path(os.environ['IDF_PATH'])/'tools/idf.py'),'-C',str(root/name),'-B',str(build_dir),'-D','XTOUCH_BOARD='+name,'-D','IDF_TARGET='+target,'-D','SDKCONFIG='+str(sdkconfig)]+options+['build']
subprocess.run(command,check=True)
from package import package
package(build_dir,'esp32-s3-touch-lcd-2.8-v2' if args.board=='s3' else 'esp32-p4-wifi6-touch-lcd-4.3')
print('Built',name,'in',build_dir)
