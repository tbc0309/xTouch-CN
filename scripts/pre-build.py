import json
import subprocess
import os

Import("env")
asset_size = os.environ.get("XTOUCH_ASSET_SIZE", "small")
if asset_size not in ("small", "large"):
    raise ValueError("XTOUCH_ASSET_SIZE must be small or large")
icon_set = os.environ.get("XTOUCH_ICON_SET", "mdi" if asset_size == "large" else "original")
if icon_set not in ("original", "mdi"):
    raise ValueError("XTOUCH_ICON_SET must be original or mdi")
env.Append(CPPDEFINES=[("XTOUCH_ICON_STYLE", int(icon_set == "mdi")),
                      ("XTOUCH_UI_HIDPI_ASSETS", int(asset_size == "large"))])
if asset_size == "large" or icon_set == "mdi":
    env.Append(CPPPATH=[env.subst("$PROJECT_LIBDEPS_DIR/$PIOENV/lvgl"), env.subst("$PROJECT_DIR/resources")])
    def omit_resource(node):
        return None
    if asset_size == "large":
        env.AddBuildMiddleware(omit_resource, "*/ui/fonts/*.c")
        env.AddBuildMiddleware(omit_resource, "*/ui/images/printer_models.c")
        env.BuildSources("$BUILD_DIR/selected-text", env.subst("$PROJECT_DIR/resources/ui-assets/fonts/large"), src_filter="+<*.c> -<compact/> -<ui_font_xlcd*.c> -<ui_font_actions.c> -<ui_font_xperiments.c> -<ui_font_text_14.c>")
        env.BuildSources("$BUILD_DIR/selected-compact", env.subst("$PROJECT_DIR/resources/ui-assets/fonts/large/compact"))
        env.BuildSources("$BUILD_DIR/selected-large", env.subst("$PROJECT_DIR/resources/ui-assets/generated/large"))
        if icon_set == "original":
            env.AddBuildMiddleware(omit_resource, "*/generated/large/ui_font_xlcd*.c")
            env.AddBuildMiddleware(omit_resource, "*/generated/large/ui_font_actions.c")
            env.AddBuildMiddleware(omit_resource, "*/generated/large/ui_font_xperiments.c")
            env.BuildSources("$BUILD_DIR/selected-icons", env.subst("$PROJECT_DIR/resources/ui-assets/generated/original/large"))
    else:
        env.AddBuildMiddleware(omit_resource, "*/ui/fonts/ui_font_xlcd.c")
        env.AddBuildMiddleware(omit_resource, "*/ui/fonts/ui_font_xlcdmin.c")
        env.BuildSources("$BUILD_DIR/selected-icons", env.subst("$PROJECT_DIR/resources/ui-assets/generated/mdi/small"))
def verify_linked_icons(source, target, env):
    subprocess.run([env.subst("$PYTHONEXE"), "scripts/check_cyd_assets.py",
                    env.subst("$BUILD_DIR/${PROGNAME}.elf"), asset_size, icon_set], check=True)
env.AddPostAction("$BUILD_DIR/${PROGNAME}.elf", verify_linked_icons)
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
