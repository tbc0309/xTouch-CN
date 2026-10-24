"""Rasterize shared MDI account badges at native 24/48px sizes.
Requires Pillow and fontTools, like the other UI asset generators.
"""
from pathlib import Path
import re
import tempfile
import xml.etree.ElementTree as ET
from fontTools.fontBuilder import FontBuilder
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.pens.cu2quPen import Cu2QuPen
from fontTools.pens.transformPen import TransformPen
from fontTools.svgLib.path import parse_path
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
MASTERS = ROOT / 'resources/ui-assets/source-icons'
ICONS = {0: 'wifi', 1: 'cloud-key-outline', 2: 'cellphone', 3: 'email-outline'}

def build_font(path):
    glyphs = {'.notdef': TTGlyphPen(None).glyph()}
    metrics = {'.notdef': (1024, 0)}
    for index, name in ICONS.items():
        pen = TTGlyphPen(None)
        target = TransformPen(Cu2QuPen(pen, .5), (1024/24, 0, 0, -1024/24, 0, 1024))
        svg = ET.parse(MASTERS / (name + '.svg')).getroot()
        for element in svg.iter('{http://www.w3.org/2000/svg}path'):
            parse_path(element.attrib['d'], target)
        glyph = pen.glyph()
        glyph.recalcBounds({})
        glyphs[name] = glyph
        metrics[name] = (1024, glyph.xMin)
    font = FontBuilder(1024, isTTF=True)
    font.setupGlyphOrder(list(glyphs))
    font.setupCharacterMap({ord(str(i)): name for i, name in ICONS.items()})
    font.setupGlyf(glyphs)
    font.setupHorizontalMetrics(metrics)
    font.setupHorizontalHeader(ascent=1024, descent=0)
    font.setupNameTable({'familyName': 'Source badges', 'styleName': 'Regular'})
    font.setupOS2(sTypoAscender=1024, sTypoDescender=0, usWinAscent=1024, usWinDescent=0)
    font.setupPost()
    font.save(path)

with tempfile.TemporaryDirectory() as directory:
    font_path = Path(directory) / 'badges.ttf'
    build_font(font_path)
    for size, header in [(24, ROOT/'src/ui/images/source_icons_compact.h'), (48, ROOT/'boards/common/p4_source_icons.h')]:
        text = header.read_text(encoding='utf-8')
        text = re.sub(r'^/\*.*?\*/', '/* Shared source badges; MDI Apache-2.0; scripts/generate_source_icons.py. */', text, count=1, flags=re.S)
        for index in ICONS:
            image = Image.new('L', (size*4, size*4))
            ImageDraw.Draw(image).text((0, size*4), str(index), font=ImageFont.truetype(str(font_path),size*4), fill=255, anchor='ls')
            image = image.resize((size,size), Image.Resampling.LANCZOS)
            assert image.getbbox(), 'Empty badge'
            # Both resolutions retain antialiased alpha edges at their native size.
            pixels = image.tobytes()
            cf = 'LV_IMG_CF_ALPHA_8BIT'
            block = f'static const uint8_t source_bits_{index}[]={{'+','.join(map(str,pixels))+'};\n'
            block += f'static const lv_img_dsc_t source_img_{index}={{.header={{.cf={cf},.w={size},.h={size}}},.data_size={len(pixels)},.data=source_bits_{index}}};'
            pattern = rf'static const uint8_t source_bits_{index}\[\]=\{{.*?\}};\s*static const lv_img_dsc_t source_img_{index}=\{{.*?\}};'
            if re.search(pattern,text,flags=re.S):
                text = re.sub(pattern,lambda _:block,text,flags=re.S)
            else:
                text = text.rstrip()+'\n'+block+'\n'
        header.write_text(text,encoding='utf-8')
print('Generated MDI source badges at 24px and 48px')
