"""Conservative progress corrections for proven objdiff false negatives.

Entries in config/semantic_matches.json are never trusted on their own. Each
entry is re-verified against the current target and rebuilt COFF objects before
its function is credited. This keeps the ordinary objdiff report authoritative
except where a stricter semantic relocation comparison proves exact equality.
"""

import json
from pathlib import Path
from typing import Any, Dict, List

from .coff_compare import (
    CoffError,
    IMAGE_SCN_CNT_CODE,
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


def apply_semantic_data_matches(
    report: Dict[str, Any],
    project_root: Path,
    manifest_path: Path,
    objdiff_config_path: Path,
    symbol_manifest_path: Path,
) -> List[str]:
    """Verify and credit executable-split data relocation aliases.

    A manifest entry is accepted only when the target and rebuilt data
    sections have identical normalized bytes, relocation locations/types,
    and independently resolved destinations.  A grouped entry additionally
    snapshots every member's flags, alignment-derived padded extent, and
    producer-specific owner.  Its members must account for the unit's entire
    remaining unmatched data.  An incomplete unit requires an explicit
    manifest opt-in so partial spans are never credited accidentally.
    """
    if not manifest_path.is_file():
        return []

    entries = json.loads(manifest_path.read_text(encoding="utf-8"))
    objdiff = json.loads(objdiff_config_path.read_text(encoding="utf-8"))
    symbol_entries = json.loads(symbol_manifest_path.read_text(encoding="utf-8"))
    symbol_addresses = image_symbol_addresses(symbol_entries)

    report_units = {unit["name"]: unit for unit in report.get("units", [])}
    config_units = {unit["name"]: unit for unit in objdiff.get("units", [])}
    categories = {item["id"]: item for item in report.get("categories", [])}
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
        if members:
            section_label = entry.get("group", "data-section-group")
            target = load(project_root / config_unit["target_path"])
            base = load(project_root / config_unit["base_path"])
            credited_size = 0
            target_section_numbers = set()
            base_section_numbers = set()
            grouped_sections = {}

            for member in members:
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

                target_snapshot = _semantic_data_member_snapshot(
                    target_owner, target_section, target_info)
                base_snapshot = _semantic_data_member_snapshot(
                    base_owner, base_section, base_info)
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
                credited_size += target_snapshot["padded_size"]
                grouped_sections[target_snapshot["section"]] = (
                    grouped_sections.get(target_snapshot["section"], 0)
                    + target_snapshot["padded_size"])

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
            if grouped_sections != unmatched_sections:
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
