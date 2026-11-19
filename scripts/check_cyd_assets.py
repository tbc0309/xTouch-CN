"""Reject a CYD ELF with the wrong linked icon resources, before packaging."""
from pathlib import Path
import struct
import sys
import re


def check(path, asset_size="small", icon_set="original"):
    if asset_size not in ("small","large") or icon_set not in ("original","mdi"):
        raise ValueError("Invalid requested asset profile")
    root=Path(__file__).resolve().parents[1]/"resources/ui-assets"
    folder=root/("legacy" if asset_size=="small" and icon_set=="original" else "generated/mdi/small" if asset_size=="small" else "generated/original/large" if icon_set=="original" else "generated/large")
    data = Path(path).read_bytes()
    if data[:6] != b'\x7fELF\x01\x01':
        raise ValueError('Expected a little-endian ELF32 firmware')
    offset = struct.unpack_from('<I', data, 32)[0]
    size, count = struct.unpack_from('<HH', data, 46)
    sections = [struct.unpack_from('<10I', data, offset+i*size) for i in range(count)]
    def linked_bytes(address, length):
        for section in sections:
            if section[1]!=8 and section[3]<=address and address+length<=section[3]+section[5]:
                at=section[4]+address-section[3]
                return data[at:at+length]
        raise ValueError('Icon data points outside file-backed ELF sections')
    symbols = {}
    for section in sections:
        if section[1] != 2:
            continue
        strings = sections[section[6]]
        names = data[strings[4]:strings[4]+strings[5]]
        for at in range(section[4], section[4]+section[5], section[9]):
            name, address, length, info, other, index = struct.unpack_from('<IIIBBH', data, at)
            label = names[name:names.find(b'\0', name)].decode('utf-8')
            symbols[label] = (address, index)
    for name in ['ui_font_xlcd','ui_font_xlcdmin']:
        reference=folder/(name+'.c')
        reference_source=reference.read_text('utf-8')
        expected=tuple(int(re.search(r'\.'+key+r'\s*=\s*(-?\d+)',reference_source)[1]) for key in ['line_height','base_line'])
        address, index = symbols[name]
        section = sections[index]
        at = section[4]+address-section[3]
        # LVGL 9.6: dsc/fallback/user_data and three callbacks precede int32 metrics.
        actual = struct.unpack_from('<ii', data, at+24)
        if actual != expected:
            raise ValueError(f'CYD {name}: linked metrics {actual}, expected {asset_size}/{icon_set} {expected}')
        # Compare the linked pixels, not just a matching size or symbol name.
        font_dsc=struct.unpack_from('<I',data,at)[0]
        bitmap=struct.unpack('<I',linked_bytes(font_dsc,4))[0]
        source=reference.read_text('utf-8').split('glyph_bitmap[]',1)[1].split('{',1)[1].split('};',1)[0]
        source=re.sub(r'/\*.*?\*/','',source,flags=re.S)
        pixels=bytes(int(v,16) for v in re.findall(r'0x([0-9a-fA-F]+)',source))
        if linked_bytes(bitmap,len(pixels))!=pixels:
            raise ValueError(f'CYD {name}: linked glyph pixels differ from the original small-screen resource')
    if asset_size=='small' and 'ui_font_actions' in symbols:
        raise ValueError('CYD contains the large-screen action font')
    print(f'PASS requested {asset_size}/{icon_set} icon metrics and glyph pixels')


if __name__ == '__main__':
    check(sys.argv[1],*sys.argv[2:])
