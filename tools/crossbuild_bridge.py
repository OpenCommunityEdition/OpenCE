"""January 2002 <-> HCEA (HCEX June 2011) cross-build reconstruction bridge.

This tool joins the January Xbox target inventory with the HCEA Xbox 360
prototype PDB evidence and emits:

1. a conservative correspondence census with per-tier function/byte/object
   counts (``census`` subcommand);
2. hash-pinned, provenance-graded implementation packets for individual
   unresolved January functions (``packet`` subcommand); and
3. a ranked execution queue over the exact tier (``rank`` subcommand).

Authority model
---------------

HCEA derives from Halo PC and contains later modifications.  Nothing in this
tool promotes HCEA names, types, parameters, or behavior to January truth.
Every emitted fact carries one of the provenance grades below, and consumers
must treat anything other than ``JANUARY_PROVEN`` as a hypothesis to verify
against January machine evidence.

The HCEA PDB is read through a DIA2Dump ``-s`` text dump whose SHA-256 is
pinned into every output.  PDB RVA/length ranges are deliberately NOT used to
slice PowerPC bytes: prior research showed separated/reordered code makes
linear [RVA, RVA+len) disassembly unsafe without an independent ownership
proof, so HCEA machine-code bodies are reported ``UNAVAILABLE``.
"""

import argparse
import hashlib
import json
import re
from collections import defaultdict
from pathlib import Path

from tools.coff_compare import CoffError, load, section_info

# -- provenance grades -------------------------------------------------------

JANUARY_PROVEN = "JANUARY_PROVEN"
CROSSBUILD_CORROBORATED = "CROSSBUILD_CORROBORATED"
CROSSBUILD_ONLY = "CROSSBUILD_ONLY"
INFERRED = "INFERRED"
CONFLICT = "CONFLICT"
UNAVAILABLE = "UNAVAILABLE"

PROVENANCE_GRADES = (
    JANUARY_PROVEN,
    CROSSBUILD_CORROBORATED,
    CROSSBUILD_ONLY,
    INFERRED,
    CONFLICT,
    UNAVAILABLE,
)

# -- correspondence tiers ----------------------------------------------------

TIER_EXACT_UNIQUE = "exact_unique"
TIER_EXACT_UNIQUE_CROSSOBJECT = "exact_unique_crossobject"
TIER_ALIAS_FAMILY = "alias_family"
TIER_AMBIGUOUS = "ambiguous"
TIER_JANUARY_ONLY = "january_only"
TIER_KNOWN_EVOLVED = "known_evolved"

SCHEMA_VERSION = "1.0.0"


class BridgeError(RuntimeError):
    pass


# -- symbol normalization ----------------------------------------------------

_STDCALL_RE = re.compile(r"^_(?P<name>[A-Za-z_$][A-Za-z0-9_$]*)@(?P<bytes>\d+)$")
_FASTCALL_RE = re.compile(r"^@(?P<name>[A-Za-z_$][A-Za-z0-9_$]*)@(?P<bytes>\d+)$")
_ANONYMOUS_RE = re.compile(r"^_?(code|data|bss|rdata)_[0-9a-f]{6,8}$")


def normalize_coff_symbol(symbol):
    """Return (normalized_name, calling_convention, arg_bytes_or_None).

    January COFF decoration only: ``_name`` cdecl, ``_name@N`` stdcall,
    ``@name@N`` fastcall.  C++ manglings are returned unchanged with
    convention ``c++`` so they can never collide with C identifiers.
    """
    if symbol.startswith("?"):
        return symbol, "c++", None
    match = _FASTCALL_RE.match(symbol)
    if match:
        return match.group("name"), "fastcall", int(match.group("bytes"))
    match = _STDCALL_RE.match(symbol)
    if match:
        return match.group("name"), "stdcall", int(match.group("bytes"))
    if symbol.startswith("_"):
        return symbol[1:], "cdecl", None
    return symbol, "none", None


def is_anonymous_symbol(symbol):
    return bool(_ANONYMOUS_RE.match(symbol))


# -- DIA dump parsing --------------------------------------------------------

_MODULE_RE = re.compile(r"^\*\* Module: (?P<name>.+?)\s*$")
_ENV_RE = re.compile(r'^CompilandEnv\s*:\s*(?P<key>\w+) = "(?P<value>.*)"')
_FUNCTION_RE = re.compile(
    r"^Function\s+:\s+(?P<scope>\w*),?\s*"
    r"\[(?P<rva>[0-9A-Fa-f]{8})\]\[(?P<seg>[0-9A-Fa-f]{4}):(?P<off>[0-9A-Fa-f]{8})\],\s+"
    r"len = (?P<len>[0-9A-Fa-f]{8}),\s+(?P<name>.+?)\s*$"
)
_PARAM_RE = re.compile(
    r"^Data\s+:\s+(?P<loc>.+?),\s+Param,\s+Type:\s+(?P<type>.+?),\s+(?P<name>\S+)\s*$"
)
_LOCAL_RE = re.compile(
    r"^Data\s+:\s+(?P<loc>.+?),\s+Local,\s+Type:\s+(?P<type>.+?),\s+(?P<name>\S+)\s*$"
)
_VERSION_RE = re.compile(
    r"^\t(?P<end>Frontend|Backend) Version: Major = (?P<major>\d+), "
    r"Minor = (?P<minor>\d+), Build = (?P<build>\d+)"
)
_LANGUAGE_RE = re.compile(r"^\tLanguage: (?P<language>.+?)\s*$")


def parse_dia_dump(path):
    """Parse a DIA2Dump ``-s`` dump into per-module function records.

    The parser is line-oriented and permissive about record types it does not
    understand, but strict about the records it consumes: a Function line that
    fails to parse raises, rather than silently dropping evidence.
    """
    modules = []
    current = None
    current_function = None
    with open(path, "r", encoding="utf-8", errors="replace") as handle:
        for line in handle:
            line = line.rstrip("\n")
            module_match = _MODULE_RE.match(line)
            if module_match:
                current = {
                    "module": module_match.group("name"),
                    "env": {},
                    "language": None,
                    "frontend_version": None,
                    "backend_version": None,
                    "functions": [],
                }
                modules.append(current)
                current_function = None
                continue
            if current is None:
                continue
            env_match = _ENV_RE.match(line)
            if env_match:
                current["env"][env_match.group("key")] = env_match.group("value")
                continue
            language_match = _LANGUAGE_RE.match(line)
            if language_match:
                current["language"] = language_match.group("language")
                continue
            version_match = _VERSION_RE.match(line)
            if version_match:
                key = ("frontend_version"
                       if version_match.group("end") == "Frontend"
                       else "backend_version")
                current[key] = "%s.%s.%s" % (
                    version_match.group("major"),
                    version_match.group("minor"),
                    version_match.group("build"),
                )
                continue
            if line.startswith("Function "):
                function_match = _FUNCTION_RE.match(line)
                if not function_match:
                    raise BridgeError(
                        "unparsed DIA Function record in %s: %r"
                        % (current["module"], line))
                current_function = {
                    "name": function_match.group("name"),
                    "scope": function_match.group("scope") or "public",
                    "rva": int(function_match.group("rva"), 16),
                    "section": int(function_match.group("seg"), 16),
                    "offset": int(function_match.group("off"), 16),
                    "length": int(function_match.group("len"), 16),
                    "params": [],
                    "locals": [],
                }
                current["functions"].append(current_function)
                continue
            if current_function is not None:
                param_match = _PARAM_RE.match(line)
                if param_match:
                    current_function["params"].append({
                        "name": param_match.group("name"),
                        "type": param_match.group("type"),
                        "location": param_match.group("loc").strip(),
                    })
                    continue
                local_match = _LOCAL_RE.match(line)
                if local_match:
                    current_function["locals"].append({
                        "name": local_match.group("name"),
                        "type": local_match.group("type"),
                        "location": local_match.group("loc").strip(),
                    })
                    continue
                if not line.strip():
                    current_function = None
    return modules


_HCEA_SOURCE_RE = re.compile(r"sources[\\/](?P<rel>.+?)\.(c|cpp|cxx)$", re.IGNORECASE)


def hcea_unit_relpath(module):
    """Map an HCEA compiland to a repository-style unit relpath, or None.

    Only compilands whose recorded source file sits under the original
    ``sources`` tree participate; MCC/engine glue (halo_*.obj under
    ReleaseFCall etc.) deliberately maps to None so it can never join by
    object identity.
    """
    source = module["env"].get("src", "")
    match = _HCEA_SOURCE_RE.search(source)
    if not match:
        return None
    rel = match.group("rel").replace("\\", "/").lower()
    return rel


def january_unit_relpath(unit_name):
    if not unit_name.startswith("source/"):
        return None
    return unit_name[len("source/"):].lower()


# -- January inventory -------------------------------------------------------

def load_january_inventory(repo_root):
    """Build the January function inventory from the ordinary and semantic
    reports plus the accepted strict ledger."""
    repo_root = Path(repo_root)
    report = json.loads((repo_root / "build" / "report.json").read_text(encoding="utf-8"))
    semantic = json.loads(
        (repo_root / "build" / "semantic_report.json").read_text(encoding="utf-8"))
    accepted = {(entry["unit"], entry["function"])
                for entry in semantic["accepted_ledger"]}
    units = []
    for unit in report.get("units", []):
        categories = unit.get("metadata", {}).get("progress_categories", [])
        if "halobetacache" not in categories:
            continue
        functions = []
        for function in unit.get("functions") or []:
            key = (unit["name"], function["name"])
            functions.append({
                "symbol": function["name"],
                "size": int(function["size"]),
                "fuzzy_match_percent": function.get("fuzzy_match_percent"),
                "accepted_exact": key in accepted,
            })
        units.append({
            "unit": unit["name"],
            "complete": bool(unit.get("metadata", {}).get("complete")),
            "functions": functions,
        })
    return units


# -- correspondence ----------------------------------------------------------

def build_correspondence(january_units, hcea_modules, known_evolved=None):
    """Join January and HCEA function populations conservatively.

    Uniqueness is computed globally on each side (multiple definitions of one
    name on either side poison the exact tier for that name).  Object-pair
    agreement separates ``exact_unique`` from ``exact_unique_crossobject``.
    """
    known_evolved = known_evolved or {}

    january_defs = defaultdict(list)
    for unit in january_units:
        unit_rel = january_unit_relpath(unit["unit"])
        for function in unit["functions"]:
            symbol = function["symbol"]
            if is_anonymous_symbol(symbol):
                continue
            normalized, convention, _ = normalize_coff_symbol(symbol)
            if convention == "c++":
                continue
            january_defs[normalized].append((unit, unit_rel, function))

    hcea_defs = defaultdict(list)
    hcea_units = set()
    for module in hcea_modules:
        rel = hcea_unit_relpath(module)
        if rel is None:
            continue
        hcea_units.add(rel)
        for function in module["functions"]:
            hcea_defs[function["name"]].append((module, rel, function))

    mappings = []
    for unit in january_units:
        unit_rel = january_unit_relpath(unit["unit"])
        for function in unit["functions"]:
            if function["accepted_exact"]:
                continue
            symbol = function["symbol"]
            record = {
                "january_unit": unit["unit"],
                "january_symbol": symbol,
                "january_size": function["size"],
                "implemented_nonexact": bool(
                    function["fuzzy_match_percent"] not in (None, 0, 0.0)),
            }
            if is_anonymous_symbol(symbol):
                record.update({"tier": TIER_JANUARY_ONLY,
                               "reason": "anonymous-symbol"})
                mappings.append(record)
                continue
            normalized, convention, arg_bytes = normalize_coff_symbol(symbol)
            if convention == "c++":
                record.update({"tier": TIER_JANUARY_ONLY,
                               "reason": "c++-mangled"})
                mappings.append(record)
                continue
            record["normalized_name"] = normalized
            record["calling_convention"] = convention
            if arg_bytes is not None:
                record["stdcall_arg_bytes"] = arg_bytes
            january_count = len(january_defs.get(normalized, []))
            hcea_matches = hcea_defs.get(normalized, [])
            if normalized in known_evolved:
                record.update({
                    "tier": TIER_KNOWN_EVOLVED,
                    "reason": known_evolved[normalized],
                    "hcea_candidates": len(hcea_matches),
                })
                mappings.append(record)
                continue
            if not hcea_matches:
                record.update({"tier": TIER_JANUARY_ONLY,
                               "reason": "no-hcea-name"})
                mappings.append(record)
                continue
            if january_count == 1 and len(hcea_matches) == 1:
                module, hcea_rel, hcea_function = hcea_matches[0]
                record["hcea_module"] = module["module"]
                record["hcea_source"] = module["env"].get("src")
                record["hcea_scope"] = hcea_function["scope"]
                record["hcea_length"] = hcea_function["length"]
                record["hcea_param_count"] = len(hcea_function["params"])
                if hcea_rel == unit_rel:
                    record["tier"] = TIER_EXACT_UNIQUE
                else:
                    record["tier"] = TIER_EXACT_UNIQUE_CROSSOBJECT
                    record["hcea_unit_relpath"] = hcea_rel
                mappings.append(record)
                continue
            same_object = [item for item in hcea_matches if item[1] == unit_rel]
            if january_count == 1 and len(same_object) == 1:
                module, hcea_rel, hcea_function = same_object[0]
                record.update({
                    "tier": TIER_ALIAS_FAMILY,
                    "reason": "hcea-name-multiply-defined; unique in paired object",
                    "hcea_module": module["module"],
                    "hcea_source": module["env"].get("src"),
                    "hcea_scope": hcea_function["scope"],
                    "hcea_length": hcea_function["length"],
                    "hcea_param_count": len(hcea_function["params"]),
                    "hcea_total_candidates": len(hcea_matches),
                })
                mappings.append(record)
                continue
            record.update({
                "tier": TIER_AMBIGUOUS,
                "reason": "january-definitions=%d hcea-definitions=%d" % (
                    january_count, len(hcea_matches)),
            })
            mappings.append(record)

    hcea_only = []
    january_names = set(january_defs)
    for name, matches in sorted(hcea_defs.items()):
        if name in january_names:
            continue
        for module, rel, function in matches:
            hcea_only.append({
                "hcea_name": name,
                "hcea_module": module["module"],
                "hcea_unit_relpath": rel,
                "hcea_length": function["length"],
            })

    return {
        "mappings": mappings,
        "hcea_only_functions": hcea_only,
        "january_units": sorted(
            {january_unit_relpath(unit["unit"]) for unit in january_units
             if january_unit_relpath(unit["unit"])}),
        "hcea_units": sorted(hcea_units),
    }


def summarize_census(correspondence):
    tiers = defaultdict(lambda: {"functions": 0, "bytes": 0, "objects": set()})
    for record in correspondence["mappings"]:
        bucket = tiers[record["tier"]]
        bucket["functions"] += 1
        bucket["bytes"] += record["january_size"]
        bucket["objects"].add(record["january_unit"])
    summary = {}
    for tier, bucket in sorted(tiers.items()):
        summary[tier] = {
            "functions": bucket["functions"],
            "january_code_bytes": bucket["bytes"],
            "objects": len(bucket["objects"]),
        }
    return summary


# -- packet emission ---------------------------------------------------------

def _sha256_file(path):
    digest = hashlib.sha256()
    with open(path, "rb") as handle:
        for chunk in iter(lambda: handle.read(1 << 20), b""):
            digest.update(chunk)
    return digest.hexdigest()


def _read_c_string(obj, section_number, offset):
    if section_number <= 0 or section_number > len(obj["sections"]):
        return None
    section = obj["sections"][section_number - 1]
    if section["raw"] == 0:
        return None
    data = obj["data"][section["raw"]:section["raw"] + section["size"]]
    if offset < 0 or offset >= len(data):
        return None
    end = data.find(b"\0", offset)
    if end == -1:
        return None
    raw = data[offset:end]
    if not raw:
        return None
    try:
        text = raw.decode("ascii")
    except UnicodeDecodeError:
        return None
    if any(ord(ch) < 0x20 and ch not in "\t\r\n" for ch in text):
        return None
    return text


def january_function_evidence(repo_root, unit, symbol):
    """Extract January-proven facts for one function from the split object."""
    repo_root = Path(repo_root)
    object_path = repo_root / "build" / "split" / (unit + ".obj")
    if not object_path.exists():
        raise BridgeError("missing split object: %s" % object_path)
    obj = load(object_path)
    info = section_info(obj, symbol)
    function_symbol = None
    for item in obj["symbols"]:
        if item["name"] == symbol and item["section"] > 0:
            function_symbol = item
            break
    section = obj["sections"][function_symbol["section"] - 1]
    raw = obj["data"][section["raw"]:section["raw"] + section["size"]]

    by_name = defaultdict(list)
    for item in obj["symbols"]:
        if item["section"] > 0:
            by_name[item["name"]].append(item)

    callees = []
    strings = []
    data_refs = []
    for relocation in info["relocations"]:
        target = relocation.get("symbolic_target") or relocation["target"]
        if target[0] == "internal":
            continue
        if target[0] == "symbol":
            name, addend = target[1], target[2]
            resolved = by_name.get(name, [])
            text = None
            if len(resolved) == 1:
                candidate = resolved[0]
                owner_section = obj["sections"][candidate["section"] - 1]
                if not owner_section["flags"] & 0x20:
                    text = _read_c_string(
                        obj, candidate["section"], candidate["value"] + addend)
            entry = {
                "address": relocation["address"],
                "type": relocation["type"],
                "symbol": name,
                "addend": addend,
            }
            if text is not None:
                entry["string"] = text
                strings.append(entry)
            elif relocation["type"] == 0x14:
                callees.append(entry)
            else:
                data_refs.append(entry)
        elif target[0] == "defined-noncode":
            entry = {
                "address": relocation["address"],
                "type": relocation["type"],
                "section": target[1],
                "anchor": target[2],
                "offset": target[3],
            }
            anchor_symbols = by_name.get(target[2], [])
            if len(anchor_symbols) == 1:
                text = _read_c_string(
                    obj, anchor_symbols[0]["section"],
                    anchor_symbols[0]["value"] + target[3])
                if text is not None:
                    entry["string"] = text
                    strings.append(entry)
                    continue
            data_refs.append(entry)
        else:
            data_refs.append({
                "address": relocation["address"],
                "type": relocation["type"],
                "target": target,
            })

    return {
        "object": unit + ".obj",
        "symbol": symbol,
        "padded_size": info["size"],
        "relocation_count": info["relocation_count"],
        "normalized_sha256": info["normalized_sha256"],
        "relocations": info["relocations"],
        "code_bytes_hex": raw.hex(),
        "callees": callees,
        "strings": strings,
        "data_references": data_refs,
        "provenance": JANUARY_PROVEN,
    }


def january_callers(repo_root, symbol, units):
    """Scan all January split objects for relocations targeting *symbol*."""
    repo_root = Path(repo_root)
    callers = []
    for unit in units:
        object_path = repo_root / "build" / "split" / (unit["unit"] + ".obj")
        if not object_path.exists():
            continue
        try:
            obj = load(object_path)
        except CoffError:
            continue
        names = {item["name"] for item in obj["symbols"] if item["section"] == 0}
        if symbol in names:
            callers.append(unit["unit"])
    return callers


def find_repo_declaration(repo_root, normalized_name):
    """Locate declarations of *normalized_name* in repository headers.

    Returns a list of {header, line, text, sha256}.  Multiple or zero
    declarations are reported as-is; the packet consumer decides.
    """
    repo_root = Path(repo_root)
    pattern = re.compile(
        r"^[^#/].*?\b%s\s*\(" % re.escape(normalized_name))
    results = []
    for header in sorted((repo_root / "source").rglob("*.h")):
        try:
            text = header.read_text(encoding="utf-8", errors="replace")
        except OSError:
            continue
        if normalized_name not in text:
            continue
        digest = hashlib.sha256(text.encode("utf-8", "replace")).hexdigest()
        for line_number, line in enumerate(text.splitlines(), 1):
            if pattern.match(line.strip()) and ";" in line:
                results.append({
                    "header": str(header.relative_to(repo_root)).replace("\\", "/"),
                    "line": line_number,
                    "text": line.strip(),
                    "header_sha256": digest,
                })
    return results


def build_packet(repo_root, census, dia_dump_path, january_unit, january_symbol,
                 hcea_modules=None):
    """Emit one hash-pinned implementation packet."""
    repo_root = Path(repo_root)
    mapping = None
    for record in census["mappings"]:
        if record["january_unit"] == january_unit \
                and record["january_symbol"] == january_symbol:
            mapping = record
            break
    if mapping is None:
        raise BridgeError(
            "no census mapping for %s:%s" % (january_unit, january_symbol))

    evidence = january_function_evidence(repo_root, january_unit, january_symbol)

    hcea = {"provenance": UNAVAILABLE}
    if mapping.get("hcea_module") and hcea_modules is not None:
        for module in hcea_modules:
            if module["module"] != mapping["hcea_module"]:
                continue
            for function in module["functions"]:
                if function["name"] == mapping.get("normalized_name"):
                    hcea = {
                        "module": module["module"],
                        "source": module["env"].get("src"),
                        "language": module["language"],
                        "frontend_version": module["frontend_version"],
                        "backend_version": module["backend_version"],
                        "scope": function["scope"],
                        "rva": function["rva"],
                        "length": function["length"],
                        "params": function["params"],
                        "locals": function["locals"],
                        "machine_code_body": UNAVAILABLE,
                        "machine_code_body_reason": (
                            "PDB RVA/length ranges are not proven to identify "
                            "contiguous PowerPC bodies; ownership unproven"),
                        "provenance": CROSSBUILD_ONLY,
                    }
                    break
            break

    normalized = mapping.get("normalized_name")
    declarations = find_repo_declaration(repo_root, normalized) if normalized else []

    abi = {"provenance": UNAVAILABLE, "comparison": []}
    if declarations and hcea.get("params") is not None:
        abi["provenance"] = INFERRED
        abi["repo_declarations"] = declarations
        abi["hcea_param_count"] = len(hcea["params"])
        abi["note"] = (
            "repo declaration is a January hypothesis; HCEA parameters are "
            "cross-build evidence; agreement upgrades fields to "
            "CROSSBUILD_CORROBORATED only after January disassembly review")
    elif hcea.get("params") is not None:
        abi["provenance"] = CROSSBUILD_ONLY
        abi["hcea_param_count"] = len(hcea["params"])

    packet = {
        "schema_version": SCHEMA_VERSION,
        "tier": mapping["tier"],
        "january": evidence,
        "january_meaningful_size": mapping["january_size"],
        "hcea": hcea,
        "abi": abi,
        "callers": january_callers(
            repo_root, january_symbol, load_january_inventory(repo_root)),
        "control_flow": {
            "provenance": UNAVAILABLE,
            "reason": "no stdlib x86 decoder; use research capstone lanes",
        },
        "acceptance": {
            "compiler": "XDK 3911 CL 13.00.9254.1",
            "flags": "/O2 /Oy- /DDEBUG /Dxbox",
            "commands": [
                "ninja build/base/%s.obj" % january_unit,
                "python -m tools.coff_compare build/split/%s.obj "
                "build/base/%s.obj %s" % (january_unit, january_unit,
                                          january_symbol),
                "python -m tools.regression_gate compare --unit %s" % january_unit,
            ],
            "standard": (
                "strict normalized byte equality; exact relocation "
                "address/type/destination/addend equality; exact data/BSS "
                "ownership; zero whole-TU regression"),
        },
        "inputs": {
            "dia_dump_sha256": _sha256_file(dia_dump_path),
            "split_object_sha256": _sha256_file(
                repo_root / "build" / "split" / (january_unit + ".obj")),
        },
        "unresolved_questions": [],
        "rejection_conditions": [
            "HCEA parameter set contradicts January call-site or disassembly evidence",
            "January relocation set cannot be produced by any legal-C shape",
            "object cannot reach whole-TU zero regression",
        ],
    }
    body = json.dumps(packet, indent=1, sort_keys=True)
    packet_id = hashlib.sha256(body.encode("utf-8")).hexdigest()
    return {"packet_sha256": packet_id, "packet": packet}


# -- ranking -----------------------------------------------------------------

def rank_exact_tier(census, january_units):
    """Rank exact-tier mappings by whole-object closure potential and size."""
    unresolved_by_unit = defaultdict(list)
    mapped_by_unit = defaultdict(list)
    for unit in january_units:
        for function in unit["functions"]:
            if not function["accepted_exact"]:
                unresolved_by_unit[unit["unit"]].append(function)
    for record in census["mappings"]:
        if record["tier"] in (TIER_EXACT_UNIQUE,):
            mapped_by_unit[record["january_unit"]].append(record)

    ranked = []
    for unit_name, records in mapped_by_unit.items():
        unresolved = unresolved_by_unit[unit_name]
        coverage = len(records) / len(unresolved) if unresolved else 0.0
        total_bytes = sum(record["january_size"] for record in records)
        for record in records:
            score = (
                coverage * 2.0
                + (1.0 if record["january_size"] <= 0x60 else 0.0)
                + (0.5 if record["january_size"] <= 0x100 else 0.0)
                + (0.25 if record.get("hcea_param_count", 9) <= 2 else 0.0)
            )
            ranked.append({
                "january_unit": unit_name,
                "january_symbol": record["january_symbol"],
                "january_size": record["january_size"],
                "object_unresolved_functions": len(unresolved),
                "object_mapped_functions": len(records),
                "object_closure_coverage": round(coverage, 3),
                "object_unresolved_bytes": sum(
                    function["size"] for function in unresolved),
                "object_mapped_bytes": total_bytes,
                "score": round(score, 3),
            })
    ranked.sort(key=lambda item: (-item["score"], item["january_size"]))
    return ranked


# -- CLI ---------------------------------------------------------------------

def _cmd_census(args):
    repo_root = Path(args.repo_root)
    january_units = load_january_inventory(repo_root)
    hcea_modules = parse_dia_dump(args.dia_dump)
    known_evolved = {}
    if args.known_evolved and Path(args.known_evolved).exists():
        known_evolved = json.loads(Path(args.known_evolved).read_text())
    correspondence = build_correspondence(
        january_units, hcea_modules, known_evolved)
    summary = summarize_census(correspondence)
    total_functions = sum(len(unit["functions"]) for unit in january_units)
    accepted = sum(
        1 for unit in january_units for function in unit["functions"]
        if function["accepted_exact"])
    census = {
        "schema_version": SCHEMA_VERSION,
        "inputs": {
            "dia_dump": str(args.dia_dump),
            "dia_dump_sha256": _sha256_file(args.dia_dump),
        },
        "january_totals": {
            "objects": len(january_units),
            "objects_matching": sum(
                1 for unit in january_units if unit["complete"]),
            "functions": total_functions,
            "accepted_exact_functions": accepted,
            "unresolved_functions": total_functions - accepted,
            "unresolved_code_bytes": sum(
                function["size"] for unit in january_units
                for function in unit["functions"]
                if not function["accepted_exact"]),
            "implemented_nonexact_functions": sum(
                1 for unit in january_units for function in unit["functions"]
                if not function["accepted_exact"]
                and function["fuzzy_match_percent"] not in (None, 0, 0.0)),
        },
        "hcea_totals": {
            "modules_total": len(hcea_modules),
            "halo_source_modules": sum(
                1 for module in hcea_modules
                if hcea_unit_relpath(module) is not None),
            "halo_source_functions": sum(
                len(module["functions"]) for module in hcea_modules
                if hcea_unit_relpath(module) is not None),
        },
        "tier_summary": summary,
    }
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    (output_dir / "fresh_crossbuild_census.json").write_text(
        json.dumps(census, indent=1, sort_keys=True), encoding="utf-8")
    (output_dir / "hcea_january_correspondence.json").write_text(
        json.dumps(correspondence, indent=1, sort_keys=True), encoding="utf-8")
    print(json.dumps(census["january_totals"], indent=1))
    print(json.dumps(census["tier_summary"], indent=1))
    return 0


def _cmd_packet(args):
    repo_root = Path(args.repo_root)
    correspondence = json.loads(Path(args.correspondence).read_text())
    hcea_modules = parse_dia_dump(args.dia_dump)
    result = build_packet(
        repo_root, correspondence, args.dia_dump,
        args.unit, args.symbol, hcea_modules)
    output_dir = Path(args.output_dir)
    output_dir.mkdir(parents=True, exist_ok=True)
    safe_name = args.symbol.strip("_").replace("@", "_")
    path = output_dir / ("packet_%s_%s.json" % (
        args.unit.replace("/", "_"), safe_name))
    path.write_text(json.dumps(result, indent=1, sort_keys=True),
                    encoding="utf-8")
    print(path)
    print("packet_sha256:", result["packet_sha256"])
    return 0


def _cmd_rank(args):
    repo_root = Path(args.repo_root)
    correspondence = json.loads(Path(args.correspondence).read_text())
    january_units = load_january_inventory(repo_root)
    ranked = rank_exact_tier(correspondence, january_units)
    output = Path(args.output)
    output.write_text(json.dumps(ranked, indent=1), encoding="utf-8")
    for item in ranked[:int(args.top)]:
        print("%6.3f %5d bytes %-40s %s" % (
            item["score"], item["january_size"], item["january_symbol"],
            item["january_unit"]))
    return 0


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    parser.add_argument("--repo-root", default=".")
    subparsers = parser.add_subparsers(dest="command", required=True)

    census_parser = subparsers.add_parser("census")
    census_parser.add_argument("--dia-dump", required=True)
    census_parser.add_argument("--output-dir", required=True)
    census_parser.add_argument("--known-evolved")
    census_parser.set_defaults(func=_cmd_census)

    packet_parser = subparsers.add_parser("packet")
    packet_parser.add_argument("--dia-dump", required=True)
    packet_parser.add_argument("--correspondence", required=True)
    packet_parser.add_argument("--unit", required=True)
    packet_parser.add_argument("--symbol", required=True)
    packet_parser.add_argument("--output-dir", required=True)
    packet_parser.set_defaults(func=_cmd_packet)

    rank_parser = subparsers.add_parser("rank")
    rank_parser.add_argument("--correspondence", required=True)
    rank_parser.add_argument("--output", required=True)
    rank_parser.add_argument("--top", default=40)
    rank_parser.set_defaults(func=_cmd_rank)

    args = parser.parse_args(argv)
    return args.func(args)


if __name__ == "__main__":
    raise SystemExit(main())
