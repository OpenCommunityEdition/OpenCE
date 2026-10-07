#!/usr/bin/env python3
"""Compile the HLSL the game wrote (debug.gpu_dump_shaders) with D3DCompile.

The shader translators (port/linux/src/nv2a_vsh.c, nv2a_psh.c) write HLSL
for the Direct3D 12 renderer; with debug.gpu_dump_shaders set, every program
the game draws with is written out as GLSL and as HLSL. This compiles each
.hlsl file there as the renderer does (vs_5_0 for vs*.hlsl, ps_5_0 for the
others), with Windows' own d3dcompiler_47.dll, and reports the ones that do
not compile. Windows only.

Usage: check_hlsl.py <folder>
"""

import ctypes
import sys
from pathlib import Path

D3DCOMPILE_OPTIMIZATION_LEVEL3 = 1 << 15


def blob_text(blob: int) -> str:
    """an ID3DBlob's bytes (its vtable: QueryInterface, AddRef, Release,
    GetBufferPointer, GetBufferSize)"""
    if not blob:
        return ""
    vtable = ctypes.cast(ctypes.cast(blob, ctypes.POINTER(ctypes.c_void_p))[0], ctypes.POINTER(ctypes.c_void_p))
    pointer = ctypes.WINFUNCTYPE(ctypes.c_void_p, ctypes.c_void_p)(vtable[3])(blob)
    size = ctypes.WINFUNCTYPE(ctypes.c_size_t, ctypes.c_void_p)(vtable[4])(blob)
    return ctypes.string_at(pointer, size).decode(errors="replace")


def main() -> int:
    if len(sys.argv) != 2:
        sys.exit(__doc__.strip().splitlines()[-1])
    compiler = ctypes.WinDLL("d3dcompiler_47.dll")
    files = sorted(Path(sys.argv[1]).glob("*.hlsl"))
    failed = 0
    for path in files:
        source = path.read_bytes()
        target = b"vs_5_0" if path.name.startswith("vs") else b"ps_5_0"
        code, errors = ctypes.c_void_p(), ctypes.c_void_p()
        result = compiler.D3DCompile(source, len(source), str(path).encode(), None, None, b"main", target,
                                     D3DCOMPILE_OPTIMIZATION_LEVEL3, 0, ctypes.byref(code), ctypes.byref(errors))
        if result & 0xffffffff:
            failed += 1
            print(f"{path.name}:\n{blob_text(errors.value).strip()}\n")
    print(f"{len(files) - failed} of {len(files)} compiled")
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
