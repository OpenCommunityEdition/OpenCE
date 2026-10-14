"""Tests of tools/guest_image_check.py on small x32 images: one that keeps
its thread-local variable emulated passes, and one whose variable uses the
thread pointer (%fs) is refused, as is one linked outside its range."""
import platform
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

from guest_arch import X32  # noqa: E402

SOURCE = "__thread int counter;\nint start(void) { return ++counter; }\n"
EMUTLS = "void *__emutls_get_address(void *control) { return control; }\n"


def build(tmp_path, flags, base=X32.image_base, extra=""):
    cc, ld = shutil.which("clang"), shutil.which("ld.lld")
    if not (cc and ld and shutil.which("llvm-objdump")):
        pytest.skip("needs clang, ld.lld and llvm-objdump")
    source = tmp_path / "image.c"
    source.write_text(SOURCE + extra)
    subprocess.run([cc, *X32.target_flags, "-O2", "-ffreestanding", *flags, "-c", str(source),
                    "-o", str(tmp_path / "image.o")], check=True)
    image = tmp_path / "image.elf"
    subprocess.run([ld, "-m", X32.ld_emulation, "-static", "-nostdlib", "-e", "start", f"-Ttext={base:#x}",
                    f"--image-base={base:#x}", str(tmp_path / "image.o"), "-o", str(image)], check=True)
    return image


def check(tmp_path, image):
    return subprocess.run([sys.executable, str(ROOT / "tools" / "guest_image_check.py"), "--arch", "x32", str(image),
                           str(tmp_path / "stamp")], capture_output=True, text=True)


def test_emulated_tls_passes(tmp_path):
    result = check(tmp_path, build(tmp_path, ["-femulated-tls"], extra=EMUTLS))
    assert result.returncode == 0, result.stderr


def test_thread_pointer_refused(tmp_path):
    result = check(tmp_path, build(tmp_path, []))
    assert result.returncode == 1
    assert "PT_TLS" in result.stderr or "thread pointer" in result.stderr


def test_outside_range_refused(tmp_path):
    result = check(tmp_path, build(tmp_path, ["-femulated-tls"], base=0x40000000, extra=EMUTLS))
    assert result.returncode == 1 and "outside" in result.stderr


def test_segment_override_refused(tmp_path):
    reader = "int thread_word(void) { int value; __asm__(\"movl %%fs:0, %0\" : \"=r\" (value)); return value; }\n"
    result = check(tmp_path, build(tmp_path, ["-femulated-tls"], extra=EMUTLS + reader))
    assert result.returncode == 1 and "thread_word uses the host's thread pointer" in result.stderr
