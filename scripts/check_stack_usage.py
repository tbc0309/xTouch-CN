"""Inventory application compiler stack frames; this is not a call-chain proof.

Large frames produce review warnings, not feature restrictions. SDK/library
call depth and target-specific worst cases still require task high-water checks.
"""
import argparse
import json
from pathlib import Path


def inventory(build, warning_bytes=4096):
    directory = Path(build) / "esp-idf/main"
    files = sorted(directory.rglob("*.su"))
    if not files:
        raise ValueError("No application .su files; rebuild with -fstack-usage")
    functions = {}
    for path in files:
        for line in path.read_text(encoding="utf-8", errors="replace").splitlines():
            fields = line.rsplit("\t", 2)
            if len(fields) != 3 or not fields[1].isdigit():
                continue
            function, size, kind = fields[0], int(fields[1]), fields[2]
            previous = functions.get(function)
            if previous is None or size > previous["bytes"]:
                functions[function] = {"function": function, "bytes": size, "kind": kind}
    if not functions:
        raise ValueError("Application stack reports contain no readable frames")
    frames = sorted(functions.values(), key=lambda value: (-value["bytes"], value["function"]))
    return {
        "scope": "individual frames compiled in application component; excludes full SDK call-chain depth",
        "units": "bytes",
        "reportFiles": len(files),
        "functions": len(frames),
        "warningBytes": warning_bytes,
        "warnings": [frame for frame in frames if frame["bytes"] > warning_bytes or frame["kind"] == "dynamic"],
        "largestFrames": frames[:40],
        "runtimeMeasurementRequired": True,
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("build", type=Path)
    parser.add_argument("--report", type=Path)
    parser.add_argument("--warn-frame", type=int, default=4096)
    args = parser.parse_args()
    try:
        report = inventory(args.build, args.warn_frame)
    except ValueError as error:
        parser.exit(1, str(error) + "\n")
    destination = args.report or args.build / "stack-usage.json"
    destination.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(f"Stack inventory: {report['functions']} functions; {len(report['warnings'])} frame review warnings; runtime margin still required")
    for frame in report["warnings"][:8]:
        print(f"STACK REVIEW {frame['bytes']} bytes: {frame['function']}")


if __name__ == "__main__":
    main()
