"""Fresh, read-only Halo residual/strict-owner census; output is research only.

Run `ninja` first, then `python research/astra_breakthroughs_20260925/census/census.py`.
Uses the production accepted ledger and unchanged strict comparator. It never
changes source, config, parks, credit, ownership or admission decisions.
"""
from __future__ import annotations

import collections
import copy
import csv
import hashlib
import itertools
import json
from pathlib import Path
import re
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[3]
OUT = Path(__file__).resolve().parent
sys.path.insert(0, str(ROOT))
import capstone
from tools import coff_compare as cc
from tools.campaign.stable_verdicts import build_snapshot
from tools.audit_object_admission import _normalize_measures, classify_units
from tools.semantic_progress import (
    apply_semantic_rejections, apply_semantic_matches,
    apply_semantic_accepted_ledger, apply_semantic_data_matches,
    revoke_incomplete_units,
)
from tools.codegen_blocker_classifier import classify, _capstone_instructions
from tools.parked_functions import validate_parked_functions


def read(path):
    return json.loads((ROOT / path).read_text(encoding="utf-8"))


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write(name, value):
    (OUT / name).write_text(json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8")


MD = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)


def allocation_from_body(body, relocation_rows):
    """Bounded syntactic frame locator, not a complete stack-depth analysis."""
    insns = list(itertools.islice(MD.disasm(body, 0), 14))
    frame = None
    allocation = None
    method = None
    relocations = {x["address"]: x for x in relocation_rows}
    for index, insn in enumerate(insns):
        if insn.mnemonic == "sub" and insn.op_str.startswith("esp,"):
            frame = int(insn.op_str.split(",")[1].strip(), 0)
            allocation, method = frame, "sub-esp"
            break
        if insn.mnemonic == "call" and index:
            relocation = relocations.get(insn.address + 1, {})
            previous = insns[index - 1]
            if (relocation.get("type") == cc.IMAGE_REL_I386_REL32
                    and relocation.get("target") == ["symbol", "__chkstk", 0]
                    and previous.mnemonic == "mov"
                    and previous.op_str.startswith("eax, 0x")):
                allocation = int(previous.op_str.split(",")[1].strip(), 0)
                method = "relocation-verified-chkstk-eax"
        if (insn.mnemonic in ("call", "int", "int3", "ud2", "hlt")
                or insn.mnemonic.startswith(("j", "ret", "iret", "loop"))):
            break
    return {"sub_esp": frame, "allocation_bytes": allocation, "allocation_method": method,
            "instructions": [f"{x.address:x}: {x.mnemonic} {x.op_str}" for x in insns]}


def prologue(obj, name):
    section = obj["sections"][cc.symbol(obj, name)["section"] - 1]
    return allocation_from_body(cc._section_bytes(obj, section),
                                cc.section_info(obj, name)["relocations"])


def main():
    raw = read("build/report.json")
    semantic = read("build/semantic_report.json")
    config = read("objdiff.json")
    accepted = {(x["unit"], x["function"]): x for x in semantic["accepted_ledger"]}
    assert len(accepted) == semantic["summary"]["accepted_exact"]
    halo_units = {u["name"]: u for u in raw["units"] if "halobetacache" in u.get("metadata", {}).get("progress_categories", [])}
    report_functions = {(u["name"], f["name"]): f for u in halo_units.values() for f in u.get("functions", [])}
    stable = build_snapshot(ROOT, ROOT / "objdiff.json")
    write("stable_current.json", stable)
    stable_names = {(k.split("::section:")[0], v["name"]): dict(v, stable_key=k) for k, v in stable.items()}
    parks = {(x["unit"], x["function"]): x for x in read("config/parked.json")["entries"]}
    rejects = collections.defaultdict(list)
    for x in read("config/object_admission_rejections.json")["entries"]:
        rejects[x["unit"]].append(x)

    corrected = copy.deepcopy(raw)
    _normalize_measures(corrected)
    apply_semantic_rejections(corrected, ROOT / "build/semantic_report.json")
    apply_semantic_matches(corrected, ROOT, ROOT / "config/semantic_matches.json", ROOT / "objdiff.json")
    apply_semantic_accepted_ledger(corrected, ROOT / "build/semantic_report.json")
    apply_semantic_data_matches(corrected, ROOT, ROOT / "config/semantic_data_matches.json", ROOT / "objdiff.json", ROOT / "config/symbols.json")
    revoked = revoke_incomplete_units(corrected)
    corrected_units = {x["name"]: x for x in corrected["units"]}
    admission = classify_units(raw, corrected, rejects)
    write("admission_current.json", dict(admission, revoked=revoked))
    write("park_validation.json", validate_parked_functions(ROOT, ROOT / "build/report.json", ROOT / "objdiff.json", ROOT / "config/parked.json"))

    evidence_files = list((ROOT / "docs/object_matching_logs").glob("*.md"))
    for pattern in ("fifty_objects*/results/**/*.md", "fifty_objects*/w/**/REVIEW.md"):
        evidence_files.extend((ROOT / "research").glob(pattern))
    evidence = [(p.relative_to(ROOT).as_posix(), p.read_text(encoding="utf-8", errors="replace")) for p in set(evidence_files)]
    units = {x["name"]: x for x in config["units"]}
    objects = {}
    input_hashes = {}

    def object_pair(unit):
        if unit not in objects:
            paths = [ROOT / units[unit][kind] for kind in ("target_path", "base_path")]
            objects[unit] = [cc.load(p) if p.is_file() else None for p in paths]
            for p in paths:
                input_hashes[p.relative_to(ROOT).as_posix()] = digest(p) if p.is_file() else None
        return objects[unit]

    rows = []
    for (unit, name), function in report_functions.items():
        if (unit, name) in accepted:
            continue
        row = {
            "unit": unit, "function": name, "meaningful_bytes": int(function["size"]),
            "objdiff_percent": function.get("fuzzy_match_percent", 0),
            "stable": stable_names.get((unit, name)),
            "park": parks.get((unit, name)),
            "object_rejections": rejects.get(unit, []),
            "source": halo_units[unit].get("metadata", {}).get("source_path"),
        }
        if row["source"] and (ROOT / row["source"]).is_file():
            input_hashes[row["source"]] = digest(ROOT / row["source"])
        # Index mentions, not proof endorsements. Complete park prose is separate.
        tokens = [name, name.lstrip("_")]
        hits = [p for p, text in evidence if any(token in text for token in tokens)]
        def evidence_key(path):
            dates = re.findall(r"202609\d\d", path)
            return (max(dates, default=""), "REVIEW" in path, path)
        row["evidence_paths"] = sorted(hits, key=evidence_key, reverse=True)
        target, base = object_pair(unit)
        try:
            ti = cc.section_info(target, name)
            row["target"] = {k: ti[k] for k in ("size", "relocation_count", "normalized_sha256")}
            row["target_prologue"] = prologue(target, name)
            bi = cc.section_info(base, name) if base else None
            if bi:
                row["base"] = {k: bi[k] for k in ("size", "relocation_count", "normalized_sha256")}
                row["base_prologue"] = prologue(base, name)
                row["strict_equal"] = cc.section_infos_equal(ti, bi)
                a, b = row["target_prologue"]["allocation_bytes"], row["base_prologue"]["allocation_bytes"]
                row["frame_delta"] = b-a if a is not None and b is not None else None
                row["padded_size_delta"] = bi["size"] - ti["size"]
                row["relocation_count_delta"] = bi["relocation_count"] - ti["relocation_count"]
                ti["instructions"] = _capstone_instructions(target, name)
                bi["instructions"] = _capstone_instructions(base, name)
                row["classifier"] = classify(ti, bi)
        except (cc.CoffError, KeyError, ValueError) as exc:
            row["inspection_error"] = str(exc)
        rows.append(row)
    rows.sort(key=lambda r: (-r["meaningful_bytes"], r["unit"], r["function"]))
    write("remaining_functions.json", rows)

    units_out = []
    for unit, report_unit in halo_units.items():
        m = corrected_units[unit]["measures"]
        members = [r for r in rows if r["unit"] == unit]
        if not report_unit.get("metadata", {}).get("complete"):
            units_out.append({
                "unit": unit, "remaining_functions": len(members),
                "remaining_meaningful_bytes": sum(r["meaningful_bytes"] for r in members),
                "data_gap": int(m.get("total_data", 0))-int(m.get("matched_data", 0)),
                "total_functions": int(m.get("total_functions", 0)),
                "functions": [r["function"] for r in members],
                "rejections": rejects.get(unit, []),
            })
    units_out.sort(key=lambda x: (x["remaining_functions"], x["data_gap"] != 0, -x["remaining_meaningful_bytes"], x["unit"]))
    write("object_closures.json", units_out)

    source_stable = {k: v for k, v in stable.items() if k.split("::section:")[0] in halo_units}
    bridge = {
        "accepted_halo_absent_from_stable": [dict(unit=u, function=n, ledger=accepted[u,n]) for (u,n) in report_functions if (u,n) in accepted and (u,n) not in stable_names],
        "residual_halo_absent_from_stable": [dict(unit=u, function=n, report=report_functions[u,n]) for (u,n) in report_functions if (u,n) not in accepted and (u,n) not in stable_names],
        "accepted_halo_nonexact_in_stable": [dict(unit=u, function=n, stable=stable_names[u,n], ledger=accepted[u,n]) for (u,n) in report_functions if (u,n) in accepted and (u,n) in stable_names and stable_names[u,n]["status"] != "E"],
        "strict_exact_halo_not_accepted": [dict(unit=u, function=n, stable=stable_names[u,n]) for (u,n) in report_functions if (u,n) not in accepted and (u,n) in stable_names and stable_names[u,n]["status"] == "E"],
    }
    write("population_bridge.json", bridge)
    halo_measures = next(x["measures"] for x in corrected["categories"] if x["id"] == "halobetacache")
    summary = {
        "head": subprocess.check_output(["git", "rev-parse", "HEAD"], cwd=ROOT, text=True).strip(),
        "halo_accepted_measures": halo_measures,
        "halo_report_functions": len(report_functions),
        "halo_remaining_functions": len(rows),
        "halo_remaining_meaningful_bytes": sum(x["meaningful_bytes"] for x in rows),
        "strict_all": dict(collections.Counter(x["status"] for x in stable.values())),
        "strict_all_functions": len(stable),
        "strict_halo": dict(collections.Counter(x["status"] for x in source_stable.values())),
        "strict_halo_functions": len(source_stable),
        "bridge_counts": {k: len(v) for k,v in bridge.items()},
        "halo_open_objects": len(units_out),
        "halo_open_objects_by_function_gap": dict(collections.Counter(x["remaining_functions"] for x in units_out)),
        "halo_zero_function_gap_objects": [x["unit"] for x in units_out if x["remaining_functions"] == 0],
        "residuals_with_active_park": sum(x["park"] is not None for x in rows),
        "park_classes": dict(collections.Counter(x["park"]["class"] if x["park"] else "not-parked" for x in rows)),
        "classifier_classes": dict(collections.Counter(x.get("classifier", {}).get("class", "no-comparison") for x in rows)),
        "nonzero_frame_delta_functions": sum(x.get("frame_delta") not in (None, 0) for x in rows),
        "nonzero_frame_delta_meaningful_bytes": sum(x["meaningful_bytes"] for x in rows if x.get("frame_delta") not in (None, 0)),
        "top10_bytes": sum(x["meaningful_bytes"] for x in rows[:10]),
        "top20_bytes": sum(x["meaningful_bytes"] for x in rows[:20]),
        "errors": [{"unit":x["unit"],"function":x["function"],"error":x["inspection_error"]} for x in rows if "inspection_error" in x],
    }
    write("summary.json", summary)
    for p in ["build/report.json", "build/semantic_report.json", "objdiff.json", "config/parked.json", "config/object_admission_rejections.json", "config/semantic_matches.json", "config/semantic_data_matches.json"]:
        input_hashes[p] = digest(ROOT / p)
    write("input_hashes.json", input_hashes)
    with (OUT / "remaining_functions.csv").open("w", newline="", encoding="utf-8") as handle:
        fields = ["rank", "unit", "function", "meaningful_bytes", "padded_target", "padded_base", "relocation_count_delta", "frame_delta", "park_class", "strict_status", "evidence"]
        writer = csv.DictWriter(handle, fieldnames=fields)
        writer.writeheader()
        for rank, row in enumerate(rows, 1):
            writer.writerow({
                "rank":rank, "unit":row["unit"], "function":row["function"],
                "meaningful_bytes":row["meaningful_bytes"],
                "padded_target":row.get("target", {}).get("size"),
                "padded_base":row.get("base", {}).get("size"),
                "relocation_count_delta":row.get("relocation_count_delta"),
                "frame_delta":row.get("frame_delta"),
                "park_class":(row["park"] or {}).get("class", "not-parked"),
                "strict_status":(row["stable"] or {}).get("status", "not-in-owner-population"),
                "evidence":"; ".join(row["evidence_paths"][:8]),
            })
    print(json.dumps(summary, indent=2))


if __name__ == "__main__":
    main()
