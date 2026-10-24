"""Fetch pinned source dependencies; ESP-IDF is installed separately."""
import json,pathlib,subprocess
root=pathlib.Path(__file__).resolve().parent
lock=json.loads((root/'dependencies.json').read_text('utf-8'))
for name,dep in lock['libraries'].items():
 target=root/'.deps'/name
 if not target.exists():
  subprocess.run(['git','clone','--filter=blob:none','--no-checkout',dep['url'],str(target)],check=True)
  subprocess.run(['git','-C',str(target),'checkout','--detach',dep['commit']],check=True)
 actual=subprocess.check_output(['git','-C',str(target),'rev-parse','HEAD'],text=True).strip()
 if actual!=dep['commit']:raise SystemExit(f'{name}: unexpected revision; review local changes before updating')
# Preserve the same negative refinement value without undefined signed left shift.
codec=root/'.deps/JPEGDEC/src/jpeg.inl'
text=codec.read_text('utf-8')
old='iNegative = ((-1) << pJPEG->cApproxBitsLow);'
new='iNegative = -iPositive;'
if old in text:codec.write_text(text.replace(old,new),'utf-8')
elif new not in text:raise SystemExit('JPEGDEC refinement patch context changed; review required')
print('Pinned source dependencies ready')

# Arduino 3.3.12 wraps Hosted with 2.x dependency metadata. The host adapter
# supports Hosted 3.0.10; pin that dependency without editing the global SDK.
manifest=root/'.deps/espressif__arduino-esp32/idf_component.yml'
text=manifest.read_text('utf-8')
import re
text,count=re.subn(r'(  espressif/esp_hosted:\n(?:(?!  [^ ]).+\n)*?    version: )[^\n]+',r'\g<1>==3.0.10',text)
if count!=1:raise SystemExit('Arduino Hosted dependency changed; review compatibility patch')
manifest.write_text(text,'utf-8')
