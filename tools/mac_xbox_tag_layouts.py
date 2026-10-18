#!/usr/bin/env python3
"""Compile the engine's tag structures for the console's ABI (clang, 32-bit, MSVC-like) and
dump their record layouts. Used by tools/mac_xbox_tag_tables.py. usage: run from the repository root."""
import subprocess, sys, os, re

DIRS = ("source source/cseries source/math source/physics source/tag_files source/memory source/cache source/camera "
        "source/bitmaps source/main source/editor source/networking source/ai source/interface source/rasterizer "
        "source/rasterizer/xbox source/text source/cutscene source/shaders source/game source/input source/objects "
        "source/items source/units source/models source/scenario source/structures source/render source/sound "
        "source/effects source/shell source/bink source/hs source/devices source/lights").split()

STUBS = "/tmp/opence-ast-stubs"

def stubs():
    """empty system headers for the port's include_next shims (the layouts need no C library)"""
    if not os.path.isdir(STUBS):
        os.makedirs(STUBS)
    for name in ("stdio.h", "stdlib.h", "string.h", "math.h", "memory.h", "wchar.h", "wctype.h", "malloc.h", "ctype.h", "time.h", "errno.h", "limits.h"):
        path = os.path.join(STUBS, name)
        text = "typedef struct _halo_stub_file FILE;\n" if name == "stdio.h" else "/* stub */\n"
        if not os.path.exists(path) or os.path.getsize(path) != len(text):
            open(path, "w").write(text)
    return STUBS

def flags():
    res = subprocess.check_output(["clang", "-print-resource-dir"]).decode().strip()
    f = ["--target=i686-linux-gnu", "-m32", "-fms-extensions", "-fshort-wchar", "-malign-double", "-fcommon", "-std=gnu89",
         "-D__STRICT_ANSI__", "-w", "-nostdinc", "-isystem", res + "/include", "-isystem", stubs(), "-Iport/linux/include",
         "-include", "port/linux/include/halo_linux_prefix.h", "-include", "port/linux/include/halo_port_limits.h",
         "-include", "port/linux/include/halo_port_capacity.h", "-include", "port/linux/include/halo_linux_source_fixups.h",
         "-Iport/include/xdk", "-I.", "-Isource/saved games", "-Iport/linux/game", "-Iport/linux/src"]
    f += ["-I" + d for d in DIRS]
    return f

def dump(source, extra=()):
    cmd = ["clang"] + flags() + list(extra) + ["-fsyntax-only", "-Xclang", "-fdump-record-layouts-canonical", source]
    p = subprocess.run(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    return p.stdout.decode("latin1"), p.stderr.decode("latin1"), p.returncode

if __name__ == "__main__":
    out, err, rc = dump(sys.argv[1])
    sys.stderr.write(err[-2000:])
    sys.stdout.write(out)
