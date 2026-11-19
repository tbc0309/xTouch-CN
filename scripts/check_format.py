"""Check the reviewed native C/C++ scope with clang-format; optionally fix it."""
from pathlib import Path
import argparse
import shutil
import subprocess

root = Path(__file__).resolve().parents[1]
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--clang-format", default=shutil.which("clang-format"))
parser.add_argument("--fix", action="store_true")
args = parser.parse_args()
if not args.clang_format:
    parser.error("Install clang-format 23.1.3 or pass --clang-format")
files = []
for line in (root / "resources/format-scope.txt").read_text("utf-8").splitlines():
    if not line or line.startswith("#"):
        continue
    path = (root / line).resolve()
    if not path.is_relative_to(root) or not path.is_file():
        parser.error("Invalid format scope: " + line)
    files.append(str(path))
command = [args.clang_format] + (["-i"] if args.fix else ["--dry-run", "--Werror"])
subprocess.run(command + files, check=True, cwd=root)
print(f"PASS clang-format: {len(files)} native source files")
