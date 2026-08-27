"""January's string literals as an oracle, with owner verification.

csplit preserved .rdata, so every ``??_C@`` COMDAT in a target object carries
January's exact string bytes.  Two uses, both fail-closed:

``defects`` — find strings we transcribed wrong.  A raw source-text near-miss
scan has a measured 87% false-positive rate (20 of 23, 2026-08-07), because
source regexes cannot see macro stringization, they match ``#include`` lines
as literals, and a January string similar to one of ours usually belongs to a
*different, unwritten* function.  This mode therefore verifies at the object
level before reporting: the January string must be absent from our **built
object** (not our source text), and its owning .text function must be written
and non-exact on our side.  Everything else is dropped with its reason.

``synopsis`` — describe unwritten code.  Strings referenced only by
target-only functions are format strings, assert expressions and command
names for code nobody has written; grouped by owning function they give a
per-function synopsis before a byte of disassembly is read.

Neither mode grants credit or edits anything.

Usage:
    python tools/string_oracle.py defects  [--config objdiff.json] [--min-ratio 0.8]
    python tools/string_oracle.py synopsis [--config objdiff.json] [--unit NAME]
        [--output research/unwritten_string_synopsis.json]
"""
import argparse
import difflib
import json
import os
import sys
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import coff_compare as cc
from residual_frontier import text_owners


def target_strings(obj):
	"""Map string-section number -> (comdat name, decoded bytes)."""
	out = {}
	for sym in obj["symbols"]:
		if sym["name"].startswith("??_C@") and sym["section"] > 0 \
				and sym["value"] == 0:
			section = obj["sections"][sym["section"] - 1]
			raw = bytes(cc._section_bytes(obj, section))[:section["size"]]
			out[sym["section"]] = (sym["name"], raw.rstrip(b"\0"))
	return out


def string_owners(obj, owners):
	"""Map string COMDAT name -> [owning .text function names]."""
	refs = {}
	for name, secno in owners.items():
		info = cc.section_info_by_number(obj, secno)
		for relocation in info["relocations"]:
			symbolic = relocation.get("symbolic_target")
			target = relocation["target"]
			ref = None
			if symbolic is not None:
				ref = symbolic[1]
			elif target[0] == "symbol":
				ref = target[1]
			elif target[0] == "defined-noncode":
				ref = target[2]
			if ref and ref.startswith("??_C@"):
				refs.setdefault(ref, []).append(name)
	return refs


def iter_units(config, only=None):
	for unit in config["units"]:
		if "base_path" not in unit:
			continue
		if "/libs/" in unit["target_path"].replace("\\", "/"):
			continue
		if only and unit["name"] != only:
			continue
		target_path = Path(unit["target_path"])
		base_path = Path(unit["base_path"])
		if target_path.exists() and base_path.exists():
			yield unit["name"], target_path, base_path


def run_defects(config, min_ratio):
	verified = []
	dropped = {"in-base-object": 0, "owner-unwritten": 0, "owner-exact": 0,
			   "no-similar": 0, "undecodable": 0}
	for name, target_path, base_path in iter_units(config):
		target = cc.load(target_path)
		base = cc.load(base_path)
		base_data = base["data"]
		strings = target_strings(target)
		if not strings:
			continue
		owners = text_owners(target)
		refs = string_owners(target, owners)
		base_owners = text_owners(base)
		base_literals = [
			raw for _, raw in target_strings(base).values()]
		for secno, (comdat, raw) in strings.items():
			if raw + b"\0" in base_data or raw in base_literals:
				continue  # we already emit it somewhere
			try:
				text = raw.decode("ascii")
			except UnicodeDecodeError:
				dropped["undecodable"] += 1
				continue
			owner_names = refs.get(comdat, [])
			written_residual = []
			for fn in owner_names:
				if fn not in base_owners:
					continue
				ti = cc.section_info_by_number(target, owners[fn])
				bi = cc.section_info_by_number(base, base_owners[fn])
				if not cc.section_infos_equal(ti, bi):
					written_residual.append(fn)
			if owner_names and not written_residual:
				key = "owner-exact" if any(
					fn in base_owners for fn in owner_names) else "owner-unwritten"
				dropped[key] += 1
				continue
			best = None
			for _, base_raw in target_strings(base).values():
				try:
					base_text = base_raw.decode("ascii")
				except UnicodeDecodeError:
					continue
				ratio = difflib.SequenceMatcher(
					None, text, base_text, autojunk=False).ratio()
				if ratio >= min_ratio and (best is None or ratio > best[0]):
					best = (ratio, base_text)
			if best is None:
				dropped["no-similar"] += 1
				continue
			verified.append({
				"unit": name,
				"january": text,
				"ours": best[1],
				"ratio": round(best[0], 3),
				"owners": written_residual or owner_names,
			})
	verified.sort(key=lambda item: -item["ratio"])
	for item in verified:
		print(f"{item['unit']}  {item['ratio']}")
		print(f"    JAN {item['january']!r}")
		print(f"    OUR {item['ours']!r}")
		print(f"    owner(s): {', '.join(item['owners']) or '(data only)'}")
	print(f"\nverified defects: {len(verified)}")
	print("dropped:", json.dumps(dropped))
	return 0


def run_synopsis(config, only, output):
	synopsis = {}
	total_functions = 0
	for name, target_path, base_path in iter_units(config, only):
		target = cc.load(target_path)
		base = cc.load(base_path)
		strings = target_strings(target)
		if not strings:
			continue
		owners = text_owners(target)
		base_owners = text_owners(base)
		refs = string_owners(target, owners)
		per_function = {}
		for comdat, owner_list in refs.items():
			for secno, (comdat_name, raw) in strings.items():
				if comdat_name != comdat:
					continue
				try:
					text = raw.decode("ascii")
				except UnicodeDecodeError:
					continue
				for fn in owner_list:
					if fn in base_owners:
						continue  # written; synopsis is for unwritten code
					per_function.setdefault(fn, []).append(text)
		if per_function:
			sized = {}
			for fn, texts in per_function.items():
				info = cc.section_info_by_number(target, owners[fn])
				sized[fn] = {"size": info["size"], "strings": sorted(set(texts))}
			synopsis[name] = sized
			total_functions += len(sized)
	if output:
		Path(output).parent.mkdir(parents=True, exist_ok=True)
		Path(output).write_text(
			json.dumps(synopsis, indent=1) + "\n", encoding="utf-8")
	print(f"units with unwritten string-bearing functions: {len(synopsis)}")
	print(f"unwritten functions with a synopsis: {total_functions}")
	if only:
		print(json.dumps(synopsis.get(only, {}), indent=1))
	return 0


def main(argv=None):
	parser = argparse.ArgumentParser(description=__doc__)
	parser.add_argument("mode", choices=("defects", "synopsis"))
	parser.add_argument("--config", default="objdiff.json")
	parser.add_argument("--min-ratio", type=float, default=0.8)
	parser.add_argument("--unit")
	parser.add_argument(
		"--output", default="research/unwritten_string_synopsis.json")
	args = parser.parse_args(argv)
	config = json.loads(Path(args.config).read_text(encoding="utf-8"))
	if args.mode == "defects":
		return run_defects(config, args.min_ratio)
	return run_synopsis(config, args.unit, args.output)


if __name__ == "__main__":
	raise SystemExit(main())
