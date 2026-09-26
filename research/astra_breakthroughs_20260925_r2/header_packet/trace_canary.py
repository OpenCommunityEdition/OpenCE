"""Observe the real allocator for unchanged source across the header packet.

No forced decisions. Header/source copies must be prepared by probe.py first.
The debugger, helper sources and compiler are hash-checked by the prior packet.
"""
from pathlib import Path
import hashlib
import importlib.util
import json
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
PRIOR = ROOT / "research/astra_breakthroughs_20260925/decal_clip"
sys.path.insert(0, str(ROOT))
sys.path.insert(0, str(PRIOR))
from verify_dependencies import verify
DEBUGGER = verify()
sys.path.insert(0, str(PRIOR / "dependencies"))
import trace as T
import oracle as O
import objfns
from tools import coff_compare as cc

T.WT = O.WT = objfns.WT = str(ROOT)
T.CL = str(ROOT / "xbox/bin/vc7/CL.Exe")
T.DEBUGGER = str(DEBUGGER)
ORIGINAL_FLAGS = T.cflags
UNIT = "source/rasterizer/rasterizer_frame_statistics"
FN = "_rasterizer_frame_statistics_draw"
OUT = ROOT / "scratch/astra_header_r2_20260925"


def sha(data):
    return hashlib.sha256(data).hexdigest()


def run(label):
    root = OUT / label
    source = root / (UNIT + ".c")
    work = OUT / ("trace_" + label)
    T.cflags = lambda unit: ["/I" + str(root / "source"), "/I" + str(root / "source/math")] + ORIGINAL_FLAGS(unit)
    seg, objpath = T.run_gated(str(source), str(work), UNIT, FN)
    decisions = O.analyse(seg)
    O.attach_terms(seg, decisions)
    model = O.check(decisions)
    instrumented = Path(objpath).read_bytes()
    (work / "instrumented.obj").write_bytes(instrumented)
    cmd = (work / "dbg_cmd.u16").read_text(encoding="utf-16-le").rstrip("\0")
    cwd = (work / "dbg_dir.u16").read_text(encoding="utf-16-le").rstrip("\0")
    replay = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True, timeout=60)
    replay.check_returncode()
    stock = Path(objpath).read_bytes()
    (work / "stock.obj").write_bytes(stock)
    l, r = bytearray(instrumented), bytearray(stock)
    l[4:8] = r[4:8] = b"\0" * 4
    observed, expected = cc.load(instrumented), cc.load(source.with_suffix(".obj").read_bytes())
    row = {"label": label, "command": cmd, "cwd": cwd,
           "source_sha256": sha(source.read_bytes()),
           "instrumented_sha256": sha(instrumented), "stock_sha256": sha(stock),
           "timestamp_only_equal": l == r,
           "target_body_matches_ungated_probe": cc.section_infos_equal(cc.section_info(observed, FN), cc.section_info(expected, FN)),
           "decisions": decisions, "model_checks": model, "pops": O.pops(seg)}
    (work / "receipt.json").write_text(json.dumps(row, indent=2) + "\n")
    print(label, "decisions", len(decisions), "model", model,
          "stock equal", l == r, "probe equal", row["target_body_matches_ungated_probe"], flush=True)
    if not l == r or not row["target_body_matches_ungated_probe"]:
        raise RuntimeError("Instrumented output did not reproduce expected compiler output")
    return row


if __name__ == "__main__":
    a, b = run("baseline"), run("owner")
    if len(a["decisions"]) != len(b["decisions"]):
        raise RuntimeError("Different trace lengths; positional comparison is invalid")
    keys = ("reg", "pri", "pos", "cost", "allowed", "init", "terms", "subs", "batch")
    differences = []
    for i, (x, y) in enumerate(zip(a["decisions"], b["decisions"])):
        change = {k: [x.get(k), y.get(k)] for k in keys if x.get(k) != y.get(k)}
        if change:
            differences.append({"decision": i, "differences": change})
    summary = {"baseline_decisions": len(a["decisions"]), "owner_decisions": len(b["decisions"]),
               "positional_decision_differences": differences,
               "limit": "Positional comparison is a diagnostic; matching records do not establish corresponding source symbols."}
    (OUT / "trace_comparison.json").write_text(json.dumps(summary, indent=2) + "\n")
    print(json.dumps(summary, indent=2))
