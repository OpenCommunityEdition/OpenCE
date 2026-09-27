"""Recompile affected consumers at /W3 against both source/header trees.

The old tree is extracted from the frozen Git commit into a private temporary
directory. Diagnostic objects stay outside production build paths.
"""
from collections import Counter
from concurrent.futures import ThreadPoolExecutor
import io
import json
from pathlib import Path
import re
import subprocess
import tempfile
import zipfile

ROOT = Path.cwd()
OUT = ROOT / "scratch/reconcile_compiler_application_20260926/warnings"


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    # Git's default archive format is tar; request ZIP explicitly for extraction.
    archive = subprocess.check_output(["git", "archive", "--format=zip", "d890c2db", "source"], cwd=ROOT)
    oldroot = Path(tempfile.mkdtemp(prefix="halo_warning_baseline_"))
    with zipfile.ZipFile(io.BytesIO(archive)) as source:
        for info in source.infolist():
            if not (oldroot / info.filename).resolve().is_relative_to(oldroot.resolve()):
                raise RuntimeError("Archive path escapes baseline directory")
        source.extractall(oldroot)
    ninja = (ROOT / "build.ninja").read_text()
    log = (ROOT / "scratch/reconcile_compiler_application_20260926/integrated_build.log").read_text()
    paths = sorted(set(re.findall(r"^\[\d+/\d+\] CL (.+)$", log, re.M)))
    units = [p.strip('"').replace("\\", "/").removeprefix("build/base/").removesuffix(".obj")
             for p in paths]

    def compile_one(unit):
        key = "build\\base\\" + unit.replace("/", "\\").replace(" ", "$ ") + ".obj:"
        start = ninja.index("cflags = ", ninja.index(key)) + len("cflags = ")
        end = ninja.index("\nbuild ", start)
        flags = re.sub(r"\s+", " ", ninja[start:end].replace("$\n", " ")).strip()
        tokens = re.findall(r'/I"[^"]+"|\S+', flags)
        tokens = [("/I" + t[3:].rstrip('"')) if t.startswith('/I"') else t for t in tokens]
        counters = []
        for label, tree in (("before", oldroot), ("after", ROOT)):
            args = []
            for token in tokens:
                if token.startswith("/Isource"):
                    token = "/I" + str(tree / token[2:])
                args.append(token)
            if label == "before" and unit == "source/physics/breakable_surfaces":
                args += ["/Ow", "/QIfist"]
            stem = unit.replace("/", "_")
            cmd = [str(ROOT / "xbox/bin/vc7/CL.Exe"), "/nologo", "/c"] + args
            cmd += ["/W3", "/Fo" + str(OUT / (label + "_" + stem + ".obj")), str(tree / (unit + ".c"))]
            r = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True)
            text = r.stdout + r.stderr
            (OUT / (label + "_" + stem + ".txt")).write_text(text)
            if r.returncode:
                raise RuntimeError(f"{label} compilation failed: {unit}; {text[-1200:]}")
            warnings = []
            for line in text.splitlines():
                if not re.search(r"warning C\d{4}", line):
                    continue
                line = line.replace("\\", "/")
                for prefix in (oldroot, ROOT):
                    line = re.sub(re.escape(prefix.as_posix() + "/"), "", line, flags=re.I)
                line = re.sub(r"\(\d+\)", "(N)", line)
                warnings.append(line.strip())
            counters.append(Counter(warnings))
        before, after = counters
        return {"unit": unit, "before": sum(before.values()), "after": sum(after.values()),
                "new": sorted((after - before).elements()), "removed": sorted((before - after).elements())}

    with ThreadPoolExecutor(max_workers=4) as pool:
        results = list(pool.map(compile_one, units))
    summary = {"baseline": "d890c2db", "baseline_directory": str(oldroot), "units": results,
               "new_warning_count": sum(len(r["new"]) for r in results)}
    (OUT / "summary.json").write_text(json.dumps(summary, indent=2) + "\n")
    print(json.dumps({"consumers": len(results), "new_warnings": summary["new_warning_count"],
                      "new": [r for r in results if r["new"]]}, indent=2))
    return 1 if summary["new_warning_count"] else 0


if __name__ == "__main__":
    raise SystemExit(main())
