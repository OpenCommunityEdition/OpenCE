"""Bounded decal-clip differential experiments; never edits production.

Run from the canonical repo: python research/astra_breakthroughs_20260925/decal_clip/probe.py
The source variants are mechanical derivatives of the preserved Claude S4 TU.
Type-width probes are diagnostic only, not proposed source reconstructions.
"""
from pathlib import Path
import difflib
import hashlib
import json
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / "scratch/astra_clip_20260925"
OUT.mkdir(parents=True, exist_ok=True)
sys.path.insert(0, str(ROOT))
from tools import coff_compare as cc
from tools.campaign import alndiff

DONOR = Path(r"C:\halo-worktrees\claude-decals-physics-20260924\scratch\ledger\cand_decals_clip_S4.c")
NAME = "_decal_clip_to_surface"
UNIT = "source/effects/decals"
source = DONOR.read_text(encoding="latin-1")
start = source.index("static void decal_clip_to_surface(", source.index("static void decal_clip_to_surface(") + 1)
end = source.index("\n}\n", start) + 2
body = source[start:end]
target = cc.load((ROOT / f"build/split/{UNIT}.obj").read_bytes())
ti = cc.section_info(target, NAME)
td = alndiff.disassemble_function(target, NAME)

COUNTER = [("short edge_iteration = 0;", "long edge_iteration = 0;")]
SIDE = [("boolean surface_on_right = edge->surface_indices[1]==surface_index;", "long surface_on_right = edge->surface_indices[1]==surface_index;")]
VARIANTS = {
    "S4_current_headers": [],
    "A_counter_long_diagnostic": COUNTER,
    "B_side_long_diagnostic": SIDE,
    "AB_widths_diagnostic": COUNTER + SIDE,
}

def run(label, edits, context="donor", cflags=(), global_edits=()):
    code = body
    for old, new in edits:
        assert old in code, (label, old)
        code = code.replace(old, new)
    path = OUT / (label + ".c")
    prefix, suffix = source[:start], source[end:]
    if context == "canonical":
        canonical = (ROOT / (UNIT + ".c")).read_text(encoding="latin-1")
        cs = canonical.index("static void decal_clip_to_surface(", canonical.index("static void decal_clip_to_surface(") + 1)
        ce = canonical.index("\n}\n", cs) + 2
        prefix, suffix = canonical[:cs], canonical[ce:]
    complete = prefix + code + suffix
    for old, new in global_edits:
        assert complete.count(old) == 1, (label, old, complete.count(old))
        complete = complete.replace(old, new)
    path.write_text(complete, encoding="latin-1", newline="\n")
    objpath = path.with_suffix(".obj")
    command = [sys.executable, "tools/campaign/gate.py", UNIT, "--source", str(path), "--all", "--out", str(objpath)]
    for flag in cflags:
        command += ["--cflag", flag]
    result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
    path.with_suffix(".gate.txt").write_text(result.stdout + result.stderr)
    if result.returncode:
        return {"label": label, "error": result.stdout + result.stderr}
    obj = cc.load(objpath.read_bytes())
    info = cc.section_info(obj, NAME)
    od = alndiff.disassemble_function(obj, NAME)
    align = alndiff.aligned_opcodes(td, od)
    asm = "\n".join(f"{i.offset:04x} {i.rendered}" for i in od)
    path.with_suffix(".asm.txt").write_text(asm)
    diff = subprocess.run([sys.executable, "tools/campaign/alndiff.py", UNIT, NAME, "--ours-object", str(objpath), "--max-lines", "0"], cwd=ROOT, capture_output=True, text=True)
    path.with_suffix(".diff.txt").write_text(diff.stdout)
    row = {
        "label": label,
        "source_sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
        "normalized_sha256": info["normalized_sha256"],
        "size": info["size"],
        "instructions": len(od),
        "diff_blocks": sum(t != "equal" for t, *rest in align),
        "exact": cc.section_infos_equal(ti, info),
        "gate_target": next(line for line in result.stdout.splitlines() if NAME in line),
        "frame": next(i.rendered for i in od if i.rendered.startswith("sub esp,")),
        "tail": [f"{i.offset:04x} {i.rendered}" for i in od if 0x4cc <= i.offset <= 0x512],
    }
    print(json.dumps(row), flush=True)
    return row

if __name__ == "__main__":
    labels = sys.argv[1:] or list(VARIANTS)
    rows = [run(label, VARIANTS[label]) for label in labels]
    (OUT / "measurements.json").write_text(json.dumps(rows, indent=2) + "\n")
