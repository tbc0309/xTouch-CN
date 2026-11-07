"""Shared filename supplement coverage; standard-library-only build validation."""
import argparse
import hashlib
import json
import re
import string
import unicodedata
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def points(path):
    return {int(cp, 16) for cp in re.findall(r"/\* U\+([0-9A-Fa-f]+)", path.read_text("utf8"))}

def ui_profiles(root=ROOT):
    small = points(root / "src/ui/fonts/ui_font_text_14.c")
    large = points(root / "resources/ui-assets/fonts/large/compact/ui_font_text_14.c")
    native = (root / "resources/ui-assets/fonts/large/ui_font_native.c").read_text("utf8")
    start = native.index("ui_native_body_14_glyph_bitmap")
    stop = native.index("const lv_font_t ui_native_body_14", start)
    native14 = {int(cp, 16) for cp in re.findall(r"/\* U\+([0-9A-Fa-f]+)", native[start:stop])}
    if small != large or small != native14:
        raise ValueError("UI body coverage differs across resource profiles; regenerate native UI fonts")
    profile = json.loads((root / "resources/fonts/file-names/profile.json").read_text("utf8"))
    file28 = root / "src/features/gcode_browser/font_28.c"
    dedicated = points(file28) if file28.exists() else set(profile["uiCodepoints"])
    return {"14": small, "28": large,
            "28-small": dedicated | points(root / "src/ui/fonts/ui_font_text_28.c")}

def wanted(root=ROOT):
    # GB2312 includes 6763 Han and 682 non-Han characters.
    codes = set()
    for lead in range(0xA1, 0xF8):
        for tail in range(0xA1, 0xFF):
            try:
                codes.add(ord(bytes([lead, tail]).decode("gb2312")))
            except UnicodeDecodeError:
                continue
    codes.update(range(0x2010, 0x2016))
    codes.update(range(0x2018, 0x2020))
    codes.update({0x2022, 0x2025, 0x2026, 0x2030, 0x2032, 0x2033,
                  0x2039, 0x203A, 0x2044})
    profile = json.loads((root / "resources/fonts/file-names/profile.json").read_text("utf8"))
    codes.update(profile["uiCodepoints"])
    return codes

def punctuation(root=ROOT):
    target = wanted(root)
    return ({ord(c) for c in string.punctuation}
            | {c for c in target if unicodedata.category(chr(c)).startswith("P")}
            | {ord(c) for c in "·￥¥£"}) & target

def families(root=ROOT):
    symbols = punctuation(root)
    captions = set(json.loads((root / "resources/fonts/file-names/profile.json").read_text("utf8"))["uiCodepoints"])
    return {"common_zh": wanted(root) - symbols, "punctuation": symbols,
            "ui_captions": captions - symbols}

def fingerprint(codes):
    return hashlib.sha256(",".join(str(cp) for cp in sorted(codes)).encode()).hexdigest()

def manifest(root=ROOT):
    target = wanted(root)
    return {"version": 2, "wantedCount": len(target), "wantedHash": fingerprint(target),
            "families": {family: {"count": len(codes), "hash": fingerprint(codes)}
                         for family, codes in families(root).items()},
            "profiles": {name: {"uiCount": len(base), "uiHash": fingerprint(base),
                                 "families": {family: {"count": len(codes - base),
                                                        "hash": fingerprint(codes - base)}
                                              for family, codes in families(root).items()}}
                         for name, base in ui_profiles(root).items()}}

def check(root=ROOT):
    directory = root / "resources/fonts/file-names"
    if json.loads((directory / "coverage.json").read_text("utf8")) != manifest(root):
        raise ValueError("UI coverage changed; regenerate filename supplements with generate_filename_fonts.py")
    for name, base in ui_profiles(root).items():
        covered = {}
        for family, requested in families(root).items():
            path = directory / (family + "_" + name.replace("-", "_") + ".c")
            actual = points(path)
            if actual & base or actual != requested - base:
                raise ValueError(f"Duplicate or missing filename supplement glyphs: {family}/{name}")
            covered[family] = actual
        # Common Chinese and the caption-only bridge are mutually exclusive.
        for mode in ("common_zh", "ui_captions"):
            if covered[mode] & covered["punctuation"]:
                raise ValueError(f"Duplicated punctuation: {mode}/{name}")
            expected = families(root)[mode] | families(root)["punctuation"] | base
            if covered[mode] | covered["punctuation"] | base != expected:
                raise ValueError(f"Combined filename coverage mismatch: {mode}/{name}")
    captions = root / "src/features/gcode_browser/view.cpp"
    if captions.exists():
        text = captions.read_text("utf8")
        text = text[text.index("const char *words"):text.index("lv_obj_t *overlay")]
        literals = [json.loads(s) for s in re.findall(r'"(?:\\.|[^"\\])*"', text)]
        current = set(range(32, 127)) | {0x2212, 0xD7} | {ord(c) for s in literals for c in s}
        profile = json.loads((directory / "profile.json").read_text("utf8"))
        if current != set(profile["uiCodepoints"]):
            raise ValueError("File UI captions changed; regenerate file-browser and filename fonts")

if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", required=True)
    parser.parse_args()
    check()
    print("PASS filename supplements: no duplicate UI glyphs; current coverage and captions")
