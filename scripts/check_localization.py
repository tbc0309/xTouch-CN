"""Validate generated glyph coverage without connecting to a printer."""
from pathlib import Path
import re

root = Path(__file__).resolve().parents[1]
font = (root / 'src/ui/fonts/ui_font_text_14.c').read_text('utf-8')
covered = {int(cp,16) for cp in re.findall(r'/\* U\+([0-9A-Fa-f]+)',font)}
missing = []
for path in (root / 'src').rglob('*'):
    if path.suffix not in ('.c', '.cpp', '.h') or 'fonts' in path.parts:
        continue
    source = path.read_text('utf-8-sig')
    # Only string literals are relevant; comments must not enlarge the subset.
    for value in re.findall(r'"((?:\\.|[^"\\])*)"', source):
        for ch in value:
            if ord(ch)>=160 and ch.isprintable() and ord(ch) not in covered:
                missing.append((str(path.relative_to(root)), ch))
assert not missing, f'Missing UI font glyphs: {missing}'
assert 'lv_font_get_glyph_dsc_fmt_txt' in font
assert '.fallback = &lv_font_montserrat_14' in font
config = (root / 'resources/lv_conf.h').read_text('utf-8')
assert '#define LV_FONT_DEFAULT &ui_font_text_14' in config
assert 'LV_FONT_DECLARE(ui_font_text_14)' in config
assert 'LV_FONT_DECLARE(ui_font_text_28)' in config
translations = []
for filename, suffix in [('zh_cn.h','ZH'),('zh_tw.h','TW'),('en.h','EN'),('fr.h','FR'),('de.h','DE'),('ru.h','RU'),('ja.h','JA'),('ko.h','KO')]:
    source = (root / 'src/ui/i18n' / filename).read_text('utf-8')
    translations.append(dict(re.findall(r'#define (XTOUCH_TEXT_\w+)_' + suffix + r' (".*")', source)))
assert all(t.keys()==translations[0].keys() for t in translations), 'Language keys differ'
for key in translations[0]:
    formats = [re.findall(r'%(?:\d+\$)?[dsu]', language[key]) for language in translations]
    assert all(f==formats[0] for f in formats), f'Format mismatch: {key}'
print(f'PASS: {len(translations[0])} complete eight-language translation keys')
print(f'PASS: {len(covered)} font glyphs; all localized source strings covered')


# Check all live UI/portal lookup sites, not just equal language-key counts.
import json
catalogs={name:json.loads((root/'resources'/f'{name}-locales.json').read_text('utf-8')) for name in ('ui','portal')}
for name,rows in catalogs.items():
    for key,values in rows.items():
        assert len(values)==8 and all(isinstance(v,str) and v.strip() for v in values), (name,key,'incomplete')
        if name=='ui':assert values[2]==key,(name,key,'lookup key changed')
        for value in values:
            assert re.findall(r'%(?:\d+\$)?[dsu]',value)==re.findall(r'%(?:\d+\$)?[dsu]',values[0]),(name,key,'format')
for folder,function,catalog in [('src','xtouch_local','ui'),('portal','t','portal')]:
    for path in (root/folder).rglob('*'):
        if path.suffix not in ('.h','.c','.cpp','.js') or 'fonts' in path.parts or path.name=='portal_assets.h':continue
        source=path.read_text('utf-8-sig')
        pattern=r'\b'+function+r'\(\s*("(?:\\.|[^"\\])*")\s*,\s*("(?:\\.|[^"\\])*")'
        for zh,en in re.findall(pattern,source):
            key=json.loads(en)
            assert key in catalogs[catalog],(str(path.relative_to(root)),key,'untranslated fallback')
            values=catalogs[catalog][key]
            if json.loads(zh)=='连接方式':assert key=='Connection type','Connection status/type must use different keys'
print(f"PASS: {len(catalogs['ui'])} UI and {len(catalogs['portal'])} portal rows; live literal lookups have all eight languages")

# Static portal labels and API-error arrays also bypass literal t(...) calls.
portal_source=(root/'portal/app.js').read_text('utf-8')
words=portal_source.split('const words = {',1)[1].split('};',1)[0]
for value in re.findall(r'\w+:\s*("(?:\\.|[^"\\])*")',words):
    assert json.loads(value) in catalogs['portal'], ('portal label',json.loads(value))
for zh,en in re.findall(r'\[\s*("(?:\\.|[^"\\])*")\s*,\s*("(?:\\.|[^"\\])*")\s*\]',portal_source):
    if any('\u4e00'<=c<='\u9fff' for c in json.loads(zh)):
        assert json.loads(en) in catalogs['portal'],('portal API error',json.loads(en))
for name,rows in catalogs.items():
    for key,values in rows.items():
        digits=re.findall(r'[0-9]+',values[0])
        assert not digits or all(re.findall(r'[0-9]+',v)==digits for v in values),(name,key,'numeric limits differ')
print('PASS: static portal labels, API-error arrays and numeric limits')

# QR headings must be covered by the title subset itself, not a smaller fallback.
heading_keys=('Scan to join Wi-Fi','Scan to manage','Scan to join Bluetooth')
for profile in ('src/ui/fonts','resources/ui-assets/fonts/large'):
    title=(root/profile/'ui_font_portal_title_22.c').read_text('utf-8')
    title_codes={int(cp,16) for cp in re.findall(r'/\* U\+([0-9A-Fa-f]+)',title)}
    for key in heading_keys:
        for value in catalogs['ui'][key]:
            assert all(ord(ch) in title_codes for ch in value if ord(ch)>=32),(profile,value,'title fallback mixes sizes')
print('PASS: all eight-language QR headings use complete title glyph subsets on both display sizes')
