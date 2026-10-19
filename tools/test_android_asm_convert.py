"""The Darwin to ELF assembly conversion of the ILP32 guest
(tools/android_asm_convert.py): string literals keep every byte. It once took
a quoted .ascii operand for a quoted symbol name and dropped its leading "_"
(and, after an escaped quote, the "_" of any word after it), which shifted
tables such as Opus's pulse cache by a byte: voice chat crashed in
decode_pulses on the arm64 and Android builds.

Conversions from floating point to an integer in a SIMD register become
the vector form, which the x86 emulator's ARM translation has: the scalar
form stopped the app with SIGILL in hud_calculate_point once a game was
drawn."""
import re
import shutil
import struct
import subprocess
import sys
from pathlib import Path

import pytest

from tools.android_asm_convert import Converter

ROOT = Path(__file__).resolve().parent.parent


def convert(*lines):
    return Converter(list(lines)).run().splitlines()


@pytest.mark.parametrize("directive", [
    '.ascii\t"_c[}]L{s{"',
    '.asciz\t"_transport_error_none"',
    '.ascii\t"v\\177\\"O7v\\177_\\037\\374"',
    '.ascii\t"=`\'\\\\_\\026"',
    '.asciz\t"ltmp0 _x @PAGE ; not a comment"',
])
def test_string_literals_are_kept(directive):
    op, args = directive.split("\t", 1)
    assert convert("l_.str:", directive) == [".Ll_.str:", f"\t{op} {args}"]


def test_symbols_are_still_renamed():
    assert convert("\t.globl\t_table", "_table:", "\tadrp\tx8, _table@PAGE",
                   "\tadd\tx8, x8, _table@PAGEOFF", '\t.long\t"_odd name"') == [
        "\t.globl table", "table:", "\tadrp x8, table", "\tadd x8, x8, :lo12:table", '\t.long "odd name"']


def rodata(elf: bytes) -> bytes:
    """the .rodata section of a 64-bit little-endian ELF object"""
    shoff, = struct.unpack_from("<Q", elf, 0x28)
    shentsize, shnum, shstrndx = struct.unpack_from("<HHH", elf, 0x3a)
    sections = [struct.unpack_from("<IIQQQQ", elf, shoff + index * shentsize) for index in range(shnum)]
    names = sections[shstrndx][4]
    for name, _, _, _, offset, size in sections:
        if elf[names + name:elf.index(b"\0", names + name)] == b".rodata":
            return elf[offset:offset + size]
    raise AssertionError("no .rodata")


def test_tables_survive_the_build(tmp_path):
    """every byte value, and the patterns that were lost, through the
    guest's pipeline (clang for arm64_32, the conversion, the assembler)"""
    clang = shutil.which("clang")
    if not clang:
        pytest.skip("no clang")
    table = bytes(range(256)) + b'_a"_b\\_c' + bytes(reversed(range(256)))
    source = tmp_path / "table.c"
    source.write_text(
        "const unsigned char table[] = {" + ",".join(str(b) for b in table) + "};\n"
        'const char text[] = "_starts with an underscore";\n')
    darwin, converted, obj = tmp_path / "table.darwin.s", tmp_path / "table.s", tmp_path / "table.o"
    try:
        subprocess.run([clang, "--target=arm64_32-apple-watchos", "-O2", "-S", str(source), "-o", str(darwin)],
                       check=True, capture_output=True)
    except subprocess.CalledProcessError:
        pytest.skip("clang has no arm64_32 target")
    subprocess.run([sys.executable, str(ROOT / "tools" / "android_asm_convert.py"), str(darwin), str(converted)],
                   check=True)
    subprocess.run([clang, "--target=aarch64-linux-gnu", "-c", str(converted), "-o", str(obj)], check=True)
    data = rodata(obj.read_bytes())
    assert table in data
    assert b"_starts with an underscore\0" in data


@pytest.mark.parametrize("instruction, replacement", [
    ("\tfcvtzs\ts0, s0", ["\tfmov s0, s0", "\tfcvtzs v0.2s, v0.2s"]),
    ("\tfcvtzu\ts2, s17", ["\tfmov s2, s17", "\tfcvtzu v2.2s, v2.2s"]),
    ("\tfcvtms\td31, d4", ["\tfmov d31, d4", "\tfcvtms v31.2d, v31.2d"]),
    ("\tfcvtau\td1, d1", ["\tfmov d1, d1", "\tfcvtau v1.2d, v1.2d"]),
    ("\tfcvtns\ts3, s4, #4", ["\tfmov s3, s4", "\tfcvtns v3.2s, v3.2s, #4"]),
    ("\tfcvtpu\td5, d6, #16", ["\tfmov d5, d6", "\tfcvtpu v5.2d, v5.2d, #16"]),
])
def test_scalar_conversions_to_simd_registers_are_replaced(instruction, replacement):
    assert convert(instruction) == replacement


@pytest.mark.parametrize("instruction", [
    "\tfcvtzs\tw0, s0", "\tfcvtzu\tx3, d1", "\tfcvtzs\tv0.4s, v1.4s", "\tscvtf\ts0, s0", "\tucvtf\td1, d2",
    "\tfcvt\td0, s0", "\tfcvtzs\tw8, s0, #4",
])
def test_other_conversions_are_kept(instruction):
    op, args = instruction.strip().split("\t")
    assert convert(instruction) == [f"\t{op} {args}"]


SCALAR_FROM_SIMD = re.compile(r"^\s*fcvt[amnpz][su]\s+[sd]\d+\s*,", re.MULTILINE)


def test_compiled_conversions_are_replaced(tmp_path):
    """what clang makes of a rounded short (as hud_calculate_point's
    point), through the guest's pipeline: the converted assembly has no
    scalar form left, and assembles"""
    clang = shutil.which("clang")
    if not clang:
        pytest.skip("no clang")
    source = tmp_path / "point.c"
    source.write_text(
        "struct point { short x, y; };\n"
        "void calculate_point(float x, float y, struct point *result)\n"
        "{ result->x = (short)__builtin_rintf(x); result->y = (short)__builtin_rintf(y); }\n"
        "void store_doubles(double a, double b, long long *result) { result[0] = (long long)a; result[1] = (long long)b; }\n")
    darwin, converted, obj = tmp_path / "point.darwin.s", tmp_path / "point.s", tmp_path / "point.o"
    try:
        subprocess.run([clang, "--target=arm64_32-apple-watchos", "-mcpu=cortex-a53", "-O2", "-mllvm",
                        "-aarch64-neon-syntax=generic", "-S", str(source), "-o", str(darwin)],
                       check=True, capture_output=True)
    except subprocess.CalledProcessError:
        pytest.skip("clang has no arm64_32 target")
    if not SCALAR_FROM_SIMD.search(darwin.read_text()):
        pytest.skip("this clang converts in general registers")
    subprocess.run([sys.executable, str(ROOT / "tools" / "android_asm_convert.py"), str(darwin), str(converted)],
                   check=True)
    text = converted.read_text()
    assert not SCALAR_FROM_SIMD.search(text)
    assert re.search(r"fcvtzs v\d+\.2s, v\d+\.2s", text)
    subprocess.run([clang, "--target=aarch64-linux-gnu", "-c", str(converted), "-o", str(obj)], check=True)
