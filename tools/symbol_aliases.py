"""Discover unique cross-name exact functions within the same COFF object.

This is advisory only.  It identifies cases where csplit gave a target
function an anonymous name while the compiler emitted the byte-identical
function under a recovered source/SDK name.  Ambiguous shapes fail closed.
"""

import argparse
import json
from collections import defaultdict
from pathlib import Path

from .coff_compare import CoffError, load, section_info


def measurement_key(measurement):
	return json.dumps(measurement, sort_keys=True, separators=(",", ":"))


def pair_unique_aliases(target_entries, base_entries):
	"""Pair only one-target/one-base exact shapes with different names."""
	target_by_shape = defaultdict(list)
	base_by_shape = defaultdict(list)
	for entry in target_entries:
		target_by_shape[entry["measurement_key"]].append(entry)
	for entry in base_entries:
		base_by_shape[entry["measurement_key"]].append(entry)
	aliases = []
	for shape, target_group in target_by_shape.items():
		base_group = base_by_shape.get(shape, [])
		if len(target_group) != 1 or len(base_group) != 1:
			continue
		target = target_group[0]
		base = base_group[0]
		if target["function"] == base["function"]:
			continue
		aliases.append({
			"target_function": target["function"],
			"base_function": base["function"],
			"size": target["size"],
			"relocation_count": target["relocation_count"],
			"normalized_sha256": target["normalized_sha256"],
		})
	return sorted(aliases, key=lambda entry: (entry["target_function"], entry["base_function"]))


def _function_entries(obj, names):
	entries = []
	for name in names:
		try:
			measurement = section_info(obj, name)
		except CoffError:
			continue
		entries.append({
			"function": name,
			"size": measurement["size"],
			"relocation_count": measurement["relocation_count"],
			"normalized_sha256": measurement["normalized_sha256"],
			"measurement_key": measurement_key(measurement),
		})
	return entries


def _defined_function_names(obj):
	return {
		symbol["name"]
		for symbol in obj["symbols"]
		if symbol["section"] > 0 and symbol["type"] == 0x20
	}


def analyze(project_root, report_path, config_path):
	project_root = Path(project_root)
	report = json.loads(Path(report_path).read_text(encoding="utf-8"))
	config = json.loads(Path(config_path).read_text(encoding="utf-8"))
	config_units = {unit["name"]: unit for unit in config.get("units", [])}
	aliases = []
	errors = []

	for report_unit in report.get("units", []):
		unit_name = report_unit["name"]
		categories = report_unit.get("metadata", {}).get("progress_categories") or []
		if "halobetacache" not in categories:
			continue
		config_unit = config_units.get(unit_name)
		if not config_unit or not config_unit.get("base_path"):
			continue
		try:
			target = load(project_root / config_unit["target_path"])
			base = load(project_root / config_unit["base_path"])
		except (CoffError, OSError, KeyError) as error:
			errors.append({"unit": unit_name, "error": str(error)})
			continue
		target_names = {
			function["name"]
			for function in report_unit.get("functions", [])
			if not function["name"].startswith("$")
		}
		base_names = _defined_function_names(base)
		missing_in_base = target_names - base_names
		extra_in_base = base_names - target_names
		for alias in pair_unique_aliases(
			_function_entries(target, missing_in_base),
			_function_entries(base, extra_in_base),
		):
			alias["unit"] = unit_name
			aliases.append(alias)

	return {
		"summary": {"aliases": len(aliases), "errors": len(errors)},
		"aliases": sorted(aliases, key=lambda entry: (entry["unit"], entry["target_function"])),
		"errors": errors,
	}


def main():
	parser = argparse.ArgumentParser(description=__doc__)
	parser.add_argument("--project", type=Path, default=Path.cwd())
	parser.add_argument("--report", type=Path, default=Path("build/report.json"))
	parser.add_argument("--config", type=Path, default=Path("objdiff.json"))
	parser.add_argument("--output", type=Path)
	args = parser.parse_args()
	root = args.project.resolve()
	result = analyze(root, root / args.report, root / args.config)
	serialized = json.dumps(result, indent=2) + "\n"
	if args.output:
		output = root / args.output
		output.parent.mkdir(parents=True, exist_ok=True)
		output.write_text(serialized, encoding="utf-8")
	print(serialized, end="")
	if result["errors"]:
		raise SystemExit(1)


if __name__ == "__main__":
	main()
