"""Observe the real compiler's allocator; no forced register decisions.

Reuses the existing proven Lane-A same-bitness debugger as a read-only
instrument. Its configuration and all output live in our scratch directory.
Original helper modules and compiler binaries remain unchanged.
"""
from pathlib import Path
import hashlib
import json
import sys
import os
import probe
from verify_dependencies import verify

LIB = Path(__file__).resolve().parent / "dependencies"
DEBUGGER = verify()
sys.path.insert(0, str(LIB))
import trace as T
T.WT = str(probe.ROOT)
T.CL = str(probe.ROOT / "xbox/bin/vc7/CL.Exe")
T.DEBUGGER = str(DEBUGGER)
import oracle as O
O.WT = str(probe.ROOT)
import objfns
objfns.WT = str(probe.ROOT)
for module in (T, O, objfns):
    if Path(module.__file__).resolve().parent != LIB.resolve():
        raise RuntimeError("Unexpected helper import: " + str(module.__file__))

def trace(label):
    source = probe.OUT / (label + ".c")
    work = probe.OUT / ("allocator_" + label)
    hashes_before = {name: hashlib.sha256((probe.ROOT / "xbox/bin/vc7" / name).read_bytes()).hexdigest() for name in ("CL.Exe", "C1.Dll", "C2.Dll")}
    seg, obj = T.run_gated(str(source), str(work), probe.UNIT, probe.NAME)
    dec = O.analyse(seg)
    O.attach_terms(seg, dec)
    checks = O.check(dec)
    plain_obj = probe.cc.load(source.with_suffix(".obj").read_bytes())
    traced_obj = probe.cc.load(Path(obj).read_bytes())
    same = probe.cc.section_infos_equal(probe.cc.section_info(plain_obj, probe.NAME), probe.cc.section_info(traced_obj, probe.NAME))
    hashes_after = {name: hashlib.sha256((probe.ROOT / "xbox/bin/vc7" / name).read_bytes()).hexdigest() for name in hashes_before}
    result = {"label": label, "code_unchanged_by_instrument": same, "compiler_unchanged": hashes_before == hashes_after, "compiler_sha256": hashes_after, "checks": checks, "decisions": dec, "pops": O.pops(seg)}
    (work / "analysis.json").write_text(json.dumps(result, indent=2) + "\n")
    print(label, "same", same, "compiler unchanged", hashes_before == hashes_after, "checks", checks)
    for k, d in enumerate(dec):
        print(k, "id", d["id"], "reg", d["reg"], "pos", hex(d["pos"]), "priority", d["pri"], "terms", d["terms"])
    return result

if __name__ == "__main__":
    for label in sys.argv[1:] or ["S4_current_headers", "C_canonical_context"]:
        trace(label)
