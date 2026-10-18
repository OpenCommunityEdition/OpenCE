"""The Bink decoder (port/linux/src/bink_decode.c, bink_tables.c), checked
with the player's own files.

The decoder's tables are read from the Xbox disc's default.xbe at run time and
its movies are the disc's (the bink folder): neither is in this repository,
and no test here keeps a byte of them. Name them with environment variables
and the tests that need them run; without, they are skipped:

    HALO_BINK_XBE=/path/to/default.xbe HALO_BINK_MOVIE=/path/to/intro.bik \
        python -m pytest -q tools/test_bink.py

HALO_BINK_FUZZ sets the number of damaged copies of the movie that the
fuzzer decodes (default 300).
"""

import os
import shutil
import subprocess
from pathlib import Path

import pytest

ROOT = Path(__file__).resolve().parent.parent
SOURCES = ["port/linux/src/bink_decode.c", "port/linux/src/bink_tables.c"]


def compiler():
    for name in ("clang", "cc", "gcc"):
        if shutil.which(name):
            return name
    pytest.skip("needs a C compiler")


def build(tmp_path, program, extra=(), flags=("-O1",)):
    output = tmp_path / program
    command = [compiler(), *flags, "-I", "port/linux/src", "-o", str(output), f"port/tools/bink_{program}.c",
               *SOURCES, *extra, "-lm"]
    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
    assert result.returncode == 0, result.stderr[-4000:]
    return output


def player_files():
    xbe = os.environ.get("HALO_BINK_XBE")
    movie = os.environ.get("HALO_BINK_MOVIE")
    if not xbe or not movie or not Path(xbe).is_file() or not Path(movie).is_file():
        pytest.skip("set HALO_BINK_XBE and HALO_BINK_MOVIE to the player's default.xbe and a movie of the disc")
    return xbe, movie


def test_tables_refuse_what_is_not_the_library(tmp_path):
    """no tables from a file that is not an Xbox executable, from one cut
    short or from an executable without the library"""
    probe = build(tmp_path, "probe")
    files = {
        "empty": b"",
        "text": b"not an executable" * 100,
        "header only": b"XBEH" + bytes(0x400),
    }
    for name, data in files.items():
        path = tmp_path / f"{name.replace(' ', '_')}.xbe"
        path.write_bytes(data)
        result = subprocess.run([str(probe), str(path), str(path)], capture_output=True, text=True)
        assert result.returncode == 1, name
        assert "tables:" in result.stderr, (name, result.stderr)


def test_movie_decodes(tmp_path):
    """every frame of a movie decodes, and its audio is as long as its
    video"""
    xbe, movie = player_files()
    probe = build(tmp_path, "probe", flags=("-O2",))
    result = subprocess.run([str(probe), xbe, movie], capture_output=True, text=True, timeout=600)
    assert result.returncode == 0, result.stderr
    lines = result.stdout.splitlines()
    assert "revision" in lines[0]
    frames = int(lines[0].split(",")[1].split()[0])
    assert lines[1].startswith(f"decoded {frames} frames"), lines
    audio = float(lines[2].split("=")[1].split("s")[0])
    video = float(lines[2].split("(video")[1].split("s")[0])
    assert abs(audio - video) < 0.1, lines[2]


def test_damaged_movies_fail_cleanly(tmp_path):
    """damaged and truncated copies of a movie never read or write out of
    bounds (the address and undefined behaviour sanitizers check)"""
    xbe, movie = player_files()
    if compiler() != "clang":
        pytest.skip("needs clang's sanitizers")
    fuzz = build(tmp_path, "fuzz", flags=("-O1", "-g", "-fsanitize=address,undefined",
                                         "-fno-sanitize-recover=undefined"))
    iterations = os.environ.get("HALO_BINK_FUZZ", "300")
    result = subprocess.run([str(fuzz), xbe, movie, iterations, "1", "40"], capture_output=True, text=True,
                            timeout=1800)
    assert result.returncode == 0, result.stderr[-4000:]
    assert "damaged copies" in result.stdout
