"""Create compact, reproducible source deltas and checked measurements."""
import difflib
import hashlib
import json
from pathlib import Path
import subprocess
import probe

HERE = Path(__file__).resolve().parent
PATCHES = HERE / "patches"
PATCHES.mkdir(exist_ok=True)
results = []
for fn in ("measurements.json", "context_measurements.json", "storage_measurements.json", "flags_measurements.json"):
    results.extend(json.loads((probe.OUT / fn).read_text()))

def patch(before, after, name, before_name, after_name):
    value = "".join(difflib.unified_diff(before.splitlines(keepends=True), after.splitlines(keepends=True), fromfile=before_name, tofile=after_name, n=3))
    (PATCHES / name).write_text(value)
    return {"path": "patches/" + name, "sha256": hashlib.sha256(value.encode()).hexdigest(), "bytes": len(value.encode())}

canonical = (probe.ROOT / "source/effects/decals.c").read_text(encoding="latin-1")
base = (probe.OUT / "C_canonical_context.c").read_text(encoding="latin-1")
base_patch = patch(canonical, base, "canonical_to_S4.patch", "a/source/effects/decals.c", "b/source/effects/decals.c")
donor_base = (probe.OUT / "S4_current_headers.c").read_text(encoding="latin-1")
for row in results:
    label = row["label"]
    p = probe.OUT / (label + ".c")
    candidate = probe.cc.load(p.with_suffix(".obj").read_bytes())
    info = probe.cc.section_info(candidate, probe.NAME)
    row["relocations_strict_equal"] = probe.cc.relocation_infos_equal(probe.ti["relocations"], info["relocations"])
    row["object_sha256"] = hashlib.sha256(p.with_suffix(".obj").read_bytes()).hexdigest()
    row["source_file"] = str(p.relative_to(probe.ROOT)).replace("\\", "/")
    if label.startswith(("A_counter", "B_side", "AB_widths")):
        parent_source, parent_label = donor_base, "S4_current_headers"
    else:
        parent_source, parent_label = base, "C_canonical_context"
    text = p.read_text(encoding="latin-1")
    if text != parent_source and label != "S4_current_headers":
        row["patch"] = patch(parent_source, text, label + ".patch", "a/" + parent_label + ".c", "b/" + label + ".c")
    row["source_sha256"] = hashlib.sha256(p.read_bytes()).hexdigest()

bridge = json.loads((probe.OUT / "web_frame_bridge/bridge.json").read_text())
allocation = json.loads((probe.OUT / "allocator_C_canonical_context/analysis.json").read_text())
counter = [m for m in bridge["matches"] if m["frame_offset"] == -20 and m["frame_type"] == "0x1004"]
for m in counter:
    p = allocation["pops"][m["pop"]]
    m["allocator_observation"] = {k: p[k] for k in ("id", "pri", "f40", "flags", "allowed", "pos", "round", "outcome")}

record = {
    "revision": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=probe.ROOT, text=True).strip(),
    "target_function": probe.NAME,
    "dependency_manifest": "dependencies/manifest.json",
    "target_size": probe.ti["size"],
    "baseline_patch": base_patch,
    "cells": results,
    "stock_body_equal_under_frame_bridge": bridge["stock_body_equal"],
    "counter_web_evidence": counter,
    "skip_route": json.loads((probe.OUT / "skip_route/route.json").read_text()),
    "spill_operand_rewrite": json.loads((probe.OUT / "spill_operand_probe/operands.json").read_text()),
    "instrumentation_checks": [json.loads((probe.OUT / label / "instrumentation_check.json").read_text()) for label in ("web_frame_bridge", "spill_operand_probe", "skip_route")],
    "best_research_source": "scratch/astra_clip_20260925/C_canonical_context.c",
    "landing_status": "HOLD: non-exact caller emits point_from_line3d; zero credit; source production unchanged",
}
(HERE / "measurements.json").write_text(json.dumps(record, indent=2) + "\n")
print("saved", len(results), "cells", len(counter), "counter observations")
print("relocations baseline equal", next(r["relocations_strict_equal"] for r in results if r["label"] == "C_canonical_context"))
print("bridge ordinary compiler body equal", bridge["stock_body_equal"])
print("counter", json.dumps(counter, indent=2))
