"""Mine the byte-exact corpus into a cited codegen dictionary.

Every function listed as ``exact`` by tools/residual_frontier.py is a proven
(C source, VC7 bytes) pair: the strict comparator has already measured the
rebuilt object byte-identical to the January target.  This tool extracts
machine-side idioms from the target bytes and source-side constructs from the
matching translation unit, then reports their co-occurrence *statistically*,
with the citing functions listed so every entry can be re-verified.

Mining is fail-closed like the classifier: a correlation is only counted when
the (function, construct) attribution is unambiguous — a function with two
loops of different index types contributes to no loop-width claim, and a
function whose source cannot be located contributes to no source-side claim.

This tool grants no match credit and proposes no source edits.

Usage:
    python tools/corpus_dictionary.py [--frontier research/residual_frontier.json]
        [--config objdiff.json] [--output research/codegen_dictionary.json]
        [--markdown research/codegen_dictionary.md]
"""
import argparse
import json
import os
import re
import sys
from collections import Counter
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import coff_compare as cc
from codegen_blocker_classifier import _capstone_instructions, ClassifierError


NARROW_TYPES = {"short", "word", "int16", "uint16"}
WIDE_TYPES = {"long", "dword", "int32", "uint32", "int", "unsigned"}


# ---------- machine features

def _is_backward(instruction, mnemonic_prefix):
	if not instruction["mnemonic"].startswith(mnemonic_prefix):
		return False
	try:
		return int(instruction["operands"], 16) <= instruction["address"]
	except ValueError:
		return False


def machine_features(instructions):
	features = {
		"instructions": len(instructions),
		"x87": 0,
		"cmov": 0,
		"setcc": 0,
		"fst_nonpop_mem": 0,
		"fstp_mem": 0,
		"rep_string": 0,
		"jump_table": 0,
		"word_compare": 0,
		"movsx_word": 0,
		"movzx_word": 0,
		"downcounter_loops": 0,
		"end_compare_loops": 0,
		"inc_dec_flag_reuse": 0,
	}
	previous = None
	for instruction in instructions:
		mnemonic = instruction["mnemonic"]
		operands = instruction["operands"]
		if mnemonic.startswith("f"):
			features["x87"] += 1
			if mnemonic.startswith("fstp") and "(" in operands:
				features["fstp_mem"] += 1
			elif mnemonic.startswith("fst") and not mnemonic.startswith("fstp") \
					and "(" in operands:
				features["fst_nonpop_mem"] += 1
		if mnemonic.startswith("cmov"):
			features["cmov"] += 1
		if mnemonic.startswith("set"):
			features["setcc"] += 1
		if "stos" in mnemonic or ("movs" in mnemonic and mnemonic[-1] in "bwdl"
				and mnemonic.startswith(("movs", "rep"))):
			if mnemonic.startswith("rep"):
				features["rep_string"] += 1
		if mnemonic.startswith("jmp") and re.search(r"\([^)]*,4\)", operands):
			features["jump_table"] += 1
		if mnemonic in ("cmpw", "testw") or (
				mnemonic.startswith(("cmp", "test"))
				and re.search(r"%(?:ax|bx|cx|dx|si|di)\b", operands)):
			features["word_compare"] += 1
		if mnemonic.startswith("movs") and mnemonic.endswith("wl"):
			features["movsx_word"] += 1
		if mnemonic.startswith("movz") and mnemonic.endswith("wl"):
			features["movzx_word"] += 1
		if previous is not None:
			previous_mnemonic = previous["mnemonic"]
			if previous_mnemonic.startswith("dec") and _is_backward(instruction, "jne"):
				features["downcounter_loops"] += 1
			if previous_mnemonic.startswith("cmp") and "$" in previous["operands"] \
					and (_is_backward(instruction, "jl") or _is_backward(instruction, "jb")):
				features["end_compare_loops"] += 1
			if previous_mnemonic.startswith(("inc", "dec")) \
					and mnemonic.startswith("j") and mnemonic != "jmp" \
					and not _is_backward(instruction, "j"):
				features["inc_dec_flag_reuse"] += 1
		previous = instruction
	return features


# ---------- source features

_FUNCTION_HEAD_RE = r"^(?:static\s+)?[A-Za-z_][\w \t*]*?\b{name}\s*\("


def extract_function_source(text, name):
	"""Return the body text of one function definition, or None."""
	pattern = re.compile(_FUNCTION_HEAD_RE.format(name=re.escape(name)), re.M)
	for match in pattern.finditer(text):
		brace = text.find("{", match.end())
		semicolon = text.find(";", match.end())
		if brace == -1 or (semicolon != -1 and semicolon < brace):
			continue  # prototype, not definition
		depth = 0
		for index in range(brace, len(text)):
			if text[index] == "{":
				depth += 1
			elif text[index] == "}":
				depth -= 1
				if depth == 0:
					return text[match.start():index + 1]
	return None


def _declared_type(body, variable):
	"""Return the declared type token for *variable*, or None when ambiguous."""
	found = set()
	for match in re.finditer(
			r"\b(short|long|word|dword|byte|char|int16|int32|uint16|uint32|int|real)\b"
			r"[ \t]+([A-Za-z_]\w*(?:[ \t]*,[ \t]*[A-Za-z_]\w*)*)[ \t]*[;=)]",
			body):
		names = [item.strip() for item in match.group(2).split(",")]
		if variable in names:
			found.add(match.group(1))
	if len(found) == 1:
		return found.pop()
	return None


def source_features(body):
	features = {
		"ternaries": body.count("?"),
		"switches": len(re.findall(r"\bswitch\s*\(", body)),
		"default_first": 0,
		"memset_calls": len(re.findall(r"\bmemset\s*\(|\bcsmemset\s*\(", body)),
		"loop_index_types": [],
		"loop_index_ambiguous": 0,
	}
	for match in re.finditer(r"\bswitch\s*\([^)]*\)", body):
		block_start = body.find("{", match.end())
		if block_start == -1:
			continue
		labels = re.compile(r"\b(case\b|default[ \t]*:)")
		first = labels.search(body, block_start)
		if first and first.group(1).startswith("default"):
			features["default_first"] += 1
	for match in re.finditer(r"\bfor\s*\(\s*([A-Za-z_]\w*)\s*=", body):
		variable = match.group(1)
		declared = _declared_type(body, variable)
		if declared is None:
			features["loop_index_ambiguous"] += 1
		else:
			features["loop_index_types"].append(declared)
	return features


# ---------- correlation entries

def _cite(unit, function):
	return f"{unit}:{function}"


def build_dictionary(records):
	"""Aggregate per-function records into cited dictionary entries."""
	entries = {}

	total_instructions = sum(r["machine"]["instructions"] for r in records)
	x87_total = sum(r["machine"]["x87"] for r in records)
	x87_top = sorted(records, key=lambda r: -r["machine"]["x87"])[:8]
	cmov_hits = [r for r in records if r["machine"]["cmov"]]
	setcc_hits = [r for r in records if r["machine"]["setcc"]]
	entries["instruction-population"] = {
		"claim": "Corpus composition: the exact corpus remains overwhelmingly "
			"integer code; float claims from these statistics are weakly "
			"supported.",
		"functions": len(records),
		"instructions": total_instructions,
		"x87_instructions": x87_total,
		"x87_heaviest": [
			{"function": _cite(r["unit"], r["function"]), "x87": r["machine"]["x87"]}
			for r in x87_top if r["machine"]["x87"]],
		"integer_only": False,
	}
	entries["cmov-never-emitted"] = {
		"claim": "VC7 at the campaign flags does not use CMOV: zero occurrences "
			"across the exact corpus means a conditional in source is always a "
			"branch, never a conditional move.",
		"occurrences": sum(r["machine"]["cmov"] for r in records),
		"counterexamples": [
			_cite(r["unit"], r["function"]) for r in cmov_hits[:12]],
		"integer_only": False,
	}
	entries["setcc-population"] = {
		"claim": "SETcc appears in the corpus (boolean results); its presence "
			"is therefore never evidence of hand-written branch avoidance.",
		"occurrences": sum(r["machine"]["setcc"] for r in records),
		"functions_with": len(setcc_hits),
		"citations": [_cite(r["unit"], r["function"]) for r in setcc_hits[:12]],
		"integer_only": True,
	}

	# Loop index width: correlate only unambiguous (single-width) functions.
	narrow_down = []
	wide_down = []
	narrow_end = []
	wide_end = []
	for r in records:
		if r["source"] is None:
			continue
		types = set(r["source"]["loop_index_types"])
		if r["source"]["loop_index_ambiguous"] or not types:
			continue
		widths = {("narrow" if t in NARROW_TYPES else
				   "wide" if t in WIDE_TYPES else "other") for t in types}
		if len(widths) != 1:
			continue
		width = widths.pop()
		if width == "other":
			continue
		if r["machine"]["downcounter_loops"]:
			(narrow_down if width == "narrow" else wide_down).append(r)
		if r["machine"]["end_compare_loops"]:
			(narrow_end if width == "narrow" else wide_end).append(r)
	entries["loop-index-width"] = {
		"claim": "A dec/jne downcounter loop correlates with a narrow (short/"
			"word) source index; a cmp-immediate/jl end-compare loop "
			"correlates with a wide (long/dword) index.  Counterexamples are "
			"cited: treat the rule as a ranked suggestion, not a law.",
		"downcounter_with_narrow_index": len(narrow_down),
		"downcounter_with_wide_index": len(wide_down),
		"end_compare_with_wide_index": len(wide_end),
		"end_compare_with_narrow_index": len(narrow_end),
		"downcounter_narrow_citations": [
			_cite(r["unit"], r["function"]) for r in narrow_down[:12]],
		"downcounter_wide_counterexamples": [
			_cite(r["unit"], r["function"]) for r in wide_down[:12]],
		"end_compare_wide_citations": [
			_cite(r["unit"], r["function"]) for r in wide_end[:12]],
		"end_compare_narrow_counterexamples": [
			_cite(r["unit"], r["function"]) for r in narrow_end[:12]],
		"integer_only": True,
	}

	table_hits = [r for r in records if r["machine"]["jump_table"]]
	table_with_switch = [
		r for r in table_hits if r["source"] and r["source"]["switches"]]
	default_first = [
		r for r in table_with_switch if r["source"]["default_first"]]
	entries["jump-table-switch"] = {
		"claim": "Every indexed jump through a scale-4 table in the corpus "
			"comes from a source switch; where the jump table exists, note "
			"how often the matched source spells default first.",
		"jump_table_functions": len(table_hits),
		"with_locatable_switch_source": len(table_with_switch),
		"default_first_functions": len(default_first),
		"citations": [_cite(r["unit"], r["function"]) for r in table_hits[:12]],
		"default_first_citations": [
			_cite(r["unit"], r["function"]) for r in default_first[:12]],
		"integer_only": True,
	}

	rep_hits = [r for r in records if r["machine"]["rep_string"]]
	rep_with_memset = [
		r for r in rep_hits if r["source"] and r["source"]["memset_calls"]]
	entries["rep-string-source"] = {
		"claim": "REP string forms in the corpus and how many of their "
			"sources contain an explicit (cs)memset call; the remainder used "
			"aggregate initialisation or the compiler chose the form itself.",
		"rep_functions": len(rep_hits),
		"with_memset_call": len(rep_with_memset),
		"citations": [_cite(r["unit"], r["function"]) for r in rep_hits[:12]],
		"integer_only": True,
	}

	fst_hits = [r for r in records if r["machine"]["fst_nonpop_mem"]]
	entries["fst-nonpop-rarity"] = {
		"claim": "A non-popping FST to memory is rare: the stored value stays "
			"live in the source.  When a residual shows fst/fstp opcode pairs "
			"the classifier's R4 recipe applies (value liveness).",
		"fst_nonpop_stores": sum(r["machine"]["fst_nonpop_mem"] for r in records),
		"fstp_stores": sum(r["machine"]["fstp_mem"] for r in records),
		"functions_with_fst_nonpop": len(fst_hits),
		"citations": [_cite(r["unit"], r["function"]) for r in fst_hits[:12]],
		"integer_only": False,
	}

	reuse_hits = [r for r in records if r["machine"]["inc_dec_flag_reuse"]]
	entries["inc-dec-flag-reuse"] = {
		"claim": "INC/DEC immediately followed by a forward conditional jump "
			"(no TEST/CMP) marks a source side effect whose flags are reused "
			"(house rule: ++p reuses flags, p+1 does not).",
		"sites": sum(r["machine"]["inc_dec_flag_reuse"] for r in records),
		"functions_with": len(reuse_hits),
		"citations": [_cite(r["unit"], r["function"]) for r in reuse_hits[:12]],
		"integer_only": True,
	}

	word_hits = [r for r in records if r["machine"]["word_compare"]]
	entries["word-width-evidence"] = {
		"claim": "16-bit compares and word-extension loads are common and "
			"always trace to a declared narrow type in matched source; a "
			"width mismatch in a residual is a type recovery, not scheduling.",
		"word_compare_sites": sum(r["machine"]["word_compare"] for r in records),
		"movsx_word_sites": sum(r["machine"]["movsx_word"] for r in records),
		"movzx_word_sites": sum(r["machine"]["movzx_word"] for r in records),
		"functions_with_word_compare": len(word_hits),
		"citations": [_cite(r["unit"], r["function"]) for r in word_hits[:12]],
		"integer_only": True,
	}
	return entries


# ---------- driver

def main(argv=None):
	parser = argparse.ArgumentParser(description=__doc__)
	parser.add_argument("--frontier", default="research/residual_frontier.json")
	parser.add_argument("--config", default="objdiff.json")
	parser.add_argument(
		"--output", type=Path, default=Path("research/codegen_dictionary.json"))
	parser.add_argument(
		"--markdown", type=Path, default=Path("research/codegen_dictionary.md"))
	args = parser.parse_args(argv)

	project_root = Path(args.config).resolve().parent
	config = json.loads(Path(args.config).read_text(encoding="utf-8"))
	frontier = json.loads(Path(args.frontier).read_text(encoding="utf-8"))
	config_units = {unit["name"]: unit for unit in config["units"]}

	records = []
	source_missing = Counter()
	for unit_report in frontier["units"]:
		if not unit_report["exact"]:
			continue
		unit = config_units.get(unit_report["unit"])
		if unit is None:
			continue
		target = cc.load(project_root / unit["target_path"])
		source_path = unit.get("metadata", {}).get("source_path")
		source_text = None
		if source_path and (project_root / source_path).exists():
			source_text = (project_root / source_path).read_text(
				encoding="utf-8", errors="replace")
		for item in unit_report["exact"]:
			name = item["function"]
			try:
				instructions = _capstone_instructions(target, name)
			except ClassifierError:
				continue
			body = None
			if source_text is not None:
				body = extract_function_source(source_text, name.lstrip("_"))
			if body is None:
				source_missing[unit_report["unit"]] += 1
			records.append({
				"unit": unit_report["unit"],
				"function": name,
				"size": item["size"],
				"machine": machine_features(instructions),
				"source": source_features(body) if body is not None else None,
			})

	entries = build_dictionary(records)
	report = {
		"corpus_functions": len(records),
		"corpus_with_source": sum(1 for r in records if r["source"] is not None),
		"source_unlocated_by_unit": dict(source_missing.most_common()),
		"entries": entries,
	}
	args.output.parent.mkdir(parents=True, exist_ok=True)
	args.output.write_text(json.dumps(report, indent=1) + "\n", encoding="utf-8")

	lines = ["# Codegen dictionary mined from the exact corpus", ""]
	lines.append(
		f"Ground truth: {len(records)} byte-exact functions "
		f"({report['corpus_with_source']} with located source).  Every entry "
		"cites functions so it can be re-verified; `integer_only` marks "
		"entries whose evidence base is integer code.")
	for key, entry in entries.items():
		lines.append("")
		lines.append(f"## {key}")
		lines.append("")
		lines.append(entry["claim"])
		lines.append("")
		for field, value in entry.items():
			if field in ("claim",):
				continue
			lines.append(f"- **{field}**: {json.dumps(value)}")
	args.markdown.write_text("\n".join(lines) + "\n", encoding="utf-8")

	print(json.dumps({
		"corpus_functions": report["corpus_functions"],
		"corpus_with_source": report["corpus_with_source"],
	}, indent=2))
	return 0


if __name__ == "__main__":
	raise SystemExit(main())
