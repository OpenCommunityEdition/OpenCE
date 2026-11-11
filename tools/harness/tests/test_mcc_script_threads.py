"""Real MCC thread storage, frame validation and old checkpoint migration."""
import re
import os
import struct
import subprocess
import sys
import zlib
from pathlib import Path
import pytest
from tools.harness import read, run, structure, enum_with, function

ROOT = Path(__file__).resolve().parents[3]


@pytest.fixture(scope="module")
def thread_tool(tmp_path_factory):
    runtime = read("source/hs/hs_runtime.c")
    types = "\n".join(structure(runtime, name) for name in ("hs_stack_frame", "hs_thread_datum"))
    types += "\n" + structure(read("source/memory/data.h"), "data_array")
    types += "\n" + enum_with(runtime, "_hs_thread_type_script =")
    types += "\n" + enum_with(runtime, "HS_THREAD_STACK_SIZE =")
    types += "\n" + enum_with(runtime, "_hs_thread_in_function_call_bit =")
    checkpoint = read("port/linux/game/mcc_checkpoint.c")
    definitions = "\n".join(line for line in checkpoint.splitlines() if line.startswith("#define MCC_CHECKPOINT_"))
    definitions += "\n" + read("port/linux/game/mcc_campaign.h")
    definitions += "\n" + read("port/linux/game/mcc_object_names.h")
    # Headers only contribute the capacity constants; no private implementation is copied.
    definitions = "\n".join(line for line in definitions.splitlines() if line.startswith("#define MCC_"))
    implementation = re.sub(r'^#include .*$', '', read("port/linux/game/mcc_script_threads.inl"), flags=re.M)
    code = definitions + "\n" + function(checkpoint, "mcc_checkpoint_script_storage") + "\n" + implementation
    code += "\n" + function(runtime, "hs_stack_push") + "\n" + function(runtime, "hs_stack_allocate")
    generated = (("types.inc", types), ("under_test.inc", code))
    work = tmp_path_factory.mktemp("mcc-threads")
    for name, text in generated:
        (work / name).write_text(text)
    binary = work / ("threads.exe" if sys.platform == "win32" else "threads")
    zlib_dir = ROOT / "port/third_party/zlib"
    result = subprocess.run([os.environ.get("CC", "clang"), "-m32", "-std=gnu89", "-O2", "-Wall", "-Werror",
        "-Wno-unused-function", "-D_CRT_SECURE_NO_WARNINGS", "-DZ_PREFIX", "-Dz_errmsg=z_port_errmsg", "-I" + str(work),
        "-I" + str(zlib_dir), str(Path(__file__).with_name("mcc_script_threads.c")),
        str(zlib_dir / "crc32.c"), "-o", str(binary)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    return binary


@pytest.mark.parametrize("case", ["lifecycle", "capacity", "roundtrip", "migration", "checksum",
    "stride", "count", "pointer", "stack", "cycle", "overlap", "result", "size", "root", "other_thread", "bounds",
    "unactivated", "missing", "flags", "type", "sleep", "unused_slot"])
def test_thread_storage(thread_tool, case):
    status, output = run(thread_tool, case)
    assert status == 0, output


@pytest.mark.parametrize("size", ["null", "known", 0, 1, 7, 8, 9, 15, 16, 17, 255, 256, 257,
                                  32767, 32768, 32769, 1048576, 1054775, 1054776])
def test_thread_checksum_matches_independent_oracle(thread_tool, size):
    data = b"" if size == "null" else b"123456789" if size == "known" else bytes(
        (i * 37 + (i >> 8) + 19) & 255 for i in range(size))
    result = subprocess.run([str(thread_tool), "crc", str(size)], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout + result.stderr
    assert int(result.stdout, 16) == zlib.crc32(data)


def test_thread_persisted_checksum_bytes(thread_tool, tmp_path):
    output = tmp_path / "image.bin"
    result = subprocess.run([str(thread_tool), "snapshot", str(output)], capture_output=True, text=True)
    assert result.returncode == 0, result.stdout + result.stderr
    image = output.read_bytes()
    assert len(image) == 2 * 1024 * 1024
    offset = image.index(b"MCST")
    assert image.count(b"MCST") == 1
    magic, version, size, checksum = struct.unpack_from("<4s3I", image, offset)
    assert magic == b"MCST" and version == 1
    # The complete 256-thread array is protected, including unused stack bytes.
    assert size == 16 + 56 + 256 * (24 + 4096)
    assert checksum == zlib.crc32(image[offset + 16:offset + size])
    assert offset == (len(image) - (65544 + 768 + 96 + 131068 + 16) - size) & ~15
    assert image[offset + size:] == b"\xa5" * (len(image) - offset - size)
