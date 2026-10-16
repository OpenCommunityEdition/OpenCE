"""The Darwin to ELF assembly conversion of the ILP32 guest
(tools/android_asm_convert.py): string literals keep every byte. It once took
a quoted .ascii operand for a quoted symbol name and dropped its leading "_"
(and, after an escaped quote, the "_" of any word after it), which shifted
tables such as Opus's pulse cache by a byte: voice chat crashed in
decode_pulses on the arm64 and Android builds."""
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
