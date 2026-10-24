"""Generate optional icon-family C resources after the SVG/TTF masters are built."""
from pathlib import Path
import subprocess,shutil
from fontTools.ttLib import TTFont
root=Path(__file__).resolve().parents[1];a=root/'resources/ui-assets';runner=[shutil.which('npx.cmd') or 'npx','--yes','lv_font_conv@1.5.3']
for family,sizeclass,size,mini in [('mdi','small',24,16),('original','large',48,32)]:
 out=a/'generated'/family/sizeclass;out.mkdir(parents=True,exist_ok=True)
 specs=[('xlcd',size),('xlcdmin',mini)]+([('actions',96),('xperiments',96)] if sizeclass=='large' else [])
 for suffix,pixels in specs:
  codes=list(map(ord,'01wz')) if suffix=='actions' else [52,53] if suffix=='xperiments' else [32]+list(range(48,54))+list(range(97,123))
  font=a/('mdi.ttf' if suffix=='xperiments' else family+'.ttf');codes=[c for c in codes if c in TTFont(font).getBestCmap()];name='ui_font_'+suffix
  subprocess.run(runner+['--font',str(font),'--size',str(pixels),'--bpp','4','--format','lvgl','--range',','.join(hex(c) for c in codes),'--no-kerning','--no-compress','--lv-include','lvgl.h','--lv-font-name',name,'-o',str(out/(name+'.c'))],check=True)
 print(family,sizeclass,flush=True)
