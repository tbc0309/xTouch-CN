"""Shared identities for native builds, packaged assets and the web installer."""
import json
from pathlib import Path

BOARDS = {
    'cyd': dict(directory='esp32-2432s028r', stem='xtouch-2.8', chip='esp32', project='xtouch_cyd',
                chip_id=0, family='ESP32', name='CYD · ESP32-2432S028R · ILI9341', flash=4<<20,
                boot_offset=0x1000, app_offset=0x10000, screen=[320,240], hint=None),
    'cyd-st7789': dict(directory='esp32-2432s028r-st7789', stem='xtouch-2.8-st7789', chip='esp32', project='xtouch_cyd_st7789',
                chip_id=0, family='ESP32', name='CYD · ESP32-2432S028R · ST7789', flash=4<<20,
                boot_offset=0x1000, app_offset=0x10000, screen=[320,240],
                hint=['仅适用于 CYD 的 ST7789 屏幕版本、XPT2046 电阻触摸及4MB Flash；不可使用 ILI9341 固件。','For the CYD ST7789 panel with XPT2046 resistive touch and 4 MB Flash. Do not use the ILI9341 firmware.'],
                releaseSupported=True, sdkconfigBase='esp32-2432s028r'),
    's3': dict(directory='esp32-s3-touch-lcd-2.8-v2', stem='esp32-s3-touch-lcd-2.8-v2', chip='esp32s3', project='xtouch_waveshare',
               chip_id=9, family='ESP32-S3', name='Waveshare · ESP32-S3-Touch-LCD-2.8 V2', flash=16<<20,
               boot_offset=0, app_offset=0x20000, screen=[320,240],
               hint=['仅适用于 S3 2.8 英寸 V2 / CST3530，不适用于 V1 或其它后缀型号。','For S3 2.8-inch V2 / CST3530 only, not V1 or other board variants.']),
    'p4': dict(directory='esp32-p4-wifi6-touch-lcd-4.3', stem='esp32-p4-wifi6-touch-lcd-4.3', chip='esp32p4', project='xtouch_waveshare',
               chip_id=18, family='ESP32-P4', name='Waveshare · ESP32-P4-WIFI6-Touch-LCD-4.3', flash=32<<20,
               boot_offset=0x2000, app_offset=0x20000, screen=[800,480],
               hint=['仅适用于所选 P4 4.3 英寸主板，适用 ESP32-P4 芯片版本 v3.x（最低 v3.0），不适用于早期 v1.x 版本。','For this P4 4.3-inch board only. Supports ESP32-P4 chip revision v3.x (minimum v3.0), not earlier v1.x revisions.']),
    'es3c35p': dict(directory='lcdwiki-es3c35p', stem='lcdwiki-es3c35p', chip='esp32s3', project='xtouch_es3c35p',
                    chip_id=9, family='ESP32-S3', name='LCDWIKI · ES3C35P', flash=16<<20,
                    boot_offset=0, app_offset=0x20000, screen=[480,320],
                    hint=['仅适用于 LCDWIKI ES3C35P 3.5 英寸 ST77922 主板。','For LCDWIKI ES3C35P 3.5-inch ST77922 boards only.']),
}
BOARDS['amoled175'] = dict(directory='esp32-s3-touch-amoled-1.75', stem='esp32-s3-touch-amoled-1.75', chip='esp32s3', project='xtouch_amoled175', chip_id=9, family='ESP32-S3', name='Waveshare · ESP32-S3-Touch-AMOLED-1.75', flash=16<<20, boot_offset=0, app_offset=0x20000, screen=[466,466], hint=['仅适用于 CO5300 / CST9217 圆屏主板。','For the CO5300 / CST9217 round board only.'], assetSize='large', layout='round', releaseSupported=True)
BUILTIN_TARGETS = tuple(BOARDS)
for profile in BOARDS.values():
    profile.setdefault('releaseChannel', 'main')


def extend_catalog(catalog, additions, exclusions=()):
    """Add board metadata without allowing a port to replace another identity."""
    import re
    result = dict(catalog)
    for key, source in additions.items():
        if not isinstance(key, str) or not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9_.-]*', key):
            raise ValueError('Invalid board key: ' + str(key))
        if key in exclusions or source.get('candidateId') in exclusions:
            continue
        if key in result:
            raise ValueError('Duplicate board target: ' + key)
        profile = dict(source)
        for field in ('directory', 'stem', 'project', 'chip'):
            if not isinstance(profile.get(field), str) or not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9_.-]*', profile[field]):
                raise ValueError('Invalid board identity: ' + key + '/' + field)
        if profile['directory'] in ('.', '..') or profile['stem'] in ('.', '..'):
            raise ValueError('Invalid board path: ' + key)
        if not isinstance(profile.get('flash'), int) or profile['flash'] <= 0 or not isinstance(profile.get('chip_id'), int):
            raise ValueError('Missing Flash/chip identity: ' + key)
        screen = profile.get('screen')
        if not isinstance(screen, list) or len(screen) != 2 or any(not isinstance(v, int) or v <= 0 for v in screen):
            raise ValueError('Expected a two-dimensional screen size: ' + key)
        for flag in ('releaseSupported', 'localOnly', 'nativeBuildAllowed'):
            if flag in profile and not isinstance(profile[flag], bool):
                raise ValueError('Expected a boolean board capability: ' + key + '/' + flag)
        profile.setdefault('releaseChannel', 'test')
        if profile['releaseChannel'] not in ('main', 'test'):
            raise ValueError('Invalid board release channel: ' + key)
        if any(p['directory'] == profile['directory'] or p['stem'] == profile['stem'] or
               (p['project'], p['chip']) == (profile['project'], profile['chip']) for p in result.values()):
            raise ValueError('Ambiguous board artifact identity: ' + key)
        if profile.get('assetSize') not in (None, 'small', 'large'):
            raise ValueError('Invalid resource profile: ' + key)
        if profile.get('layout') not in (None, 'landscape', 'round', 'square'):
            raise ValueError('Invalid screen layout: ' + key)
        if profile.get('layout') in ('round', 'square') and screen[0] != screen[1]:
            raise ValueError('Round/square layout requires equal logical dimensions: ' + key)
        result[key] = profile
    return result


exclusion_path = Path(__file__).with_name('target_exclusions.json')
EXCLUDED_TARGETS = json.loads(exclusion_path.read_text(encoding='utf8')) if exclusion_path.exists() else {}
for catalog_name in ('port_targets.json', 'bsp_targets.json'):
    path = Path(__file__).with_name(catalog_name)
    if path.exists():
        BOARDS = extend_catalog(BOARDS, json.loads(path.read_text('utf8')), EXCLUDED_TARGETS)
BY_STEM = {board['stem']: dict(id=key, **board) for key,board in BOARDS.items()}

def browser_module():
    targets={key:dict(stem=b['stem'],manifest='manifest.json' if key=='cyd' else 'manifest-'+key+'.json',
                      screen=b['screen'],hint=b['hint']) for key,b in BOARDS.items() if b.get('releaseSupported',True)}
    return '// Generated from boards/board_catalog.py.\nexport default '+json.dumps(targets,ensure_ascii=False,separators=(',',':'))+';\n'


def sdkconfig_defaults(board, root):
    """Match each project's default layers when seeding a clean build."""
    paths = [root / BOARDS[board]['directory'] / 'sdkconfig.defaults']
    if BOARDS[board].get('sdkconfigBase'):
        paths.insert(0, root / BOARDS[board]['sdkconfigBase'] / 'sdkconfig.defaults')
    if board not in ('cyd', 'cyd-st7789'):
        paths.insert(0, root / 'sdkconfig.defaults')
    return '\n'.join(path.read_text(encoding='utf-8') for path in paths)
