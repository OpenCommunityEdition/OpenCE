"""Mutation harness for the Q10 COMMON-pool verifier draft (research only; Q10 held; zero credit).

Every mutation in mutations.json is applied, alone, to a copy of tools/common_pool_verify.py inside an
isolated work tree (tools/ + this q10 directory); its named killer tests are then run.  A mutation is
KILLED only if pytest exits 1, reports no collection/setup error, and EVERY named killer test fails.

usage:
  python q10_mutate.py --repo <tree> --work <scratch dir> [--out kills.txt]           # check the catalogue
  python q10_mutate.py --repo <tree> --work <scratch dir> --discover [--write]        # find failing tests
      (--discover runs the whole test file per mutant and lists every failing test; --write stores them
       as the killers of each mutation)
Exit status 0 only if every mutation is KILLED (check mode) or killed by at least one test (discover).
"""
import argparse
import json
import os
import re
import shutil
import subprocess
import sys
from pathlib import Path

Q10 = Path("research") / "remaining_frontier_20260926" / "q10"
TARGET = Path("tools") / "common_pool_verify.py"
TESTS = "tools/test_common_pool_verify.py"
FAILED = re.compile(r"^(FAILED|ERROR) tools/test_common_pool_verify\.py::([A-Za-z0-9_]+)")


def work_tree(repo, work):
    if work.exists():
        shutil.rmtree(work)
    shutil.copytree(repo / "tools", work / "tools", ignore=shutil.ignore_patterns("__pycache__"))
    shutil.copytree(repo / Q10, work / Q10, ignore=shutil.ignore_patterns("__pycache__"))
    return work


def run_tests(tree, selectors, basetemp):
    env = dict(os.environ, PYTHONDONTWRITEBYTECODE="1")
    result = subprocess.run([sys.executable, "-m", "pytest", *selectors, "-q", "-rfE", "-p", "no:cacheprovider",
                             "--basetemp", str(basetemp)], cwd=tree, capture_output=True, text=True, env=env)
    failed, errors = set(), set()
    for line in result.stdout.splitlines():
        match = FAILED.match(line)
        if match:
            (failed if match.group(1) == "FAILED" else errors).add(match.group(2))
    tail = (result.stdout.strip().splitlines() or [result.stderr[-200:]])[-1]
    return result.returncode, failed, errors, tail


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo", required=True)
    parser.add_argument("--work", required=True)
    parser.add_argument("--catalogue", default=None)
    parser.add_argument("--discover", action="store_true")
    parser.add_argument("--write", action="store_true")
    parser.add_argument("--only", nargs="*")
    parser.add_argument("--out")
    args = parser.parse_args()
    repo, work = Path(args.repo).resolve(), Path(args.work).resolve()
    catalogue_path = Path(args.catalogue) if args.catalogue else repo / Q10 / "mutation" / "mutations.json"
    catalogue = json.loads(catalogue_path.read_text(encoding="utf-8"))
    source = (repo / TARGET).read_bytes().decode("utf-8").replace("\r\n", "\n")
    lines, bad = [], 0
    baseline = work_tree(repo, work / "baseline")
    code, failed, errors, tail = run_tests(baseline, [TESTS], work / "tmp_baseline")
    lines.append(f"BASELINE (unmutated) exit {code}: {tail}")
    if code != 0:
        print("\n".join(lines))
        return 2
    for mutation in catalogue["mutations"]:
        if args.only and mutation["id"] not in args.only:
            continue
        count = source.count(mutation["old"])
        label = f"{mutation['id']} {mutation['origin']:<5} {mutation['label']}"
        if count != 1:
            lines.append(f"BAD-MUTATION ({count} matches) {label}")
            bad += 1
            continue
        tree = work_tree(repo, work / "mut")
        (tree / TARGET).write_bytes(source.replace(mutation["old"], mutation["new"]).encode("utf-8"))
        if args.discover:
            code, failed, errors, tail = run_tests(tree, [TESTS], work / "tmp_mut")
            killed = code == 1 and failed and not errors
            if args.write and killed:
                mutation["killers"] = sorted(failed)
            lines.append(f"{'KILLED' if killed else 'SURVIVED':<8} {label}: {sorted(failed)} {tail}")
        else:
            killers = mutation["killers"]
            if not killers:
                lines.append(f"NO-KILLERS {label}")
                bad += 1
                continue
            code, failed, errors, tail = run_tests(tree, [f"{TESTS}::{k}" for k in killers], work / "tmp_mut")
            missing = sorted(set(killers) - failed)
            killed = code == 1 and not errors and not missing
            lines.append(f"{'KILLED' if killed else 'SURVIVED':<8} {label}: killers {len(killers)} failed "
                         f"{len(set(killers) & failed)}{' missing ' + str(missing) if missing else ''} | {tail}")
        if not killed:
            bad += 1
    total = len([m for m in catalogue["mutations"] if not args.only or m["id"] in args.only])
    lines.append(f"mutations {total}, not killed {bad}")
    text = "\n".join(lines) + "\n"
    sys.stdout.write(text)
    if args.out:
        Path(args.out).write_text(text, encoding="utf-8", newline="\n")   # LF on every platform (RF-DP D1)
    if args.discover and args.write:
        catalogue_path.write_text(json.dumps(catalogue, indent=1) + "\n", encoding="utf-8", newline="\n")
    return 0 if bad == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
