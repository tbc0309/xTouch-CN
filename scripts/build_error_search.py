"""Build the website-only lookup database; never embed descriptions in firmware."""
import json
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]

def build(output):
    data = json.loads((ROOT / 'docs/data/error-codes.json').read_text('utf-8'))
    rows = []
    for kind in ['hms', 'device']:
        for code, en in data[kind]:
            rows.append(dict(kind=kind, code=code.upper(), en=en,
                zh=data.get('zh', {}).get(kind, {}).get(code, ''),
                models=data.get('origins', {}).get(kind, {}).get(code, []),
                variants=data.get('variants', {}).get(kind, {}).get(code, {})))
    # Protocol acknowledgement errors are separate from HMS/print-error tables.
    rows.append(dict(kind='command', code=f'{84033543:08X}', en='MQTT command verification failed.',
        zh='MQTT 命令验证失败。', models=[], hint=[
        '部分控制功能需局域网模式及开发者模式。请在 xTouch 中使用局域网设备连接。',
        'Some controls need LAN + Dev mode. Use a LAN profile in xTouch.']))
    output.write_text(json.dumps(dict(rows=rows, source=data.get('source'), studioCommit=data.get('studioCommit')), ensure_ascii=False, separators=(',', ':')), 'utf-8')
    return len(rows)

if __name__ == '__main__':
    print('Website error codes:', build(ROOT / 'web/error-codes.json'))
