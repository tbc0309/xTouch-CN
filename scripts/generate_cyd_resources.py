"""Generate the CYD Chinese/English OPPO Sans subsets and one green Bambu logo.

Requires fontTools, Pillow and lv_font_conv 1.5.3. Other board resources are
never rewritten. Supply the same OPPO Sans weight-600 source as the main build.
"""
import argparse, json, re, subprocess, tempfile
from pathlib import Path
from fontTools.ttLib import TTFont
from fontTools.varLib.instancer import instantiateVariableFont
from PIL import Image

ROOT=Path(__file__).resolve().parents[1]

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--font',type=Path,required=True)
    p.add_argument('--node',default='node')
    p.add_argument('--converter',type=Path,required=True)
    args=p.parse_args()
    rows=json.loads((ROOT/'resources/ui-locales.json').read_text('utf-8'))
    values=[v[i] for v in rows.values() for i in (0,2)]
    # Keep only live source literals, excluding foreign catalogs, optional full
    # error descriptions and the browser (which uses the browser's own fonts).
    for source in (ROOT/'src').rglob('*'):
        if source.suffix not in ('.c','.cpp','.h') or 'fonts' in source.parts:continue
        if source.name in ('portal_assets.h','extra_locales.h','error_descriptions.h'):continue
        if 'i18n' in source.parts and source.name not in ('en.h','zh_cn.h'):continue
        values+=[text for text in re.findall(r'"(?:\\.|[^"\\])*"',source.read_text('utf-8')) if 'Français' not in text]
    codes=set(range(32,127))|{ord(c) for text in values for c in text if 0x4e00<=ord(c)<=0x9fff or 0x3000<=ord(c)<=0x303f or 0xff00<=ord(c)<=0xffef}|{0xb0,0x2103,0xb7,0x2026,0x2014}
    number_codes=set()
    for name in ('text_24','text_28'):
        number_codes|={int(v,16) for v in re.findall(r'/\* U\+([0-9a-fA-F]+)',(ROOT/'src/ui/fonts'/('ui_font_'+name+'.c')).read_text('utf-8'))}
    font=TTFont(args.font)
    if 'fvar' in font:font=instantiateVariableFont(font,{'wght':600},inplace=True)
    if font['OS/2'].usWeightClass!=600:raise ValueError('Expected weight 600')
    missing=codes-set(font.getBestCmap())
    if missing:raise ValueError('Missing OPPO Sans glyphs: '+str(sorted(missing)))
    out=ROOT/'resources/ui-assets/fonts/cyd-zh-en';out.mkdir(parents=True,exist_ok=True)
    names={'text_14':14,'text_24':24,'text_28':28,'portal_title_22':22,'project_16':16}
    with tempfile.TemporaryDirectory(prefix='xtouch-cyd-font-') as tmp:
        primary=Path(tmp)/'oppo.ttf';font.save(primary)
        for suffix,size in names.items():
            original=ROOT/'src/ui/fonts'/('ui_font_'+suffix+'.c')
            old=original.read_text('utf-8');name='ui_font_'+suffix
            # Larger UI headings retain their reviewed glyph roles, intersected
            # with the selected languages; the base font covers all live labels.
            selected=codes if suffix=='text_14' else (codes & {int(v,16) for v in re.findall(r'/\* U\+([0-9a-fA-F]+)',old)})
            if suffix in ('portal_title_22','project_16'):selected|=set(range(32,127))
            command=[args.node,str(args.converter),'--size',str(size),'--bpp','4','--format','lvgl','--no-kerning','--lv-include','ui/ui.h','--lv-font-name',name,'--lv-fallback','ui_font_symbols_14','--font',str(primary),'--range',','.join(hex(c) for c in sorted(selected) if c<128)]
            if any(c>=128 for c in selected):command+=['--symbols',''.join(chr(c) for c in sorted(selected) if c>=128)]
            command+=['--autohint-strong','-o',str(out/(name+'.c'))]
            subprocess.run(command,check=True)
            target=out/(name+'.c');text=target.read_text('utf-8')
            for field in ('line_height','base_line'):
                value=re.search(r'\.'+field+r' = (\d+)',old).group(1)
                text=re.sub(r'\.'+field+r' = \d+','.'+field+' = '+value,text)
            text=re.sub(r'^ \* Opts:.*$',' * CYD Chinese/English subset; OPPO Sans, weight 600.',text,flags=re.M)
            target.write_text(text.rstrip()+'\n',encoding='utf-8');print(name,len(selected),'glyphs')
        # Fitted labels use additional pixel sizes. These must be pruned too;
        # retaining the generic native-size file would keep foreign glyphs.
        old_native=(ROOT/'src/ui/fonts/ui_font_native.c').read_text('utf-8')
        chunks=['#include "ui/ui.h"\n#include "ui/native_fonts.h"\n']
        chunks+=['LV_FONT_DECLARE(ui_font_'+name+');\n' for name in names]
        for name in dict.fromkeys(re.findall(r'const lv_font_t (ui_native_\w+) =',old_native)):
            family,size=re.fullmatch(r'ui_native_(\w+)_(\d+)',name).groups()
            selected=codes if family=='body' else number_codes if family=='number' else set(range(32,127))|number_codes
            if family=='title':selected|={ord(c) for key in ('Scan to join Wi-Fi','Scan to manage','Scan to join Bluetooth') for i in (0,2) for c in rows[key][i]}
            target=Path(tmp)/(name+'.c')
            command=[args.node,str(args.converter),'--size',size,'--bpp','4','--format','lvgl','--no-kerning','--lv-include','ui/ui.h','--lv-font-name',name,'--font',str(primary),'--range',','.join(hex(c) for c in sorted(selected) if c<128)]
            if any(c>=128 for c in selected):command+=['--symbols',''.join(chr(c) for c in sorted(selected) if c>=128)]
            command+=['--autohint-strong','-o',str(target)]
            subprocess.run(command,check=True)
            text=target.read_text('utf-8')
            text=re.sub(r'^ \* Opts:.*$',' * CYD Chinese/English native size; OPPO Sans, weight 600.',text,flags=re.M)
            text=re.sub(r'(?<!\.)\b(glyph_bitmap|glyph_dsc|cmaps|font_dsc|unicode_list_\d+)\b',lambda m:name+'_'+m.group(1),text)
            chunks.append(text)
        chunks.append(old_native[old_native.index('static bool covers('):])
        (out/'ui_font_native.c').write_text('\n'.join(chunks),encoding='utf-8')
    image=Image.open(ROOT/'resources/ui-assets/bambu-logo.png').convert('RGBA')
    # LVGL 9 RGB565A8 stores RGB565 pixels followed by a separate alpha plane.
    green=((0>>3)<<11)|((174>>2)<<5)|(66>>3)
    data=bytearray(value for _ in image.getdata() for value in (green&255,green>>8))
    data.extend(alpha for _,_,_,alpha in image.getdata())
    generated=ROOT/'resources/ui-assets/generated/cyd';generated.mkdir(parents=True,exist_ok=True)
    text='#include "ui/printer_models.h"\nstatic const uint8_t pixels[]={\n'+','.join(hex(c) for c in data)+'\n};\n'
    text+='const lv_img_dsc_t printer_image_bambu_logo={.header={.magic=LV_IMAGE_HEADER_MAGIC,.cf=LV_IMG_CF_TRUE_COLOR_ALPHA,.w='+str(image.width)+',.h='+str(image.height)+',.stride='+str(image.width*2)+'},.data_size=sizeof(pixels),.data=pixels};\n'
    (generated/'bambu_logo.c').write_text(text,encoding='utf-8')

if __name__=='__main__':main()
