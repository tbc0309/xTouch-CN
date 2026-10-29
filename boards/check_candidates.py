"""Validate B hardware records without configuring, compiling or downloading IDF components.

This is configuration preflight, not a claim that a C/C++ compiler accepted a driver.
Candidate builds require explicit opt-in; prerelease builds and physical acceptance are recorded separately.
"""
import argparse
import json
from pathlib import Path
from urllib.parse import urlparse


def check(root):
    records = json.loads((root / 'candidate_targets.json').read_text(encoding='utf8'))
    active = json.loads((root / 'port_targets.json').read_text(encoding='utf8'))
    results = []
    bsp_path = root / 'bsp_targets.json'
    bsp_targets = json.loads(bsp_path.read_text(encoding='utf8')) if bsp_path.exists() else {}
    for name, row in records.items():
        errors = []
        promoted = name in active and active[name].get('candidateId') == name
        if name in active and not promoted:
            errors.append('Candidate must not replace an active build target')
        width, height = row['screen']
        if width == height or max(width, height) / min(width, height) >= 2:
            errors.append('Excluded screen geometry')
        if row['screenInches'] < 2.4:
            errors.append('Excluded screen size')
        if row['nativeBuildAllowed'] or row['releaseSupported'] or row['physicalTested']:
            errors.append('Unverified candidate incorrectly marked buildable/released/physical-tested')
        if row['otaChannels'] != ['main', 'test']:
            errors.append('Test policy must retain both OTA channels')
        pending = row.get('manufacturerEvidencePending') and row['selection']=='pending-touch' and not row.get('implementationTargets')
        if (not row['sources'] and not pending) or any(urlparse(url).scheme != 'https' for url in row['sources']):
            errors.append('Missing HTTPS evidence')
        local = root / 'candidates' / name / 'hardware.json'
        if not local.is_file() or json.loads(local.read_text(encoding='utf8')) != row:
            errors.append('Per-board hardware record is missing or stale')
        if len(set(row.get('pins', {}).values())) != len(row.get('pins', {})):
            errors.append('Duplicate active pin assignment requires explicit resolution')
        if not row.get('blockers'):
            errors.append('Incomplete driver must state its blockers')
        for target in row.get('implementationTargets', []):
            profile = bsp_targets.get(target) or active.get(target)
            if not profile or profile['candidateId'] != name:
                errors.append('BSP target does not match its hardware candidate')
                continue
            project = root / profile['directory']
            required = ('CMakeLists.txt', 'bsp_port.cmake', 'display_config.cmake',
                             'sdkconfig.defaults', 'partitions.csv',
                             'xtouch_board_support/CMakeLists.txt',
                             'xtouch_board_support/idf_component.yml') if profile.get('bspProfile') else (
                                 'CMakeLists.txt', 'profile.cpp', 'display_config.cmake', 'sdkconfig.defaults', 'partitions.csv')
            for filename in required:
                if not (project / filename).is_file():
                    errors.append('Missing project input: ' + filename)
            cmake = (project / 'CMakeLists.txt').read_text(encoding='utf8')
            if profile.get('bspProfile') and 'BSP_CONFIG_NO_GRAPHIC_LIB=1' not in cmake:
                errors.append('BSP must not start a second graphics library')
            if profile['nativeBuildAllowed'] or profile['releaseSupported']:
                errors.append('Uncompiled BSP target may not be enabled for release')
            for source in (('bsp_spi.cpp', 'bsp_audio.cpp', 'power.h') if profile.get('bspProfile') else ()):
                if not (root / 'candidates/common' / source).is_file():
                    errors.append('Missing BSP adapter: ' + source)
        results.append(dict(board=name,configurationValid=not errors,errors=errors,
                            nativeCompiled=bool(row.get('nativeCompiled')),physicalTested=False,blockers=row['blockers']))
    return results


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--json', type=Path)
    args = parser.parse_args()
    rows = check(Path(__file__).resolve().parent)
    payload = dict(kind='configuration-preflight-only',nativeBuildInvoked=False,rows=rows)
    if args.json:
        args.json.write_text(json.dumps(payload, ensure_ascii=False, indent=2), encoding='utf8')
    failed = sum(not row['configurationValid'] for row in rows)
    print(f'{len(rows)-failed}/{len(rows)} hardware configurations valid; this preflight did not invoke C/C++ builds')
    raise SystemExit(bool(failed))
