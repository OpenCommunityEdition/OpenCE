"""Fail-closed whole-translation-unit regression manifests for COFF-i386.

The ordinary objdiff report is useful for navigation, but it is not an
acceptance oracle.  This gate freezes every accepted function plus the full
non-code and symbol ownership shape of a translation unit.  A later check
rebuilds the selected objects and reports any loss of that evidence.

The gate deliberately does not grant matching credit.  In particular,
``NEWLY_EXACT`` is only a review lead; the normal reviewed acceptance workflow
must update the project metadata before a new clean snapshot is taken.
"""

from __future__ import annotations

import argparse
import copy
import datetime as _datetime
import hashlib
import json
import os
import re
import struct
import subprocess
import sys
from pathlib import Path
from typing import Any, Dict, Iterable, List, Mapping, Optional, Sequence, Tuple

from .coff_compare import (
    CoffError,
    IMAGE_SCN_CNT_CODE,
    IMAGE_SCN_CNT_UNINITIALIZED_DATA,
    RELOC_ENTRY_SIZE,
    SYMBOL_ENTRY_SIZE,
    image_symbol_addresses,
    load,
    section_info,
    section_info_by_number,
    section_info_resolved,
    section_infos_equal,
)
from .semantic_progress import _verify_local_label_continuation
from .audit_semantic_matches import relocation_shape_matches


SCHEMA_VERSION = 1
ADJUDICATION_SCHEMA_VERSION = 1
XDK_D3DINLINE_RECIPE = "xdk-stock-d3dinline"
IMAGE_SCN_CNT_INITIALIZED_DATA = 0x00000040
IMAGE_SCN_LNK_COMDAT = 0x00001000
IMAGE_SYM_CLASS_STATIC = 0x03
SECTION_HEADER_SIZE = 40
COFF_HEADER_SIZE = 20
BSS_SENTINEL = "BSS-NO-RAW"


class GateError(RuntimeError):
    """An input cannot be measured or classified without guessing."""


def _json_hash(value: Any) -> str:
    raw = json.dumps(
        value, sort_keys=True, separators=(",", ":"), ensure_ascii=True
    ).encode("utf-8")
    return hashlib.sha256(raw).hexdigest()


def _file_sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _read_json(path: Path) -> Any:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except FileNotFoundError as error:
        raise GateError(f"required file is missing: {path}") from error
    except json.JSONDecodeError as error:
        raise GateError(f"invalid JSON in {path}: {error}") from error


def _resolve_path(root: Path, path: Path) -> Path:
    return path if path.is_absolute() else root / path


def _git(root: Path, *arguments: str) -> str:
    process = subprocess.run(
        ["git", *arguments],
        cwd=root,
        capture_output=True,
        text=True,
        check=False,
    )
    if process.returncode:
        detail = (process.stderr or process.stdout).strip()
        raise GateError(f"git {' '.join(arguments)} failed: {detail}")
    return process.stdout.strip()


def require_clean_commit(root: Path) -> str:
    """Return HEAD only when tracked and untracked production state is clean."""
    commit = _git(root, "rev-parse", "HEAD")
    status = _git(root, "status", "--porcelain", "--untracked-files=normal")
    if status:
        raise GateError(
            "snapshot requires a clean working tree at a known commit; "
            f"git status reported:\n{status}"
        )
    return commit


def _symbol_record(symbol: Mapping[str, Any], section_id: Optional[str]) -> Dict[str, Any]:
    return {
        "name": symbol["name"],
        "value": int(symbol["value"]),
        "section": int(symbol["section"]),
        "section_id": section_id,
        "type": int(symbol["type"]),
        "storage": int(symbol["storage"]),
    }


def _section_owner_names(obj: Mapping[str, Any], section_number: int) -> List[str]:
    section_name = obj["sections"][section_number - 1]["name"]
    names = {
        item["name"]
        for item in obj["symbols"]
        if item["section"] == section_number
        and item["value"] == 0
        and item["name"] != section_name
        and not item["name"].startswith(".debug")
    }
    return sorted(names)


def _section_identities(obj: Mapping[str, Any]) -> Dict[int, str]:
    """Give each section a deterministic ownership-oriented identity.

    Owned COMDAT/data sections use their complete value-zero owner set.  Truly
    anonymous repeated sections retain an ordinal.  An ownership change then
    changes the identity and is reported as a vanished/appeared section rather
    than being paired by a fragile object-local section number.
    """
    anonymous_counts: Dict[str, int] = {}
    identities: Dict[int, str] = {}
    used: Dict[str, int] = {}
    for section in obj["sections"]:
        owners = _section_owner_names(obj, section["index"])
        if owners:
            identity = f"{section['name']}|owners=" + ",".join(owners)
        else:
            ordinal = anonymous_counts.get(section["name"], 0)
            anonymous_counts[section["name"]] = ordinal + 1
            identity = f"{section['name']}|anonymous={ordinal}"
        duplicate = used.get(identity, 0)
        used[identity] = duplicate + 1
        if duplicate:
            identity = f"{identity}|duplicate={duplicate}"
        identities[section["index"]] = identity
    return identities


def _section_aux_selection(obj: Mapping[str, Any], section_number: int) -> Optional[int]:
    """Read IMAGE_AUX_SYMBOL_SECTION.Selection without weakening coff_compare."""
    candidates = [
        item
        for item in obj["symbols"]
        if item["section"] == section_number
        and item["storage"] == IMAGE_SYM_CLASS_STATIC
        and item["name"] == obj["sections"][section_number - 1]["name"]
    ]
    if len(candidates) != 1:
        return None
    symbol = candidates[0]
    data = obj["data"]
    _, _, _, symbol_offset, symbol_count, _, _ = struct.unpack_from(
        "<HHLLLHH", data, 0
    )
    aux_index = int(symbol["index"]) + 1
    if aux_index >= symbol_count:
        return None
    aux_offset = symbol_offset + aux_index * SYMBOL_ENTRY_SIZE
    if aux_offset + SYMBOL_ENTRY_SIZE > len(data):
        return None
    return int(data[aux_offset + 14])


def _section_kind(section: Mapping[str, Any]) -> str:
    flags = int(section["flags"])
    if flags & IMAGE_SCN_CNT_CODE:
        return "CODE"
    if flags & IMAGE_SCN_CNT_UNINITIALIZED_DATA:
        return "BSS"
    if section["name"].startswith(".rdata"):
        return "RDATA"
    if flags & IMAGE_SCN_CNT_INITIALIZED_DATA:
        return "DATA"
    if section["name"].startswith(".debug"):
        return "DEBUG"
    if section["name"] == ".drectve":
        return "DIRECTIVE"
    return "OTHER"


def _section_bytes(obj: Mapping[str, Any], section: Mapping[str, Any]) -> bytearray:
    if (
        int(section["flags"]) & IMAGE_SCN_CNT_UNINITIALIZED_DATA
        and int(section["raw"]) == 0
    ):
        return bytearray(int(section["size"]))
    start = int(section["raw"])
    end = start + int(section["size"])
    if start < 0 or end > len(obj["data"]):
        raise GateError(
            f"section {section['index']} raw bytes are outside the COFF object"
        )
    return bytearray(obj["data"][start:end])


def _resolved_relocations(
    obj: Mapping[str, Any],
    section: Mapping[str, Any],
    identities: Mapping[int, str],
) -> Tuple[str, List[Dict[str, Any]]]:
    """Return a full normalized hash and ordered relocation fingerprint.

    Both the resolved destination and the un-normalized owning symbol are
    retained.  Consequently semantic destination equivalence never hides a
    storage/ownership regression.
    """
    raw = _section_bytes(obj, section)
    try:
        semantic_info = section_info_by_number(obj, int(section["index"]))
    except CoffError as error:
        raise GateError(
            f"cannot semantically resolve section {section['index']}: {error}"
        ) from error
    relocations: List[Dict[str, Any]] = []
    for relocation_index in range(int(section["reloc_count"])):
        relocation_offset = int(section["reloc"]) + relocation_index * RELOC_ENTRY_SIZE
        if relocation_offset + RELOC_ENTRY_SIZE > len(obj["data"]):
            raise GateError(
                f"section {section['index']} relocation table is truncated"
            )
        address, target_index, relocation_type = struct.unpack_from(
            "<LLH", obj["data"], relocation_offset
        )
        target = obj["by_index"].get(target_index)
        if target is None:
            raise GateError(
                f"section {section['index']} relocation {relocation_index} "
                f"references missing symbol index {target_index}"
            )
        if address + 4 > len(raw):
            raise GateError(
                f"section {section['index']} relocation {relocation_index} "
                f"address {address:#x} is outside its logical size"
            )
        addend = struct.unpack_from("<i", raw, address)[0]
        semantic = semantic_info["relocations"][relocation_index]
        if (
            int(semantic["address"]) != int(address)
            or int(semantic["type"]) != int(relocation_type)
        ):
            raise GateError(
                f"hardened relocation resolver order mismatch in section "
                f"{section['index']} at relocation {relocation_index}"
            )

        target_section = int(target["section"])

        relocations.append(
            {
                "address": int(address),
                "type": int(relocation_type),
                "addend": int(addend),
                "resolved_destination": copy.deepcopy(semantic["target"]),
                "symbolic_destination": copy.deepcopy(
                    semantic.get("symbolic_target")
                ),
                "target_symbol": _symbol_record(
                    target, identities.get(target_section)
                ),
            }
        )
    return semantic_info["normalized_sha256"], relocations


def _section_fingerprint(
    obj: Mapping[str, Any],
    section: Mapping[str, Any],
    identities: Mapping[int, str],
    image_addresses: Mapping[str, int],
) -> Dict[str, Any]:
    digest, relocations = _resolved_relocations(
        obj, section, identities
    )
    kind = _section_kind(section)
    if kind == "BSS" and int(section["raw"]) == 0:
        digest = BSS_SENTINEL
    symbols = [
        _symbol_record(item, identities.get(int(item["section"])))
        for item in obj["symbols"]
        if int(item["section"]) == int(section["index"])
    ]
    symbols.sort(
        key=lambda item: (
            item["value"], item["name"], item["type"], item["storage"]
        )
    )
    return {
        "identity": identities[int(section["index"])],
        "name": section["name"],
        "index": int(section["index"]),
        "kind": kind,
        "logical_size": int(section["size"]),
        "flags": int(section["flags"]),
        "alignment_shift": (int(section["flags"]) >> 20) & 0xF,
        "comdat": bool(int(section["flags"]) & IMAGE_SCN_LNK_COMDAT),
        "comdat_selection": _section_aux_selection(obj, int(section["index"])),
        "raw_present": int(section["raw"]) != 0,
        "normalized_sha256": digest,
        "relocation_count": int(section["reloc_count"]),
        "relocations": relocations,
        "symbols": symbols,
    }


def _object_fingerprint(
    path: Path,
    meaningful_sizes: Mapping[str, int],
    image_addresses: Mapping[str, int],
    require_meaningful_sizes: bool = False,
) -> Dict[str, Any]:
    try:
        obj = load(path)
    except (OSError, CoffError) as error:
        raise GateError(f"cannot load COFF object {path}: {error}") from error
    identities = _section_identities(obj)

    symbol_inventory = [
        _symbol_record(item, identities.get(int(item["section"])))
        for item in obj["symbols"]
    ]
    symbol_inventory.sort(
        key=lambda item: (
            item["section"], item["value"], item["name"], item["type"], item["storage"]
        )
    )

    function_symbols: Dict[str, Mapping[str, Any]] = {}
    for item in obj["symbols"]:
        section_number = int(item["section"])
        if section_number <= 0 or int(item["type"]) != 0x20:
            continue
        section = obj["sections"][section_number - 1]
        if not (int(section["flags"]) & IMAGE_SCN_CNT_CODE):
            continue
        name = item["name"]
        if name in function_symbols:
            raise GateError(f"ambiguous duplicate function symbol {name!r} in {path}")
        function_symbols[name] = item

    functions: Dict[str, Any] = {}
    for name in sorted(function_symbols):
        symbol = function_symbols[name]
        section = obj["sections"][int(symbol["section"]) - 1]
        fingerprint = _section_fingerprint(
            obj, section, identities, image_addresses
        )
        try:
            strict_info = section_info(obj, name)
        except CoffError as error:
            raise GateError(f"cannot fingerprint function {name} in {path}: {error}") from error
        fingerprint["padded_size"] = int(strict_info["size"])
        meaningful_size = meaningful_sizes.get(name)
        if require_meaningful_sizes and meaningful_size is None:
            # csplit can preserve multiple public names for one linked-image
            # function even though objdiff reports that body once.  Inherit
            # the report's meaningful size only when exactly one reported
            # function symbol occupies the identical COFF section and offset.
            # This resolves a name alias; it neither compares nor credits code.
            alias_candidates = [
                candidate_name
                for candidate_name in meaningful_sizes
                if candidate_name in function_symbols
                and int(function_symbols[candidate_name]["section"])
                == int(symbol["section"])
                and int(function_symbols[candidate_name]["value"])
                == int(symbol["value"])
            ]
            if len(alias_candidates) != 1:
                detail = (
                    "none" if not alias_candidates
                    else ", ".join(sorted(alias_candidates))
                )
                raise GateError(
                    f"ordinary report has no meaningful size for target function "
                    f"{name} in {path}; identical-location report candidates: "
                    f"{detail}"
                )
            meaningful_size = meaningful_sizes[alias_candidates[0]]
        fingerprint["meaningful_size"] = int(
            strict_info["size"] if meaningful_size is None else meaningful_size
        )
        fingerprint["strict_relocations"] = copy.deepcopy(strict_info["relocations"])
        functions[name] = fingerprint

    non_code_sections: Dict[str, Any] = {}
    for section in obj["sections"]:
        if int(section["flags"]) & IMAGE_SCN_CNT_CODE:
            continue
        fingerprint = _section_fingerprint(
            obj, section, identities, image_addresses
        )
        identity = fingerprint["identity"]
        if identity in non_code_sections:
            raise GateError(f"duplicate non-code section identity {identity!r} in {path}")
        non_code_sections[identity] = fingerprint

    return {
        "object_sha256": _file_sha256(path),
        "functions": functions,
        "non_code_sections": non_code_sections,
        "symbols": symbol_inventory,
    }


def _report_meaningful_sizes(report: Mapping[str, Any], unit_name: str) -> Dict[str, int]:
    matches = [item for item in report.get("units", []) if item.get("name") == unit_name]
    if len(matches) != 1:
        raise GateError(
            f"expected one report unit {unit_name!r}, found {len(matches)}"
        )
    result: Dict[str, int] = {}
    for function in matches[0].get("functions", []):
        name = function.get("name")
        if name in result:
            raise GateError(f"duplicate report function {unit_name}:{name}")
        try:
            result[name] = int(function["size"])
        except (KeyError, TypeError, ValueError) as error:
            raise GateError(
                f"invalid meaningful size for {unit_name}:{name}"
            ) from error
    return result


def _report_ordinary_percentages(
    report: Mapping[str, Any], unit_name: str
) -> Dict[str, float]:
    matches = [item for item in report.get("units", []) if item.get("name") == unit_name]
    if len(matches) != 1:
        raise GateError(
            f"expected one report unit {unit_name!r}, found {len(matches)}"
        )
    result: Dict[str, float] = {}
    for function in matches[0].get("functions", []):
        name = function.get("name")
        if name in result:
            raise GateError(f"duplicate report function {unit_name}:{name}")
        try:
            result[name] = float(function.get("fuzzy_match_percent", 0.0))
        except (TypeError, ValueError) as error:
            raise GateError(
                f"invalid ordinary percentage for {unit_name}:{name}"
            ) from error
    return result


def _symbol_addresses(entries: Sequence[Mapping[str, Any]]) -> Dict[str, int]:
    return image_symbol_addresses(entries)


def _exception_records(
    units: Iterable[str],
    semantic_matches: Sequence[Mapping[str, Any]],
    semantic_data_matches: Sequence[Mapping[str, Any]],
) -> List[Dict[str, Any]]:
    selected = set(units)
    records: List[Dict[str, Any]] = []
    for ledger, entries, item_key in (
        ("semantic_matches", semantic_matches, "function"),
        ("semantic_data_matches", semantic_data_matches, "symbol"),
    ):
        for entry in entries:
            if entry.get("unit") not in selected:
                continue
            canonical = copy.deepcopy(entry)
            records.append(
                {
                    "ledger": ledger,
                    "unit": entry["unit"],
                    "item": entry[item_key],
                    "identity": _json_hash(canonical),
                    "entry": canonical,
                }
            )
    records.sort(key=lambda item: (item["unit"], item["ledger"], item["item"]))
    return records


def _exceptions_for_unit(
    exception_records: Sequence[Mapping[str, Any]], unit_name: str
) -> List[Mapping[str, Any]]:
    return [item for item in exception_records if item["unit"] == unit_name]


def _find_section_for_symbol(
    obj: Mapping[str, Any], symbol_name: str, fingerprints: Mapping[str, Any]
) -> Optional[str]:
    matches = [
        item
        for item in obj["symbols"]
        if item["name"] == symbol_name and int(item["section"]) > 0
    ]
    if len(matches) != 1:
        return None
    identities = _section_identities(obj)
    identity = identities[int(matches[0]["section"])]
    return identity if identity in fingerprints else None


def _section_acceptance_view(fingerprint: Mapping[str, Any]) -> Dict[str, Any]:
    """Remove only object-local numeric section identifiers for cross-object pairing."""
    result = copy.deepcopy(fingerprint)
    result.pop("index", None)
    for symbol in result.get("symbols", []):
        symbol.pop("section", None)
    for relocation in result.get("relocations", []):
        relocation.get("target_symbol", {}).pop("section", None)
    return result


def _capture_unit(
    root: Path,
    unit_config: Mapping[str, Any],
    meaningful_sizes: Mapping[str, int],
    image_addresses: Mapping[str, int],
    exception_records: Sequence[Mapping[str, Any]],
    ordinary_percentages: Optional[Mapping[str, float]] = None,
) -> Dict[str, Any]:
    unit_name = unit_config["name"]
    target_path = _resolve_path(root, Path(unit_config["target_path"]))
    base_path = _resolve_path(root, Path(unit_config["base_path"]))
    if not target_path.is_file():
        raise GateError(f"target object is missing for {unit_name}: {target_path}")
    if not base_path.is_file():
        raise GateError(f"base object is missing for {unit_name}: {base_path}")

    target = _object_fingerprint(
        target_path,
        meaningful_sizes,
        image_addresses,
        require_meaningful_sizes=True,
    )
    base = _object_fingerprint(base_path, meaningful_sizes, image_addresses)
    target_obj = load(target_path)
    base_obj = load(base_path)
    unit_exceptions = _exceptions_for_unit(exception_records, unit_name)
    function_exceptions = {
        item["item"]: item
        for item in unit_exceptions
        if item["ledger"] == "semantic_matches"
    }
    data_exceptions = {
        item["item"]: item
        for item in unit_exceptions
        if item["ledger"] == "semantic_data_matches"
    }

    comparisons: Dict[str, Any] = {}
    ordinary_percentages = ordinary_percentages or {}
    for name, target_fingerprint in target["functions"].items():
        state = "ABSENT_IN_BASE"
        accepted = False
        exception_identity = None
        exception = function_exceptions.get(name)
        base_name = (
            exception["entry"].get("base_function", name)
            if exception is not None
            else name
        )
        base_fingerprint = base["functions"].get(base_name)
        if base_fingerprint is not None:
            try:
                target_info = section_info(target_obj, name)
                base_info = section_info(base_obj, base_name)
                exact = section_infos_equal(target_info, base_info)
            except CoffError as error:
                raise GateError(
                    f"cannot compare function {unit_name}:{name}: {error}"
                ) from error
            if exact:
                state = "STRICT_EXACT"
            elif relocation_shape_matches(target_info, base_info):
                state = "OWNERSHIP_ONLY"
            elif (
                target_info["size"] == base_info["size"]
                and target_info["normalized_sha256"]
                == base_info["normalized_sha256"]
                and target_info["relocation_count"]
                == base_info["relocation_count"]
            ):
                state = "BYTES_EQUAL_RELOCATION_SHAPE_DIFF"
            else:
                state = "NONEXACT"
            accepted = exact

            # Preserve the existing ordinary acceptance policy without
            # pretending it is strict equality.  These entries are objdiff
            # 100% and have equal normalized bytes plus relocation addresses
            # and types, but their resolved destinations remain strict-mismatch.
            # The distinct state freezes their complete evidence; it grants no
            # new matching credit.
            if ordinary_percentages.get(name) == 100.0 and not exact:
                if relocation_shape_matches(target_info, base_info):
                    state = "ORDINARY_ACCEPTED_STRICT_MISMATCH"
                    accepted = True
                else:
                    state = "ORDINARY_REJECTED_STRICT_MISMATCH"
                    accepted = False

        if exception is not None and exception["entry"].get("owner_function"):
            owner = exception["entry"]["owner_function"]
            try:
                _verify_local_label_continuation(target_obj, base_obj, name, owner)
            except Exception as error:  # semantic_progress raises its own strict error
                raise GateError(
                    f"semantic exception no longer proves {unit_name}:{name}: {error}"
                ) from error
            state = "SEMANTIC_EXACT"
            accepted = True
            exception_identity = exception["identity"]
        elif exception is not None:
            # Ordinary semantic-match entries still rely on the strict COFF
            # comparator; the ledger only corrects objdiff's presentation.
            if state != "STRICT_EXACT":
                raise GateError(
                    f"semantic match is not strict-exact: {unit_name}:{name}"
                )
            exception_identity = exception["identity"]

        comparisons[name] = {
            "state": state,
            "accepted": accepted,
            "meaningful_size": int(target_fingerprint["meaningful_size"]),
            "ordinary_percent": ordinary_percentages.get(name),
            "exception_identity": exception_identity,
        }

    section_comparisons: Dict[str, Any] = {}
    all_section_ids = sorted(
        set(target["non_code_sections"]) | set(base["non_code_sections"])
    )
    semantic_sections: Dict[str, str] = {}
    for symbol_name, exception in data_exceptions.items():
        target_identity = _find_section_for_symbol(
            target_obj, symbol_name, target["non_code_sections"]
        )
        base_identity = _find_section_for_symbol(
            base_obj, symbol_name, base["non_code_sections"]
        )
        if target_identity is None or base_identity is None:
            raise GateError(
                f"cannot locate semantic data section {unit_name}:{symbol_name}"
            )
        try:
            target_info = section_info_resolved(
                target_obj, symbol_name, image_addresses
            )
            base_info = section_info_resolved(base_obj, symbol_name, image_addresses)
        except CoffError as error:
            raise GateError(
                f"cannot verify semantic data section {unit_name}:{symbol_name}: {error}"
            ) from error
        if target_info != base_info:
            raise GateError(
                f"semantic data section is no longer exact: {unit_name}:{symbol_name}"
            )
        expected = exception["entry"].get("measurements", {})
        measured = {
            key: target_info[key]
            for key in ("size", "relocation_count", "normalized_sha256")
        }
        if expected != measured:
            raise GateError(
                f"semantic data target snapshot changed: {unit_name}:{symbol_name}"
            )
        semantic_sections[target_identity] = exception["identity"]
        if target_identity != base_identity:
            # The exception proves relocation aliases, not arbitrary section
            # identity changes.  Pairing different identities would hide the
            # ownership regression this gate exists to detect.
            raise GateError(
                f"semantic data ownership identity differs for "
                f"{unit_name}:{symbol_name}"
            )

    for identity in all_section_ids:
        target_fingerprint = target["non_code_sections"].get(identity)
        base_fingerprint = base["non_code_sections"].get(identity)
        state = "TARGET_ONLY" if base_fingerprint is None else "BASE_ONLY"
        accepted = False
        exception_identity = semantic_sections.get(identity)
        if target_fingerprint is not None and base_fingerprint is not None:
            # Section numbers are object-local.  Ownership identities, flags,
            # symbols, bytes, and relocations remain strict.
            strict_target = _section_acceptance_view(target_fingerprint)
            strict_base = _section_acceptance_view(base_fingerprint)
            if strict_target == strict_base:
                state = "STRICT_EXACT"
                accepted = True
            elif exception_identity:
                state = "SEMANTIC_EXACT"
                accepted = True
            else:
                state = "NONEXACT"
        section_comparisons[identity] = {
            "state": state,
            "accepted": accepted,
            "exception_identity": exception_identity,
        }

    return {
        "name": unit_name,
        "config": {
            "target_path": unit_config["target_path"],
            "base_path": unit_config["base_path"],
        },
        "target": target,
        "base": base,
        "functions": comparisons,
        "sections": section_comparisons,
    }


def _semantic_entries(root: Path, path: Path) -> Sequence[Mapping[str, Any]]:
    value = _read_json(_resolve_path(root, path))
    if not isinstance(value, list):
        raise GateError(f"semantic ledger must contain a JSON list: {path}")
    return value


def _unit_configs(config: Mapping[str, Any]) -> Dict[str, Mapping[str, Any]]:
    result: Dict[str, Mapping[str, Any]] = {}
    for item in config.get("units", []):
        name = item.get("name")
        if name in result:
            raise GateError(f"duplicate objdiff unit {name!r}")
        if item.get("target_path") and item.get("base_path"):
            result[name] = item
    return result


def _select_units(
    available: Mapping[str, Any], requested: Sequence[str], select_all: bool
) -> List[str]:
    if bool(requested) == bool(select_all):
        raise GateError("select exactly one of --unit (repeatable) or --all")
    selected = sorted(available) if select_all else list(dict.fromkeys(requested))
    missing = [name for name in selected if name not in available]
    if missing:
        raise GateError("unknown objdiff unit(s): " + ", ".join(missing))
    return selected


def _logical_ninja_lines(text: str) -> List[str]:
    result: List[str] = []
    current = ""
    for raw_line in text.splitlines():
        stripped = raw_line.rstrip()
        continuation = stripped.endswith("$")
        if continuation:
            stripped = stripped[:-1]
        current += stripped.strip() if not current else " " + stripped.strip()
        if not continuation:
            if current:
                result.append(current)
            current = ""
    if current:
        result.append(current)
    return result


def _build_environment(
    root: Path, build_ninja: Path, units: Mapping[str, Mapping[str, Any]]
) -> Dict[str, Any]:
    path = _resolve_path(root, build_ninja)
    try:
        logical_lines = _logical_ninja_lines(path.read_text(encoding="utf-8"))
    except FileNotFoundError as error:
        raise GateError(f"generated Ninja file is missing: {path}") from error

    rule_command = None
    for index, line in enumerate(logical_lines):
        if line == "rule cl":
            for candidate in logical_lines[index + 1 : index + 5]:
                if candidate.startswith("command = "):
                    rule_command = candidate[len("command = ") :]
                    break
            break
    if not rule_command:
        raise GateError("cannot locate the cl rule in build.ninja")
    compiler_token = rule_command.split()[0]
    compiler_path = _resolve_path(root, Path(compiler_token.replace("/", os.sep)))
    if not compiler_path.is_file():
        raise GateError(f"compiler from build.ninja is missing: {compiler_path}")
    version_process = subprocess.run(
        [str(compiler_path)],
        cwd=root,
        capture_output=True,
        text=True,
        timeout=20,
        check=False,
    )
    version = (version_process.stdout + version_process.stderr).strip()
    if not version:
        raise GateError(f"compiler produced no version output: {compiler_path}")

    flags_by_unit: Dict[str, str] = {}
    for unit_name, unit in units.items():
        base_path = str(unit["base_path"]).replace("/", "\\")
        build_prefix = f"build {base_path}: cl "
        matches = [
            index
            for index, line in enumerate(logical_lines)
            if line.startswith(build_prefix)
        ]
        if len(matches) != 1:
            raise GateError(
                f"expected one Ninja build edge for {unit_name}, found {len(matches)}"
            )
        flags = None
        for candidate in logical_lines[matches[0] + 1 : matches[0] + 5]:
            if candidate.startswith("cflags = "):
                flags = candidate[len("cflags = ") :]
                break
        if flags is None:
            raise GateError(f"cannot locate cflags for {unit_name}")
        flags_by_unit[unit_name] = flags

    return {
        "rule_command": rule_command,
        "compiler_path": compiler_token,
        "compiler_sha256": _file_sha256(compiler_path),
        "compiler_version": version,
        "flags_by_unit": flags_by_unit,
    }


def _build_units(
    root: Path,
    ninja: str,
    unit_configs: Sequence[Mapping[str, Any]],
    report_target: Optional[str] = None,
) -> None:
    targets = [str(item["base_path"]) for item in unit_configs]
    if report_target:
        targets.append(report_target)
    process = subprocess.run(
        [ninja, *targets], cwd=root, text=True, capture_output=True, check=False
    )
    if process.returncode:
        detail = (process.stdout + process.stderr)[-4000:]
        raise GateError(f"Ninja object build failed:\n{detail}")


def create_manifest(
    root: Path,
    commit: str,
    selected_units: Sequence[str],
    config: Mapping[str, Any],
    report: Mapping[str, Any],
    semantic_matches: Sequence[Mapping[str, Any]],
    semantic_data_matches: Sequence[Mapping[str, Any]],
    image_symbol_entries: Sequence[Mapping[str, Any]],
    build_environment: Mapping[str, Any],
) -> Dict[str, Any]:
    available = _unit_configs(config)
    exceptions = _exception_records(
        selected_units, semantic_matches, semantic_data_matches
    )
    addresses = _symbol_addresses(image_symbol_entries)
    units: Dict[str, Any] = {}
    for unit_name in selected_units:
        meaningful_sizes = _report_meaningful_sizes(report, unit_name)
        units[unit_name] = _capture_unit(
            root,
            available[unit_name],
            meaningful_sizes,
            addresses,
            exceptions,
            _report_ordinary_percentages(report, unit_name),
        )
    return {
        "schema_version": SCHEMA_VERSION,
        "commit": commit,
        "generated_utc": _datetime.datetime.now(
            _datetime.timezone.utc
        ).isoformat(),
        "config_sha256": _json_hash(config),
        "build_environment": copy.deepcopy(build_environment),
        "semantic_exceptions": exceptions,
        "units": units,
    }


def _diff_maps(
    baseline: Mapping[str, Any], current: Mapping[str, Any]
) -> Tuple[List[str], List[str], List[str]]:
    vanished = sorted(set(baseline) - set(current))
    appeared = sorted(set(current) - set(baseline))
    changed = sorted(
        key for key in set(baseline) & set(current) if baseline[key] != current[key]
    )
    return vanished, appeared, changed


def _function_code_evidence(fingerprint: Mapping[str, Any]) -> Dict[str, Any]:
    """Return the runtime-relevant evidence an adjudication may never waive."""
    return {
        "padded_size": fingerprint.get("padded_size"),
        "normalized_sha256": fingerprint.get("normalized_sha256"),
        "relocation_count": fingerprint.get("relocation_count"),
        "relocations": [
            {
                "address": item.get("address"),
                "type": item.get("type"),
                "addend": item.get("addend"),
                "resolved_destination": copy.deepcopy(
                    item.get("resolved_destination")
                ),
                "symbolic_destination": copy.deepcopy(
                    item.get("symbolic_destination")
                ),
            }
            for item in fingerprint.get("relocations", [])
        ],
    }


def _comdat_delta_only(
    before: Mapping[str, Any],
    after: Mapping[str, Any],
    entry: Mapping[str, Any],
) -> bool:
    """Prove that a wrapper changed only its recorded COMDAT selection.

    The linked XBE cannot retain source-object COMDAT selection.  This helper
    is intentionally recipe-specific and does not create a general metadata
    ignore rule.
    """
    if entry.get("recipe") != XDK_D3DINLINE_RECIPE:
        return False
    if before.get("kind") != "CODE" or after.get("kind") != "CODE":
        return False
    if not before.get("comdat") or not after.get("comdat"):
        return False
    if before.get("comdat_selection") != entry.get("before_comdat_selection"):
        return False
    if after.get("comdat_selection") != entry.get("after_comdat_selection"):
        return False
    if entry.get("before_comdat_selection") != 1:
        return False
    if entry.get("after_comdat_selection") != 2:
        return False

    normalized_before = copy.deepcopy(before)
    normalized_after = copy.deepcopy(after)
    normalized_before["comdat_selection"] = "ADJUDICATED"
    normalized_after["comdat_selection"] = "ADJUDICATED"
    return normalized_before == normalized_after


def _index_adjudications(
    value: Optional[Mapping[str, Any]],
) -> Tuple[Dict[Tuple[str, str], Mapping[str, Any]], Dict[Tuple[str, str], Mapping[str, Any]]]:
    if value is None:
        return {}, {}
    if value.get("schema_version") != ADJUDICATION_SCHEMA_VERSION:
        raise GateError("unsupported or missing adjudication schema version")
    function_entries: Dict[Tuple[str, str], Mapping[str, Any]] = {}
    debug_entries: Dict[Tuple[str, str], Mapping[str, Any]] = {}
    for entry in value.get("functions", []):
        key = (entry.get("unit"), entry.get("function"))
        if not all(isinstance(item, str) and item for item in key):
            raise GateError("function adjudication requires unit and function")
        if key in function_entries:
            raise GateError(f"duplicate function adjudication: {key[0]}:{key[1]}")
        function_entries[key] = entry
    for entry in value.get("debug_sections", []):
        key = (entry.get("unit"), entry.get("section"))
        if not all(isinstance(item, str) and item for item in key):
            raise GateError("debug adjudication requires unit and section")
        if key in debug_entries:
            raise GateError(f"duplicate debug adjudication: {key[0]}:{key[1]}")
        debug_entries[key] = entry
    return function_entries, debug_entries


def _adjudicate_function_change(
    unit_name: str,
    function_name: str,
    before_target: Mapping[str, Any],
    after_target: Mapping[str, Any],
    before_base: Mapping[str, Any],
    after_base: Mapping[str, Any],
    entry: Optional[Mapping[str, Any]],
) -> bool:
    if entry is None:
        return False
    if entry.get("source_recipe") != "stock XDK D3DINLINE (static __forceinline)":
        return False
    if entry.get("before_fingerprint_sha256") != _json_hash(before_base):
        return False
    if entry.get("after_fingerprint_sha256") != _json_hash(after_base):
        return False
    if entry.get("target_evidence_sha256") != _json_hash(
        _function_code_evidence(after_target)
    ):
        return False
    if before_target != after_target:
        return False
    if _function_code_evidence(before_base) != _function_code_evidence(after_base):
        return False
    if not _comdat_delta_only(before_base, after_base, entry):
        return False
    return True


def _adjudicate_debug_change(
    before: Mapping[str, Any],
    after: Mapping[str, Any],
    entry: Optional[Mapping[str, Any]],
) -> bool:
    """Permit one exact, pre-recorded compiler-debug transition only."""
    if entry is None or entry.get("recipe") != XDK_D3DINLINE_RECIPE:
        return False
    if not str(before.get("name", "")).startswith(".debug"):
        return False
    if not str(after.get("name", "")).startswith(".debug"):
        return False
    return (
        entry.get("before_fingerprint_sha256") == _json_hash(before)
        and entry.get("after_fingerprint_sha256") == _json_hash(after)
    )


def compare_manifests(
    baseline: Mapping[str, Any],
    current: Mapping[str, Any],
    adjudications: Optional[Mapping[str, Any]] = None,
) -> Dict[str, Any]:
    result: Dict[str, Any] = {
        "ok": True,
        "failures": [],
        "warnings": [],
        "units": {},
    }

    def fail(kind: str, message: str, **detail: Any) -> None:
        result["ok"] = False
        record = {"kind": kind, "message": message}
        record.update(detail)
        result["failures"].append(record)

    try:
        function_adjudications, debug_adjudications = _index_adjudications(
            adjudications
        )
    except GateError as error:
        fail("UNKNOWN", str(error))
        return result
    consumed_functions = set()
    consumed_debug_sections = set()

    if baseline.get("schema_version") != SCHEMA_VERSION:
        fail("UNKNOWN", "unsupported or missing baseline schema version")
        return result
    if baseline.get("build_environment") != current.get("build_environment"):
        fail("UNKNOWN", "compiler identity or per-unit flags changed")
    if baseline.get("config_sha256") != current.get("config_sha256"):
        fail("UNKNOWN", "objdiff configuration changed")
    if baseline.get("semantic_exceptions") != current.get("semantic_exceptions"):
        fail("UNKNOWN", "semantic-exception identities changed")

    baseline_units = baseline.get("units", {})
    current_units = current.get("units", {})
    vanished_units, appeared_units, _ = _diff_maps(baseline_units, current_units)
    for unit_name in vanished_units:
        fail("UNKNOWN", f"baseline unit vanished: {unit_name}", unit=unit_name)
    for unit_name in appeared_units:
        fail("UNKNOWN", f"unexpected unit appeared: {unit_name}", unit=unit_name)

    for unit_name in sorted(set(baseline_units) & set(current_units)):
        before = baseline_units[unit_name]
        after = current_units[unit_name]
        unit_result = {
            "still_exact": [],
            "newly_exact": [],
            "changed_nonexact": [],
        }
        result["units"][unit_name] = unit_result

        if before["target"] != after["target"]:
            fail(
                "UNKNOWN",
                f"target object evidence changed for {unit_name}",
                unit=unit_name,
            )

        before_functions = before["functions"]
        after_functions = after["functions"]
        vanished, appeared, _ = _diff_maps(before_functions, after_functions)
        for name in vanished:
            fail(
                "SYMBOL_SET_CHANGED",
                f"target function vanished: {unit_name}:{name}",
                unit=unit_name,
                item=name,
            )
        for name in appeared:
            fail(
                "SYMBOL_SET_CHANGED",
                f"target function appeared: {unit_name}:{name}",
                unit=unit_name,
                item=name,
            )

        for name in sorted(set(before_functions) & set(after_functions)):
            previous = before_functions[name]
            now = after_functions[name]
            previous_base_fingerprint = before["base"]["functions"].get(name)
            current_base_fingerprint = after["base"]["functions"].get(name)
            if previous["accepted"]:
                if not now["accepted"]:
                    fail(
                        "REGRESSED",
                        f"accepted function regressed: {unit_name}:{name}",
                        unit=unit_name,
                        item=name,
                        before=previous["state"],
                        after=now["state"],
                    )
                elif (
                    previous != now
                    or previous_base_fingerprint != current_base_fingerprint
                ):
                    key = (unit_name, name)
                    entry = function_adjudications.get(key)
                    if _adjudicate_function_change(
                        unit_name,
                        name,
                        before["target"]["functions"].get(name, {}),
                        after["target"]["functions"].get(name, {}),
                        previous_base_fingerprint or {},
                        current_base_fingerprint or {},
                        entry,
                    ):
                        consumed_functions.add(key)
                        unit_result.setdefault("adjudicated_exact", []).append(name)
                        result["warnings"].append(
                            {
                                "kind": "ADJUDICATED_COMDAT_CHANGE",
                                "unit": unit_name,
                                "item": name,
                                "message": (
                                    "exact code and relocation evidence preserved; "
                                    "reviewed stock-XDK COMDAT transition accepted"
                                ),
                            }
                        )
                    else:
                        fail(
                            "UNKNOWN",
                            f"accepted function evidence changed: {unit_name}:{name}",
                            unit=unit_name,
                            item=name,
                        )
                else:
                    unit_result["still_exact"].append(name)
            elif now["accepted"]:
                unit_result["newly_exact"].append(name)
                result["warnings"].append(
                    {
                        "kind": "NEWLY_EXACT",
                        "unit": unit_name,
                        "item": name,
                        "message": "review required; no matching credit was granted",
                    }
                )
            elif (
                previous != now
                or previous_base_fingerprint != current_base_fingerprint
            ):
                unit_result["changed_nonexact"].append(name)

        # Freeze every non-code section in the rebuilt object, even in an
        # incomplete unit.  This is what catches a static BSS becoming a COMMON
        # symbol while all functions remain byte-identical.
        before_sections = before["base"]["non_code_sections"]
        after_sections = after["base"]["non_code_sections"]
        vanished, appeared, changed = _diff_maps(before_sections, after_sections)
        for identity in vanished + appeared:
            fail(
                "DATA_CHANGED",
                f"rebuilt non-code section changed: {unit_name}:{identity}",
                unit=unit_name,
                item=identity,
            )
        for identity in changed:
            key = (unit_name, identity)
            if _adjudicate_debug_change(
                before_sections[identity],
                after_sections[identity],
                debug_adjudications.get(key),
            ):
                consumed_debug_sections.add(key)
                result["warnings"].append(
                    {
                        "kind": "ADJUDICATED_DEBUG_CHANGE",
                        "unit": unit_name,
                        "item": identity,
                        "message": (
                            "exact pre-recorded stock-XDK compiler-debug "
                            "transition accepted"
                        ),
                    }
                )
            else:
                fail(
                    "DATA_CHANGED",
                    f"rebuilt non-code section changed: {unit_name}:{identity}",
                    unit=unit_name,
                    item=identity,
                )

        before_symbols = before["base"]["symbols"]
        after_symbols = after["base"]["symbols"]
        if before_symbols != after_symbols:
            fail(
                "SYMBOL_SET_CHANGED",
                f"rebuilt symbol ownership changed for {unit_name}",
                unit=unit_name,
            )

    unused_functions = sorted(set(function_adjudications) - consumed_functions)
    unused_debug_sections = sorted(
        set(debug_adjudications) - consumed_debug_sections
    )
    for unit_name, name in unused_functions:
        fail(
            "UNKNOWN",
            f"function adjudication was not consumed: {unit_name}:{name}",
            unit=unit_name,
            item=name,
        )
    for unit_name, identity in unused_debug_sections:
        fail(
            "UNKNOWN",
            f"debug adjudication was not consumed: {unit_name}:{identity}",
            unit=unit_name,
            item=identity,
        )

    return result


def load_baseline(path: Path) -> Mapping[str, Any]:
    if not path.is_file():
        raise GateError(f"regression baseline is missing: {path}")
    value = _read_json(path)
    if value.get("schema_version") != SCHEMA_VERSION:
        raise GateError(
            f"unsupported regression baseline schema: {value.get('schema_version')!r}"
        )
    if not value.get("commit"):
        raise GateError("regression baseline does not record a clean commit")
    return value


def _write_json(path: Path, value: Any) -> None:
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2, sort_keys=True) + "\n", encoding="utf-8")


def _common_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(add_help=False)
    parser.add_argument("--root", type=Path, default=Path.cwd())
    parser.add_argument("--manifest", type=Path, default=Path("build/regression_manifest.json"))
    parser.add_argument("--config", type=Path, default=Path("objdiff.json"))
    parser.add_argument("--report", type=Path, default=Path("build/report.json"))
    parser.add_argument("--semantic-matches", type=Path, default=Path("config/semantic_matches.json"))
    parser.add_argument("--semantic-data-matches", type=Path, default=Path("config/semantic_data_matches.json"))
    parser.add_argument("--symbols", type=Path, default=Path("config/symbols.json"))
    parser.add_argument("--build-ninja", type=Path, default=Path("build.ninja"))
    parser.add_argument("--ninja", default="ninja")
    parser.add_argument("--unit", action="append", default=[])
    parser.add_argument("--all", action="store_true")
    parser.add_argument("--no-build", action="store_true", help=argparse.SUPPRESS)
    return parser


def _capture_from_cli(
    args: argparse.Namespace,
    selected: Sequence[str],
    config: Mapping[str, Any],
    baseline: Optional[Mapping[str, Any]] = None,
) -> Dict[str, Any]:
    root = args.root.resolve()
    available = _unit_configs(config)
    selected_configs = {name: available[name] for name in selected}
    if not args.no_build:
        _build_units(
            root,
            args.ninja,
            list(selected_configs.values()),
            str(args.report),
        )
    report = _read_json(_resolve_path(root, args.report))
    semantic_matches = _semantic_entries(root, args.semantic_matches)
    semantic_data_matches = _semantic_entries(root, args.semantic_data_matches)
    symbol_entries = _read_json(_resolve_path(root, args.symbols))
    build_environment = _build_environment(root, args.build_ninja, selected_configs)
    commit = _git(root, "rev-parse", "HEAD")
    return create_manifest(
        root,
        commit,
        selected,
        config,
        report,
        semantic_matches,
        semantic_data_matches,
        symbol_entries,
        build_environment,
    )


def main(argv: Optional[Sequence[str]] = None) -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    subparsers = parser.add_subparsers(dest="command", required=True)
    snapshot_parser = subparsers.add_parser("snapshot", parents=[_common_parser()])
    check_parser = subparsers.add_parser("check", parents=[_common_parser()])
    check_parser.add_argument(
        "--adjudications",
        type=Path,
        help=(
            "explicit one-shot reviewed evidence changes; omitted by default "
            "so ordinary checks remain fully strict"
        ),
    )
    explain_parser = subparsers.add_parser("explain")
    explain_parser.add_argument("unit")
    explain_parser.add_argument("item")
    explain_parser.add_argument("--manifest", type=Path, default=Path("build/regression_manifest.json"))
    args = parser.parse_args(argv)

    try:
        if args.command == "explain":
            baseline = load_baseline(args.manifest)
            unit = baseline.get("units", {}).get(args.unit)
            if unit is None:
                raise GateError(f"unit not present in baseline: {args.unit}")
            if args.item in unit["functions"]:
                payload = {
                    "kind": "function",
                    "comparison": unit["functions"][args.item],
                    "target": unit["target"]["functions"].get(args.item),
                    "base": unit["base"]["functions"].get(args.item),
                }
            elif args.item in unit["sections"]:
                payload = {
                    "kind": "section",
                    "comparison": unit["sections"][args.item],
                    "target": unit["target"]["non_code_sections"].get(args.item),
                    "base": unit["base"]["non_code_sections"].get(args.item),
                }
            else:
                raise GateError(
                    f"item not present in baseline: {args.unit}:{args.item}"
                )
            print(json.dumps(payload, indent=2, sort_keys=True))
            return 0

        root = args.root.resolve()
        config = _read_json(_resolve_path(root, args.config))
        available = _unit_configs(config)

        if args.command == "snapshot":
            selected = _select_units(available, args.unit, args.all)
            commit = require_clean_commit(root)
            manifest = _capture_from_cli(args, selected, config)
            manifest["commit"] = commit
            output = _resolve_path(root, args.manifest)
            _write_json(output, manifest)
            print(
                json.dumps(
                    {
                        "status": "SNAPSHOT_WRITTEN",
                        "manifest": str(output),
                        "commit": commit,
                        "units": selected,
                    },
                    indent=2,
                )
            )
            return 0

        baseline_path = _resolve_path(root, args.manifest)
        baseline = load_baseline(baseline_path)
        baseline_names = sorted(baseline["units"])
        if args.unit or args.all:
            selected = _select_units(available, args.unit, args.all)
            if set(selected) != set(baseline_names):
                raise GateError(
                    "check selection must exactly match the baseline unit set"
                )
        else:
            selected = baseline_names
        current = _capture_from_cli(args, selected, config, baseline)
        adjudications = None
        if args.adjudications is not None:
            adjudications = _read_json(
                _resolve_path(root, args.adjudications)
            )
        comparison = compare_manifests(
            baseline, current, adjudications=adjudications
        )
        print(json.dumps(comparison, indent=2, sort_keys=True))
        return 0 if comparison["ok"] else 1
    except GateError as error:
        print(f"regression gate error: {error}", file=sys.stderr)
        return 2


if __name__ == "__main__":
    raise SystemExit(main())
