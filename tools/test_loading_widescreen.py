"""Run the production loading presenter against a hidden SDL/OpenGL context.

Requires clang and a built Windows SDL3 dependency, or SDL3's pkg-config file
on Linux (use Xvfb there). No maps or retail artwork are needed.
"""
from pathlib import Path
import os
import re
import shutil
import subprocess
import sys

import pytest

ROOT = Path(__file__).resolve().parents[1]


@pytest.mark.parametrize("gles", [False, True], ids=["desktop", "gles"])
def test_loading_presenter(tmp_path, gles):
    compiler = shutil.which("clang")
    if not compiler:
        pytest.skip("clang is required")
    sdl = sorted((ROOT / "build/windows/third_party").glob("SDL3-*/include/SDL3/SDL.h"))
    command = [compiler, "-std=gnu11", "-O2", "-Wall", "-Werror"]
    if sys.platform == "win32":
        if not sdl:
            pytest.skip("build Windows SDL3 first")
        dependency = sdl[-1].parents[2]
        command += ["--target=i686-pc-windows-msvc", "-fuse-ld=lld",
                    "-I" + str(dependency / "include")]
        libraries = [str(dependency / "lib/x86/SDL3.lib")]
        shutil.copy2(dependency / "lib/x86/SDL3.dll", tmp_path)
    else:
        if not shutil.which("pkg-config"):
            pytest.skip("pkg-config and SDL3 are required")
        result = subprocess.run(["pkg-config", "--cflags", "--libs", "sdl3"], capture_output=True, text=True)
        if result.returncode:
            pytest.skip("SDL3 development package is required")
        libraries = result.stdout.split()
    implementation = (ROOT / "port/linux/src/loading_widescreen.c").read_text()
    implementation = re.sub(r'^#include "[^\n]+"\s*$', '', implementation, flags=re.M)
    fixture = (ROOT / "tools/harness/tests/loading_widescreen.c").read_text()
    if gles:
        command += ["-DHALO_GLES"]
        fixture = fixture.replace("SDL_GL_CONTEXT_MAJOR_VERSION,4", "SDL_GL_CONTEXT_MAJOR_VERSION,3")
        fixture = fixture.replace("SDL_GL_CONTEXT_MINOR_VERSION,5", "SDL_GL_CONTEXT_MINOR_VERSION,0")
        fixture = fixture.replace("SDL_GL_CONTEXT_PROFILE_CORE", "SDL_GL_CONTEXT_PROFILE_ES")
    # The production GL calls use dynamically loaded pointers too. Load every
    # function the presenter or recording fixture actually calls.
    names = sorted(set(re.findall(r'\b(gl[A-Z]\w+)\s*\(', implementation + fixture)))
    declarations = "\n".join(f'static __typeof__(&{name}) ptr_{name};\n#define {name} ptr_{name}' for name in names)
    bindings = "\n".join(f'{name} = (__typeof__({name}))SDL_GL_GetProcAddress("{name}"); assert({name});' for name in names)
    source = fixture.replace("/* GL POINTERS */", declarations).replace("/* GL BINDINGS */", bindings)
    source = source.replace("/* PRODUCTION */", implementation)
    path = tmp_path / "loading.c"
    path.write_text(source)
    binary = tmp_path / "loading.exe"
    result = subprocess.run(command + [str(path), *libraries, "-o", str(binary)], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout + result.stderr
    result = subprocess.run([str(binary)], cwd=tmp_path, capture_output=True, text=True, timeout=40,
                            creationflags=subprocess.CREATE_NO_WINDOW if os.name == "nt" else 0)
    assert result.returncode == 0, result.stdout + result.stderr
