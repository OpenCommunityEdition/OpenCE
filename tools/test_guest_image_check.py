"""Tests of tools/guest_image_check.py on small x32 images: one that keeps
its thread-local variable emulated passes, and one whose variable uses the
thread pointer (%fs) is refused, as is one linked outside its range. Images
linked with the guest's own script (port/android/guest/guest.ld) test the
constructors: an ELF guest's 4-byte .init_array entries pass, an 8-byte one
is refused, and a Mach-O guest's link discards .init_array."""
import platform
import shutil
import subprocess
import sys
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(ROOT / "tools"))

from guest_arch import ARM64_32, X32  # noqa: E402
from guest_image_check import map_problems  # noqa: E402

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


GUEST_LD = ROOT / "port" / "android" / "guest" / "guest.ld"
# (volatile: clang evaluates a constructor that only stores a constant)
CONSTRUCTOR = ("__attribute__((section(\".guest_header\"), used)) const int __guest_header = 1;\n"
               "volatile int constructed;\n"
               "__attribute__((constructor)) static void construct(void) { constructed = 1; }\n")
# (a constructor of 8 bytes, as an ELF64 object's .init_array has)
WIDE_CONSTRUCTOR = "__asm__(\".section .init_array,\\\"aw\\\"\\n.quad start\\n.text\");\n"


def link_guest(tmp_path, extra, fragments="elf"):
    """an x32 image linked as the guest's (tools/android_build.py), with
    the constructors of the fragments' folder, and its map"""
    image = build(tmp_path, ["-femulated-tls"], extra=EMUTLS + extra)
    guest = tmp_path / "guest.elf"
    subprocess.run([shutil.which("ld.lld"), "-m", X32.ld_emulation, "-static", "-nostdlib", "-T", str(GUEST_LD),
                    "-L", str(GUEST_LD.parent / "ld" / fragments), f"--defsym=__guest_image_base={X32.image_base:#x}",
                    "-Map", f"{guest}.map", "-o", str(guest), str(tmp_path / "image.o")], check=True)
    assert image.exists()
    return guest


def check_guest(tmp_path, image):
    return subprocess.run([sys.executable, str(ROOT / "tools" / "guest_image_check.py"), "--arch", "x32",
                           "--map", f"{image}.map", str(image), str(tmp_path / "stamp")],
                          capture_output=True, text=True)


def section_size(image, name):
    output = subprocess.run([shutil.which("llvm-objdump") or "llvm-objdump", "-h", str(image)], capture_output=True,
                            text=True, check=True).stdout
    sizes = [int(line.split()[2], 16) for line in output.splitlines() if len(line.split()) > 2
             and line.split()[1] == name]
    return sizes[0] if sizes else None


def test_constructors_pass(tmp_path):
    image = link_guest(tmp_path, CONSTRUCTOR)
    assert section_size(image, ".guest_init") == 4
    result = check_guest(tmp_path, image)
    assert result.returncode == 0, result.stderr


def test_wide_constructor_refused(tmp_path):
    result = check_guest(tmp_path, link_guest(tmp_path, CONSTRUCTOR + WIDE_CONSTRUCTOR))
    assert result.returncode == 1 and "is not the address of code" in result.stderr


def test_mach_o_guest_discards_init_array(tmp_path):
    image = link_guest(tmp_path, CONSTRUCTOR + WIDE_CONSTRUCTOR, fragments="macho")
    assert section_size(image, ".guest_init") == 0
    assert section_size(image, ".init_array") is None


def test_map_names_foreign_constructors():
    link_map = (
        "     VMA      LMA     Size Align Out     In      Symbol\n"
        "88400000 88400000       c     8 .guest_init\n"
        "88400000 88400000       0     1         __guest_init_start = .\n"
        "88400000 88400000       4     4         obj/game.o:(.guest_init)\n"
        "88400008 88400008       8     8         libclang_rt.builtins-aarch64.a(cpu_model.o):(.init_array.90)\n"
        "88400010 88400010       0     1         __guest_init_end = .\n"
        "88410000 88410000     1000    16 .bss\n"
        "88410000 88410000     1000    16         obj/game.o:(.init_array)\n"
    )
    problems = map_problems(link_map, ARM64_32)
    assert len(problems) == 1 and "cpu_model.o" in problems[0]
    assert len(map_problems(link_map, X32)) == 1
