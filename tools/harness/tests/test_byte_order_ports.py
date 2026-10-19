"""The byte order conversions of internet play (ports and addresses), run as a little-endian and as a big-endian machine
runs them (the Mac's PowerPC): a host order value is already in network order there, so it must pass through. The
negative controls use the old unconditional swaps, which the big-endian run must catch."""

import re
import sys
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import CHECK_FAILED, build, function, read, run  # noqa: E402

CASES = ["signal-port", "upnp-port", "p2p-short", "p2p-long"]

OLD_SHORT = "{ return (unsigned short)(value << 8 | value >> 8); }"


def renamed(code, old, new):
    return re.sub(r"\b" + old + r"\b", new, code)


def generated(big_endian, old=False):
    signal = function(read("port/linux/src/p2p_signal.c"), "network_short")
    upnp = function(read("port/linux/src/posix_upnp.c"), "swap_short")
    p2p_source = read("port/linux/src/p2p.c")
    short = function(p2p_source, "network_short")
    long_ = function(p2p_source, "network_long")
    probe = function(p2p_source, "host_is_little_endian")
    if old:
        signal = signal[:signal.index("{")] + OLD_SHORT
        upnp = upnp[:upnp.index("{")] + OLD_SHORT
        short = short[:short.index("{")] + OLD_SHORT
        long_ = long_[:long_.index("{")] + "{ return ((value & 0xFF) << 24) | ((value & 0xFF00) << 8) | ((value >> 8) & 0xFF00) | ((value >> 24) & 0xFF); }"
    code = probe + renamed(signal, "network_short", "signal_network_short") + renamed(upnp, "swap_short", "upnp_swap_short")
    code += renamed(short, "network_short", "p2p_network_short") + renamed(long_, "network_long", "p2p_network_long")
    if big_endian:
        # (the byte that tells the order, read from the other end of the short: 0 on a big-endian machine)
        code = code.replace("*(const unsigned char *)&probe", "*((const unsigned char *)&probe + 1)")
    # a swap on a little-endian host, none on a big-endian one
    swap = "SWAP" if not big_endian else "KEEP"
    defs = f"""
#define SWAP(v) ((unsigned short)((v) << 8 | (v) >> 8))
#define KEEP(v) ((unsigned short)(v))
#define EXPECT_SHORT(v) {swap}(v)
#define EXPECT_LONG(v) ({'(((v) & 0xFF) << 24) | (((v) & 0xFF00) << 8) | (((v) >> 8) & 0xFF00) | (((v) >> 24) & 0xFF)' if not big_endian else '((v) & 0xFFFFFFFFUL)'})
"""
    return (("snippets.inc", defs + code),)


@pytest.mark.parametrize("big_endian", [False, True])
@pytest.mark.parametrize("case", CASES)
def test_case(case, big_endian):
    status, output = run(build("byte_order_ports", generated(big_endian)), case)
    assert status == 0, output


@pytest.mark.parametrize("case", CASES)
def test_negative_control_old_swaps_fail_big_endian(case):
    status, output = run(build("byte_order_ports", generated(True, old=True)), case)
    assert status == CHECK_FAILED, f"the old swap passed '{case}' as a big-endian machine runs it: {output}"


@pytest.mark.parametrize("case", CASES)
def test_old_swaps_pass_little_endian(case):
    status, output = run(build("byte_order_ports", generated(False, old=True)), case)
    assert status == 0, output
