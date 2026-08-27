"""Pair unwritten helper functions with the exact function that inlines them.

Many small target functions are `static` helpers whose call sites the compiler
inlined completely.  January's link kept the unreferenced COMDAT, so the helper
appears in the target object with **no relocation referencing it anywhere in the
tree**.  Such a helper cannot be reconstructed on its own:

* written as an unreferenced ``static`` it is eliminated by the compiler, and
* written as a global it gets a cdecl frame instead of the register convention
  VC7 uses for statics whose call sites it controls.

The body only materialises once a call site exists in the same translation unit.
The cheap case is a call site that is **already byte-exact**: factoring the
expression out of it emits the helper while the caller keeps inlining it, so one
edit is gated by two functions at once.

This tool finds those pairs.  For every unwritten, unreferenced, small target
function it looks for already-exact functions in the same object that could
contain the inlined body, scoring on two independent signals:

* **callee identity** - the inlining caller must reference everything the helper
  references, so the helper's callee set must be a subset of the candidate's;
* **immediates** - distinctive 4-byte constants in the helper body (float bit
  patterns, masks, magic numbers) survive inlining even when registers change.

Output is advisory: it narrows a 4000-function frontier to a short ranked list
of (helper, likely caller) pairs.  Confirm each by reading the caller's source
and gating the extraction.  Nothing here grants matching credit.

Usage::

    python -m tools.inlined_helper_candidates [--config objdiff.json]
                                              [--max-size 96] [--json out.json]
"""

import argparse
import json
import os
import sys

from .coff_compare import load, section_info, section_infos_equal, _section_bytes


def _text_functions(obj):
    out = {}
    for symbol in obj["symbols"]:
        if symbol["section"] <= 0:
            continue
        section = obj["sections"][symbol["section"] - 1]
        if section["name"] != ".text":
            continue
        name = symbol["name"]
        if not name.startswith("_") or name.startswith("??"):
            continue
        out[name] = bytes(_section_bytes(obj, section))[:section["size"]]
    return out


def _callees(obj, name):
    try:
        info = section_info(obj, name)
    except Exception:
        return None
    return {
        (reloc.get("symbolic_target") or reloc["target"])[1]
        for reloc in info["relocations"]
    }


def _immediates(body):
    """Distinctive little-endian dwords, skipping padding and tiny values."""
    found = set()
    for index in range(len(body) - 3):
        word = body[index:index + 4]
        value = int.from_bytes(word, "little")
        if 0x1000 < value < 0xFFFFF000 and word != b"\x90\x90\x90\x90":
            found.add(word)
    return found


def candidates_for_unit(target, base, max_size):
    functions = _text_functions(target)
    base_names = {s["name"] for s in base["symbols"] if s["section"] > 0}

    exact, unwritten = set(), set()
    for name in functions:
        if name not in base_names:
            unwritten.add(name)
            continue
        try:
            if section_infos_equal(section_info(target, name),
                                   section_info(base, name)):
                exact.add(name)
        except Exception:
            pass
    if not unwritten or not exact:
        return []

    referenced = set()
    for name in functions:
        names = _callees(target, name)
        if names:
            referenced |= names

    results = []
    for helper in sorted(unwritten):
        if helper in referenced:
            continue  # already has a call site: ordinary group work
        body = functions[helper]
        if not body or len(body) > max_size:
            continue
        helper_callees = _callees(target, helper) or set()
        helper_immediates = _immediates(body)

        scored = []
        for name in exact:
            callees = _callees(target, name)
            if callees is None:
                continue
            score = 0
            if helper_callees and helper_callees <= callees:
                score += 2
            if helper_immediates:
                hits = sum(1 for w in helper_immediates if w in functions[name])
                if hits == len(helper_immediates):
                    score += 2
                elif hits:
                    score += 1
            if score >= 2:
                scored.append((score, name))
        if scored:
            scored.sort(key=lambda item: (-item[0], item[1]))
            results.append({
                "helper": helper,
                "size": len(body),
                "callers": [name for _, name in scored[:3]],
                "score": scored[0][0],
            })
    return results


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--config", default="objdiff.json")
    parser.add_argument("--max-size", type=int, default=96,
                        help="only consider helpers up to this many bytes")
    parser.add_argument("--json", type=argparse.FileType("w"),
                        help="write the full result as JSON")
    args = parser.parse_args(argv)

    units = json.load(open(args.config, encoding="utf-8"))["units"]
    report = []
    for unit in units:
        if unit["name"].startswith("libs/"):
            continue
        target_path, base_path = unit.get("target_path"), unit.get("base_path")
        if not target_path or not base_path:
            continue
        if not (os.path.exists(target_path) and os.path.exists(base_path)):
            continue
        try:
            target, base = load(target_path), load(base_path)
        except Exception:
            continue
        for item in candidates_for_unit(target, base, args.max_size):
            item["unit"] = unit["name"]
            report.append(item)

    report.sort(key=lambda item: (item["unit"], item["size"]))
    print(f"candidate (unwritten helper -> exact inlining caller) pairs: "
          f"{len(report)}\n")
    for item in report:
        print(f"  {item['unit']:42} {item['helper']:30} {item['size']:4}B "
              f"<- {item['callers']}")
    if args.json:
        json.dump(report, args.json, indent=1)
        args.json.write("\n")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
