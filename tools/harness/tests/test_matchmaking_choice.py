"""The MATCHMAKING screen's rules (menu_functions.c)."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, enum_with, function, mutated, read, run  # noqa: E402

CASES = ["ctf_only"]

NEGATIVE_CONTROLS = {
    "ffa-always-allowed": (("size != P2P_MM_FFA_6 ||", "1 ||"), "ctf_only"),
    "ctf-has-ffa": (("playlists != 1u << P2P_MM_CTF", "1"), "ctf_only"),
    "greyed-without-a-playlist": (("playlists != 1u << P2P_MM_CTF", "(playlists & ~(1u << P2P_MM_CTF))"), "ctf_only"),
}


def generated(fault=None):
    source = read("port/linux/game/menu_functions.c")
    header = read("port/linux/src/p2p.h")
    names = re.search(r"^const char \*const p2p_mm_playlist_names\[.*$", read("port/linux/src/p2p_matchmaking.c"),
                      re.M).group(0)
    code = "\n".join([function(read("port/linux/src/p2p_matchmaking.c"), "mm_playlist_has_size")] +
                     [function(source, "matchmaking_size_after")])
    if fault:
        code = mutated(code, *fault)
    return (("under_test.inc", "\n".join([enum_with(header, "P2P_MM_SLAYER"), enum_with(header, "P2P_MM_1V1"),
                                          names, code]) + "\n"),)


@pytest.mark.parametrize("case", CASES)
def test_case(case):
    status, output = run(build("matchmaking_choice", generated()), case)
    assert status == 0, output


@pytest.mark.parametrize("control", NEGATIVE_CONTROLS)
def test_negative_control(control):
    fault, case = NEGATIVE_CONTROLS[control]
    status, output = run(build("matchmaking_choice", generated(fault)), case)
    assert status == CHECK_FAILED, f"menu_functions.c with a fault ({control}) passed '{case}': the test cannot see it"
