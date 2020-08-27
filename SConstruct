#!python
env = Environment()

AddOption("--tovedebug", action="store_true", help="build for debugging", default=False)

AddOption("--static", action="store_true", help="build static library", default=False)

AddOption(
    "--arch",
    dest="arch",
    type="string",
    nargs=1,
    action="store",
    help="CPU architecture to use (e.g. sandybridge, haswell)",
    default=None,
)

AddOption(
    "--f16c", action="store_true", help="assume hardware support for 16-bit floating point numbers", default=False
)

sources = [
    "src/cpp/version.cpp",
    "src/cpp/interface/api.cpp",
    "src/cpp/graphics.cpp",
    "src/cpp/nsvg.cpp",
    "src/cpp/paint.cpp",
    "src/cpp/path.cpp",
    "src/cpp/references.cpp",
    "src/cpp/subpath.cpp",
    "src/cpp/mesh/flatten.cpp",
    "src/cpp/mesh/mesh.cpp",
    "src/cpp/mesh/meshifier.cpp",
    "src/cpp/mesh/partition.cpp",
    "src/cpp/mesh/triangles.cpp",
    "src/cpp/gpux/curve_data.cpp",
    "src/cpp/gpux/geometry_data.cpp",
    "src/cpp/gpux/geometry_feed.cpp",
    "src/cpp/shader/gen.cpp",
    "src/thirdparty/clipper.cpp",
    "src/thirdparty/polypartition/src/polypartition.cpp",
    "src/thirdparty/tinyxml2/tinyxml2.cpp",
    "src/cpp/warn.cpp",
]

if env["PLATFORM"] == "win32":
    env["CCFLAGS"] = " /EHsc /std:c++17 "

    if not GetOption("tovedebug"):
        env["CCFLAGS"] += " /Oi /Ot /Oy /Ob2 /GF /Gy /fp:fast "
        # enabling /O2 (or /Og) will crash demos/retro for some reason.

        # concering CPU architectures and AVX:

        # by default, we do not specify a value for /arch - even though in theory, /arch:AVX should be
        # fine for processors not older than 2011 (https://en.wikipedia.org/wiki/Advanced_Vector_Extensions),
        # in practice setting AVX causes issues (see https://github.com/poke1024/tove2d/issues/24).
        # for /arch:AVX2, we even need Haswell architectures.

        # the main benefit for enabling AVX2 would probably be src/thirdparty/nanosvg/tove/svgrast.cpp,
        # where various dithering operations are optimizable for SIMD (AVX can then be seen in asm code).

        arch = GetOption("arch")
        if arch == "sandybridge":
            env["CCFLAGS"] += " /arch:AVX "
        elif arch == "haswell":
            env["CCFLAGS"] += " /arch:AVX2 "
        elif arch is not None:
            raise RuntimeError("please specify sandybridge or haswell for arch")

        env["LINKFLAGS"] = " /OPT:REF "
    else:
        env["CCFLAGS"] += " /DEBUG:FULL "
        env["LINKFLAGS"] = " /DEBUG:FULL "

else:
    # -Wreorder -Wunused-variable

    CCFLAGS = " -std=c++17 -fvisibility=hidden -funsafe-math-optimizations "

    if GetOption("arch"):
        CCFLAGS += " -march=%s " % GetOption("arch")

    if GetOption("tovedebug"):
        CCFLAGS += "-g "
    else:
        CCFLAGS += "-O3 "

    if env["PLATFORM"] == "posix" and GetOption("f16c"):
        CCFLAGS += " -mf16c "

    if GetOption("f16c"):
        CCFLAGS += " -DTOVE_F16C=1 "

    env["CCFLAGS"] = CCFLAGS

env["CPPPATH"] = "src/thirdparty/fp16/include"

# prepare git hash based version string.

import subprocess
import datetime
import traceback
import re
import os
import sys


def get_version_string():
    try:
        os.chdir(Dir(".").abspath)
        args = ["git", "describe", "--tags", "--long"]
        res = subprocess.check_output(args).strip()
        if sys.version_info >= (3, 0):
            res = res.decode("utf8")
        return res
    except:
        print("failed to determine tove version.")
        traceback.print_exc()
        return "unknown"


with open("src/cpp/version.in.cpp", "r") as v_in:
    with open("src/cpp/version.cpp", "w") as v_out:
        v_out.write(v_in.read().replace("TOVE_VERSION", get_version_string()))


def minify_lua(target, source, env):
    # glue together the lua library.

    include_pattern = re.compile('\s*--!!\s*include\s*"([^"]+)"\s*')
    import_pattern = re.compile('\s*--!!\s*import\s*"([^"]+)"\s*as\s*(\w+)\s*')

    def lua_import(path):
        source = []

        basepath, _ = os.path.split(path)
        filename, ext = os.path.splitext(path)

        with open(path) as f:
            lines = f.readlines()

        for line in lines:
            m = include_pattern.match(line)
            if m:
                included_path = os.path.join(basepath, m.group(1))
                source.extend(lua_import(included_path))
                continue

            m = import_pattern.match(line)
            if m:
                imported_path = os.path.join(basepath, m.group(1))
                source.append("local %s = (function()\n" % m.group(2))
                source.extend(lua_import(imported_path))
                source.append("end)()\n")
                continue

            if ext == ".lua" and (line.strip().startswith("-- ") or line.strip() == "--"):
                line = None

            if ext == ".h":
                line = re.sub("^EXPORT ", "", line)

            if line:
                source.append(line)

        return source

    assert len(target) == 1
    with open(target[0].abspath, "w") as f:
        f.write("--  This file has been autogenerated. DO NOT CHANGE IT.\n--\n")
        for s in source:
            if os.path.split(s.abspath)[1] == "main.lua":
                f.write("".join(lua_import(s.abspath)))


def package_sl(target, source, env):
    with open(source[0].abspath, "r") as sl:
        with open(target[0].abspath, "w") as out:
            out.write('w << R"GLSL(\n')
            out.write(sl.read())
            out.write(')GLSL";\n')


env.Append(BUILDERS={"MinifyLua": Builder(action=minify_lua), "PackageSL": Builder(action=package_sl)})

env.PackageSL("src/glsl/fill.frag.inc", Glob("src/glsl/fill.frag"))

env.PackageSL("src/glsl/line.vert.inc", Glob("src/glsl/line.vert"))

if not GetOption("static"):
    lib = env.SharedLibrary(target="tove/libTove", source=sources)
else:
    lib = env.StaticLibrary(target="tove/libtove", source=sources)

env.MinifyLua(
    "tove/init.lua",
    Glob("src/cpp/interface/api.h")
    + Glob("src/cpp/interface/types.h")
    + Glob("src/lua/*.lua")
    + Glob("src/lua/core/*.lua"),
)
