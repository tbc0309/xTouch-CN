"""Run the installed IDF GCC analyzer on selected owned sources, without relinking firmware."""
import argparse
import json
from pathlib import Path
import shlex
import subprocess
import os


def split_command(value):
    if os.name != "nt":
        return shlex.split(value)
    import ctypes
    from ctypes import wintypes
    count = ctypes.c_int()
    split = ctypes.windll.shell32.CommandLineToArgvW
    split.argtypes = [wintypes.LPCWSTR, ctypes.POINTER(ctypes.c_int)]
    split.restype = ctypes.POINTER(wintypes.LPWSTR)
    words = split(value, ctypes.byref(count))
    if not words:
        raise ctypes.WinError()
    try:
        return [words[i] for i in range(count.value)]
    finally:
        ctypes.windll.kernel32.LocalFree(ctypes.cast(words, ctypes.c_void_p))

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("build", type=Path)
parser.add_argument("--output", type=Path, required=True)
parser.add_argument("--experimental-cpp", action="store_true", help="Diagnostic experiment only: GCC documents this analyzer as suitable for C")
args = parser.parse_args()
args.output.mkdir(parents=True, exist_ok=True)
selected = {"message_bus.c", "errors.c", "ota_request.c"}
if args.experimental_cpp:
    selected.update({"socket.cpp", "vault.cpp"})
commands = json.loads((args.build / "compile_commands.json").read_text(encoding="utf-8"))
results = []
for entry in commands:
    source = Path(entry["file"])
    if source.name not in selected:
        continue
    # Preserve backslashes in the Windows compilation database.
    command = split_command(entry["command"])
    object_path = str((args.output / (source.name + ".o")).resolve())
    command[command.index("-o") + 1] = object_path
    # Do not overwrite Ninja's dependency files while analyzing a separate object.
    if "-MF" in command:
        command[command.index("-MF") + 1] = object_path + ".d"
    if "-MT" in command:
        command[command.index("-MT") + 1] = object_path
    command += ["-fanalyzer", "-fdiagnostics-color=never"]
    if os.name == "nt":
        # SDK include lists can exceed CreateProcess's command-line limit.
        # GCC response files use backslash escaping, independently of the CRT.
        prefix = command[:1]
        if Path(command[0]).stem.lower() == "ccache":
            prefix = command[:2]
        response = (args.output / (source.name + ".rsp")).resolve()
        response.write_text("\n".join('"' + value.replace('\\', '\\\\').replace('"', '\\"') + '"'
                                      for value in command[len(prefix):]) + "\n", encoding="utf-8")
        command = prefix + ["@" + str(response)]
    result = subprocess.run(command, cwd=entry["directory"], capture_output=True, timeout=180)
    log = (result.stdout + result.stderr).decode("utf-8", errors="replace")
    (args.output / (source.name + ".log")).write_text(log, encoding="utf-8")
    warnings = [line for line in log.splitlines() if "-Wanalyzer-" in line or "-Werror=analyzer-" in line]
    results.append({"source": source.name, "exit": result.returncode, "analyzerWarnings": warnings})
    print(source.name, result.returncode, "analyzer warnings:", len(warnings), flush=True)
(args.output / "results.json").write_text(json.dumps(results, indent=2), encoding="utf-8")
if len(results) != len(selected) or any(row["exit"] or row["analyzerWarnings"] for row in results):
    raise SystemExit(1)
