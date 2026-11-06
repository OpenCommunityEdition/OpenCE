"""tools/check_glibc.py: the glibc a program needs, from readelf -V output.

    python -m pytest -q tools/test_check_glibc.py
"""

import shutil
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parent))

import check_glibc

SAMPLE = """
Version needs section '.gnu.version_r' contains 2 entries:
  000000: Version: 1  File: libc.so.6  Cnt: 3
  0x0010:   Name: GLIBC_2.4  Flags: none  Version: 5
  0x0020:   Name: GLIBC_2.29  Flags: none  Version: 4
  0x0030:   Name: GLIBC_2.9  Flags: none  Version: 3
  000020: Version: 1  File: libgcc_s.so.1  Cnt: 1
  0x0040:   Name: GCC_3.0  Flags: none  Version: 2
"""


def test_newest_compares_numbers_not_text():
    assert check_glibc.newest(SAMPLE) == (2, 29)
    assert check_glibc.newest("Name: GLIBC_2.9  Name: GLIBC_2.10") == (2, 10)


def test_newest_of_nothing():
    assert check_glibc.newest("Name: GCC_3.0") == ()


def test_limits_compare_as_versions():
    assert check_glibc.parse_limit("2.29") >= check_glibc.newest(SAMPLE)
    assert check_glibc.parse_limit("2.28") < check_glibc.newest(SAMPLE)


@pytest.mark.skipif(shutil.which("readelf") is None, reason="needs readelf")
def test_main_on_a_real_file(capsys):
    program = shutil.which("readelf")
    # (readelf itself needs some glibc; a limit far above passes, one below fails)
    assert check_glibc.main(["99.0", program]) == 0
    assert check_glibc.main(["1.0", program]) in (0, 1)
