"""Conservative progress corrections for proven objdiff false negatives.

Entries in config/semantic_matches.json are never trusted on their own. Each
entry is re-verified against the current target and rebuilt COFF objects before
its function is credited. The sibling semantic report may then supply further
accepted functions, but only through its fail-closed COFF ledger and only after
the report identity, proof source, size, uniqueness and rejection set agree.
This keeps the ordinary objdiff report authoritative except where a stricter
semantic relocation comparison proves exact equality.
"""

import copy
import hashlib
import json
import struct
import subprocess
import tempfile
from collections import Counter
from pathlib import Path
from typing import Any, Callable, Dict, List, Optional

from .coff_compare import (
    CoffError,
    IMAGE_SCN_CNT_CODE,
    IMAGE_SCN_CNT_UNINITIALIZED_DATA,
    SYMBOL_ENTRY_SIZE,
    image_symbol_addresses,
    load,
    section_info,
    section_info_by_number,
    section_info_resolved,
    section_info_source_relative,
    section_infos_equal,
)


class SemanticProgressError(RuntimeError):
    pass


def _verify_local_label_continuation(target, base, label_name, owner_name):
    """Prove that an objdiff ``$L`` pseudo-function is an exact continuation.

    csplit can expose a compiler-local jump-table label as an external,
    function-typed symbol while the rebuilt COFF emits a differently named
    static label at the same offset.  The label is creditable only when the
    complete owning COMDAT is semantically exact, both labels are unique at
    that offset, and an internal relocation in the owner proves the offset is
    an actual encoded destination.  Any ambiguity fails closed.
    """
    if not label_name.startswith("$"):
        raise SemanticProgressError(
            f"local-label match does not name a local label: {label_name}")

    target_labels = [
        item for item in target["symbols"]
        if item["name"] == label_name and item["section"] > 0
    ]
    if len(target_labels) != 1:
        raise SemanticProgressError(
            f"expected one target local label {label_name}, found {len(target_labels)}")
    target_label = target_labels[0]
    if target_label["value"] <= 0:
        raise SemanticProgressError(
            f"target local label is not a continuation: {label_name}")

    target_owners = [
        item for item in target["symbols"]
        if item["name"] == owner_name and item["section"] > 0
    ]
    base_owners = [
        item for item in base["symbols"]
        if item["name"] == owner_name and item["section"] > 0
    ]
    if len(target_owners) != 1 or len(base_owners) != 1:
        raise SemanticProgressError(
            f"expected unique local-label owner {owner_name}, found "
            f"{len(target_owners)}/{len(base_owners)}")
    target_owner = target_owners[0]
    base_owner = base_owners[0]
    if target_owner["value"] != 0 or base_owner["value"] != 0:
        raise SemanticProgressError(
            f"local-label owner is not a COMDAT entry: {owner_name}")
    if target_label["section"] != target_owner["section"]:
        raise SemanticProgressError(
            f"target local label is outside owner {owner_name}: {label_name}")

    target_info = section_info(target, owner_name)
    base_info = section_info(base, owner_name)
    if not section_infos_equal(target_info, base_info):
        raise SemanticProgressError(
            f"local-label owner is no longer exact: {owner_name}")

    offset = target_label["value"]
    base_labels = [
        item for item in base["symbols"]
        if item["section"] == base_owner["section"]
        and item["value"] == offset
        and item["name"].startswith("$")
    ]
    if len(base_labels) != 1:
        raise SemanticProgressError(
            f"expected one base local destination at {offset:#x}, found {len(base_labels)}")
    if not any(
        relocation["target"] == ["internal", offset]
        for relocation in target_info["relocations"]
    ):
        raise SemanticProgressError(
            f"target local label has no proven internal relocation: {label_name}")

    return target_info


def _percent(numerator: int, denominator: int) -> float:
    return 100.0 * numerator / denominator if denominator else 0.0


def _credit(measures: Dict[str, Any], code_bytes: int) -> None:
    measures["matched_code"] = int(measures.get("matched_code", 0)) + code_bytes
    measures["matched_functions"] = int(measures.get("matched_functions", 0)) + 1
    measures["matched_code_percent"] = _percent(
        measures["matched_code"], int(measures.get("total_code", 0))
    )
    measures["matched_functions_percent"] = _percent(
        measures["matched_functions"], int(measures.get("total_functions", 0))
    )


def _debit(measures: Dict[str, Any], code_bytes: int) -> None:
    measures["matched_code"] = int(measures.get("matched_code", 0)) - code_bytes
    measures["matched_functions"] = int(measures.get("matched_functions", 0)) - 1
    if measures["matched_code"] < 0 or measures["matched_functions"] < 0:
        raise SemanticProgressError("semantic rejection would make progress negative")
    measures["matched_code_percent"] = _percent(
        measures["matched_code"], int(measures.get("total_code", 0))
    )
    measures["matched_functions_percent"] = _percent(
        measures["matched_functions"], int(measures.get("total_functions", 0))
    )


def _revoke_completion(
    measures: Dict[str, Any], code_bytes: int, data_bytes: int
) -> None:
    """Remove one previously complete unit from aggregate progress measures."""
    measures["complete_code"] = int(measures.get("complete_code", 0)) - code_bytes
    measures["complete_data"] = int(measures.get("complete_data", 0)) - data_bytes
    measures["complete_units"] = int(measures.get("complete_units", 0)) - 1
    if (
        measures["complete_code"] < 0
        or measures["complete_data"] < 0
        or measures["complete_units"] < 0
    ):
        raise SemanticProgressError("semantic rejection would make completion negative")
    measures["complete_code_percent"] = _percent(
        measures["complete_code"], int(measures.get("total_code", 0))
    )
    measures["complete_data_percent"] = _percent(
        measures["complete_data"], int(measures.get("total_data", 0))
    )


def _credit_data(measures: Dict[str, Any], data_bytes: int) -> None:
    measures["matched_data"] = int(measures.get("matched_data", 0)) + data_bytes
    total_data = int(measures.get("total_data", 0))
    if measures["matched_data"] > total_data:
        raise SemanticProgressError("semantic data credit exceeds total data")
    measures["matched_data_percent"] = _percent(
        measures["matched_data"], total_data)


def revoke_incomplete_units(report: Dict[str, Any]) -> List[str]:
    """Revoke config-level completion when measured content is incomplete.

    ``metadata.complete`` originates from the manually maintained Matching
    label.  It must not grant linked-object credit when the current report,
    after strict semantic corrections, still contains unmatched functions or
    data.  This is deliberately a one-way safety gate: it can revoke a stale
    label, but it never promotes an object to complete.
    """
    categories = {item["id"]: item for item in report.get("categories", [])}
    revoked = []

    for report_unit in report.get("units", []):
        if not report_unit.get("metadata", {}).get("complete", False):
            continue

        unit_measures = report_unit.get("measures", {})
        matched_functions = int(unit_measures.get("matched_functions", 0))
        total_functions = int(unit_measures.get("total_functions", 0))
        matched_data = int(unit_measures.get("matched_data", 0))
        total_data = int(unit_measures.get("total_data", 0))
        missing_functions = total_functions - matched_functions
        missing_data = total_data - matched_data
        if missing_functions < 0 or missing_data < 0:
            raise SemanticProgressError(
                f"unit progress exceeds totals: {report_unit.get('name', '<unknown>')}"
            )
        if missing_functions == 0 and missing_data == 0:
            continue

        if int(unit_measures.get("complete_units", 0)) != 1:
            raise SemanticProgressError(
                f"complete unit has inconsistent completion measures: "
                f"{report_unit.get('name', '<unknown>')}"
            )

        complete_code = int(unit_measures.get("complete_code", 0))
        complete_data = int(unit_measures.get("complete_data", 0))
        _revoke_completion(report["measures"], complete_code, complete_data)

        progress_categories = report_unit.get("metadata", {}).get(
            "progress_categories", []
        )
        if isinstance(progress_categories, str):
            progress_categories = [progress_categories]
        for category_id in progress_categories:
            if category_id not in categories:
                raise SemanticProgressError(
                    f"progress category not found: {category_id}"
                )
            _revoke_completion(
                categories[category_id]["measures"], complete_code, complete_data
            )

        unit_measures["complete_code"] = 0
        unit_measures["complete_data"] = 0
        unit_measures["complete_units"] = 0
        unit_measures["complete_code_percent"] = 0.0
        unit_measures["complete_data_percent"] = 0.0
        report_unit["metadata"]["complete"] = False
        revoked.append(
            f"{report_unit['name']} ({missing_functions} unmatched functions, "
            f"{missing_data} unmatched data bytes)"
        )

    return revoked


def _section_ownership_snapshot(obj: Dict[str, Any], section_name: str) -> Dict[str, Any]:
    sections = [section for section in obj["sections"] if section["name"] == section_name]
    if len(sections) != 1:
        raise SemanticProgressError(
            f"expected one {section_name} section, found {len(sections)}"
        )

    section = sections[0]
    info = section_info_by_number(obj, int(section["index"]))
    symbols = sorted(
        (
            {
                "name": symbol["name"],
                "value": int(symbol["value"]),
                "type": int(symbol["type"]),
                "storage": int(symbol["storage"]),
            }
            for symbol in obj["symbols"]
            if int(symbol["section"]) == int(section["index"])
            and symbol["name"] != section_name
        ),
        key=lambda symbol: (symbol["value"], symbol["name"]),
    )
    return {
        "size": int(section["size"]),
        "flags": int(section["flags"]),
        "relocation_count": int(info["relocation_count"]),
        "normalized_sha256": info["normalized_sha256"],
        "symbols": symbols,
    }


def require_symbol_ownership_snapshots(
    project_root: Path,
    manifest_path: Path,
    objdiff_config_path: Path,
) -> List[str]:
    """Require exact COFF section ownership for admission-sensitive data.

    Zero-filled BSS can compare byte-exact even when symbols move or change
    linkage.  This manifest records the complete named-symbol set for a
    section and validates both the csplit target and rebuilt object.  It is a
    pure safety gate: it grants no progress credit and any drift fails closed.
    """
    if not manifest_path.is_file():
        return []

    entries = json.loads(manifest_path.read_text(encoding="utf-8"))
    objdiff = json.loads(objdiff_config_path.read_text(encoding="utf-8"))
    config_units = {unit["name"]: unit for unit in objdiff.get("units", [])}
    validated = []

    for entry in entries:
        unit_name = entry["unit"]
        section_name = entry["section"]
        if unit_name not in config_units:
            raise SemanticProgressError(
                f"ownership snapshot unit not found: {unit_name}"
            )
        config_unit = config_units[unit_name]
        try:
            target = load(project_root / config_unit["target_path"])
            base = load(project_root / config_unit["base_path"])
            target_snapshot = _section_ownership_snapshot(target, section_name)
            base_snapshot = _section_ownership_snapshot(base, section_name)
        except (CoffError, KeyError, OSError) as error:
            raise SemanticProgressError(
                f"cannot verify ownership snapshot {unit_name}:{section_name}: {error}"
            ) from error

        expected = entry.get("snapshot", {})
        if target_snapshot != expected:
            raise SemanticProgressError(
                f"target ownership snapshot changed: {unit_name}:{section_name}"
            )
        if base_snapshot != expected:
            raise SemanticProgressError(
                f"rebuilt ownership snapshot changed: {unit_name}:{section_name}"
            )

        validated.append(
            f"{unit_name}:{section_name} ({len(expected.get('symbols', []))} symbols)"
        )

    return validated


def apply_semantic_rejections(
    report: Dict[str, Any],
    semantic_report_path: Path,
) -> List[str]:
    """Remove objdiff credits rejected by the stricter COFF-shape audit."""
    if not semantic_report_path.is_file():
        return []

    semantic_report = json.loads(semantic_report_path.read_text(encoding="utf-8"))
    report_units = {unit["name"]: unit for unit in report.get("units", [])}
    categories = {item["id"]: item for item in report.get("categories", [])}
    rejected = []
    revoked_units = set()

    for entry in semantic_report.get("ordinary_rejected", []):
        unit_name = entry["unit"]
        function_name = entry["function"]
        report_unit = report_units.get(unit_name)
        if report_unit is None:
            raise SemanticProgressError(f"semantic rejection unit not found: {unit_name}")
        functions = [
            function for function in report_unit.get("functions", [])
            if function.get("name") == function_name
        ]
        if len(functions) != 1 or functions[0].get("fuzzy_match_percent") != 100.0:
            raise SemanticProgressError(
                f"semantic rejection is not an objdiff exact function: "
                f"{unit_name}:{function_name}"
            )

        code_bytes = int(functions[0]["size"])
        _debit(report["measures"], code_bytes)
        _debit(report_unit["measures"], code_bytes)

        progress_categories = report_unit.get("metadata", {}).get(
            "progress_categories", []
        )
        if isinstance(progress_categories, str):
            progress_categories = [progress_categories]
        for category_id in progress_categories:
            if category_id not in categories:
                raise SemanticProgressError(f"progress category not found: {category_id}")
            _debit(categories[category_id]["measures"], code_bytes)

        if unit_name not in revoked_units and report_unit.get("metadata", {}).get(
            "complete", False
        ):
            unit_measures = report_unit["measures"]
            complete_code = int(unit_measures.get("complete_code", 0))
            complete_data = int(unit_measures.get("complete_data", 0))
            _revoke_completion(report["measures"], complete_code, complete_data)
            for category_id in progress_categories:
                _revoke_completion(
                    categories[category_id]["measures"], complete_code, complete_data
                )
            unit_measures["complete_code"] = 0
            unit_measures["complete_data"] = 0
            unit_measures["complete_units"] = 0
            unit_measures["complete_code_percent"] = 0.0
            unit_measures["complete_data_percent"] = 0.0
            report_unit["metadata"]["complete"] = False
            revoked_units.add(unit_name)

        rejected.append(
            f"{unit_name}:{function_name} (-{code_bytes} code bytes, -1 function)"
        )

    return rejected


def apply_semantic_matches(
    report: Dict[str, Any],
    project_root: Path,
    manifest_path: Path,
    objdiff_config_path: Path,
) -> List[str]:
    """Verify and credit manifest entries that objdiff did not count exactly.

    Returns human-readable notes for credited entries. Missing, ambiguous, or
    non-equal evidence raises instead of silently inflating progress.
    """
    if not manifest_path.is_file():
        return []

    entries = json.loads(manifest_path.read_text(encoding="utf-8"))
    objdiff = json.loads(objdiff_config_path.read_text(encoding="utf-8"))
    report_units = {unit["name"]: unit for unit in report.get("units", [])}
    config_units = {unit["name"]: unit for unit in objdiff.get("units", [])}
    categories = {item["id"]: item for item in report.get("categories", [])}
    credited = []

    for entry in entries:
        unit_name = entry["unit"]
        function_name = entry["function"]
        if unit_name not in report_units or unit_name not in config_units:
            raise SemanticProgressError(f"semantic match unit not found: {unit_name}")

        report_unit = report_units[unit_name]
        functions = [
            function for function in report_unit.get("functions", [])
            if function.get("name") == function_name
        ]
        if len(functions) != 1:
            raise SemanticProgressError(
                f"expected one report function {unit_name}:{function_name}, found {len(functions)}"
            )
        function = functions[0]
        if function.get("fuzzy_match_percent") == 100.0:
            continue

        config_unit = config_units[unit_name]
        try:
            target = load(project_root / config_unit["target_path"])
            base = load(project_root / config_unit["base_path"])
            owner_function = entry.get("owner_function")
            if owner_function:
                target_info = _verify_local_label_continuation(
                    target, base, function_name, owner_function)
                base_info = target_info
            else:
                target_info = section_info(target, function_name)
                # csplit may expose a source/SDK function under an anonymous
                # image name while VC7 emits its authentic public name.  An
                # explicit per-unit alias is safe only because the unchanged
                # strict comparator below still proves the complete function
                # shape, including relocation destinations and addends.
                base_info = section_info(
                    base, entry.get("base_function", function_name))
        except (CoffError, KeyError, OSError) as error:
            raise SemanticProgressError(
                f"cannot verify semantic match {unit_name}:{function_name}: {error}"
            ) from error
        if not section_infos_equal(target_info, base_info):
            raise SemanticProgressError(
                f"semantic match is no longer exact: {unit_name}:{function_name}"
            )

        code_bytes = int(function["size"])
        _credit(report["measures"], code_bytes)
        _credit(report_unit["measures"], code_bytes)
        function["fuzzy_match_percent"] = 100.0

        progress_categories = report_unit.get("metadata", {}).get("progress_categories", [])
        if isinstance(progress_categories, str):
            progress_categories = [progress_categories]
        for category_id in progress_categories:
            if category_id not in categories:
                raise SemanticProgressError(f"progress category not found: {category_id}")
            _credit(categories[category_id]["measures"], code_bytes)

        credited.append(
            f"{unit_name}:{function_name} (+{code_bytes} code bytes, +1 function)"
        )

    return credited


def apply_semantic_accepted_ledger(
    report: Dict[str, Any],
    semantic_report_path: Path,
) -> List[str]:
    """Credit additional functions proven by the generated COFF audit.

    ``audit_semantic_matches.py`` produces ``accepted_ledger`` from the live
    target and rebuilt objects.  This consumer deliberately admits only entries
    carrying the ``semantic-coff`` proof source.  Ordinary objdiff entries are
    already counted, compiler-local continuation labels are not promoted here,
    and any duplicate, missing, size-mismatched or rejected identity fails
    closed.  Explicit ``semantic_matches.json`` entries should be applied first;
    they mark their report function exact and are therefore not double-counted.
    """
    if not semantic_report_path.is_file():
        return []

    semantic_report = json.loads(
        semantic_report_path.read_text(encoding="utf-8"))
    entries = semantic_report.get("accepted_ledger")
    if not isinstance(entries, list):
        raise SemanticProgressError(
            "semantic report accepted_ledger must be a list")

    expected_count = semantic_report.get("summary", {}).get("accepted_exact")
    if expected_count is not None and int(expected_count) != len(entries):
        raise SemanticProgressError(
            "semantic report accepted ledger count does not match summary")

    rejected_keys = set()
    for entry in semantic_report.get("ordinary_rejected", []):
        if not isinstance(entry, dict):
            raise SemanticProgressError(
                "semantic report ordinary_rejected entry must be an object")
        unit_name = entry.get("unit")
        function_name = entry.get("function")
        if isinstance(unit_name, str) and isinstance(function_name, str):
            rejected_keys.add((unit_name, function_name))

    report_units = {unit["name"]: unit for unit in report.get("units", [])}
    categories = {item["id"]: item for item in report.get("categories", [])}
    seen = set()
    credited = []

    for entry in entries:
        if not isinstance(entry, dict):
            raise SemanticProgressError(
                "semantic report accepted ledger entry must be an object")
        unit_name = entry.get("unit")
        function_name = entry.get("function")
        if not isinstance(unit_name, str) or not isinstance(function_name, str):
            raise SemanticProgressError(
                "semantic report accepted ledger entry lacks an identity")
        key = (unit_name, function_name)
        if key in seen:
            raise SemanticProgressError(
                f"duplicate semantic accepted ledger entry: "
                f"{unit_name}:{function_name}")
        seen.add(key)
        if key in rejected_keys:
            raise SemanticProgressError(
                f"semantic accepted ledger overlaps rejection: "
                f"{unit_name}:{function_name}")

        proof_sources = entry.get("proof_sources")
        if not isinstance(proof_sources, list):
            raise SemanticProgressError(
                f"semantic accepted ledger lacks proof sources: "
                f"{unit_name}:{function_name}")
        if "semantic-coff" not in proof_sources:
            continue
        if function_name.startswith("$"):
            raise SemanticProgressError(
                f"semantic COFF ledger may not promote a local continuation: "
                f"{unit_name}:{function_name}")

        report_unit = report_units.get(unit_name)
        if report_unit is None:
            raise SemanticProgressError(
                f"semantic accepted ledger unit not found: {unit_name}")
        functions = [
            function for function in report_unit.get("functions", [])
            if function.get("name") == function_name
        ]
        if len(functions) != 1:
            raise SemanticProgressError(
                f"expected one report function {unit_name}:{function_name}, "
                f"found {len(functions)}")
        function = functions[0]
        try:
            code_bytes = int(entry["code_bytes"])
            report_bytes = int(function["size"])
        except (KeyError, TypeError, ValueError) as error:
            raise SemanticProgressError(
                f"invalid semantic accepted size for "
                f"{unit_name}:{function_name}") from error
        if code_bytes <= 0 or report_bytes != code_bytes:
            raise SemanticProgressError(
                f"semantic accepted size differs for "
                f"{unit_name}:{function_name}: {code_bytes}/{report_bytes}")
        if float(function.get("fuzzy_match_percent", 0.0)) == 100.0:
            continue

        _credit(report["measures"], code_bytes)
        _credit(report_unit["measures"], code_bytes)
        function["fuzzy_match_percent"] = 100.0

        progress_categories = report_unit.get(
            "metadata", {}).get("progress_categories", [])
        if isinstance(progress_categories, str):
            progress_categories = [progress_categories]
        for category_id in progress_categories:
            if category_id not in categories:
                raise SemanticProgressError(
                    f"semantic accepted category not found: {category_id}")
            _credit(categories[category_id]["measures"], code_bytes)

        credited.append(
            f"{unit_name}:{function_name} "
            f"(+{code_bytes} code bytes, +1 function)")

    return credited


def _unique_defined_symbol(obj, name, description):
    matches = [
        item for item in obj["symbols"]
        if item["name"] == name and item["section"] > 0
    ]
    if len(matches) != 1:
        raise CoffError(
            f"expected one {description} {name!r}, found {len(matches)}")
    return matches[0]


def _semantic_data_member_snapshot(owner, section, info):
    if int(section["flags"]) & IMAGE_SCN_CNT_CODE:
        raise CoffError(
            f"semantic data owner {owner['name']!r} names code")
    alignment_code = (int(section["flags"]) >> 20) & 0xF
    if alignment_code == 0:
        alignment = 1
    elif 1 <= alignment_code <= 14:
        alignment = 1 << (alignment_code - 1)
    else:
        raise CoffError(
            f"invalid COFF section alignment code {alignment_code}")
    padded_size = (int(section["size"]) + alignment - 1) & ~(alignment - 1)
    return {
        "section": section["name"],
        "size": info["size"],
        "padded_size": padded_size,
        "flags": int(section["flags"]),
        "relocation_count": info["relocation_count"],
        "normalized_sha256": info["normalized_sha256"],
        "owner": {
            "value": int(owner["value"]),
            "type": int(owner["type"]),
            "storage": int(owner["storage"]),
        },
    }


# Extent model for grouped semantic data entries.  A grouped entry without an
# ``extent_model`` field keeps the legacy per-member sum.  An entry that names
# the model is sized the way the frozen objdiff-cli 3.3.1 report sizes its data
# sections (measured in research/opus_data_verifier_20260925) and must pass
# every additional check below.  There is no default: only an entry that pins
# the model by name uses it.
OBJDIFF_331_COMBINED_EXTENT = "objdiff-3.3.1-combined"
SEMANTIC_DATA_EXTENT_MODELS = frozenset({OBJDIFF_331_COMBINED_EXTENT})

_IMAGE_SCN_CNT_INITIALIZED_DATA = 0x00000040
_IMAGE_SCN_LNK_COMDAT = 0x00001000
_IMAGE_SCN_MEM_DISCARDABLE = 0x02000000
_IMAGE_SCN_MEM_EXECUTE = 0x20000000
_IMAGE_SYM_CLASS_STATIC = 3
_IMAGE_COMDAT_SELECT_ANY = 2

_MODEL_ENTRY_KEYS = frozenset({
    "unit", "group", "extent_model", "allow_incomplete_unit", "reason",
    "members", "surplus"})
_MODEL_MEMBER_KEYS = frozenset({"symbol", "measurements"})
_MODEL_SURPLUS_KEYS = frozenset({"symbol", "provider", "measurements"})

# The extent model is a measured property of one scorer binary.  A model entry
# is verified only against a fresh report that this exact binary generates
# from the unit's current target and rebuilt objects (see
# _require_rescored_report).  Upgrading the scorer therefore fails every model
# entry closed until the model is re-measured and reviewed.
PINNED_EXTENT_SCORERS = {
    OBJDIFF_331_COMBINED_EXTENT:
        "090987aa22c0fe9b7d252b2b44c2c0c92c5dd3e9b5965d353060802226a13677",
}
DEFAULT_SCORER_PATH = Path("build") / "tools" / "objdiff-cli.exe"
_IMAGE_SYM_CLASS_EXTERNAL = 2
# A model member's COMDAT selection must be none or select-any: the other
# selections depend on auxiliary fields (checksum, associated section) that the
# executable split does not reproduce and the member checks do not compare.
_MODEL_ALLOWED_SELECTIONS = frozenset({None, 0, _IMAGE_COMDAT_SELECT_ANY})


def _objdiff_data_section(section):
    """Return whether objdiff 3.3.1 reports a COFF section as data or bss."""
    flags = int(section["flags"])
    if flags & (IMAGE_SCN_CNT_CODE | _IMAGE_SCN_MEM_EXECUTE):
        return False
    if flags & _IMAGE_SCN_CNT_INITIALIZED_DATA:
        return not flags & _IMAGE_SCN_MEM_DISCARDABLE
    return bool(flags & IMAGE_SCN_CNT_UNINITIALIZED_DATA)


def _objdiff_base_name(section):
    """Return the name objdiff folds a data section under (before ``$``)."""
    name = section["name"]
    if name.startswith("/"):
        raise SemanticProgressError(
            f"unsupported long COFF data section name {name}")
    if "\x00" in name:
        # The loader keeps bytes after an embedded NUL; objdiff (the object
        # crate) stops at it, so the two would group the section differently.
        raise SemanticProgressError(
            f"unsupported COFF data section name with an embedded NUL "
            f"{name!r}")
    return name[:name.rfind("$")] if "$" in name else name


def _objdiff_report_data_groups(obj):
    """Group data sections under the report section objdiff 3.3.1 names.

    Sections whose names agree before the last ``$`` share a report section.
    A lone section keeps its full name; two or more report the base name.
    """
    groups = {}
    for section in obj["sections"]:
        if _objdiff_data_section(section):
            groups.setdefault(_objdiff_base_name(section), []).append(section)
    report_groups = {}
    for base_name, sections in groups.items():
        report_name = sections[0]["name"] if len(sections) == 1 else base_name
        if report_name in report_groups:
            raise SemanticProgressError(
                f"ambiguous objdiff data section name {report_name}")
        report_groups[report_name] = sections
    return report_groups


def _objdiff_report_extent(sections):
    """Return objdiff 3.3.1's reported size of one report data section.

    A lone section reports its raw size.  Otherwise the sections are ordered
    ``$`` names first, then by name (stable, so section-table order among
    equal names); each size is added and the running offset is aligned to
    max(alignment, 4) after every section, the last included.  Alignment
    code 0 is 16 bytes (the object crate's COFF default); codes above 14 are
    rejected rather than guessed.
    """
    if len(sections) == 1:
        return int(sections[0]["size"])
    offset = 0
    for section in sorted(
            sections, key=lambda item: ("$" not in item["name"], item["name"])):
        code = (int(section["flags"]) >> 20) & 0xF
        if code > 14:
            raise SemanticProgressError(
                f"invalid COFF section alignment code {code} in section "
                f"{section['index']}")
        alignment = max(1 << (code - 1) if code else 16, 4)
        offset = (offset + int(section["size"]) + alignment - 1) \
            & ~(alignment - 1)
    return offset


def _objdiff_group_coverage(target, member_numbers, label):
    """Return the target report groups a grouped entry touches.

    Complete coverage: every target section that objdiff folds into a report
    section the entry touches must be a member, and every member must be such
    a section.  Section identity is compared, never summed sizes, so a subset
    of members whose sizes happen to add up to the report extent is rejected.
    """
    touched = {}
    covered = set()
    for report_name, sections in _objdiff_report_data_groups(target).items():
        numbers = {int(section["index"]) for section in sections}
        if not numbers & member_numbers:
            continue
        missing = sorted(numbers - member_numbers)
        if missing:
            raise SemanticProgressError(
                f"semantic data group does not cover every target "
                f"{report_name} section: {label} (missing {missing[:8]})")
        touched[report_name] = sections
        covered |= numbers
    stray = sorted(member_numbers - covered)
    if stray:
        raise SemanticProgressError(
            f"semantic data group member is not an objdiff data section: "
            f"{label} (sections {stray[:8]})")
    return touched


def _report_count(value, description):
    """Parse an objdiff byte count (a digit string or a non-negative int)."""
    if isinstance(value, str) and value.isascii() and value.isdigit():
        return int(value)
    if isinstance(value, int) and not isinstance(value, bool) and value >= 0:
        return value
    raise SemanticProgressError(f"malformed report {description}: {value!r}")


def _unit_unmatched_data(report_unit, unit_name):
    measures = report_unit.get("measures", {})
    total_data = _report_count(
        measures.get("total_data", 0), f"total_data for {unit_name}")
    matched_data = _report_count(
        measures.get("matched_data", 0), f"matched_data for {unit_name}")
    if matched_data > total_data:
        raise SemanticProgressError(
            f"malformed report data totals for {unit_name}: "
            f"{matched_data}/{total_data}")
    return total_data - matched_data


def _require_report_binding(target, report_unit, unit_name, label):
    """Bind the report's unit to this target object.

    Every data section objdiff would report for the target must appear in the
    report at its modelled size, and the unit's total_data must be their sum.
    A report produced from a different object, a different scorer or a
    mis-sized section fails closed.
    """
    report_sizes = {}
    for report_section in report_unit.get("sections", []):
        name = report_section.get("name")
        if name in report_sizes:
            raise SemanticProgressError(
                f"duplicate report section {name}: {unit_name}:{label}")
        report_sizes[name] = _report_count(
            report_section.get("size", 0), f"size of {unit_name} {name}")
    modelled_total = 0
    for report_name, sections in _objdiff_report_data_groups(target).items():
        extent = _objdiff_report_extent(sections)
        if report_sizes.get(report_name) != extent:
            raise SemanticProgressError(
                f"report section {report_name} is not the target's modelled "
                f"extent {extent}: {unit_name}:{label} "
                f"(report {report_sizes.get(report_name)!r})")
        modelled_total += extent
    total_data = _report_count(
        report_unit.get("measures", {}).get("total_data", 0),
        f"total_data for {unit_name}")
    if total_data != modelled_total:
        raise SemanticProgressError(
            f"report total_data is not the target's modelled data extent: "
            f"{unit_name}:{label} ({total_data} != {modelled_total})")


def rescore_unit_with_pinned_scorer(project_root, objdiff_config, config_unit,
                                    extent_model, scorer_path=None):
    """Score one unit's current objects with the scorer pinned to the model.

    The scorer binary must exist and hash to the value pinned for the extent
    model.  It is run on a single-unit project whose target and base paths are
    the unit's own objects, and exactly that one unit must come back.  Any
    missing prerequisite or scorer failure fails closed.
    """
    pinned = PINNED_EXTENT_SCORERS.get(extent_model)
    if pinned is None:
        raise SemanticProgressError(
            f"no scorer is pinned for extent model {extent_model!r}")
    scorer = Path(scorer_path) if scorer_path is not None \
        else Path(project_root) / DEFAULT_SCORER_PATH
    if not scorer.is_file():
        raise SemanticProgressError(f"pinned scorer is missing: {scorer}")
    digest = hashlib.sha256(scorer.read_bytes()).hexdigest()
    if digest != pinned:
        raise SemanticProgressError(
            f"scorer {scorer} is not the binary pinned for {extent_model} "
            f"(sha256 {digest})")
    unit = copy.deepcopy(config_unit)
    for key in ("target_path", "base_path"):
        path = (Path(project_root) / unit[key]).resolve()
        if not path.is_file():
            raise SemanticProgressError(
                f"cannot rescore {unit['name']}: {key} is missing: {path}")
        unit[key] = str(path)
    project = {
        "min_version": objdiff_config.get("min_version"),
        "progress_categories": copy.deepcopy(
            objdiff_config.get("progress_categories", [])),
        "units": [unit],
    }
    with tempfile.TemporaryDirectory(prefix="semantic_rescore_") as work:
        work_path = Path(work)
        (work_path / "objdiff.json").write_text(
            json.dumps(project), encoding="utf-8")
        output = work_path / "report.json"
        try:
            process = subprocess.run(
                [str(scorer), "report", "generate", "-p", str(work_path),
                 "-o", str(output)],
                capture_output=True, text=True, timeout=600, check=False)
        except (OSError, subprocess.SubprocessError) as error:
            raise SemanticProgressError(
                f"pinned scorer could not run for {unit['name']}: {error}"
            ) from error
        if process.returncode or not output.is_file():
            raise SemanticProgressError(
                f"pinned scorer failed for {unit['name']} "
                f"(exit {process.returncode}): "
                f"{(process.stdout + process.stderr)[-400:]}")
        fresh = json.loads(output.read_text(encoding="utf-8"))
    units = fresh.get("units", [])
    if len(units) != 1 or units[0].get("name") != unit["name"]:
        raise SemanticProgressError(
            f"pinned scorer did not report exactly {unit['name']}")
    return units[0]


def _report_data_view(report_unit, unit_name):
    """The parts of a report unit that data verification relies on.

    Earlier progress stages may correct function rows and code measures; no
    stage before this one changes the section rows or the data measures.
    """
    measures = report_unit.get("measures", {})
    return {
        "name": report_unit.get("name"),
        "sections": copy.deepcopy(report_unit.get("sections", [])),
        "total_data": _report_count(
            measures.get("total_data", 0), f"total_data for {unit_name}"),
        "matched_data": _report_count(
            measures.get("matched_data", 0), f"matched_data for {unit_name}"),
    }


def _require_rescored_report(project_root, objdiff_config, config_unit,
                             extent_model, report_unit, unit_name, label,
                             rescore):
    """Bind the live report unit to the actual objects and the pinned scorer.

    The unit is re-scored from its current target and rebuilt objects by the
    scorer pinned to the extent model; the live report's section rows and data
    measures must equal that fresh report.  A stale report, one generated from
    other objects or for another unit, or one produced by another scorer fails
    closed.
    """
    fresh = rescore(project_root, objdiff_config, config_unit, extent_model)
    if _report_data_view(fresh, unit_name) \
            != _report_data_view(report_unit, unit_name):
        raise SemanticProgressError(
            f"report is not the pinned scorer's report of the current "
            f"objects (stale, other unit or other scorer): "
            f"{unit_name}:{label}")


def _defined_section_symbols(obj, section_number):
    """Every symbol defined in one section: name, offset, type, storage."""
    return sorted(
        (item["name"], int(item["value"]), int(item["type"]),
         int(item["storage"]))
        for item in obj["symbols"]
        if int(item["section"]) == section_number)


def _comdat_selection(obj, section_number):
    """Return the section-definition COMDAT selection (None if absent).

    The loader skips auxiliary records, so the section symbol's auxiliary
    record is read from the raw symbol table.  More than one definition
    symbol for a section is ambiguous and fails closed.
    """
    data = obj["data"]
    symbol_offset = struct.unpack_from("<L", data, 8)[0]
    section_name = obj["sections"][section_number - 1]["name"]
    selections = []
    for item in obj["symbols"]:
        if int(item["section"]) != section_number \
                or item["name"] != section_name \
                or int(item["storage"]) != _IMAGE_SYM_CLASS_STATIC \
                or int(item["value"]) != 0:
            continue
        entry = symbol_offset + item["index"] * SYMBOL_ENTRY_SIZE
        if data[entry + 17] < 1:
            continue
        auxiliary = entry + SYMBOL_ENTRY_SIZE
        if auxiliary + SYMBOL_ENTRY_SIZE > len(data):
            raise CoffError(
                f"section {section_number} definition record is truncated")
        selections.append(data[auxiliary + 14])
    if len(selections) > 1:
        raise CoffError(
            f"section {section_number} has {len(selections)} definition "
            f"symbols")
    return selections[0] if selections else None


def _comdat_key(obj, section_number):
    """The COMDAT key symbol: the first symbol after the section definition."""
    section_name = obj["sections"][section_number - 1]["name"]
    definition_seen = False
    for item in obj["symbols"]:
        if int(item["section"]) != section_number:
            continue
        if not definition_seen:
            definition_seen = (
                item["name"] == section_name
                and int(item["storage"]) == _IMAGE_SYM_CLASS_STATIC
                and int(item["value"]) == 0)
            continue
        return item["name"]
    return None


def _linkage_class(item):
    """Where a relocation target is defined (or imported) and its storage."""
    number = int(item["section"])
    where = ("undefined" if number == 0 else "absolute" if number == -1
             else "debug" if number == -2 else "defined")
    return where, int(item["storage"])


def _relocation_linkage(obj, section_number):
    """(address, linkage class, target section) of every relocation, in order."""
    section = obj["sections"][section_number - 1]
    rows = []
    for index in range(section["reloc_count"]):
        address, symbol_index, _ = struct.unpack_from(
            "<LLH", obj["data"], section["reloc"] + index * 10)
        target = obj["by_index"][symbol_index]
        rows.append((address, _linkage_class(target), int(target["section"])))
    return rows


def _require_decodable_names(obj, side, unit_name):
    """The loader decodes names with 'replace', so two different byte names
    can decode alike: any U+FFFD in a symbol or section name fails closed."""
    for item in list(obj["symbols"]) + list(obj["sections"]):
        if "\ufffd" in item["name"]:
            raise SemanticProgressError(
                f"undecodable COFF name in the {side} object: {unit_name}")


def _verify_model_member(target, base, target_owner, base_owner, symbol_name,
                         symbol_addresses, unit_name):
    """Checks a pinned-model member adds to the shared member path.

    - the owner is the same symbol in both objects (offset, type, storage),
      not merely the same as its pinned snapshot;
    - the complete symbol table of the section is identical, so no symbol is
      added, removed, renamed, moved or given another storage class;
    - the COMDAT selection of the section definition is identical;
    - every relocation resolves to a final image address through the
      independently recovered symbol map, and the resolved lists are equal.
    """
    label = f"{unit_name}:{symbol_name}"
    for key in ("value", "type", "storage"):
        if int(target_owner[key]) != int(base_owner[key]):
            raise SemanticProgressError(
                f"semantic data group member owner differs: {label} ({key})")
    if _defined_section_symbols(target, target_owner["section"]) \
            != _defined_section_symbols(base, base_owner["section"]):
        raise SemanticProgressError(
            f"semantic data group member symbol table differs: {label}")
    try:
        target_selection = _comdat_selection(target, target_owner["section"])
        base_selection = _comdat_selection(base, base_owner["section"])
        target_resolved = section_info_resolved(
            target, symbol_name, symbol_addresses)
        base_resolved = section_info_resolved(
            base, symbol_name, symbol_addresses)
    except (CoffError, KeyError, OSError) as error:
        raise SemanticProgressError(
            f"cannot resolve semantic data group member {label}: {error}"
        ) from error
    if target_selection != base_selection:
        raise SemanticProgressError(
            f"semantic data group member COMDAT selection differs: {label}")
    for resolved in (target_resolved, base_resolved):
        for relocation in resolved["relocations"]:
            if relocation["target"][0] != "address":
                raise SemanticProgressError(
                    f"semantic data group member relocation has no image "
                    f"address: {label} at {relocation['address']:#x}")
    if target_resolved != base_resolved:
        raise SemanticProgressError(
            f"semantic data group member resolved relocations differ: {label}")
    if target_selection not in _MODEL_ALLOWED_SELECTIONS:
        raise SemanticProgressError(
            f"semantic data group member COMDAT selection is not supported: "
            f"{label} ({target_selection})")
    target_section = target["sections"][target_owner["section"] - 1]
    if int(target_section["flags"]) & _IMAGE_SCN_LNK_COMDAT \
            and _comdat_key(target, target_owner["section"]) \
            != _comdat_key(base, base_owner["section"]):
        raise SemanticProgressError(
            f"semantic data group member COMDAT key symbol differs: {label}")
    # Relocation targets are otherwise compared by name and address only: a
    # target January imports but ours defines locally (or the reverse, or a
    # static/external flip) must also match.  Differences are returned; the
    # caller allows only a January import matched to our external definition
    # inside a verified, provider-proved surplus section.
    differing = []
    for (address, target_class, _), (_, base_class, base_section) in zip(
            _relocation_linkage(target, target_owner["section"]),
            _relocation_linkage(base, base_owner["section"])):
        if target_class != base_class:
            differing.append((symbol_name, address, target_class, base_class,
                              base_section))
    return differing


def _select_any_definition(obj, symbol_name, description):
    """A single-symbol select-any COMDAT definition: (section, info, selection)."""
    try:
        owner = _unique_defined_symbol(obj, symbol_name, description)
        section = obj["sections"][owner["section"] - 1]
        info = section_info_by_number(obj, owner["section"])
        selection = _comdat_selection(obj, owner["section"])
    except (CoffError, KeyError, IndexError) as error:
        raise SemanticProgressError(
            f"cannot read {description} {symbol_name!r}: {error}") from error
    if int(owner["storage"]) != _IMAGE_SYM_CLASS_EXTERNAL \
            or int(owner["value"]) != 0:
        raise SemanticProgressError(
            f"{description} {symbol_name!r} is not an external at offset 0")
    if not int(section["flags"]) & _IMAGE_SCN_LNK_COMDAT \
            or selection != _IMAGE_COMDAT_SELECT_ANY:
        raise SemanticProgressError(
            f"{description} {symbol_name!r} is not a select-any COMDAT")
    expected = sorted([
        (section["name"], 0, 0, _IMAGE_SYM_CLASS_STATIC),
        (symbol_name, 0, int(owner["type"]), int(owner["storage"]))])
    if _defined_section_symbols(obj, owner["section"]) != expected:
        raise SemanticProgressError(
            f"{description} {symbol_name!r} shares its section")
    return section, info, selection


def _january_definers(project_root, config_units, symbol_names):
    """Map each name to the units whose January (target) object defines it."""
    names = {name: name.encode("latin-1") for name in symbol_names}
    definers = {name: [] for name in names}
    for unit_name, config_unit in sorted(config_units.items()):
        target_path = config_unit.get("target_path")
        if not target_path:
            continue
        path = Path(project_root) / target_path
        if not path.is_file():
            raise SemanticProgressError(
                f"cannot establish surplus providers: January object missing "
                f"for {unit_name}: {path}")
        data = path.read_bytes()
        present = [name for name, raw in names.items() if raw in data]
        if not present:
            continue
        try:
            obj = load(data)
        except CoffError as error:
            raise SemanticProgressError(
                f"cannot read January object for {unit_name}: {error}"
            ) from error
        for name in present:
            if any(item["name"] == name and int(item["section"]) > 0
                   for item in obj["symbols"]):
                definers[name].append(unit_name)
    return definers


def _verify_surplus_providers(project_root, config_units, target, base,
                              unit_name, surplus):
    """Prove every surplus literal is a folded copy of one selected provider.

    For each item: January's object for this unit references the symbol only
    as an undefined external; exactly one January object defines it, and that
    is the pinned provider unit; the provider's January definition and its
    current rebuilt definition are single-symbol select-any COMDATs identical
    to this unit's rebuilt copy (strict comparator, flags and selection).  The
    copies can therefore only fold into the provider's, and the surplus bytes
    are never credited here.
    """
    if not surplus:
        return
    definers = _january_definers(
        project_root, config_units, [item["symbol"] for item in surplus])
    for item in surplus:
        symbol_name = item["symbol"]
        provider = item["provider"]
        item_label = f"{unit_name}:{symbol_name}"
        references = [entry for entry in target["symbols"]
                      if entry["name"] == symbol_name]
        if not references or any(
                int(entry["section"]) != 0
                or int(entry["storage"]) != _IMAGE_SYM_CLASS_EXTERNAL
                for entry in references):
            raise SemanticProgressError(
                f"semantic data surplus is not a January undefined "
                f"reference of this unit: {item_label}")
        if not isinstance(provider, str) or provider == unit_name \
                or provider not in config_units:
            raise SemanticProgressError(
                f"semantic data surplus names an invalid provider "
                f"{provider!r}: {item_label}")
        if definers.get(symbol_name) != [provider]:
            raise SemanticProgressError(
                f"semantic data surplus provider is not January's unique "
                f"definer: {item_label} (definers "
                f"{definers.get(symbol_name)})")
        provider_config = config_units[provider]
        try:
            january_provider = load(
                Path(project_root) / provider_config["target_path"])
            rebuilt_provider = load(
                Path(project_root) / provider_config["base_path"])
        except (CoffError, KeyError, OSError) as error:
            raise SemanticProgressError(
                f"cannot read surplus provider {provider}: {error}"
            ) from error
        ours = _select_any_definition(
            base, symbol_name, "rebuilt surplus copy")
        for obj, description in (
                (january_provider, f"January provider {provider}"),
                (rebuilt_provider, f"rebuilt provider {provider}")):
            theirs = _select_any_definition(obj, symbol_name, description)
            if int(theirs[0]["flags"]) != int(ours[0]["flags"]) \
                    or theirs[0]["size"] != ours[0]["size"] \
                    or not section_infos_equal(theirs[1], ours[1]):
                raise SemanticProgressError(
                    f"semantic data surplus differs from {description}: "
                    f"{item_label}")


def _verify_declared_surplus(target, base, touched, base_member_numbers,
                             surplus, unit_name, label):
    """Require every rebuilt-only section in a touched group to be declared.

    A section the rebuilt object adds to a report section the entry covers
    (for example a string literal the linker folded out of January's object)
    earns no credit.  It must still be listed with a pinned snapshot, be a
    select-any COMDAT holding exactly one symbol, and not be defined by the
    target.  Anything undeclared fails closed.
    """
    if surplus is None:
        surplus = []
    if not isinstance(surplus, list):
        raise SemanticProgressError(
            f"semantic data surplus must be a list: {unit_name}:{label}")
    touched_names = {
        _objdiff_base_name(section)
        for sections in touched.values() for section in sections}
    surplus_numbers = set()
    for item in surplus:
        if not isinstance(item, dict) or set(item) != _MODEL_SURPLUS_KEYS \
                or not isinstance(item["symbol"], str) \
                or not isinstance(item["provider"], str):
            raise SemanticProgressError(
                f"malformed semantic data surplus item: {unit_name}:{label}")
        symbol_name = item["symbol"]
        item_label = f"{unit_name}:{symbol_name}"
        if any(entry["name"] == symbol_name and int(entry["section"]) > 0
               for entry in target["symbols"]):
            raise SemanticProgressError(
                f"semantic data surplus is defined by the target: {item_label}")
        try:
            owner = _unique_defined_symbol(
                base, symbol_name, "semantic data surplus owner")
            section = base["sections"][owner["section"] - 1]
            info = section_info(base, symbol_name)
            snapshot = _semantic_data_member_snapshot(owner, section, info)
            selection = _comdat_selection(base, owner["section"])
        except (CoffError, KeyError, OSError) as error:
            raise SemanticProgressError(
                f"cannot verify semantic data surplus {item_label}: {error}"
            ) from error
        number = int(owner["section"])
        if number in surplus_numbers or number in base_member_numbers:
            raise SemanticProgressError(
                f"semantic data surplus repeats a section: {item_label}")
        surplus_numbers.add(number)
        if not _objdiff_data_section(section) \
                or _objdiff_base_name(section) not in touched_names:
            raise SemanticProgressError(
                f"semantic data surplus is outside the covered report "
                f"sections: {item_label}")
        if not int(section["flags"]) & _IMAGE_SCN_LNK_COMDAT \
                or selection != _IMAGE_COMDAT_SELECT_ANY:
            raise SemanticProgressError(
                f"semantic data surplus is not a select-any COMDAT: "
                f"{item_label}")
        expected_symbols = sorted([
            (section["name"], 0, 0, _IMAGE_SYM_CLASS_STATIC),
            (symbol_name, 0, int(owner["type"]), int(owner["storage"]))])
        if int(owner["value"]) != 0 \
                or _defined_section_symbols(base, number) != expected_symbols:
            raise SemanticProgressError(
                f"semantic data surplus section holds other symbols: "
                f"{item_label}")
        if item["measurements"] != {"base": snapshot}:
            raise SemanticProgressError(
                f"semantic data surplus snapshot changed: {item_label}")

    undeclared = []
    for section in base["sections"]:
        number = int(section["index"])
        if not _objdiff_data_section(section) \
                or _objdiff_base_name(section) not in touched_names \
                or number in base_member_numbers \
                or number in surplus_numbers:
            continue
        undeclared.append(number)
    if undeclared:
        names = [
            item["name"] for item in base["symbols"]
            if int(item["section"]) in undeclared
            and item["name"] != base["sections"][int(item["section"]) - 1]["name"]]
        raise SemanticProgressError(
            f"semantic data group leaves rebuilt sections undeclared: "
            f"{unit_name}:{label} (sections {undeclared[:8]}, "
            f"symbols {names[:4]})")
    return surplus_numbers


def apply_semantic_data_matches(
    report: Dict[str, Any],
    project_root: Path,
    manifest_path: Path,
    objdiff_config_path: Path,
    symbol_manifest_path: Path,
    rescore_report_unit: Optional[Callable[..., Dict[str, Any]]] = None,
) -> List[str]:
    """Verify and credit executable-split data relocation aliases.

    A manifest entry is accepted only when the target and rebuilt data
    sections have identical normalized bytes, relocation locations/types,
    and independently resolved destinations.  A grouped entry additionally
    snapshots every member's flags, alignment-derived padded extent, and
    producer-specific owner.  Its members must be every target section of
    each report section they touch (complete coverage, never a sum of sizes)
    and must account for the unit's entire remaining unmatched data.  An
    incomplete unit requires an explicit manifest opt-in so partial spans are
    never credited accidentally.  A group pinned to
    ``extent_model: objdiff-3.3.1-combined`` is sized the way that scorer
    sizes report sections, binds the report to the target object, requires
    identical owners, section symbol tables, COMDAT selections and
    image-resolved relocations, declares every rebuilt-only section it
    touches, and is a zero-credit no-op once the report already matches all
    of the unit's data.  A model entry is also verified against a fresh report
    of its unit's current objects from the scorer pinned to the model
    (``rescore_report_unit`` substitutes that scorer, for fixtures only), and
    each declared surplus section must be a folded copy of its January
    provider's select-any definition.
    """
    rescore = rescore_report_unit or rescore_unit_with_pinned_scorer
    if not manifest_path.is_file():
        return []

    entries = json.loads(manifest_path.read_text(encoding="utf-8"))
    objdiff = json.loads(objdiff_config_path.read_text(encoding="utf-8"))
    symbol_entries = json.loads(symbol_manifest_path.read_text(encoding="utf-8"))
    symbol_addresses = image_symbol_addresses(symbol_entries)
    # A malformed manifest fails with this module's error, never a crash.
    if not isinstance(entries, list) or not all(
            isinstance(entry, dict) and isinstance(entry.get("unit"), str)
            for entry in entries):
        raise SemanticProgressError(
            "semantic data manifest must be a list of entries naming a unit")

    report_units = {unit["name"]: unit for unit in report.get("units", [])}
    config_units = {unit["name"]: unit for unit in objdiff.get("units", [])}
    categories = {item["id"]: item for item in report.get("categories", [])}
    unit_entry_counts = Counter(entry.get("unit") for entry in entries)
    credited = []

    for entry in entries:
        unit_name = entry["unit"]
        if unit_name not in report_units or unit_name not in config_units:
            raise SemanticProgressError(
                f"semantic data match unit not found: {unit_name}")
        report_unit = report_units[unit_name]
        config_unit = config_units[unit_name]
        if (
            not config_unit.get("metadata", {}).get("complete")
            and not entry.get("allow_incomplete_unit", False)
        ):
            raise SemanticProgressError(
                f"semantic data unit is not marked complete: {unit_name}")

        members = entry.get("members")
        extent_model = entry.get("extent_model")
        # Only an ABSENT key selects the legacy path.  A present key must be a
        # known model string; an explicit JSON null or any non-string value is
        # rejected, never read as legacy.
        if "extent_model" in entry and not isinstance(extent_model, str):
            raise SemanticProgressError(
                f"semantic data extent_model must be a string when present: "
                f"{unit_name}: {extent_model!r}")
        if extent_model is not None and not members:
            raise SemanticProgressError(
                f"semantic data extent_model needs a grouped entry: "
                f"{unit_name}")
        if "surplus" in entry and extent_model is None:
            raise SemanticProgressError(
                f"semantic data surplus needs an extent_model entry: "
                f"{unit_name}")
        if members:
            section_label = entry.get("group", "data-section-group")
            if extent_model is not None:
                if extent_model not in SEMANTIC_DATA_EXTENT_MODELS:
                    raise SemanticProgressError(
                        f"unknown semantic data extent model "
                        f"{extent_model!r}: {unit_name}:{section_label}")
                if entry.get("credit_raw_size", False):
                    raise SemanticProgressError(
                        f"semantic data extent model conflicts with "
                        f"credit_raw_size: {unit_name}:{section_label}")
                unknown = sorted(set(entry) - _MODEL_ENTRY_KEYS)
                if unknown:
                    raise SemanticProgressError(
                        f"unknown semantic data entry keys {unknown}: "
                        f"{unit_name}:{section_label}")
                if "group" not in entry:
                    # A model entry names its group explicitly; the legacy
                    # default label is not an identity.
                    raise SemanticProgressError(
                        f"semantic data extent-model entry needs a group: "
                        f"{unit_name}")
                if not isinstance(members, list) \
                        or not isinstance(section_label, str) \
                        or not section_label \
                        or not isinstance(entry.get("reason"), str) \
                        or not entry["reason"].strip() \
                        or not isinstance(
                            entry.get("allow_incomplete_unit", False), bool):
                    raise SemanticProgressError(
                        f"malformed semantic data extent-model entry: "
                        f"{unit_name}:{section_label}")
                if unit_entry_counts[unit_name] != 1:
                    raise SemanticProgressError(
                        f"semantic data unit has more than one entry: "
                        f"{unit_name}")
                _require_rescored_report(
                    project_root, objdiff, config_unit, extent_model,
                    report_unit, unit_name, section_label, rescore)
            credit_size_key = (
                "size" if entry.get("credit_raw_size", False)
                else "padded_size")
            target = load(project_root / config_unit["target_path"])
            base = load(project_root / config_unit["base_path"])
            if extent_model is not None:
                _require_decodable_names(target, "target", unit_name)
                _require_decodable_names(base, "rebuilt", unit_name)
            linkage_differences = []
            credited_size = 0
            target_section_numbers = set()
            base_section_numbers = set()
            grouped_sections = {}

            for member in members:
                if extent_model is not None and (
                        not isinstance(member, dict)
                        or set(member) != _MODEL_MEMBER_KEYS
                        or not isinstance(member["symbol"], str)):
                    raise SemanticProgressError(
                        f"malformed semantic data extent-model member: "
                        f"{unit_name}:{section_label}")
                target_symbol = member["symbol"]
                base_symbol = member.get("base_symbol", target_symbol)
                source_function = member.get("source_function")
                base_source_function = member.get(
                    "base_source_function", source_function)

                try:
                    if source_function:
                        target_info = section_info_source_relative(
                            target, target_symbol, source_function)
                        base_info = section_info_source_relative(
                            base, base_symbol, base_source_function)
                    else:
                        target_info = section_info(target, target_symbol)
                        base_info = section_info(base, base_symbol)

                    target_owner = _unique_defined_symbol(
                        target, target_symbol, "semantic data target owner")
                    base_owner = _unique_defined_symbol(
                        base, base_symbol, "semantic data base owner")
                    target_section = target["sections"][
                        target_owner["section"] - 1]
                    base_section = base["sections"][base_owner["section"] - 1]
                    target_snapshot = _semantic_data_member_snapshot(
                        target_owner, target_section, target_info)
                    base_snapshot = _semantic_data_member_snapshot(
                        base_owner, base_section, base_info)
                except (CoffError, KeyError, OSError) as error:
                    raise SemanticProgressError(
                        f"cannot verify semantic data group member "
                        f"{unit_name}:{target_symbol}: {error}") from error

                if target_owner["section"] in target_section_numbers \
                        or base_owner["section"] in base_section_numbers:
                    raise SemanticProgressError(
                        f"semantic data group repeats a section: "
                        f"{unit_name}:{target_symbol}")
                target_section_numbers.add(target_owner["section"])
                base_section_numbers.add(base_owner["section"])

                if not section_infos_equal(target_info, base_info):
                    raise SemanticProgressError(
                        f"semantic data group member is no longer exact: "
                        f"{unit_name}:{target_symbol}")

                for key in ("section", "size", "padded_size", "flags"):
                    if target_snapshot[key] != base_snapshot[key]:
                        raise SemanticProgressError(
                            f"semantic data group member layout differs: "
                            f"{unit_name}:{target_symbol} ({key})")

                expected = member.get("measurements", {})
                snapshot = {
                    "target": target_snapshot,
                    "base": base_snapshot,
                }
                if expected != snapshot:
                    raise SemanticProgressError(
                        f"semantic data group member snapshot changed: "
                        f"{unit_name}:{target_symbol}")
                if extent_model is not None:
                    linkage_differences.extend(_verify_model_member(
                        target, base, target_owner, base_owner, target_symbol,
                        symbol_addresses, unit_name))
                credited_size += target_snapshot[credit_size_key]
                grouped_sections[target_snapshot["section"]] = (
                    grouped_sections.get(target_snapshot["section"], 0)
                    + target_snapshot[credit_size_key])

            touched = _objdiff_group_coverage(
                target, target_section_numbers,
                f"{unit_name}:{section_label}")
            if extent_model == OBJDIFF_331_COMBINED_EXTENT:
                grouped_sections = {
                    report_name: _objdiff_report_extent(sections)
                    for report_name, sections in touched.items()}
                credited_size = sum(grouped_sections.values())

            unmatched_sections = {}
            for report_section in report_unit.get("sections", []):
                if report_section.get("name") == ".text" \
                        or float(report_section.get(
                            "fuzzy_match_percent", 0.0)) == 100.0:
                    continue
                name = report_section["name"]
                unmatched_sections[name] = (
                    unmatched_sections.get(name, 0)
                    + int(report_section.get("size", 0)))
            if extent_model == OBJDIFF_331_COMBINED_EXTENT:
                # A report that already matches all of this unit's data
                # leaves nothing to cover: the members were still verified
                # exact above, and nothing is credited below.
                if _unit_unmatched_data(report_unit, unit_name) \
                        and grouped_sections != unmatched_sections:
                    raise SemanticProgressError(
                        f"semantic data group does not cover the reported "
                        f"unmatched sections: {unit_name}:{section_label}")
                _require_report_binding(
                    target, report_unit, unit_name, section_label)
                surplus_numbers = _verify_declared_surplus(
                    target, base, touched, base_section_numbers,
                    entry.get("surplus"), unit_name, section_label)
                _verify_surplus_providers(
                    project_root, config_units, target, base, unit_name,
                    entry.get("surplus") or [])
                for (symbol, address, target_class, base_class,
                     base_target_section) in linkage_differences:
                    if target_class == ("undefined", _IMAGE_SYM_CLASS_EXTERNAL) \
                            and base_class == (
                                "defined", _IMAGE_SYM_CLASS_EXTERNAL) \
                            and base_target_section in surplus_numbers:
                        continue
                    raise SemanticProgressError(
                        f"semantic data group member relocation target linkage "
                        f"differs: {unit_name}:{symbol} at {address:#x} "
                        f"({target_class} vs {base_class})")
            elif grouped_sections != unmatched_sections:
                raise SemanticProgressError(
                    f"semantic data group does not cover the reported "
                    f"unmatched sections: {unit_name}:{section_label}")
        else:
            section_symbol = entry["symbol"]
            section_label = section_symbol
            try:
                target_info = section_info_resolved(
                    load(project_root / config_unit["target_path"]),
                    section_symbol,
                    symbol_addresses,
                )
                base_info = section_info_resolved(
                    load(project_root / config_unit["base_path"]),
                    section_symbol,
                    symbol_addresses,
                )
            except (CoffError, KeyError, OSError) as error:
                raise SemanticProgressError(
                    f"cannot verify semantic data match "
                    f"{unit_name}:{section_symbol}: {error}") from error
            if target_info != base_info:
                raise SemanticProgressError(
                    f"semantic data match is no longer exact: "
                    f"{unit_name}:{section_symbol}")

            expected = entry.get("measurements", {})
            snapshot = {
                key: target_info[key]
                for key in ("size", "relocation_count", "normalized_sha256")
            }
            if expected != snapshot:
                raise SemanticProgressError(
                    f"semantic data target snapshot changed: "
                    f"{unit_name}:{section_symbol}")
            credited_size = target_info["size"]
            # A single-section entry must name the unit's unmatched report
            # section itself: a lone objdiff data group whose report section
            # is the only unmatched one.  Equal size alone would let an
            # identical section elsewhere in the unit (even one the report
            # already matches) stand in for a different, unverified section.
            if _unit_unmatched_data(report_unit, unit_name):
                try:
                    target_object = load(
                        project_root / config_unit["target_path"])
                    owner_section = int(_unique_defined_symbol(
                        target_object, section_symbol,
                        "semantic data target owner")["section"])
                except (CoffError, KeyError, OSError) as error:
                    raise SemanticProgressError(
                        f"cannot verify semantic data match "
                        f"{unit_name}:{section_symbol}: {error}") from error
                single_touched = _objdiff_group_coverage(
                    target_object, {owner_section},
                    f"{unit_name}:{section_symbol}")
                single_sections = {
                    report_name: _objdiff_report_extent(sections)
                    for report_name, sections in single_touched.items()}
                single_unmatched = {}
                for report_section in report_unit.get("sections", []):
                    if report_section.get("name") == ".text" \
                            or float(report_section.get(
                                "fuzzy_match_percent", 0.0)) == 100.0:
                        continue
                    name = report_section["name"]
                    single_unmatched[name] = (
                        single_unmatched.get(name, 0)
                        + int(report_section.get("size", 0)))
                if single_sections != single_unmatched:
                    raise SemanticProgressError(
                        f"semantic data section is not the unit's unmatched "
                        f"report section: {unit_name}:{section_symbol}")

        unit_measures = report_unit["measures"]
        unmatched_data = (
            int(unit_measures.get("total_data", 0))
            - int(unit_measures.get("matched_data", 0))
        )
        if unmatched_data == 0:
            continue
        if unmatched_data != credited_size:
            raise SemanticProgressError(
                f"semantic data section does not cover all unmatched data: "
                f"{unit_name}:{section_label} covers {credited_size}, "
                f"remaining {unmatched_data}")

        _credit_data(report["measures"], unmatched_data)
        _credit_data(unit_measures, unmatched_data)
        progress_categories = report_unit.get("metadata", {}).get(
            "progress_categories", [])
        if isinstance(progress_categories, str):
            progress_categories = [progress_categories]
        for category_id in progress_categories:
            if category_id not in categories:
                raise SemanticProgressError(
                    f"semantic data category not found: {category_id}")
            _credit_data(categories[category_id]["measures"], unmatched_data)

        credited.append(
            f"{unit_name}:{section_label} (+{unmatched_data} data bytes)")

    return credited
