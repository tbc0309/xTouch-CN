"""Generate OPPO Sans 600 numeric/unit glyphs for the shared home status row."""
import argparse
import re
import shutil
import subprocess
import tempfile
from pathlib import Path

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--font', type=Path, required=True, help='OPPO Sans Semibold (600) TTF')
args = parser.parse_args()
npx = shutil.which('npx.cmd') or shutil.which('npx')
if not npx:
    parser.error('Node.js/npx is required')
parts = ['/* OPPO Sans 600; shared home clock and thermometer glyphs, 14/28 px. */\n#pragma once\n#include "features/ui_profile.h"\n']
with tempfile.TemporaryDirectory(prefix='xtouch-status-font-') as temp:
    for size, sensor in ((14, True), (28, True), (14, False), (28, False)):
        output = Path(temp) / f'status-{size}.c'
        subprocess.run([npx, '--yes', 'lv_font_conv@1.5.3', '--font', str(args.font),
                        '--size', str(size), '--range', ('0x20,0x25,0x2d-0x2e,0x30-0x3a,0x7e,0x2103' if sensor else '0x30-0x3a'), '--bpp', '4',
                        '--format', 'lvgl', '--autohint-strong', '--no-kerning', '--lv-include', 'lvgl.h',
                        '--lv-font-name', f'ui_font_home_status_{size}', '-o', str(output)], check=True)
        text = re.sub(r'/\*\*.*?\*/', '', output.read_text('utf-8'), count=1, flags=re.S)
        names = {'glyph_bitmap', 'glyph_dsc', 'cmaps', 'font_dsc', 'cache'}
        names.update(re.findall(r'\b(?:unicode_list_\d+|glyph_id_ofs_list_\d+)\b', text))
        for name in names:
            text = re.sub(r'(?<!\.)\b' + name + r'\b', name + '_' + str(size), text)
        profile = 'XTOUCH_UI_HIDPI_ASSETS' if size == 28 else '!XTOUCH_UI_HIDPI_ASSETS'
        feature = 'XTOUCH_FEATURE_XIAOMI_BLE' if sensor else '!XTOUCH_FEATURE_XIAOMI_BLE'
        parts.append(f'#if ({profile}) && ({feature})\n' + text + '\n#endif\n')
(root / 'src/ui/home_status_font.h').write_text('\n'.join(parts), encoding='utf-8')
