import json
import subprocess
import os

Import("env")
icon_set = os.environ.get("XTOUCH_ICON_SET", "original")
if icon_set not in ("original", "mdi"):
    raise ValueError("XTOUCH_ICON_SET must be original or mdi")
env.Append(CPPDEFINES=[("XTOUCH_ICON_STYLE", int(icon_set == "mdi"))])
if icon_set == "mdi":
    def omit_legacy_icon(node):
        return None
    env.AddBuildMiddleware(omit_legacy_icon, "*/ui/fonts/ui_font_xlcd.c")
    env.AddBuildMiddleware(omit_legacy_icon, "*/ui/fonts/ui_font_xlcdmin.c")
    env.BuildSources("$BUILD_DIR/selected-icons", "resources/ui-assets/generated/mdi/small")
subprocess.run([env.subst("$PYTHONEXE"), "scripts/embed_portal.py"], check=True)

with open("version.json", encoding="utf-8") as version_file:
    version = json.load(version_file)["version"]
env.Append(CPPDEFINES=[("XTOUCH_FIRMWARE_VERSION", '\\"' + version + '\\"')])


# Make sure 'vscode init' is not the current command
def is_pio_build():
    from SCons.Script import DefaultEnvironment
    env = DefaultEnvironment()
    if "IsCleanTarget" in dir(env) and env.IsCleanTarget():
        return False
    return not env.IsIntegrationDump()


if is_pio_build() and os.environ.get("XTOUCH_REFRESH_ERRORS") == "1":
    print(f"XTOUCH PREBUILD")
    result = subprocess.run([env.subst("$PYTHONEXE"), 'scripts/refresh_error_reference.py'],
                            text=True, check=True, capture_output=True)
    print(result.stdout)

subprocess.run([env.subst("$PYTHONEXE"), "scripts/generate_error_docs.py"], check=True)
if os.environ.get("XTOUCH_ERROR_SELFTEST") == "1":
    env.Append(CPPDEFINES=["XTOUCH_ERROR_SELFTEST"])
