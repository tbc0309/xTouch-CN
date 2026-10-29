"""Shared identities for native builds, packaged assets and the web installer."""
import json

BOARDS = {
    'cyd': dict(directory='esp32-2432s028r', stem='xtouch-2.8', chip='esp32', project='xtouch_cyd',
                chip_id=0, family='ESP32', name='CYD · ESP32-2432S028R', flash=4<<20,
                boot_offset=0x1000, app_offset=0x10000, screen=[320,240], hint=None),
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
BY_STEM = {board['stem']: dict(id=key, **board) for key,board in BOARDS.items()}

def browser_module():
    targets={key:dict(stem=b['stem'],manifest='manifest.json' if key=='cyd' else 'manifest-'+key+'.json',
                      screen=b['screen'],hint=b['hint']) for key,b in BOARDS.items()}
    return '// Generated from boards/board_catalog.py.\nexport default '+json.dumps(targets,ensure_ascii=False,separators=(',',':'))+';\n'


def sdkconfig_defaults(board, root):
    """Match each project's default layers when seeding a clean build."""
    paths = [root / BOARDS[board]['directory'] / 'sdkconfig.defaults']
    if board != 'cyd':
        paths.insert(0, root / 'sdkconfig.defaults')
    return '\n'.join(path.read_text(encoding='utf-8') for path in paths)
