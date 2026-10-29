"""Build a static installer using verified, already-released firmware artifacts."""
import argparse
import hashlib
import json
import shutil
import sys
from pathlib import Path

root = Path(__file__).resolve().parents[1]
sys.path.insert(0,str(root/'boards'))
from partition_layout import read_layout, installer_segments
from board_catalog import BOARDS, BY_STEM, browser_module
from package import validate_identity
parser = argparse.ArgumentParser()
parser.add_argument('--firmware-dir', required=True, type=Path)
parser.add_argument('--output', default='_site', type=Path)
parser.add_argument('--ota-manifest', type=Path,
                    help='Local candidate manifest; defaults to the boards/esp32-2432s028r/ota.json')
parser.add_argument('--board-firmware-dir', action='append', default=[], type=Path,
                    help='Additional supported-board artifacts directory produced by boards/package.py')
parser.add_argument('--allow-partial',action='store_true',help='Development preview only: allow omitted board packages')
args = parser.parse_args()
manifest = json.loads((args.ota_manifest or root / 'boards/esp32-2432s028r/ota.json').read_text('utf-8'))
version = json.loads((root / 'version.json').read_text('utf-8'))['version']
if manifest['version'] != version:
    raise SystemExit('Source version and OTA manifest differ')
app = (args.firmware_dir / 'xtouch-2.8.bin').read_bytes()
validate_identity(app,'xtouch-2.8','esp32','xtouch_cyd')
full = (args.firmware_dir / 'xtouch-2.8-full.bin').read_bytes()
if (len(app) != manifest['size'] or hashlib.md5(app).hexdigest() != manifest['md5']
    or app[48:80].split(b'\0',1)[0] != version.encode('ascii')):
    raise SystemExit('Application size or MD5 mismatch')
if full[0x10000:0x10000 + len(app)] != app or len(full) >= 0x3d0000:
    raise SystemExit('Full image does not match the application or overlaps configuration')
if full[0x1000] != 0xe9 or full[0x8000:0x8002] != b'\xaa\x50':
    raise SystemExit('Bootloader or partition table missing')
source = root / 'web'
if not (source / 'vendor/install-button.js').is_file():
    raise SystemExit('Run npm ci and npm run bundle in web/ first')
output = args.output.resolve()
if output == root or root in output.parents and output.name not in ('_site', 'web-preview'):
    raise SystemExit('Choose a dedicated _site or web-preview output directory')
output.mkdir(parents=True, exist_ok=True)
# Also invalidate cached pages when only the installer dependencies change.
installer_bytes = (source / 'vendor/install-button.js').read_bytes()
installer_name = 'install-button-' + hashlib.sha256(installer_bytes).hexdigest()[:12] + '.js'
(output / 'vendor').mkdir(exist_ok=True)
(output / 'vendor' / installer_name).write_bytes(installer_bytes)
zones = root / 'resources/timezones'
zone_data = json.loads((zones / 'iana-posix.json').read_text('utf-8'))
zone_module = ('export default ' + json.dumps(zone_data, ensure_ascii=False, separators=(',', ':')) + ';\n').encode('utf-8')
zone_name = 'timezones-' + hashlib.sha256(zone_module).hexdigest()[:12] + '.js'
(output / zone_name).write_bytes(zone_module)
shutil.copy2(zones / 'LICENSE', output / 'timezones-LICENSE.txt')
shutil.copy2(zones / 'SOURCE.txt', output / 'timezones-SOURCE.txt')
from generate_locales import language_assets
language_assets()
language_module=(source/'language.js').read_bytes()
language_name='language-'+hashlib.sha256(language_module).hexdigest()[:12]+'.js'
(output/language_name).write_bytes(language_module)
def language_import(text):return text.replace('./language.js','./'+language_name)

generator = language_import((source / 'config-generator.js').read_text('utf-8')).replace('./timezones.js', './' + zone_name).encode('utf-8')
generator_name = 'config-generator-' + hashlib.sha256(generator).hexdigest()[:12] + '.js'
(output / generator_name).write_bytes(generator)
footer = (source / 'page-footer.js').read_bytes()
footer_name = 'page-footer-' + hashlib.sha256(footer).hexdigest()[:12] + '.js'
(output / footer_name).write_bytes(footer)
config_page = language_import((source / 'config-page.js').read_text('utf-8')).replace('./config-generator.js', './' + generator_name).replace('./page-footer.js', './' + footer_name)
config_page_name = 'config-page-' + hashlib.sha256(config_page.encode()).hexdigest()[:12] + '.js'
(output / config_page_name).write_text(config_page, 'utf-8')
catalog = browser_module().encode('utf-8')
catalog_name = 'board-targets-' + hashlib.sha256(catalog).hexdigest()[:12] + '.js'
(output / catalog_name).write_bytes(catalog)
extra_modules={}
for module in ('installer-channel.js','test-board-targets.js'):
    content=(source/module).read_bytes()
    hashed_name=Path(module).stem+'-'+hashlib.sha256(content).hexdigest()[:12]+'.js'
    (output/hashed_name).write_bytes(content)
    extra_modules[module]=hashed_name
app_source = language_import((source / 'app.js').read_text('utf-8')).replace('./vendor/install-button.js', './vendor/' + installer_name).replace('./page-footer.js', './' + footer_name).replace('./board-targets.js','./'+catalog_name)
for module,hashed_name in extra_modules.items():app_source=app_source.replace('./'+module,'./'+hashed_name)
app_name = 'app-' + hashlib.sha256(app_source.encode()).hexdigest()[:12] + '.js'
(output / app_name).write_text(app_source, 'utf-8')
css_source = (source / 'style.css').read_text('utf-8')
css_name = 'style-' + hashlib.sha256(css_source.encode()).hexdigest()[:12] + '.css'
(output / css_name).write_text(css_source, 'utf-8')
html = (source / 'index.html').read_text('utf-8')
html = html.replace('src="app.js"', f'src="{app_name}"').replace('href="style.css"', f'href="{css_name}"')
(output / 'index.html').write_text(html, 'utf-8')
config_html = (source / 'config.html').read_text('utf-8').replace('src="config-page.js"', f'src="{config_page_name}"').replace('href="style.css"', f'href="{css_name}"')
(output / 'config.html').write_text(config_html, 'utf-8')
from build_error_search import build as build_errors
error_html = (source / 'errors.html').read_text('utf-8').replace('href="style.css"', f'href="{css_name}"')
for asset in ('errors.css', 'error-search.js', 'errors.js'):
    content = (source / asset).read_text('utf-8')
    if asset == 'errors.js':
        content = language_import(content).replace('./error-search.js', './' + search_name).replace('./page-footer.js', './' + footer_name)
    path = Path(asset)
    hashed_name = path.stem + '-' + hashlib.sha256(content.encode()).hexdigest()[:12] + path.suffix
    (output / hashed_name).write_text(content, 'utf-8')
    error_html = error_html.replace(asset, hashed_name)
    if asset == 'error-search.js':
        search_name = hashed_name
(output / 'errors.html').write_text(error_html, 'utf-8')
build_errors(output / 'error-codes.json')
shutil.copytree(source / 'node_modules/esp-web-tools', output / 'licenses/esp-web-tools',
                ignore=lambda directory,names: [n for n in names if n not in ('LICENSE','LICENSE.md','LICENSE.txt')],
                dirs_exist_ok=True)
shutil.copytree(source / 'flasher',output / 'licenses/esp-flasher-stub',
                ignore=lambda directory,names:[n for n in names if n not in ('LICENSE-APACHE','LICENSE-MIT','LICENSE.spark-md5','LICENSE.esptool','NOTICE.md')],
                dirs_exist_ok=True)
(output / 'firmware').mkdir(exist_ok=True)
def installer_parts(stem, image):
    # A merged image contains FF padding over NVS. Flashing that padding would
    # erase device-bound credential keys even with "Erase device" unchecked.
    parts=[]
    for index,(offset,data) in enumerate(installer_segments(image)):
        path=f'firmware/{stem}-install-{index}.bin'
        (output/path).write_bytes(data)
        parts.append({'path':path,'offset':offset})
    return parts
shutil.copy2(args.firmware_dir / 'xtouch-2.8-full.bin', output / 'firmware/xtouch-2.8-full.bin')
cyd_metadata = {'version': version, 'fullPath': 'firmware/xtouch-2.8-full.bin',
            'fullSha256': hashlib.sha256(full).hexdigest(), 'size':len(full), 'appUrl':manifest['url']}
metadata = {'version':version,'boards':[{'id':'cyd','name':'CYD · ESP32-2432S028R','manifest':'manifest.json',**cyd_metadata}]}
seen = set()
for folder in args.board_firmware_dir:
    record = json.loads((folder / 'manifest.json').read_text('utf-8'))
    application=record.get('application','')
    target=BY_STEM.get(application[:-4]) if application.endswith('.bin') else None
    if target is None or target['id']=='cyd' or target['chip']!=record.get('chip'):
        raise SystemExit('Unknown board firmware target')
    key,stem,family,title,chip_id,boot_offset=(target[k] for k in ('id','stem','family','name','chip_id','boot_offset'))
    if key in seen or record.get('version') != version:
        raise SystemExit('Duplicate board or mismatched board version')
    seen.add(key)
    board_app = (folder / (stem + '.bin')).read_bytes()
    board_full = (folder / (stem + '-full.bin')).read_bytes()
    validate_identity(board_app,stem,record['chip'],record.get('project'))
    layout=read_layout(board_full[0x8000:0x9000])
    slot=layout['ota_0']['size']
    expected_flash=target['flash']
    if layout['spiffs']['offset']+layout['spiffs']['size']!=expected_flash:
        raise SystemExit('Incorrect Flash partition capacity: '+key)
    if (record.get('application') != stem+'.bin' or record.get('fullImage') != stem+'-full.bin'
        or record.get('applicationOffset') != 0x20000 or record.get('applicationBytes') != len(board_app)
        or hashlib.sha256(board_app).hexdigest() != record.get('sha256')
        or hashlib.sha256(board_full).hexdigest() != record.get('fullSha256')
        or len(board_app) > slot or len(board_app) < 36
        or board_app[0] != 0xe9 or int.from_bytes(board_app[12:14],'little') != chip_id
        or board_app[32:36] != bytes.fromhex('3254cdab')
        or board_app[48:80].split(b'\0',1)[0] != version.encode('ascii')
        or len(board_full) > layout['ota_1']['offset'] or board_full[0x20000:] != board_app
        or board_full[boot_offset] != 0xe9 or board_full[0x8000:0x8002] != b'\xaa\x50'):
        raise SystemExit('Invalid board image, chip, partition layout, or checksum: '+key)
    board_path = 'firmware/'+stem+'-full.bin'
    shutil.copy2(folder/(stem+'-full.bin'),output/board_path)
    board_manifest = {'name':'xTouch-CN · '+title,'version':version,'new_install_prompt_erase':True,
                      'new_install_improv_wait_time':0,'builds':[{'chipFamily':family,'improv':False,
                      'parts':installer_parts(stem,board_full)}]}
    manifest_path = 'manifest-'+key+'.json'
    (output/manifest_path).write_text(json.dumps(board_manifest,indent=2)+'\n','utf-8')
    metadata['boards'].append({'id':key,'name':title,'version':version,'manifest':manifest_path,
                              'fullPath':board_path,'fullSha256':record['fullSha256'],
                              'size':len(board_full),'appUrl':f'https://github.com/tbc0309/xTouch-CN/releases/download/{version}/{stem}.bin'})
if not args.allow_partial and seen!=set(BOARDS)-{'cyd'}:raise SystemExit('Missing supported board packages: '+', '.join(sorted(set(BOARDS)-{'cyd'}-seen)))
web_manifest = {'name':'xTouch-CN','version':version,'new_install_prompt_erase':True,
                'new_install_improv_wait_time':0,'builds':[{'chipFamily':'ESP32','improv':False,
                'parts':installer_parts('xtouch-2.8',full)}]}
for name,value in [('firmware.json',metadata),('manifest.json',web_manifest)]:
    (output / name).write_text(json.dumps(value,indent=2)+'\n','utf-8')
(output / '.nojekyll').write_text('','utf-8')
print(f'Built {version}: verified application, merged image and same-origin installer')
