"""Generate the shared, embedded common-Chinese filename fonts."""
import argparse
import json
import re
import subprocess
import tempfile
import shutil
from filename_font_coverage import ui_profiles, wanted, families, manifest, check
from pathlib import Path

from fontTools.ttLib import TTFont

ROOT = Path(__file__).resolve().parents[1]


def ranges(codes):
    result = []
    start = last = None
    for code in sorted(codes):
        if last is not None and code == last + 1:
            last = code
            continue
        if start is not None:
            result.append(hex(start) if start == last else f"{hex(start)}-{hex(last)}")
        start = last = code
    if start is not None:
        result.append(hex(start) if start == last else f"{hex(start)}-{hex(last)}")
    return ",".join(result)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--font", type=Path, required=True)
    parser.add_argument("--fallback-font", type=Path, action="append", required=True)
    parser.add_argument("--converter", type=Path)
    args = parser.parse_args()
    if args.converter is None:
        npx = shutil.which("npx.cmd") or shutil.which("npx")
        if not npx:
            parser.error("Node.js/npx or --converter is required")
        with tempfile.TemporaryDirectory(prefix="xtouch-font-resolve-") as temp:
            resolver = Path(temp) / "resolve.js"
            resolver.write_text(
                "const fs=require('fs'),p=require('path');"
                "for(const d of process.env.PATH.split(p.delimiter)){"
                "const f=p.resolve(d,'../lv_font_conv/lv_font_conv.js');"
                "if(fs.existsSync(f)){process.stdout.write(f);process.exit(0);}}"
                "process.exit(1);", encoding="utf8")
            args.converter = Path(subprocess.check_output(
                [npx, "--yes", "--package=lv_font_conv@1.5.3", "node", str(resolver)],
                text=True).strip())
    if args.converter.name == "cli.js":
        entry = args.converter.parent.parent / "lv_font_conv.js"
        if entry.exists():
            args.converter = entry
    directory = ROOT / "resources/fonts/file-names"
    profile = json.loads((directory / "profile.json").read_text("utf8"))
    captions = ROOT / "src/features/gcode_browser/view.cpp"
    if captions.exists():
        source = captions.read_text("utf8")
        source = source[source.index("const char *words"):source.index("lv_obj_t *overlay")]
        literals = [json.loads(value) for value in re.findall(r'"(?:\\.|[^"\\])*"', source)]
        profile["uiCodepoints"] = sorted(set(range(32, 127)) | {0x2212, 0xD7}
                                         | {ord(char) for text in literals for char in text})
        (directory / "profile.json").write_text(json.dumps(profile, ensure_ascii=False, indent=2)
                                                + "\n", encoding="utf8")
    requested = wanted(ROOT)
    bases = ui_profiles(ROOT)
    fonts = [args.font] + args.fallback_font
    maps = [TTFont(path).getBestCmap() for path in fonts]
    groups = [[] for _ in fonts]
    for code in sorted(requested):
        index = next((i for i, cmap in enumerate(maps) if code in cmap), None)
        if index is None:
            raise ValueError(f"Missing glyph U+{code:04X}")
        groups[index].append(code)
    for family, family_codes in families(ROOT).items():
        for variant, size in (("14", 14), ("28", 28), ("28-small", 28)):
            codes = family_codes - bases[variant]
            ordered = sorted(codes)
            suffix = variant.replace("-", "_")
            name = f"xtouch_filename_{family}_{suffix}"
            target = directory / f"{family}_{suffix}.c"
            # The compact LVGL descriptor has a 20-bit bitmap index. A fallback
            # chain keeps the same coverage valid even with small UI resources.
            chunks = [ordered] if size == 14 else [ordered[i:i + 3000]
                                                   for i in range(0, len(ordered), 3000)]
            if not chunks:
                chunks = [[]]
            names = [name] + [f"{name}_part_{i}" for i in range(1, len(chunks))]
            pieces = []
            with tempfile.TemporaryDirectory(prefix="xtouch-filename-font-") as temp:
                runner = Path(temp) / "convert.js"
                settings = Path(temp) / "arguments.json"
                runner.write_text(
                    "const fs=require('fs');"
                    "const c=JSON.parse(fs.readFileSync(process.argv[2],'utf8'));"
                    "process.argv=['node',c.converter,...c.args];require(c.converter);",
                    encoding="utf8")
                for index, chunk in enumerate(chunks):
                    chunk_codes = set(chunk)
                    if not chunk_codes:
                        fallback = "XTOUCH_FILENAME_COMMON_NEXT_FONT" if family in ("common_zh", "ui_captions") else "XTOUCH_FILENAME_TERMINAL_FONT"
                        metrics = profile["metrics"][str(size)]
                        pieces.append("#include <lvgl.h>\nstatic bool no_glyph(const lv_font_t *f, lv_font_glyph_dsc_t *g, uint32_t c, uint32_t n) { (void)f; (void)g; (void)c; (void)n; return false; }\n"
                                      + f"const lv_font_t {name} = {{.get_glyph_dsc=no_glyph, .line_height={metrics['line_height']}, .base_line={metrics['base_line']}, .fallback=&{fallback}}};\n")
                        continue
                    generated = Path(temp) / f"part_{index}.c"
                    command = ["--size", str(size), "--bpp", "4", "--format", "lvgl",
                               "--no-kerning", "--lv-include", "lvgl.h",
                               "--lv-font-name", names[index]]
                    for font, group in zip(fonts, groups):
                        selected = chunk_codes.intersection(group)
                        if selected:
                            command += ["--font", str(font), "--range", ranges(selected),
                                        "--autohint-strong"]
                    command += ["--lv-fallback", names[index + 1]
                                if index + 1 < len(names) else ("XTOUCH_FILENAME_COMMON_NEXT_FONT" if family in ("common_zh", "ui_captions")
                                        else "XTOUCH_FILENAME_TERMINAL_FONT")]
                    settings.write_text(json.dumps({"converter": str(args.converter.resolve()),
                                                    "args": command + ["-o", str(generated)]}),
                                        encoding="utf8")
                    subprocess.run(["node", str(runner), str(settings)], check=True)
                    text = generated.read_text("utf8")
                    actual = {int(code, 16) for code in re.findall(r"/\* U\+([0-9A-Fa-f]+)", text)}
                    if actual != chunk_codes:
                        raise ValueError(f"Coverage mismatch in {names[index]}")
                    bitmap = re.search(r"glyph_bitmap\[\]\s*=\s*\{(.*?)\};", text, re.S)
                    if not bitmap or len(re.findall(r"0x[0-9a-fA-F]+", bitmap.group(1))) >= (1 << 20):
                        raise ValueError("Font chunk exceeds compact LVGL bitmap index; reduce chunk size")
                    text = re.sub(r"^ \* Opts:.*$",
                                  " * Common Chinese filenames, punctuation and UI captions; generated by scripts/generate_filename_fonts.py.",
                                  text, flags=re.M)
                    for field, value in profile["metrics"][str(size)].items():
                        text = re.sub(r"\." + field + r" = \d+", f".{field} = {value}", text)
                    if len(chunks) > 1:
                        # Converter-local arrays share names; qualify variables while
                        # preserving structure member names such as .glyph_bitmap.
                        variables = set(re.findall(
                            r"^static[^\n]*?\b(\w+)(?:\[[^\]]*\])?\s*(?=[=;])", text, re.M))
                        if not {"glyph_bitmap", "glyph_dsc", "cmaps", "font_dsc"} <= variables:
                            raise ValueError("Unexpected font converter layout")
                        pattern = r"(?<!\.)\b(" + "|".join(sorted(variables)) + r")\b"
                        text = re.sub(pattern, lambda match: names[index] + "_" + match[0], text)
                    pieces.append(text.rstrip())
            declarations = "".join(f"LV_FONT_DECLARE({part});\n" for part in names[1:])
            resource_guard = ""
            if size == 28:
                resource_guard = " && " + ("!" if variant == "28-small" else "") + "XTOUCH_UI_HIDPI_ASSETS"
            enabled = "XTOUCH_FILENAME_COMMON_ZH" if family == "common_zh" else "XTOUCH_FILENAME_PUNCTUATION"
            if family == "ui_captions":
                enabled = "(XTOUCH_FILENAME_PUNCTUATION && !XTOUCH_FILENAME_COMMON_ZH)"
            text = ('#include "features/filename_font.h"\n'
                    f'#if {enabled} && XTOUCH_FILENAME_FONT_PIXELS == {size}{resource_guard}\n'
                    + declarations + "\n\n".join(pieces) + "\n\n#endif\n")
            text = "\n".join(line.rstrip() for line in text.splitlines()).rstrip() + "\n"
            if not target.exists() or target.read_text("utf8") != text:
                temporary = target.with_suffix(".c.tmp")
                temporary.write_text(text, encoding="utf8")
                temporary.replace(target)
            print(f"{name}: {len(codes)} supplemental glyphs, {len(chunks)} chunks; UI overlap zero", flush=True)
    (directory / "coverage.json").write_text(json.dumps(manifest(ROOT), indent=2) + "\n", encoding="utf8")
    check(ROOT)


if __name__ == "__main__":
    main()
