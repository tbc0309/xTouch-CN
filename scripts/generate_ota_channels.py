"""Generate exact board identity and OTA policy; usable on clean clones and forks."""
import argparse
import csv
import json
import pathlib
import re
import sys
from release_policy import read as release_identity


def slot_bytes(path):
    slots = {}
    with path.open(encoding='utf-8') as source:
        for row in csv.reader(line for line in source if line.strip() and not line.lstrip().startswith('#')):
            if len(row) < 5 or row[0].strip() not in ('ota_0', 'ota_1'):
                continue
            name, kind, subtype = (v.strip() for v in row[:3])
            if name in slots or kind != 'app' or subtype != name:
                raise ValueError('Invalid or duplicate OTA slot: ' + name)
            size = row[4].strip()
            scale = 1024 if size[-1:].upper() == 'K' else 1024 * 1024 if size[-1:].upper() == 'M' else 1
            slots[name] = int(size[:-1] if scale != 1 else size, 0) * scale
    if set(slots) != {'ota_0', 'ota_1'} or slots['ota_0'] != slots['ota_1'] or slots['ota_0'] <= 0 or slots['ota_0'] % 65536:
        raise ValueError('Expected two equally sized, aligned OTA slots')
    return slots['ota_0']


def generate(root, directory, catalog=None):
    if catalog is None:
        sys.path.insert(0, str(root / 'boards'))
        from board_catalog import BOARDS
        catalog = BOARDS
    matches = [board for board in catalog.values() if board['directory'] == directory]
    if len(matches) != 1:
        raise ValueError('Unknown or ambiguous board: ' + directory)
    board = matches[0]
    release = release_identity(root)
    policy = json.loads((root / 'resources/ota_channels.json').read_text('utf8'))
    repo = policy['repository']
    if not isinstance(repo, str) or not re.fullmatch(r'[A-Za-z0-9][A-Za-z0-9_.-]*/[A-Za-z0-9][A-Za-z0-9_.-]*', repo):
        raise ValueError('Expected an owner/repository OTA identity')
    limit = slot_bytes(root / 'boards' / directory / 'partitions.csv')
    if limit > board['flash']:
        raise ValueError('OTA slot exceeds board Flash capacity')
    paths = ['/' + repo + '/' + channel + '/boards/' + directory + '/ota.json' for channel in release['channels']]
    identity = '#pragma once\n#define XTOUCH_FIRMWARE_FILENAME_OVERRIDE ' + json.dumps('/' + board['stem'] + '.bin')
    identity += '\n#define XTOUCH_FIRMWARE_LIMIT_OVERRIDE ' + str(limit) + 'U\n#define XTOUCH_HTTP_UPLOAD_LIMIT_BYTES ' + str(limit + 4096) + 'U\n'
    header = '#pragma once\n#include "xtouch_board_build.h"\n#define XTOUCH_OTA_GENERATED 1\n'
    header += '#define XTOUCH_OTA_REPOSITORY ' + json.dumps(repo) + '\n#define XTOUCH_OTA_ALLOW_TEST_VERSION ' + str(int(release['prerelease'])) + '\n'
    header += '#define XTOUCH_OTA_MANIFEST_PATH ' + json.dumps(paths[0]) + '\n#define XTOUCH_OTA_MANIFEST_URL "https://raw.githubusercontent.com" XTOUCH_OTA_MANIFEST_PATH\n'
    header += 'static const char *const xtouch_ota_manifest_paths[] = {' + ','.join(json.dumps(v) for v in paths) + '};\n'
    return header, identity


def write_if_changed(path, text):
    path.parent.mkdir(parents=True, exist_ok=True)
    if not path.exists() or path.read_text(encoding='utf8') != text:
        path.write_text(text, encoding='utf8')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--board', required=True)
    parser.add_argument('--output', type=pathlib.Path, required=True)
    args = parser.parse_args()
    try:
        header, identity = generate(pathlib.Path(__file__).resolve().parents[1], args.board)
    except (ValueError, KeyError) as error:
        parser.error(str(error))
    write_if_changed(args.output, header)
    write_if_changed(args.output.with_name('xtouch_board_build.h'), identity)
    root = pathlib.Path(__file__).resolve().parents[1]
    for name in ('power', 'network'):
        path = root / 'boards' / args.board / (name + '.h')
        if not path.exists():
            path = root / 'boards/common' / (name + '_none.h')
        write_if_changed(args.output.with_name('xtouch_board_' + name + '.h'),
                         '#pragma once\n#include "' + path.as_posix() + '"\n')



if __name__ == '__main__':
    main()
