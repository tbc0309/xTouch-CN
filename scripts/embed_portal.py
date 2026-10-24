"""Embed the readable, self-contained portal assets as deterministic gzip blobs."""
from pathlib import Path
import json
import zopfli.gzip
import subprocess
from generate_locales import generate, language_assets
root=Path(__file__).resolve().parents[1]
language_assets()
language_options=''.join('<option value="'+row['code']+'">'+row['name']+'</option>' for row in json.loads((root/'resources/languages.json').read_text('utf-8'))).encode('utf-8')
parts=['#pragma once','#include <Arduino.h>','#include "features/config.h"']
for source,symbol in [('index.html','index'),('style.css','style'),('app.js','script')]:
 raw=(root/'portal'/source).read_bytes()
 if source=='index.html':raw=raw.replace(b'__LANGUAGE_OPTIONS__',language_options).replace(b'__FIRMWARE_VERSION__',json.loads((root/'version.json').read_text('utf-8'))['version'].encode('ascii'))
 if source=='app.js':
  zones=json.loads((root/'resources/timezones/iana-posix.json').read_text('utf-8'))
  clock=b'const XTouchTimezones='+json.dumps(zones,separators=(',',':')).encode()+b';\n'+(root/'portal/clock.js').read_bytes()
  raw=generate()+clock+raw
 if source.endswith('.css'):
  raw=subprocess.run(['node',str(root/'scripts/minify-portal.cjs'),source.rsplit('.',1)[1]],input=raw,capture_output=True,check=True).stdout
 if source=='index.html':
  # Select a fixed HTML variant at compile time. The browser does not probe
  # hardware or wait for /api/info to decide which settings are available.
  for condition,filename in [("#if defined(XTOUCH_WAVESHARE) && defined(XTOUCH_BOARD_S3)","esp32-s3-touch-lcd-2.8-v2.bin"),("#elif defined(XTOUCH_WAVESHARE)","esp32-p4-wifi6-touch-lcd-4.3.bin"),("#else","xtouch-2.8.bin")]:
   parts.append(condition)
   for mask in range(8):
    page=raw.replace(b'__FIRMWARE_FILENAME__',filename.encode('ascii'))
    for bit,element in [(1,b'audioOptions'),(2,b'indicatorOption'),(4,b'deepOptions')]:
     if not mask&bit:page=page.replace(b'id="'+element+b'"',b'id="'+element+b'" hidden')
    data=zopfli.gzip.compress(page,numiterations=50)
    parts.append('#if (XTOUCH_FEATURE_AUDIO + 2*(XTOUCH_FEATURE_INDICATOR && XTOUCH_HAS_RGB_LED) + 4*XTOUCH_FEATURE_DEEP_SLEEP) == '+str(mask))
    parts.append('static const uint8_t portal_index[] PROGMEM = {\n'+','.join(hex(v) for v in data)+'\n};\n#endif')
  parts.append("#endif")
 elif source=='app.js':
  # Omitted authentication modules never enter the generated browser resource.
  for mask in range(4):
   token_enabled=bool(mask&1); login_enabled=bool(mask&2)
   prefix=('const XTouchTokenEntry='+str(token_enabled).lower()+';const XTouchAccountLogin='+str(login_enabled).lower()+';\n').encode()
   modules=b''
   if token_enabled:modules+=(root/'portal/account-token.js').read_bytes()+b'\n'
   if login_enabled:modules+=(root/'portal/account-login.js').read_bytes()+b'\n'
   parts.append('#if (XTOUCH_FEATURE_CLOUD_TOKEN + 2*XTOUCH_FEATURE_CLOUD_LOGIN) == '+str(mask))
   for sensor in [True,False]:
    parts.append('#if XTOUCH_FEATURE_XIAOMI_BLE' if sensor else '#else')
    page=prefix+raw+modules
    if sensor:page+=b'\n'+(root/'portal/environment.js').read_bytes()
    page=subprocess.run(['node',str(root/'scripts/minify-portal.cjs'),'js'],input=page,capture_output=True,check=True).stdout
    data=zopfli.gzip.compress(page,numiterations=20)
    parts.append('static const uint8_t portal_script[] PROGMEM = {\n'+','.join(hex(v) for v in data)+'\n};')
   parts.append('#endif\n#endif')
 else:
  data=zopfli.gzip.compress(raw,numiterations=50)
  parts.append('static const uint8_t portal_'+symbol+'[] PROGMEM = {\n'+','.join(hex(v) for v in data)+'\n};')

target=root/'src/xtouch/portal_assets.h'
content='\n'.join(parts)+'\n'
if not target.exists() or target.read_text(encoding='utf-8')!=content:
 target.write_text(content,encoding='utf-8')
