"""Build large-screen MDI icons and the official SVG logo at native sizes.
Small-screen glyphs retain the original xTouch fonts. Logo paths come from
https://xperiments.in/. MDI mappings and adjusted masters live beside generated assets. Normal firmware builds use generated files.
"""
from pathlib import Path
import re,subprocess,shutil,xml.etree.ElementTree as ET,tempfile,json
from fontTools.fontBuilder import FontBuilder
from fontTools.pens.ttGlyphPen import TTGlyphPen
from fontTools.pens.cu2quPen import Cu2QuPen
from fontTools.pens.transformPen import TransformPen
from fontTools.pens.boundsPen import BoundsPen
from fontTools.pens.recordingPen import RecordingPen
from fontTools.svgLib.path import parse_path
ROOT=Path(__file__).resolve().parents[1];ASSETS=ROOT/'resources/ui-assets';SOURCE=ASSETS/'original';LARGE=ASSETS/'generated/large';LARGE.mkdir(parents=True,exist_ok=True)
codes=[32]+list(range(48,54))+list(range(97,123));names={c:'g'+str(c) for c in codes};fb=FontBuilder(1024,isTTF=True);fb.setupGlyphOrder(['.notdef']+list(names.values()));glyf={'.notdef':TTGlyphPen(None).glyph()};metrics={'.notdef':(1024,0)}
def paths(path,pen,transform=None,x_only=False):
 for item in ET.parse(path).getroot().iter():
  if not item.tag.endswith('path'):continue
  tr=item.get('transform',''); m=re.search(r'matrix\(([^)]+)\)',tr)
  if m: matrix=tuple(map(float,re.split(r'[ ,]+',m.group(1).strip())))
  else:
   m=re.search(r'translate\(([-\d.]+)[ ,]+([-\d.]+)\)',tr);x,y=map(float,m.groups()) if m else (0,0);matrix=(1,0,0,1,x,y)
  target=TransformPen(pen,matrix)
  if not x_only:parse_path(item.get('d'),target)
  else:
   recording=RecordingPen();parse_path(item.get('d'),recording);contour=[]
   for command,args in recording.value:
    contour.append((command,args))
    if command in ('closePath','endPath'):
     part=RecordingPen();part.value=contour;bounds=BoundsPen(None);part.replay(bounds)
     if bounds.bounds and bounds.bounds[2]<35:part.replay(target)
     contour=[]
mapping=json.loads((ASSETS/'mdi-map.json').read_text('utf-8'))
for code,name in names.items():
 pen=TTGlyphPen(None)
 if code==32:advance=512
 else:
  path=SOURCE/'logo.svg' if code in (52,53) else ASSETS/'candidates/mdi'/(mapping[chr(code)]+'.svg');bounds=BoundsPen(None);paths(path,bounds);x1,y1,x2,y2=bounds.bounds;w,h=x2-x1,y2-y1
  scale=900/h if code in (52,53) else 1;advance=round(w*scale)+124 if code in (52,53) else 1024
  target=TransformPen(Cu2QuPen(pen,.5,reverse_direction=False),(scale,0,0,-scale,(advance-w*scale)/2-x1*scale,512+(y1+y2)*scale/2));paths(path,target,x_only=code==53)
 glyph=pen.glyph();glyph.recalcBounds(None);glyf[name]=glyph
 # Preserve the SVG side bearing. Zeroing it shifts every glyph left during
 # FreeType rasterization even though its SVG outline is centered.
 metrics[name]=(advance,glyph.xMin if glyph.numberOfContours else 0)
fb.setupGlyf(glyf);fb.setupHorizontalMetrics(metrics);fb.setupHorizontalHeader(ascent=1024,descent=0);fb.setupCharacterMap({c:n for c,n in names.items()});fb.setupNameTable({'familyName':'xTouch Shared MDI','styleName':'Regular','uniqueFontIdentifier':'xTouch Shared MDI outline family','fullName':'xTouch Shared MDI','psName':'xTouchSharedMDI'});fb.setupOS2(sTypoAscender=1024,sTypoDescender=0,usWinAscent=1024,usWinDescent=0);fb.setupPost();font=ASSETS/'mdi.ttf';fb.save(font)
runner=[shutil.which('npx.cmd') or shutil.which('npx'),'--yes','lv_font_conv@1.5.3']
for out,name,size,subset in [(ROOT/'src/ui/fonts','ui_font_xperiments',32,[52,53]),(LARGE,'ui_font_xlcd',48,codes),(LARGE,'ui_font_xlcdmin',32,codes),(LARGE,'ui_font_actions',96,list(map(ord,'01wz'))),(LARGE,'ui_font_xperiments',96,[52,53])]:
 dest=out/(name+'.c');subprocess.run(runner+['--font',str(font),'--size',str(size),'--bpp','4','--format','lvgl','--range',','.join(hex(c) for c in subset),'--no-kerning','--no-compress','--lv-include','lvgl.h','--lv-font-name',name,'-o',str(dest)],check=True)
 s=dest.read_text('utf-8');s=re.sub(r'^ \* Opts:.*$',' * Shared MDI icons and official SVG logo; scripts/generate_ui_assets.py.',s,flags=re.M);s=re.sub(r'\.line_height = \d+',f'.line_height = {size}',s);s=re.sub(r'\.base_line = \d+','.base_line = 0',s);dest.write_text(s,encoding='utf-8');print('Generated',name,size,flush=True)

for name in ['ui_font_xlcd','ui_font_xlcdmin']:
 shutil.copy2(ASSETS/'legacy'/(name+'.c'),ROOT/'src/ui/fonts'/(name+'.c'))

# Update P4 heat symbols only; retain the original small-screen icons.
from generate_toolhead_icons import generate as generate_toolheads
generate_toolheads()
