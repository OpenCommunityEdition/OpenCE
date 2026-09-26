"""Compare observable return-value contracts at matched direct call sites.

Research only: differences are hypotheses, never automatic type edits or credit.
Pairs repeated callees only when the entire direct-call sequence agrees; unique
callees can be paired independently. Stops at control-flow boundaries and skips
incomplete linear decoding. --override UNIT=OBJ supports scratch positive controls.
"""
from __future__ import annotations

import argparse
from collections import Counter
import json
from pathlib import Path
import sys

import capstone as cs
from capstone.x86 import X86_OP_REG, X86_OP_IMM

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT))
from tools import coff_compare as cc

MD = cs.Cs(cs.CS_ARCH_X86, cs.CS_MODE_32)
MD.detail = True
ALIASES = {name: (group[0], width) for group in (
    ("eax", "ax", "al", "ah"), ("ebx", "bx", "bl", "bh"),
    ("ecx", "cx", "cl", "ch"), ("edx", "dx", "dl", "dh"),
    ("esi", "si"), ("edi", "di"), ("ebp", "bp"), ("esp", "sp"),
) for name, width in zip(group, (32, 16, 8, 8))}


def functions(obj):
    return {s["name"]: s for s in obj["symbols"]
            if s["type"] == 0x20 and s["section"] > 0 and s["value"] == 0
            and s["storage"] in (2, 3)
            and obj["sections"][s["section"] - 1]["name"] == ".text"}


def register_family(ins, register):
    name = ins.reg_name(register)
    return ALIASES.get(name, (name, 0))[0]


def observation(insns, start, block_entries=()):
    """Recognise bounded local patterns, not a complete ABI/type inference."""
    following = []
    cleanup = copy = None
    copied_to = None
    extension = None
    narrow = None
    eax_live = True
    for position, ins in enumerate(insns[start + 1:start + 13]):
        if (ins.address in block_entries or ins.mnemonic.startswith("loop") or
                ins.mnemonic in ("ud2", "hlt") or
                any(ins.group(group) for group in (cs.CS_GRP_CALL, cs.CS_GRP_JUMP,
                                                   cs.CS_GRP_RET, cs.CS_GRP_IRET,
                                                   cs.CS_GRP_INT))):
            break
        following.append(f"{ins.address:x}: {ins.mnemonic} {ins.op_str}".rstrip())
        ops = ins.operands
        if ins.mnemonic == "add" and len(ops) == 2 and ops[0].type == X86_OP_REG:
            if ins.reg_name(ops[0].reg) == "esp" and ops[1].type == X86_OP_IMM:
                cleanup = position if cleanup is None else cleanup
        if ins.mnemonic in ("movsx", "movzx") and len(ops) == 2 and ops[1].type == X86_OP_REG:
            src = ins.reg_name(ops[1].reg)
            if src in ("al", "ax") and eax_live and extension is None:
                extension = [ins.mnemonic, ALIASES[src][1]]
        new_copy = None
        if ins.mnemonic == "mov" and len(ops) == 2 and all(op.type == X86_OP_REG for op in ops):
            dst, src = (ins.reg_name(op.reg) for op in ops)
            if src == "eax" and dst != "eax" and eax_live and copy is None:
                new_copy = ALIASES.get(dst, (dst, 32))[0]
        if ins.mnemonic in ("cmp", "test") and copied_to:
            for op in ops:
                if op.type == X86_OP_REG:
                    reg = ins.reg_name(op.reg)
                    base, width = ALIASES.get(reg, (reg, 32))
                    if base == copied_to and width < 32 and reg not in ("ah", "bh", "ch", "dh"):
                        narrow = width
        if narrow is not None:
            break
        # A partial write also invalidates provenance of the full return value.
        written = {register_family(ins, reg) for reg in ins.regs_access()[1]}
        if "eax" in written:
            eax_live = False
        if copied_to in written:
            copy, copied_to = None, None
        if new_copy is not None:
            copy, copied_to = position, new_copy
    return {"extension": extension,
            "narrow_compare_width": narrow,
            "cleanup_before_copy": (cleanup < copy if cleanup is not None and copy is not None and narrow else None),
            "instructions": following}


def block_entry_offsets(insns):
    return {ins.operands[0].imm for ins in insns
            if (ins.group(cs.CS_GRP_JUMP) or ins.mnemonic.startswith("loop"))
            and ins.operands and ins.operands[0].type == X86_OP_IMM}


def calls(obj, name, symbol):
    sec = obj["sections"][symbol["section"] - 1]
    raw = bytes(cc._section_bytes(obj, sec))
    info = cc.section_info(obj, name)
    relocs = {r["address"]: r for r in info["relocations"] if r["type"] == 20}
    insns = list(MD.disasm(raw, 0))
    decoded_bytes = insns[-1].address + insns[-1].size if insns else 0
    if decoded_bytes != len(raw):
        return [], {"decoded_bytes": decoded_bytes, "section_bytes": len(raw)}
    block_entries = block_entry_offsets(insns)
    result = []
    for index, ins in enumerate(insns):
        if ins.mnemonic != "call":
            continue
        if not ins.operands or ins.operands[0].type != X86_OP_IMM:
            result.append({"callee": None, "offset": ins.address})
            continue
        reloc = relocs.get(ins.address + ins.size - 4)
        if not reloc:
            result.append({"callee": None, "offset": ins.address})
            continue
        target = reloc.get("symbolic_target") or reloc["target"]
        if len(target) != 3 or target[0] != "symbol" or target[2] != 0:
            result.append({"callee": None, "offset": ins.address})
            continue
        result.append({"callee": target[1], "offset": ins.address,
                       **observation(insns, index, block_entries)})
    return result, None


def paired(left, right):
    ln, rn = [x["callee"] for x in left], [x["callee"] for x in right]
    if ln == rn and None not in ln:
        yield from zip(left, right)
        return
    lc, rc = Counter(ln), Counter(rn)
    byname = {x["callee"]: x for x in right}
    for item in left:
        if item["callee"] is not None and lc[item["callee"]] == rc[item["callee"]] == 1:
            yield item, byname[item["callee"]]


def main():
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--project", type=Path, default=ROOT)
    ap.add_argument("--unit", action="append")
    ap.add_argument("--override", action="append", default=[])
    ap.add_argument("--output", type=Path)
    args = ap.parse_args()
    overrides = dict(x.split("=", 1) for x in args.override)
    config = json.loads((args.project / "objdiff.json").read_text())
    counts, findings, errors = Counter(), [], []
    for unit in config["units"]:
        name = unit["name"]
        if not name.startswith("source/") or (args.unit and name not in args.unit):
            continue
        if not unit.get("base_path") or not unit.get("target_path"):
            counts["units_without_comparison_pair"] += 1
            continue
        try:
            target_path = args.project / unit["target_path"]
            ours_path = Path(overrides[name]) if name in overrides else args.project / unit["base_path"]
            target, ours = cc.load(target_path.read_bytes()), cc.load(ours_path.read_bytes())
        except (KeyError, OSError, ValueError) as exc:
            errors.append({"unit": name, "error": str(exc)})
            continue
        tf, of = functions(target), functions(ours)
        counts["units"] += 1
        for fn in sorted(tf.keys() & of.keys()):
            exact = cc.section_infos_equal(cc.section_info(target, fn), cc.section_info(ours, fn))
            counts["exact_functions" if exact else "residual_functions"] += 1
            (left, ld), (right, rd) = calls(target, fn, tf[fn]), calls(ours, fn, of[fn])
            if ld or rd:
                counts["functions_skipped_incomplete_linear_decode"] += 1
                continue
            for a, b in paired(left, right):
                counts["paired_calls"] += 1
                reasons = []
                if a["extension"] is not None and b["extension"] is not None and a["extension"] != b["extension"]:
                    reasons.append("different-explicit-return-extension")
                if (a["narrow_compare_width"] == b["narrow_compare_width"]
                        and a["narrow_compare_width"] is not None
                        and a["cleanup_before_copy"] is not None and b["cleanup_before_copy"] is not None
                        and a["cleanup_before_copy"] != b["cleanup_before_copy"]):
                    reasons.append("narrow-compare-cleanup-order")
                if reasons:
                    counts["exact_control_findings" if exact else "residual_findings"] += 1
                    findings.append({"unit": name, "function": fn, "callee": a["callee"],
                                     "strict_exact": exact, "reasons": reasons, "target": a, "ours": b})
    result = {"summary": dict(counts), "findings": findings, "errors": errors,
              "limits": ["Pattern evidence is not a declaration or proof of original source.",
                         "Repeated calls are paired only with identical complete direct-call sequences.",
                         "Only direct symbolic REL32 calls and bounded straight-line continuations are observed.",
                         "This is linear decoding, not complete CFG recovery; indirect-switch reachability is not inferred.",
                         "Register redefinitions and branch entries terminate return-value provenance.",
                         "Stack adjustment order is a scheduling cue, not proof of the callee's type or cleanup ownership.",
                         "Unobserved indirect/addend calls disable repeated-name sequence pairing; unique names remain heuristic correspondences.",
                         "No production source, declarations, or matching credit is modified."]}
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result, indent=2))
    return 1 if counts["exact_control_findings"] else 0


if __name__ == "__main__":
    raise SystemExit(main())
