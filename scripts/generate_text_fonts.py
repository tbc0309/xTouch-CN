"""Regenerate UI text font subsets. Source font files are not redistributed."""
import argparse, re, shutil, subprocess, tempfile, sys, json
from pathlib import Path
from fontTools.ttLib import TTFont
from fontTools.varLib.instancer import instantiateVariableFont

ROOT=Path(__file__).resolve().parents[1]

def ranges(codes):
    values=sorted(set(codes));result=[]
    if not values:return ''
    start=last=values[0]
    for value in values[1:]:
        if value==last+1:last=value;continue
        result.append(hex(start) if start==last else hex(start)+'-'+hex(last));start=last=value
    result.append(hex(start) if start==last else hex(start)+'-'+hex(last))
    return ','.join(result)

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--font',type=Path,required=True)
    parser.add_argument('--weight',type=int,default=600)
    parser.add_argument('--error-details-only',action='store_true',help='Refresh optional full error-description subsets only')
    parser.add_argument('--titles-only',action='store_true',help='Refresh QR headings and regenerate native subsets')
    parser.add_argument('--fallback-font',type=Path,action='append',required=True)
    args=parser.parse_args()
    font=TTFont(args.font)
    if 'fvar' in font:
        axes={axis.axisTag:axis for axis in font['fvar'].axes}
        if 'wght' not in axes or not axes['wght'].minValue<=args.weight<=axes['wght'].maxValue:parser.error('Unsupported font weight')
        font=instantiateVariableFont(font,{'wght':args.weight},inplace=True)
    if font['OS/2'].usWeightClass!=args.weight:parser.error('Primary font must match the requested weight')
    for candidate in args.fallback_font:
        if TTFont(candidate)['OS/2'].usWeightClass!=args.weight:parser.error('Fallback fonts must be instantiated at the requested weight')
    small=ROOT/'src/ui/fonts';large=ROOT/'resources/ui-assets/fonts/large'
    targets=[small/(name+'.c') for name in ['ui_font_text_14','ui_font_text_24','ui_font_text_28','ui_font_portal_title_22']]
    targets+=[small/'ui_font_project_16.c']
    targets+=[ROOT/'resources/ui-assets/fonts/small/ui_font_camera_14.c',large/'compact/ui_font_text_14.c']
    targets += [ROOT/'resources/ui-assets/fonts/small/ui_font_error_14.c',large/'ui_font_text_14.c']
    targets += [large/(name+'.c') for name in ['ui_font_text_24','ui_font_text_28','ui_font_portal_title_22','ui_font_camera_14']]
    rows=json.loads((ROOT/'resources/ui-locales.json').read_text('utf-8'))
    heading_keys=['Scan to join Wi-Fi','Scan to manage','Scan to join Bluetooth']
    heading_codes={ord(ch) for key in heading_keys for value in rows[key] for ch in value if ord(ch)>=32}
    if args.titles_only:targets=[target for target in targets if target.name=='ui_font_portal_title_22.c']
    if args.error_details_only:targets=[large/'ui_font_text_14.c',ROOT/'resources/ui-assets/fonts/small/ui_font_error_14.c']
    npx=shutil.which('npx.cmd') or shutil.which('npx')
    if not npx:parser.error('Node.js/npx is required')
    # Include current translations when rebuilding the main subset.
    source='\n'.join(p.read_text('utf-8-sig') for p in (ROOT/'src').rglob('*') if p.suffix in ('.c','.cpp','.h') and 'fonts' not in p.parts and p.name!='portal_assets.h')
    literals=[token for token in re.findall(r'"(?:\\.|[^"\\])*"|/\*[\s\S]*?\*/|//[^\n]*',source) if token.startswith('"')]
    ui_codes=set(range(32,127))|{ord(ch) for literal in literals for ch in literal if ord(ch)>=160 and ch.isprintable()}
    descriptions=(ROOT/'src/xtouch/error_descriptions.h').read_text('utf-8')
    error_codes={ord(ch) for token in re.findall(r'\"(?:\\.|[^\"\\])*\"',descriptions) for ch in json.loads(token) if ord(ch)>=32}
    with tempfile.TemporaryDirectory(prefix='xtouch-font-') as tmp:
        primary=Path(tmp)/'primary.ttf';font.save(primary)
        paths=[primary]+args.fallback_font;maps=[TTFont(p).getBestCmap() for p in paths]
        for target in targets:
            old=target.read_text('utf-8');codes={int(v,16) for v in re.findall(r'/\* U\+([0-9A-Fa-f]+)',old)}
            if target.name in ('ui_font_text_14.c','ui_font_error_14.c'):codes=set(ui_codes)
            if target==large/'ui_font_text_14.c' or target.name=='ui_font_error_14.c':codes|=error_codes
            if target.name=='ui_font_portal_title_22.c':codes|=heading_codes
            # QR headings and project URLs must not mix smaller fallback Latin glyphs.
            if target.name in ('ui_font_portal_title_22.c','ui_font_project_16.c'):codes.update(range(32,127))
            groups=[[] for _ in paths]
            for code in sorted(codes):
                match=next((i for i,cmap in enumerate(maps) if code in cmap),None)
                if match is None:raise ValueError('Missing glyph '+hex(code))
                groups[match].append(code)
            size=int(re.search(r'\* Size: (\d+)',old).group(1));name=re.search(r'const lv_font_t (\w+)\s*=',old).group(1)
            include='../ui.h' if target.parent==small else ('ui/ui.h' if target.name=='ui_font_camera_14.c' and target.parent.name=='small' else 'lvgl.h')
            cmd=[npx,'--yes','lv_font_conv@1.5.3','--size',str(size),'--bpp','4','--format','lvgl','--no-kerning','--lv-include',include,'--lv-font-name',name,'--lv-fallback','ui_font_symbols_'+('28' if 'large' in target.parts else '14')]
            for path,group in zip(paths,groups):
                if group:
                    cmd+=['--font',str(path)]
                    ascii_codes=[code for code in group if code<128]
                    if ascii_codes:cmd+=['--range',ranges(ascii_codes)]
                    # Literal non-ASCII symbols avoid Windows' batch-command
                    # length limit for large, sparse CJK subsets.
                    symbols=''.join(chr(code) for code in group if code>=128)
                    if symbols:cmd+=['--symbols',symbols]
                    cmd+=['--autohint-strong']
            subprocess.run(cmd+['-o',str(target)],check=True)
            text=target.read_text('utf-8');text=re.sub(r'^ \* Opts:.*$',' * UI text subset, weight '+str(args.weight)+'; generated by scripts/generate_text_fonts.py.',text,flags=re.M)
            for field in ([] if target.name=='ui_font_project_16.c' else ['line_height','base_line']):
                value=re.search(r'\.'+field+r' = (\d+)',old).group(1)
                text=re.sub(r'\.'+field+r' = \d+','.'+field+' = '+value,text)
            target.write_text(text.rstrip()+'\n',encoding='utf-8')
            print(target.relative_to(ROOT),len(codes),'glyphs')

    if args.error_details_only:return
    fallback_args=[arg for path in args.fallback_font for arg in ('--fallback-font',str(path))]
    subprocess.run([sys.executable,str(ROOT/'scripts/generate_native_fonts.py'),'--font',str(args.font),*fallback_args,'--weight',str(args.weight)],check=True)

if __name__=='__main__':main()
