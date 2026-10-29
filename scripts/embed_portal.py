"""Embed the readable, self-contained portal assets as deterministic gzip blobs."""
from pathlib import Path
import json, sys
limited="--zh-en" in sys.argv
import zopfli.gzip
import subprocess, shutil
from html.parser import HTMLParser


class PanelSelector(HTMLParser):
 """Remove disabled panels at build time, including nested controls."""
 void_tags={'area','base','br','col','embed','hr','img','input','link','meta','param','source','track','wbr'}
 def __init__(self,disabled):
  super().__init__(convert_charrefs=False)
  self.disabled=disabled;self.depth=0;self.parts=[]
 def handle_starttag(self,tag,attrs):
  if self.depth:
   if tag not in self.void_tags:self.depth+=1
  elif dict(attrs).get('id') in self.disabled:
   if tag not in self.void_tags:self.depth=1
  else:self.parts.append(self.get_starttag_text())
 def handle_startendtag(self,tag,attrs):
  if not self.depth and dict(attrs).get('id') not in self.disabled:self.parts.append(self.get_starttag_text())
 def handle_endtag(self,tag):
  if self.depth:
   if tag not in self.void_tags:self.depth-=1
  else:self.parts.append('</'+tag+'>')
 def handle_data(self,data):
  if not self.depth:self.parts.append(data)
 def handle_entityref(self,name):self.handle_data('&'+name+';')
 def handle_charref(self,name):self.handle_data('&#'+name+';')
 def handle_decl(self,decl):self.handle_data('<!'+decl+'>')
 def handle_comment(self,data):self.handle_data('<!--'+data+'-->')


def select_panels(page,mask):
 selector=PanelSelector({name for bit,name in [(1,'audioOptions'),(2,'indicatorOption'),(4,'deepOptions')] if not mask&bit})
 selector.feed(page.decode('utf-8'));selector.close()
 assert not selector.depth,'Unbalanced optional panel markup'
 return ''.join(selector.parts).encode('utf-8')

node=shutil.which("node")
if not node:raise SystemExit("Node.js is required to build portal assets")
from generate_locales import generate, language_assets
from check_portal_assets import source_digest
root=Path(__file__).resolve().parents[1]
language_assets()
language_options=''.join('<option value="'+row['code']+'">'+row['name']+'</option>' for row in json.loads((root/'resources/languages.json').read_text('utf-8')) if not limited or row['code'] in ('zh','en')).encode('utf-8')
parts=['#pragma once','#include "platform/idf/text.h"','#include "features/config.h"']
for source,symbol in [('index.html','index'),('style.css','style'),('app.js','script')]:
 raw=(root/'portal'/source).read_bytes()
 if source=='index.html':raw=raw.replace(b'__LANGUAGE_OPTIONS__',language_options).replace(b'__FIRMWARE_VERSION__',json.loads((root/'version.json').read_text('utf-8'))['version'].encode('ascii'))
 if source=='app.js':
  zones=json.loads((root/'resources/timezones/iana-posix.json').read_text('utf-8'))
  clock=b'const XTouchTimezones='+json.dumps(zones,separators=(',',':')).encode()+b';\n'+(root/'portal/clock.js').read_bytes()
  raw=generate(limited=limited)+clock+raw
 if source.endswith('.css'):
  raw=subprocess.run([node,str(root/'scripts/minify-portal.cjs'),source.rsplit('.',1)[1]],input=raw,capture_output=True,check=True).stdout
 if source=='index.html':
  # Select a fixed HTML variant at compile time. The browser does not probe
  # hardware or wait for /api/info to decide which settings are available.
  for condition,filename in [("#if defined(XTOUCH_BOARD_ES3C35P)","lcdwiki-es3c35p.bin"),("#elif defined(XTOUCH_WAVESHARE) && defined(XTOUCH_BOARD_S3)","esp32-s3-touch-lcd-2.8-v2.bin"),("#elif defined(XTOUCH_WAVESHARE)","esp32-p4-wifi6-touch-lcd-4.3.bin"),("#else","xtouch-2.8.bin")]:
   parts.append(condition)
   for mask in range(8):
    page=raw.replace(b'__FIRMWARE_FILENAME__',filename.encode('ascii'))
    page=select_panels(page,mask)
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
    page=subprocess.run([node,str(root/'scripts/minify-portal.cjs'),'js'],input=page,capture_output=True,check=True).stdout
    data=zopfli.gzip.compress(page,numiterations=20)
    parts.append('static const uint8_t portal_script[] PROGMEM = {\n'+','.join(hex(v) for v in data)+'\n};')
   parts.append('#endif\n#endif')
 else:
  data=zopfli.gzip.compress(raw,numiterations=50)
  parts.append('static const uint8_t portal_'+symbol+'[] PROGMEM = {\n'+','.join(hex(v) for v in data)+'\n};')

target=root/('src/xtouch/portal_assets_zh_en.h' if limited else 'src/xtouch/portal_assets.h')
content='// Portal source SHA256: '+source_digest(limited)+'\n'+'\n'.join(parts)+'\n'
if not target.exists() or target.read_text(encoding='utf-8')!=content:
 target.write_text(content,encoding='utf-8')
