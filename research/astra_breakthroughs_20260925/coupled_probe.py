"""Run BASE/A/B/AB source experiments with one unchanged whole-TU gate.

This is a diagnostic, not an admission command. Requires two evidence-led
factors; never creates filler, relaxes the comparator, or edits production.
Candidate sources, object files, commands and gate logs remain in scratch.
"""
from __future__ import annotations

import argparse
from hashlib import sha256
import json
import os
from pathlib import Path
import subprocess
import sys
from datetime import datetime, timezone

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
from tools import coff_compare as cc


def digest(path):
    return sha256(path.read_bytes()).hexdigest()


def apply_factor(source, factor):
    if not factor.get("hypothesis") or not factor.get("prediction"):
        raise ValueError("Each factor requires a hypothesis and observable prediction")
    for edit in factor["edits"]:
        old, new = edit["old"], edit["new"]
        expected = edit.get("count", 1)
        if not old or expected < 1 or source.count(old) != expected:
            raise ValueError("Edit occurrence mismatch; no fuzzy/partial application allowed")
        source = source.replace(old, new)
    return source


def owner_names(obj):
    return sorted({s["name"] for s in obj["symbols"]
                   if s["type"] == 0x20 and s["section"] > 0 and s["value"] == 0
                   and s["storage"] in (2, 3)
                   and obj["sections"][s["section"] - 1]["name"] == ".text"})


def compare(target, ours):
    ours_names = set(owner_names(ours))
    rows = {}
    for name in owner_names(target):
        original = cc.section_info(target, name)
        compiled = cc.section_info(ours, name) if name in ours_names else None
        rows[name] = {
            "exact": bool(compiled and cc.section_infos_equal(original, compiled)),
            "target": original,
            "ours": compiled,
        }
    return rows


def run(args):
    manifest = json.loads(args.manifest.read_text())
    unit = manifest["unit"]
    # Restrict this runner to actual source TUs, not arbitrary process commands.
    source_path = (ROOT / (unit + ".c")).resolve()
    if not source_path.is_relative_to(ROOT / "source"):
        raise ValueError("Unit must resolve within source/")
    expected = manifest["source_sha256"]
    if digest(source_path) != expected:
        raise ValueError("Production source changed: refresh the experiment, do not force it")
    source = source_path.read_bytes().decode("latin-1").replace("\r\n", "\n")
    factors = manifest["factors"]
    if set(factors) != {"A", "B"}:
        raise ValueError("Exactly two named factors, A and B, are required")
    a = apply_factor(source, factors["A"])
    b = apply_factor(source, factors["B"])
    ab = apply_factor(a, factors["B"])
    # Independent transformations must commute; otherwise supply a new design.
    if ab != apply_factor(b, factors["A"]):
        raise ValueError("A/B overlap or order dependence: not an independent 2x2 experiment")
    target_path = ROOT / "build/split" / (unit + ".obj")
    base_path = ROOT / "build/base" / (unit + ".obj")
    target = cc.load(target_path.read_bytes())
    production = compare(target, cc.load(base_path.read_bytes()))
    for fn in manifest["functions"]:
        if fn not in production:
            raise ValueError(f"Not a target owner: {fn}")
    output = args.output.resolve()
    if not output.is_relative_to(ROOT / "scratch"):
        raise ValueError("Compiled evidence must stay under this checkout's scratch/")
    output.mkdir(parents=True, exist_ok=False)
    header_hash = sha256()
    for header in sorted((ROOT / "source").rglob("*.h")):
        header_hash.update(header.relative_to(ROOT).as_posix().encode())
        header_hash.update(bytes.fromhex(digest(header)))
    compiler = Path(os.environ.get("HALO_CL", str(ROOT / "xbox/bin/vc7/CL.Exe")))
    if not compiler.is_absolute():
        compiler = ROOT / compiler
    if not compiler.exists():
        compiler = Path("C:/halo-worktrees/claude-finish-hs-20260816/xbox/bin/vc7/CL.Exe")
    toolchain = {str(path): digest(path) for path in
                 (compiler, compiler.with_name("C1.Dll"), compiler.with_name("C2.Dll"))
                 if path.exists()}
    receipt = {
        "timestamp": datetime.now(timezone.utc).isoformat(),
        "head": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
        "manifest": manifest, "source_sha256": expected,
        "source_header_tree_sha256": header_hash.hexdigest(),
        "build_ninja_sha256": digest(ROOT / "build.ninja"),
        "gate_sha256": digest(ROOT / "tools/campaign/gate.py"),
        "comparator_sha256": digest(ROOT / "tools/coff_compare.py"),
        "compiler_sha256": toolchain,
        "target_sha256": digest(target_path), "baseline_object_sha256": digest(base_path),
        "credit_awarded": 0, "cells": {},
        "limits": ["Only source-TU factors; no header/config edits supported.",
                   "Full-TU exact controls are not a full-board/link/admission sweep.",
                   "Target equality does not authenticate source or override a hold.",
                   "External SDK headers are not included in source_header_tree_sha256."],
    }
    for label, body in (("BASE", source), ("A", a), ("B", b), ("AB", ab)):
        candidate = output / (label + ".c")
        objpath = output / (label + ".obj")
        candidate.write_bytes(body.replace("\n", "\r\n").encode("latin-1"))
        command = [sys.executable, str(ROOT / "tools/campaign/gate.py"), unit,
                   "--source", str(candidate), "--all", "--out", str(objpath)]
        result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        (output / (label + ".log")).write_text(result.stdout + result.stderr)
        cell = {"command": command, "exit_code": result.returncode,
                "source_sha256": digest(candidate)}
        receipt["cells"][label] = cell
        if result.returncode == 0 and objpath.exists():
            obj = cc.load(objpath.read_bytes())
            rows = compare(target, obj)
            cell.update({
                "object_sha256": digest(objpath),
                "rows": rows,
                "lost_inherited_exact": [n for n, r in rows.items()
                                         if production[n]["exact"] and not r["exact"]],
                "gained_exact": [n for n, r in rows.items()
                                 if not production[n]["exact"] and r["exact"]],
                "extra_function_owners": sorted(set(owner_names(obj)) - set(production)),
            })
            if label == "BASE":
                drift = [n for n, r in rows.items()
                         if bool(r["ours"]) != bool(production[n]["ours"]) or
                         (r["ours"] and not cc.section_infos_equal(r["ours"], production[n]["ours"]))]
                cell["baseline_drift"] = drift
                if drift:
                    (output / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
                    raise ValueError("Unmodified compile differs from build/base; stopping before A/B/AB")
        (output / "receipt.json").write_text(json.dumps(receipt, indent=2) + "\n")
        if result.returncode:
            raise RuntimeError(f"{label} compile failed; evidence retained")
        print(label, "targets", {n: cell["rows"][n]["exact"] for n in manifest["functions"]},
              "lost", cell["lost_inherited_exact"], "gained", cell["gained_exact"])
    return 0


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("manifest", type=Path)
    ap.add_argument("--output", required=True, type=Path)
    return run(ap.parse_args())


if __name__ == "__main__":
    raise SystemExit(main())
