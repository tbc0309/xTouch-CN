"""Run from an ESP-IDF 6.1 terminal: python boards/build.py s3|p4|cyd|es3c35p."""
import argparse,os,pathlib,subprocess,sys,shutil,re
from board_catalog import BOARDS, sdkconfig_defaults
root=pathlib.Path(__file__).resolve().parent
parser=argparse.ArgumentParser();parser.add_argument('board',choices=[*BOARDS,'esp3c35p'])
parser.add_argument('--without',nargs='*',choices=['audio','deep_sleep','indicator','camera','print_preview','xiaomi_ble','cloud_token','cloud_login'],default=[])
parser.add_argument('--external-chamber',action='store_true',help='CYD only: compile optional GPIO22 DS18B20 adapter (default OFF)')
radio=parser.add_mutually_exclusive_group()
radio.add_argument('--ble-provisioning',action='store_true',help='Include optional BLE Wi-Fi provisioning')
radio.add_argument('--xiaomi-ble',action='store_true',help='Include Xiaomi BLE thermometer receiver (default)')
parser.add_argument('--icons',choices=['original','mdi'])
parser.add_argument('--test-automation',action='store_true',help='Laboratory-only automation probes; never include in release images')
parser.add_argument('--provisioning-only',action='store_true',help='Use only provisioning probes; requires --test-automation')
parser.add_argument('--assets',choices=['small','large'],help='Resource size; defaults to small on CYD/S3 and large on P4/ES3C35P')
args=parser.parse_args()
if args.provisioning_only and not args.test_automation:parser.error('--provisioning-only requires --test-automation')
if args.board=='esp3c35p':args.board='es3c35p'
if args.external_chamber and args.board!='cyd':parser.error('--external-chamber requires cyd')
if args.ble_provisioning and args.board not in ('s3','es3c35p'):parser.error('--ble-provisioning supports only s3 and es3c35p')
if args.xiaomi_ble and 'xiaomi_ble' in args.without:parser.error('--xiaomi-ble conflicts with --without xiaomi_ble')
args.xiaomi_ble=not args.ble_provisioning and 'xiaomi_ble' not in args.without
if not os.environ.get('IDF_PATH'):raise SystemExit('Activate ESP-IDF 6.1 first (export.ps1 / export.sh)')
subprocess.run([sys.executable,str(root/'prepare.py')],check=True)
# Generate the matching language/module variants from readable portal sources.
# A changed shared JS file must never leave CYD's embedded portal behind.
subprocess.run([sys.executable,str(root.parent/'scripts/embed_portal.py')]+(['--zh-en'] if args.board=='cyd' else []),check=True)
name=BOARDS[args.board]['directory']
target=BOARDS[args.board]['chip']
build_dir=root/('build-'+args.board+('-ble' if args.ble_provisioning else '-no-sensor' if not args.xiaomi_ble else '')+('-external-chamber' if args.external_chamber else ''))
sdkconfig=build_dir/'sdkconfig'
if not sdkconfig.is_file():sdkconfig=root/name/'sdkconfig'
if True: # Every variant owns its SDK config; omitted BLE must also free controller RAM.
 build_dir.mkdir(exist_ok=True)
 # A fresh checkout has defaults, but no generated ordinary-build sdkconfig yet.
 text=(sdkconfig.read_text(encoding='utf-8') if sdkconfig.is_file() else
       sdkconfig_defaults(args.board, root))
 radio=args.ble_provisioning or args.xiaomi_ble
 values={'BT_ENABLED':'y' if radio else 'n','BT_NIMBLE_ENABLED':'y' if radio else 'n','BT_BLUEDROID_ENABLED':'n','ESP_PROTOCOMM_SUPPORT_SECURITY_VERSION_1':'y'}
 if args.board=='cyd':values['BT_CONTROLLER_ENABLED']='y' if radio else 'n'
 if args.board=='p4':values.update(BT_CONTROLLER_DISABLED='y',BT_NIMBLE_TRANSPORT_UART='n',ESP_HOSTED_HOST_FEAT_BT='y' if radio else 'n')
 for key,value in values.items():
  text=re.sub(r'^(?:CONFIG_'+key+r'=.*|# CONFIG_'+key+r' is not set)\n?', '',text,flags=re.M)
  text+=('CONFIG_'+key+'='+value if value!='n' else '# CONFIG_'+key+' is not set')+'\n'
 # LVGL 9 removed these LVGL 8 settings. Do not carry them from old SDK files.
 text=re.sub(r'^CONFIG_LV_(MEM_SIZE_KILOBYTES|ASSERT_HANDLER_INCLUDE)=.*\n?', '',text,flags=re.M)
 for key in ['LV_BUILD_EXAMPLES','LV_BUILD_DEMOS']:
  text=re.sub(r'^(?:CONFIG_'+key+r'=.*|# CONFIG_'+key+r' is not set)\n?', '',text,flags=re.M)
  text+='# CONFIG_'+key+' is not set\n'
 sdkconfig=build_dir/'sdkconfig'
 if not sdkconfig.is_file() or sdkconfig.read_text(encoding='utf-8')!=text:sdkconfig.write_text(text,encoding='utf-8')
options=['-D','XTOUCH_TEST_PROVISIONING_ONLY='+('ON' if args.provisioning_only else 'OFF'),'-D','XTOUCH_TEST_DEFER_BLE=OFF','-D','XTOUCH_TEST_AUTOMATION='+('ON' if args.test_automation else 'OFF'),'-D','XTOUCH_FEATURE_XIAOMI_BLE='+('ON' if args.xiaomi_ble else 'OFF'),'-D','XTOUCH_FEATURE_BLE_PROVISIONING='+('ON' if args.ble_provisioning else 'OFF')]
options+=['-D','XTOUCH_FEATURE_EXTERNAL_CHAMBER='+('ON' if args.external_chamber else 'OFF')]
for feature in ['audio','deep_sleep','indicator','camera','print_preview','cloud_token','cloud_login']:
 options+=['-D','XTOUCH_FEATURE_'+feature.upper()+'='+('OFF' if feature in args.without or (args.board=='cyd' and feature in ['camera','print_preview']) else 'ON')]
options+=['-D','XTOUCH_ICON_SET='+(args.icons or ''),'-D','XTOUCH_ASSET_SIZE='+(args.assets or ('large' if args.board in ('p4','es3c35p') else 'small'))]
command=[sys.executable,str(pathlib.Path(os.environ['IDF_PATH'])/'tools/idf.py'),'-C',str(root/name),'-B',str(build_dir),'-D','XTOUCH_BOARD='+name,'-D','IDF_TARGET='+target,'-D','SDKCONFIG='+str(sdkconfig)]+options+['build']
subprocess.run(command,check=True)
subprocess.run([sys.executable,str(root.parent/'scripts/check_native_idf.py'),str(build_dir)]+([] if args.test_automation else ['--release']),check=True)
from package import package
package(build_dir,BOARDS[args.board]['stem'])
print('Built',name,'in',build_dir)
