"""Enable whole-program optimization after Arduino's default flags are loaded."""
Import("env")
env["LINKFLAGS"] = [flag for flag in env["LINKFLAGS"] if flag != "-fno-lto"]
env.Append(LINKFLAGS=["-flto"])
env.Replace(AR="xtensa-esp32-elf-gcc-ar", RANLIB="xtensa-esp32-elf-gcc-ranlib")
