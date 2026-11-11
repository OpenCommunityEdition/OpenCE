"""Run the production map-window code with the real Wasm allocator and threads."""
import os
from pathlib import Path
import shutil
import subprocess

import pytest

ROOT = Path(__file__).resolve().parents[1]


@pytest.mark.parametrize("memory", [0x01000000, 0x88000000])
def test_wasm_map_memory_and_heap_exhaustion(tmp_path, memory):
    emcc = os.environ.get("WEB_EMCC") or shutil.which("emcc")
    node = os.environ.get("EMSDK_NODE") or shutil.which("node")
    if not emcc or not node:
        pytest.skip("requires Emscripten and Node for the actual Wasm allocator")
    output = tmp_path / "map_memory.js"
    subprocess.run([
        emcc, str(ROOT / "port/web/tests/map_memory_test.c"),
        str(ROOT / "port/web/src/web_memory.c"),
        "-I" + str(ROOT / "port/web/src"),
        "-I" + str(ROOT / "port/linux/src"),
        "-DHALO_WEB=1", "-std=gnu89", "-O2", "-pthread",
        "-sPROXY_TO_PTHREAD=1", "-sPTHREAD_POOL_SIZE=5",
        "-sALLOW_MEMORY_GROWTH=0", "-sABORTING_MALLOC=0",
        "-sEXIT_RUNTIME=1", "-sENVIRONMENT=node",
        f"-sINITIAL_MEMORY={memory}", "-o", str(output),
    ], check=True, cwd=ROOT, capture_output=True, text=True, timeout=120)
    result = subprocess.run([node, str(output)], cwd=ROOT, capture_output=True,
                            text=True, timeout=90)
    assert result.returncode == 0, result.stdout + result.stderr
    expected = "map windows preserved" if memory == 0x88000000 else "insufficient map-window memory refused"
    assert expected in result.stdout, result.stdout + result.stderr
