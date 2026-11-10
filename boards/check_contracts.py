"""Validate board adapter identities and OTA contracts without starting IDF."""
import json
import sys
from pathlib import Path
from board_catalog import BOARDS
ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / 'scripts'))
from generate_ota_channels import generate, slot_bytes


def check(root=ROOT, catalog=BOARDS):
    counts = {'targets': len(catalog), 'explicitStorage': 0, 'storageDisabled': 0}
    for name, board in catalog.items():
        directory = root / 'boards' / board['directory']
        for required in ('CMakeLists.txt', 'sdkconfig.defaults', 'partitions.csv'):
            if not (directory / required).is_file():
                raise ValueError(name + ': missing adapter file ' + required)
        if slot_bytes(directory / 'partitions.csv') > board['flash']:
            raise ValueError(name + ': invalid application capacity')
        generate(root, board['directory'], catalog)
        if (directory / 'storage.cpp').is_file():
            counts['explicitStorage'] += 1
        else:
            counts['storageDisabled'] += 1
    return counts


if __name__ == '__main__':
    try:
        print(json.dumps(check()))
    except (ValueError, KeyError) as error:
        raise SystemExit(str(error))
