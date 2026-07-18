"""Classify nonexact functions by same-translation-unit caller context.

The tool is deliberately conservative.  It never grants exactness and it does
not claim that adding callers will repair a helper.  It answers the narrower
scheduling question: whether source-shape work is justified now, or whether
absent same-object callers should be reconstructed and measured first.
"""

import argparse
import json
from collections import defaultdict
from pathlib import Path

from .coff_compare import CoffError, IMAGE_REL_I386_DIR32, IMAGE_REL_I386_REL32, load, section_info


VERDICTS = {
	"ready-to-shape",
	"blocked-on-callers",
	"park-candidate",
	"structural",
}


def classify_candidate(
	base_present,
	objdiff_percent,
	relocation_identities_equal,
	absent_callers,
	size_equal,
	relocation_count_equal,
):
	"""Return a fail-closed scheduling verdict for one nonexact function."""
	if not base_present:
		return "structural"
	if absent_callers and objdiff_percent >= 70.0:
		return "blocked-on-callers"
	if relocation_identities_equal and size_equal and not absent_callers:
		return "park-candidate"
	if not size_equal or not relocation_count_equal or objdiff_percent < 70.0:
		return "structural"
	return "ready-to-shape"


def _relocation_identities_equal(target_info, base_info):
	return target_info["relocations"] == base_info["relocations"]


def analyze(project_root, report_path, config_path):
	project_root = Path(project_root)
	report = json.loads(Path(report_path).read_text(encoding="utf-8"))
	config = json.loads(Path(config_path).read_text(encoding="utf-8"))
	config_units = {unit["name"]: unit for unit in config.get("units", [])}
	results = []
	errors = []

	for report_unit in report.get("units", []):
		unit_name = report_unit["name"]
		config_unit = config_units.get(unit_name)
		if not config_unit or "base_path" not in config_unit:
			continue
		functions = {
			function["name"]: function
			for function in report_unit.get("functions", [])
			if not function["name"].startswith("$")
		}
		if not functions:
			continue
		try:
			target = load(project_root / config_unit["target_path"])
			base = load(project_root / config_unit["base_path"])
		except (CoffError, OSError) as error:
			errors.append({"unit": unit_name, "error": str(error)})
			continue

		target_info = {}
		base_info = {}
		for function_name in functions:
			try:
				target_info[function_name] = section_info(target, function_name)
			except CoffError:
				continue
			try:
				base_info[function_name] = section_info(base, function_name)
			except CoffError:
				base_info[function_name] = None

		callers = defaultdict(set)
		address_taken = set()
		for caller_name, info in target_info.items():
			for relocation in info["relocations"]:
				target_identity = relocation["target"]
				if len(target_identity) < 2 or target_identity[0] != "symbol":
					continue
				callee_name = target_identity[1]
				if callee_name not in functions:
					continue
				if relocation["type"] == IMAGE_REL_I386_REL32:
					callers[callee_name].add(caller_name)
				elif relocation["type"] == IMAGE_REL_I386_DIR32:
					address_taken.add(callee_name)

		for function_name, target_measurement in target_info.items():
			base_measurement = base_info[function_name]
			if base_measurement is not None and target_measurement == base_measurement:
				continue
			function_callers = sorted(callers[function_name])
			absent_callers = sorted(
				caller for caller in function_callers if base_info.get(caller) is None
			)
			percent = float(functions[function_name].get("fuzzy_match_percent", 0.0))
			if base_measurement is None:
				size_equal = False
				relocation_count_equal = False
				relocation_identities_equal = False
				base_snapshot = None
			else:
				size_equal = target_measurement["size"] == base_measurement["size"]
				relocation_count_equal = (
					target_measurement["relocation_count"] ==
					base_measurement["relocation_count"]
				)
				relocation_identities_equal = _relocation_identities_equal(
					target_measurement, base_measurement
				)
				base_snapshot = {
					"size": base_measurement["size"],
					"relocation_count": base_measurement["relocation_count"],
					"normalized_sha256": base_measurement["normalized_sha256"],
				}

			verdict = classify_candidate(
				base_measurement is not None,
				percent,
				relocation_identities_equal,
				absent_callers,
				size_equal,
				relocation_count_equal,
			)
			results.append({
				"unit": unit_name,
				"function": function_name,
				"callers": function_callers,
				"callers_absent": absent_callers,
				"address_taken": function_name in address_taken,
				"objdiff_percent": percent,
				"target": {
					"size": target_measurement["size"],
					"relocation_count": target_measurement["relocation_count"],
					"normalized_sha256": target_measurement["normalized_sha256"],
				},
				"base": base_snapshot,
				"relocation_identities_equal": relocation_identities_equal,
				"verdict": verdict,
			})

	return {
		"summary": {
			"functions": len(results),
			"errors": len(errors),
			"verdicts": {
				verdict: sum(item["verdict"] == verdict for item in results)
				for verdict in sorted(VERDICTS)
			},
		},
		"functions": sorted(results, key=lambda item: (item["unit"], item["function"])),
		"errors": errors,
	}


def main():
	parser = argparse.ArgumentParser(description=__doc__)
	parser.add_argument("--project", type=Path, default=Path.cwd())
	parser.add_argument("--report", type=Path, default=Path("build/report.json"))
	parser.add_argument("--config", type=Path, default=Path("objdiff.json"))
	parser.add_argument("--output", type=Path)
	parser.add_argument("--unit", action="append", help="limit output to an object name")
	args = parser.parse_args()
	root = args.project.resolve()
	result = analyze(root, root / args.report, root / args.config)
	if args.unit:
		allowed = set(args.unit)
		result["functions"] = [
			item for item in result["functions"] if item["unit"] in allowed
		]
		result["summary"]["functions"] = len(result["functions"])
		result["summary"]["verdicts"] = {
			verdict: sum(item["verdict"] == verdict for item in result["functions"])
			for verdict in sorted(VERDICTS)
		}
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
