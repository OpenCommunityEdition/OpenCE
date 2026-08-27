"""Census of flag-diagnostic callee identities across every unit.

A compiler flag that changes which helper symbol a call site references is
decidable from level-1 evidence: the target object's own relocations. The
diagnostic families were demonstrated with the campaign compiler
(XDK 3911 CL 13.00.9254.1) by compiling one probe TU under
``/O2 /Oy-`` x {``/Op``, ``/QIfist``}:

======================  ==================  =====================
call                    default /O2         with /Op
======================  ==================  =====================
sin cos tan atan atan2  inline x87, no ref  _sin _cos _tan ...
sqrt exp log log10 fabs inline x87, no ref  _sqrt _exp _log ...
asin acos pow fmod      __CIasin __CIpow..  _asin _acos _pow _fmod
floor ceil              _floor _ceil        _floor _ceil (never diagnostic)
float/double -> int     __ftol2             __ftol2 (see /QIfist)
======================  ==================  =====================

``/QIfist`` replaces the ``__ftol2`` call with inline ``fistp`` and is
independent of ``/Op``.  Therefore, from a target object alone:

- a reference to ``__ftol``/``__ftol2``  =>  the unit had **no** ``/QIfist``;
- fist/fistp instructions outside any ``__ftol*`` body, with no
  ``__ftol*`` reference  =>  the unit **had** ``/QIfist``;
- a reference to a library math name in the /Op-diagnostic set  =>  the unit
  **had** ``/Op`` (or an equivalent intrinsic-suppressing state);
- a reference to a ``__CI*`` name  =>  the unit had **no** ``/Op``;
- both at once in one object is a contradiction worth loud reporting.

The census applies those oracles to every unit's target and rebuilt base
object, compares them with the configured per-unit flags, and reports every
unit- and function-level contradiction with the residual bytes behind it.
This tool changes nothing and grants no credit.

Usage:
    python tools/flag_provenance_census.py [--config objdiff.json]
        [--frontier research/residual_frontier.json]
        [--output research/flag_provenance_census.json]
"""
import argparse
import json
import os
import re
import sys
from pathlib import Path

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import coff_compare as cc
import capstone

_md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)

FTOL_NAMES = {"__ftol", "__ftol2"}
OP_LIBRARY_NAMES = {
	"_sin", "_cos", "_tan", "_asin", "_acos", "_atan", "_atan2",
	"_sqrt", "_exp", "_log", "_log10", "_pow", "_fmod", "_fabs",
}
CI_NAMES_COMPILER_EMITS = {"__CIasin", "__CIacos", "__CIpow", "__CIfmod"}
CI_NAMES_ANOMALOUS = {
	"__CIsin", "__CIcos", "__CItan", "__CIatan", "__CIatan2",
	"__CIsqrt", "__CIexp", "__CIlog", "__CIlog10",
}
CI_NAMES = CI_NAMES_COMPILER_EMITS | CI_NAMES_ANOMALOUS
ALL_DIAGNOSTIC = FTOL_NAMES | OP_LIBRARY_NAMES | CI_NAMES


def text_owners(obj):
	preferred = {}
	for sym in obj["symbols"]:
		if sym["section"] <= 0 or sym["value"] != 0:
			continue
		if sym["name"].startswith(("$", ".")):
			continue
		section = obj["sections"][sym["section"] - 1]
		if not section["name"].startswith(".text"):
			continue
		is_external = sym["storage"] == cc.IMAGE_SYM_CLASS_EXTERNAL
		current = preferred.get(sym["section"])
		if current is None or (is_external and not current[1]):
			preferred[sym["section"]] = (sym["name"], is_external)
	owners = {}
	for section_number, (name, _) in preferred.items():
		owners.setdefault(name, section_number)
	return owners


def _function_helpers(obj, section_number):
	"""Diagnostic helper names referenced by one function section."""
	try:
		info = cc.section_info_by_number(obj, section_number)
	except cc.CoffError:
		return set(), 0
	names = set()
	for relocation in info["relocations"]:
		target = relocation["target"]
		if target[0] == "symbol" and target[1] in ALL_DIAGNOSTIC:
			names.add(target[1])
		symbolic = relocation.get("symbolic_target")
		if symbolic and symbolic[1] in ALL_DIAGNOSTIC:
			names.add(symbolic[1])
	return names, info["size"]


def _fist_sites(obj, section_number, owner_name):
	"""Return validated fist/fistp sites: [(address, width_bytes, context)].

	The ``__ftol*`` bodies themselves contain fistp; a unit that *defines*
	__ftol (libcmt) must not count, so callers exclude those owners by name.
	Jump-table and translation-table bytes in .text can decode as anything,
	so a site only counts when a real x87 instruction precedes it within
	three instructions — a conversion always loads its operand first.
	"""
	if owner_name.lstrip("_").startswith("ftol"):
		return []
	section = obj["sections"][section_number - 1]
	raw = bytearray(cc._section_bytes(obj, section))[:section["size"]]
	for entry in range(section["reloc_count"]):
		offset = section["reloc"] + entry * cc.RELOC_ENTRY_SIZE
		address = int.from_bytes(obj["data"][offset:offset + 4], "little")
		if address + 4 <= len(raw):
			raw[address:address + 4] = b"\0\0\0\0"
	decoded = list(_md.disasm(bytes(raw), 0))
	sites = []
	for index, insn in enumerate(decoded):
		if not insn.mnemonic.startswith("fist"):
			continue
		window = decoded[max(0, index - 3):index]
		if not any(item.mnemonic.startswith("f") for item in window):
			continue
		if "qword" in insn.op_str:
			width = 8
		elif "dword" in insn.op_str:
			width = 4
		else:
			width = 2
		context = "; ".join(
			f"{item.mnemonic} {item.op_str}" for item in decoded[max(0, index - 2):index + 1])
		sites.append({"address": insn.address, "width": width, "context": context})
	return sites


def measure_object(obj):
	owners = text_owners(obj)
	per_function = {}
	unit_helpers = set()
	unit_fist_sites = []
	for name, section_number in owners.items():
		helpers, size = _function_helpers(obj, section_number)
		sites = _fist_sites(obj, section_number, name)
		per_function[name] = {
			"helpers": sorted(helpers), "size": size, "fist_sites": sites}
		unit_helpers |= helpers
		unit_fist_sites.extend(dict(site, function=name) for site in sites)
	return per_function, unit_helpers, unit_fist_sites


def classify_unit(target_helpers, target_fist_sites, configured_flags):
	"""Return per-flag verdicts from target evidence alone.

	Width matters: measured on this compiler, ``/QIfist`` always converts
	through a 64-bit ``fistpll`` regardless of source or destination width,
	while the January inline sites are 32-bit ``fistpl``.  A 32-bit inline
	site is therefore *never* /QIfist output — it is an inlined original
	``__asm fld/fistp`` helper — and only a qword site is compatible with
	/QIfist at all.
	"""
	verdicts = {}
	ftol = sorted(target_helpers & FTOL_NAMES)
	narrow_sites = [s for s in target_fist_sites if s["width"] != 8]
	qword_sites = [s for s in target_fist_sites if s["width"] == 8]
	if ftol:
		verdicts["/QIfist"] = ("contradicted-if-configured", ftol)
	elif qword_sites and not narrow_sites:
		verdicts["/QIfist"] = (
			"compatible", [f"{len(qword_sites)} qword inline site(s)"])
	elif narrow_sites:
		verdicts["/QIfist"] = (
			"asm-helper-sites",
			[f"{len(narrow_sites)} narrow inline fistp site(s); "
			 "/QIfist emits fistpll, cannot produce these"])
	else:
		verdicts["/QIfist"] = ("not-diagnostic", [])

	library = sorted(target_helpers & OP_LIBRARY_NAMES)
	intrinsic = sorted(target_helpers & CI_NAMES)
	if library and intrinsic:
		verdicts["/Op"] = ("mixed-evidence", library + intrinsic)
	elif library:
		verdicts["/Op"] = ("required", library)
	elif intrinsic:
		verdicts["/Op"] = ("contradicted-if-configured", intrinsic)
	else:
		verdicts["/Op"] = ("not-diagnostic", [])

	contradictions = []
	has_qifist = "/QIfist" in configured_flags
	has_op = "/Op" in configured_flags
	if verdicts["/QIfist"][0] == "contradicted-if-configured" and has_qifist:
		contradictions.append("/QIfist configured but target references " + ",".join(ftol))
	if verdicts["/QIfist"][0] == "asm-helper-sites" and has_qifist:
		contradictions.append(
			"/QIfist configured but target inline sites are narrow fistp, "
			"which /QIfist (always fistpll) cannot emit")
	if verdicts["/Op"][0] == "contradicted-if-configured" and has_op:
		contradictions.append("/Op configured but target references " + ",".join(intrinsic))
	if verdicts["/Op"][0] == "required" and not has_op:
		contradictions.append("/Op absent but target references " + ",".join(library))
	return verdicts, contradictions


def unit_flags(build_ninja_text, base_path):
	"""Extract the cflags ninja actually uses for one unit's base object."""
	needle = "build " + base_path.replace("/", "\\") + ": cl"
	index = build_ninja_text.find(needle)
	if index < 0:
		return None
	block_end = build_ninja_text.find("\nbuild ", index + 1)
	block = build_ninja_text[index:block_end if block_end > 0 else None]
	match = re.search(r"cflags = (.*?)(?=\n[a-z]|\nbuild |\Z)", block, re.S)
	if not match:
		return None
	flags = match.group(1).replace("$\n", " ")
	return [token for token in flags.split() if token.startswith("/")
			and not token.startswith("/I")]


def main(argv=None):
	parser = argparse.ArgumentParser(description=__doc__)
	parser.add_argument("--config", default="objdiff.json")
	parser.add_argument("--frontier", default="research/residual_frontier.json")
	parser.add_argument(
		"--output", type=Path,
		default=Path("research/flag_provenance_census.json"))
	args = parser.parse_args(argv)

	project_root = Path(args.config).resolve().parent
	config = json.loads(Path(args.config).read_text(encoding="utf-8"))
	frontier = json.loads(Path(args.frontier).read_text(encoding="utf-8"))
	build_ninja = (project_root / "build.ninja").read_text(encoding="utf-8")
	residuals_by_unit = {
		unit["unit"]: unit["residuals"] for unit in frontier["units"]}

	units_report = []
	for unit in config["units"]:
		if "base_path" not in unit:
			continue
		if "/libs/" in unit["target_path"].replace("\\", "/"):
			continue
		target_path = project_root / unit["target_path"]
		base_path = project_root / unit["base_path"]
		if not target_path.exists() or not base_path.exists():
			continue
		target = cc.load(target_path)
		base = cc.load(base_path)
		target_functions, target_helpers, target_fist_sites = measure_object(target)
		base_functions, base_helpers, _ = measure_object(base)
		configured = unit_flags(build_ninja, unit["base_path"]) or []
		verdicts, contradictions = classify_unit(
			target_helpers, target_fist_sites, configured)

		residual_names = {
			item["function"] for item in residuals_by_unit.get(unit["name"], [])}
		function_mismatches = []
		helper_blocked = []
		for name, target_entry in target_functions.items():
			base_entry = base_functions.get(name)
			if base_entry is None:
				continue
			if set(target_entry["helpers"]) != set(base_entry["helpers"]):
				function_mismatches.append({
					"function": name,
					"residual": name in residual_names,
					"target_helpers": target_entry["helpers"],
					"base_helpers": base_entry["helpers"],
					"target_size": target_entry["size"],
				})
			narrow = [s for s in target_entry["fist_sites"] if s["width"] != 8]
			if narrow and set(base_entry["helpers"]) & FTOL_NAMES \
					and not set(target_entry["helpers"]) & FTOL_NAMES:
				helper_blocked.append({
					"function": name,
					"residual": name in residual_names,
					"narrow_sites": len(narrow),
					"contexts": [s["context"] for s in narrow[:3]],
					"target_size": target_entry["size"],
				})

		if not (target_helpers | base_helpers) and not target_fist_sites \
				and not contradictions:
			continue  # nothing diagnostic in this unit; keep the report small
		units_report.append({
			"unit": unit["name"],
			"configured_flags": configured,
			"target_helpers": sorted(target_helpers),
			"base_helpers": sorted(base_helpers),
			"target_fist_widths": sorted(
				{site["width"] for site in target_fist_sites}),
			"verdicts": {flag: {"verdict": verdict, "evidence": evidence}
						 for flag, (verdict, evidence) in verdicts.items()},
			"contradictions": contradictions,
			"function_helper_mismatches": function_mismatches,
			"asm_helper_blocked_functions": helper_blocked,
			"residual_functions": len(residuals_by_unit.get(unit["name"], [])),
			"residual_bytes": sum(
				item["target_size"]
				for item in residuals_by_unit.get(unit["name"], [])),
		})

	contradicted = [u for u in units_report if u["contradictions"]]
	helper_units = [u for u in units_report if u["asm_helper_blocked_functions"]]
	anomalies = [
		u["unit"] for u in units_report
		if set(u["target_helpers"]) & CI_NAMES_ANOMALOUS]
	blocked_fn = [
		dict(item, unit=u["unit"])
		for u in units_report for item in u["asm_helper_blocked_functions"]]
	blocked_residual = [item for item in blocked_fn if item["residual"]]
	summary = {
		"units_with_diagnostic_evidence": len(units_report),
		"units_contradicted": len(contradicted),
		"contradicted_units": {
			u["unit"]: {
				"contradictions": u["contradictions"],
				"residual_functions": u["residual_functions"],
				"residual_bytes": u["residual_bytes"],
			} for u in contradicted},
		"units_with_asm_helper_sites": len(helper_units),
		"asm_helper_blocked_functions": len(blocked_fn),
		"asm_helper_blocked_residuals": len(blocked_residual),
		"asm_helper_blocked_residual_bytes": sum(
			item["target_size"] for item in blocked_residual),
		"asm_helper_blocked_residual_list": [
			f"{item['unit']}:{item['function']} ({item['target_size']}B)"
			for item in blocked_residual],
		"anomalous_CI_targets": anomalies,
	}
	report = {"summary": summary, "units": units_report}
	args.output.parent.mkdir(parents=True, exist_ok=True)
	args.output.write_text(json.dumps(report, indent=1) + "\n", encoding="utf-8")
	print(json.dumps(summary, indent=2))
	return 0


if __name__ == "__main__":
	raise SystemExit(main())
