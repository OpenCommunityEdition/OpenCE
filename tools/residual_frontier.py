"""Measure the residual frontier: classify every non-exact shared function.

For every objdiff unit whose target (csplit) and base (rebuilt) objects both
exist, enumerate the .text COMDAT owner symbols the two objects share, test
each pair with the strict comparator, and run the fail-closed codegen blocker
classifier on every non-exact pair.  The size of the resulting UNKNOWN bucket,
weighted by target bytes, is the measure of how much residual work is not yet
mechanically diagnosable.

Functions present only in the target are unwritten source, not residuals; they
are counted separately and never classified.  This tool grants no match
credit and writes no ledger.

Usage:
    python tools/residual_frontier.py [--config objdiff.json] [--output out.json]
        [--source-only] [--unit NAME ...]
"""
import argparse
import json
import re
import os
import sys
from collections import Counter, defaultdict
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import coff_compare as cc
from codegen_blocker_classifier import (
	ClassifierError,
	_capstone_instructions,
	classify,
)


def text_owners(obj):
	"""Map owner symbol name -> section number for every .text COMDAT.

	The owner is the symbol at offset zero of a code section.  When several
	symbols sit at offset zero the external one wins; compiler-local labels
	(``$L...``) are never owners.
	"""
	owners = {}
	preferred = {}
	for sym in obj["symbols"]:
		if sym["section"] <= 0 or sym["value"] != 0:
			continue
		# Section symbols (".text") and compiler-local labels ("$L...") are
		# never owners; a static function's own symbol is.
		if sym["name"].startswith(("$", ".")):
			continue
		section = obj["sections"][sym["section"] - 1]
		if not section["name"].startswith(".text"):
			continue
		is_external = sym["storage"] == cc.IMAGE_SYM_CLASS_EXTERNAL
		current = preferred.get(sym["section"])
		if current is None or (is_external and not current[1]):
			preferred[sym["section"]] = (sym["name"], is_external)
	for section_number, (name, _) in preferred.items():
		# Duplicate owner names across sections make identity ambiguous;
		# keep the first and let the comparison fail closed via section_info.
		owners.setdefault(name, section_number)
	return owners


_STRING_NAME_PREFIX = "??_C@"
_PLACEHOLDER_RE = re.compile(r"^_?(code|data|bss|rdata|text)_[0-9a-fA-F]{8}$")


def _pair_legitimacy(target_name, base_name):
	"""How safe is treating one rename pair as naming rather than semantics?

	- ``placeholder-ours``: our name is a csplit-style placeholder and the
	  target carries a real name — rename ours to the target's (zlib/trees).
	- ``placeholder-target``: the target carries the placeholder and our
	  source invented a real name — §4 says the target's name wins; rename
	  ours back (a §4 violation to also fix at the definition site).
	- ``crt-variant``: the names differ only by a leading underscore — our
	  source spelled the other CRT alias (``lseek`` vs ``_lseek``); fix the
	  call spelling.
	- ``both-real``: two unrelated real names.  **Never** naming debt on this
	  evidence alone — equal masked bytes can hide a wrong callee (playbook),
	  so this downgrades the classification to unknown.
	"""
	target_placeholder = bool(_PLACEHOLDER_RE.match(target_name))
	base_placeholder = bool(_PLACEHOLDER_RE.match(base_name))
	if base_placeholder and not target_placeholder:
		return "placeholder-ours"
	if target_placeholder and not base_placeholder:
		return "placeholder-target"
	if target_placeholder and base_placeholder:
		return "placeholder-both"
	if target_name.lstrip("_") == base_name.lstrip("_"):
		return "crt-variant"
	return "both-real"


def _relocation_names(info):
	"""Multiset of symbol/anchor names referenced by a section's relocations."""
	names = []
	for relocation in info["relocations"]:
		target = relocation["target"]
		if target[0] == "symbol":
			names.append(target[1])
		elif target[0] == "defined-noncode":
			names.append(target[2])
	return names


def _rename_target(target, mapping):
	if target[0] == "symbol" and target[1] in mapping:
		return ["symbol", mapping[target[1]], *target[2:]]
	if target[0] == "defined-noncode" and target[2] in mapping:
		return [target[0], target[1], mapping[target[2]], *target[3:]]
	return target


def classify_artifact(target_info, base_info):
	"""Classify the naming component of one residual, failing closed.

	Returns a dict with ``kind`` one of:

	- ``pure-codegen``: relocation names agree; the residual is real.
	- ``false-residual``: a consistent positional rename of our symbols makes
	  the strict comparison pass entirely — bytes and layout were already
	  equal and only naming differed.  (Verified via the same
	  ``relocation_infos_equal`` the comparator uses; never a widened rule.)
	- ``naming-debt``: a consistent rename removes the name differences but a
	  real difference remains after it.
	- ``string-content``: paired string-literal constants differ — a real
	  source difference (e.g. an assert or format text), not naming.
	- ``real``: relocation layout differs beyond naming (count/address/type).
	- ``unknown``: name differences with no consistent resolution.
	"""
	target_names = _relocation_names(target_info)
	base_names = _relocation_names(base_info)
	if sorted(target_names) == sorted(base_names):
		return {"kind": "pure-codegen"}

	target_relocations = target_info["relocations"]
	base_relocations = base_info["relocations"]
	if len(target_relocations) != len(base_relocations):
		return {
			"kind": "real",
			"detail": "relocation counts differ; name delta is secondary",
			"target_only": sorted(set(target_names) - set(base_names)),
			"base_only": sorted(set(base_names) - set(target_names)),
		}

	mapping = {}
	string_pairs = []
	for target_item, base_item in zip(target_relocations, base_relocations):
		if target_item["address"] != base_item["address"] \
				or target_item["type"] != base_item["type"]:
			return {
				"kind": "real",
				"detail": "relocation addresses/types differ; name delta is secondary",
				"target_only": sorted(set(target_names) - set(base_names)),
				"base_only": sorted(set(base_names) - set(target_names)),
			}
		def name_view(item):
			"""(name, addend) for symbol-like targets, across representations.

			csplit may resolve a reference to a defined-noncode anchor while
			the rebuilt object spells a plain symbol (or vice versa); the
			``symbolic_target`` preserved by section_info recovers the
			original spelling so the two forms can be paired by name.
			"""
			symbolic = item.get("symbolic_target")
			if symbolic is not None:
				return (symbolic[1], symbolic[2])
			target = item["target"]
			if target[0] == "symbol":
				return (target[1], target[2])
			if target[0] == "defined-noncode":
				return (target[2], target[3])
			return None

		t_view = name_view(target_item)
		b_view = name_view(base_item)
		pairs = []
		if t_view is not None and b_view is not None:
			if t_view[1] != b_view[1] and t_view[0] != b_view[0]:
				return {
					"kind": "real",
					"detail": "paired references differ in both name and "
						"addend; not a naming problem",
				}
			pairs.append((t_view[0], b_view[0]))
		elif (t_view is None) != (b_view is None):
			return {
				"kind": "unknown",
				"detail": "one side resolves to internal/absolute where the "
					"other names a symbol — representation mismatch",
			}
		for t_name, b_name in pairs:
			if t_name == b_name:
				continue
			if t_name.startswith(_STRING_NAME_PREFIX) \
					or b_name.startswith(_STRING_NAME_PREFIX):
				string_pairs.append((t_name, b_name))
				continue
			if mapping.get(b_name, t_name) != t_name:
				return {
					"kind": "unknown",
					"detail": f"inconsistent rename: {b_name} maps to both "
						f"{mapping[b_name]} and {t_name}",
				}
			mapping[b_name] = t_name
	if len(set(mapping.values())) != len(mapping):
		return {"kind": "unknown", "detail": "rename mapping is not a bijection"}
	if string_pairs:
		return {
			"kind": "string-content",
			"pairs": [list(pair) for pair in string_pairs[:6]],
		}

	legitimacy = {
		f"{b}->{t}": _pair_legitimacy(t, b) for b, t in mapping.items()}
	if "both-real" in legitimacy.values():
		return {
			"kind": "unknown",
			"detail": "rename pairs two unrelated real names; equal masked "
				"bytes can hide a wrong callee — needs review",
			"rename_map": {b: t for b, t in sorted(mapping.items())},
			"pair_legitimacy": legitimacy,
		}

	renamed = [
		dict(item, target=_rename_target(item["target"], mapping))
		for item in base_relocations
	]
	resolved_equal = (
		target_info["size"] == base_info["size"]
		and target_info["normalized_sha256"] == base_info["normalized_sha256"]
		and cc.relocation_infos_equal(target_relocations, renamed)
	)
	result = {
		"kind": "false-residual" if resolved_equal else "naming-debt",
		"rename_map": {b: t for b, t in sorted(mapping.items())},
		"pair_legitimacy": legitimacy,
	}
	if not resolved_equal:
		result["detail"] = "rename removes the name delta; bytes still differ"
	return result


def owner_name_candidates(target, base, target_owners, base_owners):
	"""Pair unmatched owners whose (size, normalized sha) match uniquely.

	The normalized hash zeroes relocation slots, so equality is independent
	of symbol naming — a unique pairing is a mechanical rename candidate.
	Anything ambiguous (duplicate hashes on either side) is dropped.
	"""
	def keyed(obj, names, owners):
		table = {}
		for name in names:
			try:
				info = cc.section_info_by_number(obj, owners[name])
			except cc.CoffError:
				continue
			key = (info["size"], info["normalized_sha256"])
			table.setdefault(key, []).append(name)
		return table

	target_only = set(target_owners) - set(base_owners)
	base_only = set(base_owners) - set(target_owners)
	target_table = keyed(target, target_only, target_owners)
	base_table = keyed(base, base_only, base_owners)
	pairs = []
	for key, target_names in target_table.items():
		base_names = base_table.get(key, [])
		if len(target_names) == 1 and len(base_names) == 1:
			pairs.append({
				"target_name": target_names[0],
				"base_name": base_names[0],
				"size": key[0],
			})
	return sorted(pairs, key=lambda item: item["target_name"])


def measure_unit(project_root, unit):
	if "base_path" not in unit:
		return None
	target_path = project_root / unit["target_path"]
	base_path = project_root / unit["base_path"]
	if not target_path.exists() or not base_path.exists():
		return None
	target = cc.load(target_path)
	base = cc.load(base_path)
	target_owners = text_owners(target)
	base_owners = text_owners(base)
	shared = sorted(set(target_owners) & set(base_owners))
	result = {
		"unit": unit["name"],
		"target_only": sorted(set(target_owners) - set(base_owners)),
		"base_only": sorted(set(base_owners) - set(target_owners)),
		"exact": [],
		"residuals": [],
		"errors": [],
	}
	for name in shared:
		try:
			target_info = cc.section_info_by_number(target, target_owners[name])
			base_info = cc.section_info_by_number(base, base_owners[name])
		except cc.CoffError as error:
			result["errors"].append({"function": name, "error": str(error)})
			continue
		if cc.section_infos_equal(target_info, base_info):
			result["exact"].append({"function": name, "size": target_info["size"]})
			continue
		try:
			target_info["instructions"] = _capstone_instructions(target, name)
			base_info["instructions"] = _capstone_instructions(base, name)
			classification = classify(target_info, base_info)
		except ClassifierError as error:
			result["errors"].append({"function": name, "error": str(error)})
			continue
		result["residuals"].append({
			"function": name,
			"target_size": target_info["size"],
			"base_size": base_info["size"],
			"kind": classification["kind"],
			"class": classification["class"],
			"recipe": classification["recipe"],
			"confidence": classification["confidence"],
			"strict_metadata": classification["strict_metadata"],
			"artifact": classify_artifact(target_info, base_info),
		})
	result["owner_name_candidates"] = owner_name_candidates(
		target, base, target_owners, base_owners)
	return result


def main(argv=None):
	parser = argparse.ArgumentParser(description=__doc__)
	parser.add_argument("--config", default="objdiff.json")
	parser.add_argument("--output", type=Path)
	parser.add_argument(
		"--source-only", action="store_true",
		help="skip units whose target lives under build/split/libs")
	parser.add_argument("--unit", action="append", default=[])
	args = parser.parse_args(argv)

	project_root = Path(args.config).resolve().parent
	config = json.loads(Path(args.config).read_text(encoding="utf-8"))

	units = []
	skipped = []
	for unit in config["units"]:
		if args.unit and unit["name"] not in args.unit:
			continue
		if args.source_only and "/libs/" in unit["target_path"].replace("\\", "/"):
			continue
		measured = measure_unit(project_root, unit)
		if measured is None:
			skipped.append(unit["name"])
			continue
		units.append(measured)

	class_counts = Counter()
	class_bytes = Counter()
	artifact_counts = Counter()
	artifact_bytes = Counter()
	owner_candidate_count = 0
	per_unit_unknown = defaultdict(lambda: [0, 0])
	exact_count = 0
	exact_bytes = 0
	target_only = 0
	error_count = 0
	for unit in units:
		exact_count += len(unit["exact"])
		exact_bytes += sum(item["size"] for item in unit["exact"])
		target_only += len(unit["target_only"])
		error_count += len(unit["errors"])
		owner_candidate_count += len(unit.get("owner_name_candidates", []))
		for item in unit["residuals"]:
			key = item["class"]
			class_counts[key] += 1
			class_bytes[key] += item["target_size"]
			artifact_counts[item["artifact"]["kind"]] += 1
			artifact_bytes[item["artifact"]["kind"]] += item["target_size"]
			if item["kind"] == "UNKNOWN":
				per_unit_unknown[unit["unit"]][0] += 1
				per_unit_unknown[unit["unit"]][1] += item["target_size"]

	residual_count = sum(class_counts.values())
	unknown_count = class_counts.get("unclassified", 0)
	unknown_bytes = class_bytes.get("unclassified", 0)
	summary = {
		"units_measured": len(units),
		"units_skipped_missing_object": len(skipped),
		"shared_functions_exact": exact_count,
		"shared_functions_exact_bytes": exact_bytes,
		"shared_functions_residual": residual_count,
		"residual_bytes": sum(class_bytes.values()),
		"unknown_functions": unknown_count,
		"unknown_bytes": unknown_bytes,
		"target_only_functions": target_only,
		"comparison_errors": error_count,
		"classes": {
			key: {"functions": class_counts[key], "target_bytes": class_bytes[key]}
			for key in sorted(class_counts)
		},
		"artifact_kinds": {
			key: {"functions": artifact_counts[key],
				  "target_bytes": artifact_bytes[key]}
			for key in sorted(artifact_counts)
		},
		"owner_name_candidates": owner_candidate_count,
		"unknown_by_unit": {
			name: {"functions": counts[0], "target_bytes": counts[1]}
			for name, counts in sorted(
				per_unit_unknown.items(), key=lambda kv: -kv[1][1])
		},
	}
	report = {"summary": summary, "units": units, "skipped": skipped}
	if args.output:
		args.output.parent.mkdir(parents=True, exist_ok=True)
		args.output.write_text(
			json.dumps(report, indent=1) + "\n", encoding="utf-8")

	print(json.dumps(summary, indent=2))
	return 0


if __name__ == "__main__":
	raise SystemExit(main())
