"""Read the ESP-IDF binary partition table used by a packaged image."""
import struct

def read_layout(data):
    layout = {}
    for pos in range(0, len(data)-31, 32):
        magic, kind, subtype, offset, size, label, flags = struct.unpack_from('<HBBII16sI', data, pos)
        if magic != 0x50AA:
            break
        name = label.split(b'\0', 1)[0].decode('ascii')
        if not size or name in layout:
            raise ValueError('Invalid partition table')
        layout[name] = dict(type=kind, subtype=subtype, offset=offset, size=size)
    for name in ('ota_0', 'ota_1', 'spiffs'):
        if name not in layout:
            raise ValueError('Missing required partition: '+name)
    for name, subtype in [('ota_0',16),('ota_1',17)]:
        entry=layout[name]
        if entry['type']!=0 or entry['subtype']!=subtype or entry['offset']%65536 or entry['size']%65536:
            raise ValueError('Invalid OTA application partition')
    if 'otadata' not in layout or layout['otadata']['type']!=1 or layout['otadata']['subtype']!=0 or layout['otadata']['size']!=8192:
        raise ValueError('Invalid OTA data partition')
    ordered=sorted(layout.values(), key=lambda p:p['offset'])
    if any(a['offset']+a['size']>b['offset'] for a,b in zip(ordered,ordered[1:])):
        raise ValueError('Overlapping partitions')
    if layout['ota_0']['offset'] != 0x20000 or layout['ota_0']['size'] != layout['ota_1']['size']:
        raise ValueError('Invalid OTA layout')
    return layout
