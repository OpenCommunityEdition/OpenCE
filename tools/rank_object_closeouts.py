"""Rank honest Halo object closeout candidates from the strict evidence ledger.

The ordinary objdiff report is useful for navigation, but its function count
contains local labels and its percentages can disagree with the hardened COFF
comparison.  This tool combines the ordinary inventory with the reviewed
semantic ledger, object status, and parked-function catalog.  It never grants
matching credit.
"""

from __future__ import print_function

import argparse
import collections
import json
import os
import sys


class RankingError(Exception):
	pass


def _load_json(path):
	try:
		with open(path, "r") as stream:
			return json.load(stream)
	except (IOError, ValueError) as error:
		raise RankingError("%s: %s" % (path, error))


def _walk_config(value):
	if isinstance(value, dict):
		if (
			isinstance(value.get("name"), str)
			and value["name"].endswith(".c")
			and "status" in value
		):
			yield value
		for child in value.values():
			for entry in _walk_config(child):
				yield entry
	elif isinstance(value, list):
		for child in value:
			for entry in _walk_config(child):
				yield entry


def _unit_name(source_name):
	return source_name[:-2] if source_name.endswith(".c") else source_name


def _integer(measures, key):
	value = measures.get(key, 0)
	try:
		return int(value)
	except (TypeError, ValueError):
		return 0


def build_rankings(
	report,
	semantic_report,
	config,
	parked,
	ledger_units=None,
):
	ledger_units = set(ledger_units or [])
	status_by_unit = {}
	for entry in _walk_config(config):
		status_by_unit[_unit_name(entry["name"])] = entry["status"]

	accepted = collections.defaultdict(set)
	for entry in semantic_report.get("accepted_ledger", []):
		accepted[entry["unit"]].add(entry["function"])

	parked_by_unit = collections.defaultdict(set)
	for entry in parked.get("entries", []):
		parked_by_unit[entry["unit"]].add(entry["function"])

	rankings = []
	for unit in report.get("units", []):
		metadata = unit.get("metadata", {})
		if "halobetacache" not in metadata.get("progress_categories", []):
			continue

		name = unit["name"]
		status = status_by_unit.get(name, "Unknown")
		measures = unit.get("measures", {})
		if status == "Matching" or _integer(measures, "complete_units"):
			continue

		functions = unit.get("functions", [])
		accepted_names = accepted[name]
		remaining_names = [
			function["name"]
			for function in functions
			if function["name"] not in accepted_names
		]
		remaining_code_bytes = sum(
			int(function.get("size", 0))
			for function in functions
			if function["name"] in remaining_names
		)
		parked_names = sorted(
			set(remaining_names).intersection(parked_by_unit[name]))
		unparked_names = sorted(set(remaining_names) - set(parked_names))

		data_gap = max(
			0,
			_integer(measures, "total_data")
			- _integer(measures, "matched_data"))
		if not remaining_names and data_gap:
			closeout_class = "DATA_CLOSEOUT"
		elif remaining_names and not unparked_names:
			closeout_class = "PROVEN_BLOCKED"
		elif name in ledger_units:
			closeout_class = "LEDGER_REVIEW"
		elif len(unparked_names) <= 5 and data_gap == 0:
			closeout_class = "NEAR_CLOSEOUT"
		else:
			closeout_class = "RECONSTRUCTION"

		# Ranking is deliberately simple and auditable.  Parked residuals sort
		# last; otherwise prefer fewer functions, exact owned data, and fewer
		# remaining code bytes.
		class_order = {
			"DATA_CLOSEOUT": 0,
			"NEAR_CLOSEOUT": 1,
			"RECONSTRUCTION": 2,
			"LEDGER_REVIEW": 3,
			"PROVEN_BLOCKED": 4,
		}[closeout_class]
		sort_key = (
			class_order,
			len(unparked_names),
			1 if data_gap else 0,
			data_gap,
			remaining_code_bytes,
			name,
		)
		rankings.append({
			"unit": name,
			"status": status,
			"class": closeout_class,
			"accepted_functions": len(accepted_names),
			"total_functions": _integer(measures, "total_functions"),
			"remaining_functions": remaining_names,
			"parked_functions": parked_names,
			"unparked_functions": unparked_names,
			"remaining_code_bytes": remaining_code_bytes,
			"raw_data_gap": data_gap,
			"raw_data_percent": measures.get("matched_data_percent", 0),
			"object_ledger_present": name in ledger_units,
			"_sort_key": sort_key,
		})

	rankings.sort(key=lambda entry: entry["_sort_key"])
	for entry in rankings:
		del entry["_sort_key"]
	return rankings


def _print_table(rankings, limit):
	header = (
		"class",
		"functions",
		"code",
		"data",
		"unit",
	)
	print("%-16s %-11s %8s %8s %s" % header)
	for entry in rankings[:limit]:
		functions = "%d/%d" % (
			len(entry["unparked_functions"]),
			len(entry["remaining_functions"]))
		print(
			"%-16s %-11s %8d %8d %s"
			% (
				entry["class"],
				functions,
				entry["remaining_code_bytes"],
				entry["raw_data_gap"],
				entry["unit"],
			))


def main(argv=None):
	parser = argparse.ArgumentParser(
		description="Rank strict object closeouts without granting credit")
	parser.add_argument("--report", default="build/report.json")
	parser.add_argument(
		"--semantic-report", default="build/semantic_report.json")
	parser.add_argument("--config", default="config/config.json")
	parser.add_argument("--parked", default="config/parked.json")
	parser.add_argument(
		"--ledger-dir", default="docs/object_matching_logs")
	parser.add_argument("--limit", type=int, default=40)
	parser.add_argument("--json", action="store_true")
	args = parser.parse_args(argv)

	try:
		ledger_units = set()
		if os.path.isdir(args.ledger_dir):
			for filename in os.listdir(args.ledger_dir):
				if filename.endswith("_obj.md"):
					ledger_units.add(filename[:-7])
		rankings = build_rankings(
			_load_json(args.report),
			_load_json(args.semantic_report),
			_load_json(args.config),
			_load_json(args.parked),
			ledger_units={
				unit["name"]
				for unit in _load_json(args.report).get("units", [])
				if unit["name"].rsplit("/", 1)[-1] in ledger_units
			})
	except RankingError as error:
		print(error, file=sys.stderr)
		return 2

	if args.json:
		print(json.dumps(rankings[:args.limit], indent=2, sort_keys=True))
	else:
		_print_table(rankings, args.limit)
	return 0


if __name__ == "__main__":
	sys.exit(main())
