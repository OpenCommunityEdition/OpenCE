"""Re-measure genuine-owner inline recovery in scratch, not production.

No declaration-count compensation, header-position sweep or policy exception.
Control and candidate compile with the current TU flags. A missing/new owner is
reported, not automatically credited. All generated evidence stays in scratch.
"""
from pathlib import Path
import argparse
import hashlib
import json
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / "scratch/astra_header_r2_20260925"
sys.path.insert(0, str(ROOT))
from tools import coff_compare as cc

UNITS = ["source/physics/collisions", "source/math/matrix_math",
         "source/bitmaps/bitmap_drawing", "source/rasterizer/rasterizer_frame_statistics",
         "source/render/render_cameras", "source/units/units"]
HEADER = "source/math/real_math.h"
PROTOTYPE = "boolean valid_real_plane3d(real_plane3d const *plane);"
ANCHOR = "__inline boolean valid_real_vector3d_axes2("
BODY = """boolean valid_real_plane3d(
\treal_plane3d const *plane)
{
\treturn
\t\tvalid_real_normal3d(&plane->n) &&
\t\tvalid_real(plane->d);
}"""


def read(relative):
    return (ROOT / relative).read_bytes().decode("latin-1").replace("\r\n", "\n")


def exactly(text, before, after):
    if text.count(before) != 1:
        raise ValueError(f"Expected one occurrence: {before[:80]}")
    return text.replace(before, after)


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def flags(unit):
    ninja = (ROOT / "build.ninja").read_text().replace("$\n", " ")
    key = "build\\base\\" + unit.replace("/", "\\").replace(" ", "$ ") + ".obj:"
    start = ninja.index("cflags = ", ninja.index(key)) + len("cflags = ")
    text = re.sub(r"\s+", " ", ninja[start:ninja.index("\nbuild ", start)]).strip()
    return [("/I" + t[3:].rstrip('"')) if t.startswith('/I"') else t
            for t in re.findall(r'/I"[^"]+"|\S+', text)]


def owners(obj):
    return {s["name"] for s in obj["symbols"] if s["type"] == 0x20
            and s["section"] > 0 and s["value"] == 0 and s["storage"] in (2, 3)
            and obj["sections"][s["section"] - 1]["name"] == ".text"}


def delta(unit, path):
    target = cc.load((ROOT / ("build/split/" + unit + ".obj")).read_bytes())
    base = cc.load((ROOT / ("build/base/" + unit + ".obj")).read_bytes())
    obj = cc.load(path.read_bytes())
    tn, bn, on = owners(target), owners(base), owners(obj)
    exact = lambda o, n: n in owners(o) and cc.section_infos_equal(cc.section_info(target, n), cc.section_info(o, n))
    changed = [n for n in sorted(bn & on)
               if not cc.section_infos_equal(cc.section_info(base, n), cc.section_info(obj, n))]
    return {"changed": changed, "added": sorted(on - bn), "removed": sorted(bn - on),
            "gained": [n for n in sorted(tn) if exact(obj, n) and not exact(base, n)],
            "lost": [n for n in sorted(tn) if exact(base, n) and not exact(obj, n)],
            "rows": {n: {"target": cc.section_info(target, n),
                         "base": cc.section_info(base, n) if n in bn else None,
                         "candidate": cc.section_info(obj, n) if n in on else None}
                     for n in changed if n in tn}}


def run(label, unit, header, body):
    root = OUT / label
    hpath, cpath = root / HEADER, root / (unit + ".c")
    hpath.parent.mkdir(parents=True, exist_ok=True)
    cpath.parent.mkdir(parents=True, exist_ok=True)
    hpath.write_bytes(header.replace("\n", "\r\n").encode("latin-1"))
    cpath.write_bytes(body.replace("\n", "\r\n").encode("latin-1"))
    objpath = cpath.with_suffix(".obj")
    cmd = [str(ROOT / "xbox/bin/vc7/CL.Exe"), "/nologo", "/c", "/showIncludes",
           "/I" + str(root / "source"), "/I" + str(root / "source/math")]
    cmd += flags(unit) + ["/I" + str((ROOT / unit).parent), "/Fo" + str(objpath), str(cpath)]
    result = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True)
    cpath.with_suffix(".log").write_text(result.stdout + result.stderr)
    row = {"unit": unit, "command": cmd, "exit_code": result.returncode,
           "header_sha256": digest(hpath), "source_sha256": digest(cpath)}
    includes = [line.split("including file:", 1)[1].strip()
                for line in (result.stdout + result.stderr).splitlines()
                if "including file:" in line]
    selected = [str(Path(p).resolve()) for p in includes
                if Path(p).name.lower() == "real_math.h"]
    row["real_math_includes"] = selected
    row["shadow_header_confirmed"] = bool(selected) and all(
        Path(p).resolve() == hpath.resolve() for p in selected)
    if not row["shadow_header_confirmed"]:
        raise RuntimeError(f"Wrong or missing real_math.h include: {selected}")
    if result.returncode == 0:
        row.update(delta(unit, objpath))
        row["object_sha256"] = digest(objpath)
    else:
        row["error"] = (result.stdout + result.stderr)[-3000:]
    print(label, unit, "gained", row.get("gained"), "lost", row.get("lost"),
          "changed", row.get("changed"), "exit", result.returncode)
    return row


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--unit", action="append")
    args = ap.parse_args()
    header = read(HEADER)
    owner = exactly(header, PROTOTYPE, "")
    owner = exactly(owner, ANCHOR, "__inline " + BODY + "\n\n" + ANCHOR)
    report = {"baseline": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
              "credit": 0, "cells": {}, "production_inputs": {},
              "header_sha256": digest(ROOT / HEADER),
              "build_ninja_sha256": digest(ROOT / "build.ninja"),
              "compiler_sha256": digest(ROOT / "xbox/bin/vc7/CL.Exe")}
    for unit in args.unit or UNITS:
        text = read(unit + ".c")
        report["production_inputs"][unit] = digest(ROOT / (unit + ".c"))
        control = run("baseline", unit, header, text)
        report["cells"][unit] = {"baseline": control}
        if control["exit_code"] or any(control[k] for k in ("changed", "added", "removed")):
            raise RuntimeError("Baseline does not reproduce production code owners")
        if unit == "source/math/matrix_math":
            text = exactly(text, BODY, "")
        elif unit == "source/physics/collisions":
            text = exactly(text, "valid_real_normal3d(&collision->plane.n) && valid_real(collision->plane.d)",
                           "valid_real_plane3d(&collision->plane)")
        report["cells"][unit]["owner"] = run("owner", unit, owner, text)
    report["production_unchanged"] = (
        digest(ROOT / HEADER) == report["header_sha256"] and
        all(digest(ROOT / (unit + ".c")) == h
            for unit, h in report["production_inputs"].items()))
    OUT.mkdir(exist_ok=True)
    (OUT / "results.json").write_text(json.dumps(report, indent=2) + "\n")
    print("Production unchanged", report["production_unchanged"])


if __name__ == "__main__":
    main()
