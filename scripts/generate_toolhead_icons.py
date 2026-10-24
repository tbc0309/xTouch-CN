"""Replace P4 icon glyphs from editable SVG masters without changing other glyphs.

Requires Pillow/fontTools, as do the other UI asset generators. Firmware builds
consume the generated C files and do not run this tool.
"""
from pathlib import Path
import re
import tempfile
import xml.etree.ElementTree as ET
from fontTools.fontBuilder import FontBuilder
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.pens.cu2quPen import Cu2QuPen
from fontTools.pens.transformPen import TransformPen
from fontTools.pens.boundsPen import BoundsPen
from fontTools.svgLib.path import parse_path
from PIL import Image, ImageDraw, ImageFont

ROOT = Path(__file__).resolve().parents[1]
ASSETS = ROOT / "resources/ui-assets"
MASTERS = {"f": "nozzle-heat-below", "e": "bed-heat-above", "p": "printhead-module"}


def build_font(destination):
    glyphs = {".notdef": TTGlyphPen(None).glyph()}
    metrics = {".notdef": (1024, 0)}
    for char, name in MASTERS.items():
        pen = TTGlyphPen(None)
        svg = ET.parse(ASSETS / "source-icons" / (name + ".svg")).getroot()
        paths = list(svg.iter("{http://www.w3.org/2000/svg}path"))
        bounds = BoundsPen(None)
        for item in paths:
            parse_path(item.attrib["d"], bounds)
        x1, y1, x2, y2 = bounds.bounds
        # Match the existing MDI family's 900/1024 visible envelope, not
        # the source SVG canvas. Center the visible shape in the same cell.
        scale = 900 / max(x2-x1, y2-y1)
        target = TransformPen(Cu2QuPen(pen, 0.5),
                              (scale, 0, 0, -scale,
                               512-(x1+x2)*scale/2, 512+(y1+y2)*scale/2))
        for item in paths:
            parse_path(item.attrib["d"], target)
        glyph = pen.glyph()
        glyph.recalcBounds(None)
        glyphs[char] = glyph
        metrics[char] = (1024, glyph.xMin)
    font = FontBuilder(1024, isTTF=True)
    font.setupGlyphOrder(list(glyphs))
    font.setupCharacterMap({ord(char): char for char in MASTERS})
    font.setupGlyf(glyphs)
    font.setupHorizontalMetrics(metrics)
    font.setupHorizontalHeader(ascent=1024, descent=0)
    font.setupNameTable({"familyName": "xTouch toolhead icons", "styleName": "Regular"})
    font.setupOS2(sTypoAscender=1024, sTypoDescender=0, usWinAscent=1024, usWinDescent=0)
    font.setupPost()
    font.save(destination)


def replace_glyphs(path, font_path):
    source = path.read_text("utf-8")
    bpp = int(re.search(r"\.bpp\s*=\s*(\d+)", source)[1])
    assert bpp in (2, 4)
    assert re.search(r"\.bitmap_format\s*=\s*0\b", source)
    bitmap = re.search(r"\bglyph_bitmap\[\]\s*=\s*\{(.*?)\};", source, re.S)
    descriptors = re.search(r"\bglyph_dsc\[\]\s*=\s*\{(.*?)\};", source, re.S)
    pixels = bytes(int(v, 16) for v in re.findall(r"0x([0-9a-fA-F]+)", bitmap[1]))
    rows = [dict((k, int(v)) for k, v in re.findall(r"\.(\w+)\s*=\s*(-?\d+)", row))
            for row in re.findall(r"\{([^{}]*\.bitmap_index[^{}]*)\}", descriptors[1])]
    first = int(re.search(r"\.range_start\s*=\s*97,.*?\.glyph_id_start\s*=\s*(\d+)", source, re.S)[1])
    replacements = {first + ord(char) - 97: char for char in MASTERS}
    height = int(re.search(r"\.line_height\s*=\s*(\d+)", source)[1])
    baseline = int(re.search(r"\.base_line\s*=\s*(\d+)", source)[1])
    packed = bytearray()
    for index, row in enumerate(rows):
        length = (row["box_w"] * row["box_h"] * bpp + 7) // 8
        data = pixels[row["bitmap_index"]:row["bitmap_index"] + length]
        if index in replacements:
            size = round(row["adv_w"] / 16)
            image = Image.new("L", (size, size))
            ImageDraw.Draw(image).text((0, size), replacements[index],
                                      font=ImageFont.truetype(str(font_path), size), fill=255, anchor="ls")
            box = image.getbbox()
            assert box
            levels = [(v * ((1 << bpp)-1) + 127) // 255 for v in image.crop(box).tobytes()]
            group = 8 // bpp
            levels.extend([0] * (-len(levels) % group))
            data = bytes(sum(levels[i+j] << (8-bpp*(j+1)) for j in range(group))
                         for i in range(0, len(levels), group))
            row.update(box_w=box[2]-box[0], box_h=box[3]-box[1], ofs_x=box[0],
                       ofs_y=height-baseline-(height-size)//2-box[3])
        row["bitmap_index"] = len(packed)
        packed.extend(data)
    # Preserve each unrelated glyph byte-for-byte, including its metrics.
    old_rows = [dict((k, int(v)) for k, v in re.findall(r"\.(\w+)\s*=\s*(-?\d+)", row))
                for row in re.findall(r"\{([^{}]*\.bitmap_index[^{}]*)\}", descriptors[1])]
    for index, (old, new) in enumerate(zip(old_rows, rows)):
        if index in replacements:
            continue
        length = (old["box_w"] * old["box_h"] * bpp + 7) // 8
        assert pixels[old["bitmap_index"]:old["bitmap_index"]+length] == packed[new["bitmap_index"]:new["bitmap_index"]+length]
        assert all(old[k] == new[k] for k in old if k != "bitmap_index")
    body = "\n" + "\n".join("    " + ",".join(hex(v) for v in packed[i:i+20]) + ","
                            for i in range(0, len(packed), 20)) + "\n"
    rows_text = "\n" + "\n".join("    {" + ", ".join("."+k+" = "+str(v) for k, v in row.items()) + "}," for row in rows) + "\n"
    updated = source[:descriptors.start(1)] + rows_text + source[descriptors.end(1):]
    updated = updated[:bitmap.start(1)] + body + updated[bitmap.end(1):]
    if updated != source:
        path.write_text(updated, encoding="utf-8")


def generate():
    with tempfile.TemporaryDirectory() as directory:
        font_path = Path(directory) / "toolhead.ttf"
        build_font(font_path)
        for folder in [ASSETS / "generated/large"]:
            for name in ["ui_font_xlcd", "ui_font_xlcdmin"]:
                path = folder / (name + ".c")
                if path.exists():
                    replace_glyphs(path, font_path)


if __name__ == "__main__":
    generate()
