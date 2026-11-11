"""Preliminary MCC sharing policy, portable names and bounded untrusted strings."""
from pathlib import Path
import shutil
import subprocess
import sys

import pytest
from mcc_test_support import compile_tool

ROOT = Path(__file__).resolve().parents[1]
RULES = ROOT / "port/linux/game/mcc_share_rules.c"
PACKAGE = ROOT / "port/linux/game/mcc_package.c"
DRIVER = ROOT / "tools/tests/mcc_share_rules.c"
CASES = ["policy", "stock", "workshop", "states", "names", "lengths", "resources", "reasons"]


@pytest.fixture(scope="module")
def share_tool(tmp_path_factory):
    return compile_tool(tmp_path_factory.mktemp("mcc-share"), [DRIVER, RULES, PACKAGE])


@pytest.mark.parametrize("case", CASES)
def test_share_rules(share_tool, case):
    result = subprocess.run([str(share_tool), case], capture_output=True, text=True, timeout=30)
    assert result.returncode == 0, result.stderr


@pytest.mark.parametrize("before,after,case", [
    ("package->sidecar!=MCC_SHARE_SIDECAR_ABSENT",
     "package->sidecar==MCC_SHARE_SIDECAR_UNREADABLE", "states"),
    ("package->content==MCC_SHARE_CONTENT_STOCK", "0", "stock"),
    ("memchr(package->name,0,package->name_length)", "0", "lengths"),
    ('mcc_share_same(device,"con")', '0', "names"),
    ('mcc_share_same(level,"bitmaps")', '0', "names"),
    ('if (!package->host_opt_in)', 'if (0)', "policy"),
])
def test_negative_controls(tmp_path, before, after, case):
    """Each selected regression must detect its intended fault in the real code."""
    source = RULES.read_text()
    assert source.count(before) == 1
    changed = tmp_path / "rules.c"
    changed.write_text(source.replace(before, after))
    exe = compile_tool(tmp_path, [DRIVER, changed, PACKAGE])
    result = subprocess.run([str(exe), case], capture_output=True, text=True, timeout=30)
    assert result.returncode == 1, result.stderr


def test_sanitized_rules(tmp_path):
    """Linux CI executes every case with address/undefined-behaviour checks."""
    if not sys.platform.startswith("linux"):
        pytest.skip("sanitizer runtime exercised by Linux CI")
    clang = shutil.which("clang")
    assert clang, "clang is required for the Linux security checks"
    exe = tmp_path / "sanitized"
    result = subprocess.run([clang, "-std=gnu89", "-g", "-O1", "-Wall", "-Wextra", "-Werror",
                             "-fsanitize=address,undefined", "-fno-sanitize-recover=all",
                             "-I", str(RULES.parent), str(DRIVER), str(RULES), str(PACKAGE),
                             "-o", str(exe)], capture_output=True, text=True)
    assert result.returncode == 0, result.stderr
    for case in CASES:
        result = subprocess.run([str(exe), case], capture_output=True, text=True, timeout=30)
        assert result.returncode == 0, result.stderr


def test_fuzzed_rules(tmp_path):
    """Bounded libFuzzer run over exact-length buffers, states and resource leaves."""
    if not sys.platform.startswith("linux"):
        pytest.skip("libFuzzer runtime exercised by Linux CI")
    clang = shutil.which("clang")
    assert clang, "clang is required for the Linux security checks"
    source = tmp_path / "fuzz.c"
    source.write_text(r'''
#include "mcc_share_rules.h"
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
int LLVMFuzzerTestOneInput(uint8_t const *data,size_t size) {
 struct mcc_share_package p;
 enum mcc_share_reason reason;
 size_t length;
 char *name;
 if(size<4)return 0;
 length=size-4;
 /* ASan catches even a one-byte overread beyond the declared string. */
 name=(char *)malloc(length ? length : 1);
 if(!name)abort();
 memcpy(name,data+4,length);
 memset(&p,0,sizeof(p));
 p.name=name;p.name_length=length;
 p.sidecar=(enum mcc_share_sidecar)(int8_t)data[0];
 p.content=(enum mcc_share_content)(int8_t)data[1];
 p.content_known=(int8_t)data[2];p.host_opt_in=(int8_t)data[3];
 reason=mcc_share_decide(&p);
 if(mcc_share_eligible(&p)!=(reason==MCC_SHARE_ELIGIBLE))abort();
 if(reason==MCC_SHARE_ELIGIBLE &&
    ((p.sidecar!=MCC_SHARE_SIDECAR_ABSENT && p.sidecar!=MCC_SHARE_SIDECAR_VALID) || p.content!=MCC_SHARE_CONTENT_CUSTOM ||
     !p.content_known || !p.host_opt_in || length>63 || memchr(name,0,length)))abort();
 if(!mcc_share_reason_word(reason))abort();
 if(mcc_share_file_allowed(name,length) && (length<10 || length>11 || memchr(name,0,length)))abort();
 p.name_length=(size_t)-1;
 if(mcc_share_decide(&p)!=MCC_SHARE_BAD_NAME || mcc_share_file_allowed(name,(size_t)-1))abort();
 free(name);
 return 0;
}
''')
    exe = tmp_path / "fuzz"
    built = subprocess.run([clang, "-std=gnu89", "-g", "-O1", "-Wall", "-Wextra", "-Werror",
                            "-fsanitize=fuzzer,address,undefined", "-fno-sanitize-recover=all",
                            "-I", str(RULES.parent), str(source), str(RULES), str(PACKAGE),
                            "-o", str(exe)], capture_output=True, text=True)
    assert built.returncode == 0, built.stderr
    corpus = tmp_path / "corpus"
    corpus.mkdir()
    seeds = [b"mcc_maps/custom/level", b"mcc_maps/custom/CON", b"mcc_maps/123/a10",
             b"mcc_maps/BloodGulch/x", b"mcc_maps/_shared/x", b"mcc_maps/custom/bitmaps",
             b"mcc_maps/a/../b", b"mcc_maps/a/b\0suffix", b"bitmaps.map", b"SOUNDS.MAP"]
    for i, name in enumerate(seeds):
        for sidecar in (0, 1):  # Missing and valid Workshop origins obey the same content policy.
            (corpus / f"{i}-{sidecar}").write_bytes(bytes([sidecar, 1, 1, 1]) + name)
    result = subprocess.run([str(exe), "-runs=100000", "-seed=94", "-max_len=132",
                             "-timeout=5", "-rss_limit_mb=512", str(corpus)],
                            cwd=tmp_path, capture_output=True, text=True, timeout=90)
    print(result.stderr)
    assert result.returncode == 0, result.stderr
