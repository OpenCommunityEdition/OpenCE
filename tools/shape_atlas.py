"""Find reusable target-code shapes without granting exact-match credit.

The atlas is a scheduling aid.  It groups target functions only when their
relocation-masked bytes, padded size, relocation count, and relocation-type
sequence agree.  An exact donor means that a proven source template exists;
the recipient must still pass the ordinary strict comparator after it is
implemented in its own translation unit.
"""

import argparse
import json
from collections import defaultdict
from pathlib import Path

from .coff_compare import CoffError, load, section_info


def _shape_key(measurement):
	return (
		measurement["size"],
		measurement["relocation_count"],
		measurement["normalized_sha256"],
		tuple(relocation["type"] for relocation in measurement["relocations"]),
	)


def build_atlas(catalog, minimum_bulk_members=3):
	"""Return transfer and unsolved-bulk groups from catalog entries."""
	groups = defaultdict(list)
	for entry in catalog:
		groups[tuple(entry["shape_key"])].append(entry)

	transfers = []
	bulk = []
	for shape_key, members in groups.items():
		exact = [member for member in members if member["status"] == "exact"]
		nonexact = [member for member in members if member["status"] != "exact"]
		if exact and nonexact:
			transfers.append({
				"shape": _shape_record(shape_key),
				"donors": sorted(exact, key=_member_sort_key),
				"recipients": sorted(nonexact, key=_member_sort_key),
				"recipient_bytes": sum(member["size"] for member in nonexact),
			})
		elif not exact and len(nonexact) >= minimum_bulk_members:
			bulk.append({
				"shape": _shape_record(shape_key),
				"members": sorted(nonexact, key=_member_sort_key),
				"total_bytes": sum(member["size"] for member in nonexact),
			})

	transfers.sort(key=lambda group: (-group["recipient_bytes"], group["shape"]["sha256"]))
	bulk.sort(key=lambda group: (-group["total_bytes"], group["shape"]["sha256"]))
	return transfers, bulk


def _shape_record(shape_key):
	size, relocation_count, sha256, relocation_types = shape_key
	return {
		"size": size,
		"relocation_count": relocation_count,
		"sha256": sha256,
		"relocation_types": list(relocation_types),
	}


def _member_sort_key(member):
	return member["unit"], member["function"]


def analyze(project_root, report_path, semantic_path, config_path):
	project_root = Path(project_root)
	report = json.loads(Path(report_path).read_text(encoding="utf-8"))
	semantic = json.loads(Path(semantic_path).read_text(encoding="utf-8"))
	config = json.loads(Path(config_path).read_text(encoding="utf-8"))
	config_units = {unit["name"]: unit for unit in config.get("units", [])}
	exact = {
		(entry["unit"], entry["function"])
		for entry in semantic.get("accepted_ledger", [])
	}
	catalog = []
	errors = []

	for report_unit in report.get("units", []):
		unit_name = report_unit["name"]
		categories = report_unit.get("metadata", {}).get("progress_categories") or []
		if "halobetacache" not in categories:
			continue
		config_unit = config_units.get(unit_name)
		if not config_unit:
			continue
		try:
			target = load(project_root / config_unit["target_path"])
			base_path = config_unit.get("base_path")
			base = load(project_root / base_path) if base_path else None
		except (CoffError, OSError, KeyError) as error:
			errors.append({"unit": unit_name, "error": str(error)})
			continue

		for function in report_unit.get("functions", []):
			function_name = function["name"]
			if function_name.startswith("$"):
				continue
			try:
				measurement = section_info(target, function_name)
			except CoffError:
				continue
			if base is None:
				base_present = False
			else:
				try:
					section_info(base, function_name)
					base_present = True
				except CoffError:
					base_present = False
			status = "exact" if (unit_name, function_name) in exact else (
				"implemented" if base_present else "absent"
			)
			catalog.append({
				"unit": unit_name,
				"function": function_name,
				"status": status,
				"size": measurement["size"],
				"relocation_count": measurement["relocation_count"],
				"normalized_sha256": measurement["normalized_sha256"],
				"shape_key": list(_shape_key(measurement)),
			})

	transfers, bulk = build_atlas(catalog)
	return {
		"summary": {
			"functions": len(catalog),
			"distinct_shapes": len({tuple(entry["shape_key"]) for entry in catalog}),
			"transfer_groups": len(transfers),
			"transfer_recipients": sum(len(group["recipients"]) for group in transfers),
			"transfer_bytes": sum(group["recipient_bytes"] for group in transfers),
			"bulk_groups": len(bulk),
			"bulk_bytes": sum(group["total_bytes"] for group in bulk),
			"errors": len(errors),
		},
		"transfer": transfers,
		"bulk": bulk,
		"errors": errors,
	}


def main():
	parser = argparse.ArgumentParser(description=__doc__)
	parser.add_argument("--project", type=Path, default=Path.cwd())
	parser.add_argument("--report", type=Path, default=Path("build/report.json"))
	parser.add_argument("--semantic", type=Path, default=Path("build/semantic_report.json"))
	parser.add_argument("--config", type=Path, default=Path("objdiff.json"))
	parser.add_argument("--output", type=Path)
	args = parser.parse_args()
	root = args.project.resolve()
	result = analyze(root, root / args.report, root / args.semantic, root / args.config)
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
