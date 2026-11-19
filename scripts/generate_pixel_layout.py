"""Generate the native/browser 32px font and indexed palette from one source."""
from pathlib import Path
import json,re
ROOT=Path(__file__).resolve().parents[1]
def generate():
 d=json.loads((ROOT/'resources/pixel-layout.json').read_text(encoding='utf-8'));assert len(d['palette'])==64
 palette='#pragma once\n#include <stdint.h>\nnamespace pixel_screen {\nstatic constexpr uint8_t palette[][3] = {\n'+''.join('    {'+', '.join(map(str,c))+'},\n' for c in d['palette'])+'};\nstatic constexpr unsigned paletteCount=64, paletteBits=6;\nstatic_assert(sizeof(palette)/sizeof(palette[0])==paletteCount,"Palette size");\n}\n'
 header='#pragma once\n#include <stdint.h>\nnamespace pixel_screen { namespace print_layout {\nstruct Glyph { char ch; uint8_t width, rows[5]; };\nstatic constexpr Glyph font[]={\n'
 for ch,rows in d['glyphs'].items():header+="    {'"+ch+"', "+str(len(rows[0]))+", {"+', '.join(str(int(row,2)) for row in rows)+'}},\n'
 header+='};\nstatic constexpr uint8_t progressRainbow[]={'+', '.join(map(str,d['progressRainbowIndices']))+'};\nstatic constexpr unsigned rainbowBandPixels='+str(d['progressRainbowBandPixels'])+';\nstatic constexpr unsigned frameMs='+str(d['frameMs'])+', headerHold='+str(d['headerHoldFrames'])+', scrollHold='+str(d['scrollHoldFrames'])+', scrollStep='+str(d['scrollPixelFrames'])+', minimumCycleFrames='+str(d['minimumCycleFrames'])+';\nenum : uint8_t { '+', '.join(k+'='+str(v) for k,v in d['colors'].items())+' };\n} }\n'
 for name,text in [('src/features/pixel_screen/palette.h',palette),('src/features/pixel_screen/layout_data.h',header)]:
  p=ROOT/name
  if not p.exists() or p.read_text(encoding='utf-8')!=text:p.write_text(text,encoding='utf-8')
 p=ROOT/'portal/pixel-art.js';s=p.read_text(encoding='utf-8');block='// BEGIN GENERATED PRINT LAYOUT DATA\nconst layoutGlyphs='+json.dumps(d['glyphs'],separators=(',',':'))+';\nconst layoutColorIds='+json.dumps(d['colors'],separators=(',',':'))+';\nconst layoutPalette='+json.dumps(['#'+''.join(format(v,'02x') for v in c) for c in d['palette']],separators=(',',':'))+';\nconst layoutTiming='+json.dumps({k:d[k] for k in ['frameMs','headerHoldFrames','scrollHoldFrames','scrollPixelFrames','minimumCycleFrames']},separators=(',',':'))+';\nconst layoutRainbow='+json.dumps(d['progressRainbowIndices'])+';\nconst layoutRainbowBandPixels='+str(d['progressRainbowBandPixels'])+';\n// END GENERATED PRINT LAYOUT DATA'
 if '// BEGIN GENERATED PRINT LAYOUT DATA' in s:s=re.sub(r'// BEGIN GENERATED PRINT LAYOUT DATA[\s\S]*?// END GENERATED PRINT LAYOUT DATA',lambda m:block,s)
 else:s=s.replace('let state={};','let state={};\n'+block)
 s=re.sub(r' palette:\[[^\n]*\]',lambda m:' palette:'+json.dumps(['#'+''.join(format(v,'02x') for v in c) for c in d['palette']],separators=(',',':')),s)
 if p.read_text(encoding='utf-8')!=s:p.write_text(s,encoding='utf-8')
if __name__=='__main__':generate()
