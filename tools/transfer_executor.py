"""Fail-closed exact-shape transfer candidate and review-packet generator.

This module never modifies repository files and never decides exactness.  It
emits only an unapplied candidate patch outside the repository.  Acceptance is
reserved for the hardened comparator, whole-TU regression gate, and whole-object
data/BSS gate in a separate disposable build worktree.
"""

from __future__ import annotations

import argparse
import difflib
import hashlib
import json
import shutil
import re
import struct
import sys
import tempfile
from pathlib import Path
from typing import Any, Dict, List, Mapping, Optional, Sequence, Tuple

if __package__ in (None, ""):
    sys.path.insert(0, str(Path(__file__).resolve().parents[1]))

from tools.coff_compare import (
    CoffError,
    load,
    section_info,
    section_infos_equal,
)
from tools.regression_gate import GateError, _object_fingerprint


REQUIRED_PROOFS = (
    "sizes_equal",
    "meaningful_equal",
    "reloc_count_equal",
    "reloc_addr_type_equal",
    "storage_class_equal",
    "prologue_equal",
    "nontrivial",
    "substitution_consistent",
)
FORBIDDEN_CONSTANT_PREFIXES = (
    "??_C@",
    "__real@",
    "__xmm@",
    "__mask@",
)
CODE_SYMBOL_TYPE = 0x20
RELOC_ENTRY_SIZE = 10


class Refusal(RuntimeError):
    """Terminal, non-retriable refusal for one transfer pair."""

    def __init__(self, code: str, detail: str):
        super().__init__(f"{code}: {detail}")
        self.code = code
        self.detail = detail

    def as_dict(self) -> Dict[str, str]:
        return {"code": self.code, "detail": self.detail}


def _sha256_bytes(data: bytes) -> str:
    return hashlib.sha256(data).hexdigest()


def _json_bytes(value: Any) -> bytes:
    return (json.dumps(value, indent=2, sort_keys=True) + "\n").encode("utf-8")


def _inside(path: Path, root: Path) -> bool:
    path = path.resolve()
    root = root.resolve()
    return path == root or root in path.parents


def _checked_repo_path(root: Path, relative: str, suffix: str) -> Path:
    if not isinstance(relative, str) or not relative.startswith("source/"):
        raise Refusal("invalid-unit-path", f"unit is not a source path: {relative!r}")
    candidate = (root / (relative + suffix)).resolve()
    if not _inside(candidate, root):
        raise Refusal("invalid-unit-path", f"unit escapes repository: {relative!r}")
    return candidate


def _read_json(path: Path) -> Any:
    try:
        return json.loads(path.read_text(encoding="utf-8"))
    except (OSError, UnicodeError, json.JSONDecodeError) as error:
        raise Refusal("invalid-json", f"cannot read {path}: {error}") from error


def _select_pair(
    atlas: Any,
    recipient_unit: str,
    recipient_function: str,
    donor_unit: Optional[str] = None,
    donor_function: Optional[str] = None,
) -> Mapping[str, Any]:
    if not isinstance(atlas, list):
        raise Refusal("invalid-atlas", "top-level atlas value must be a list")
    matches: List[Mapping[str, Any]] = []
    for item in atlas:
        if not isinstance(item, dict):
            raise Refusal("invalid-atlas", "every atlas entry must be an object")
        donor = item.get("donor")
        recipient = item.get("recipient")
        if not isinstance(donor, dict) or not isinstance(recipient, dict):
            raise Refusal("invalid-atlas", "pair donor/recipient must be objects")
        if (
            recipient.get("unit") == recipient_unit
            and recipient.get("function") == recipient_function
            and (donor_unit is None or donor.get("unit") == donor_unit)
            and (donor_function is None or donor.get("function") == donor_function)
        ):
            matches.append(item)
    if not matches:
        raise Refusal(
            "pair-not-found",
            f"no pair targets {recipient_unit}:{recipient_function}",
        )
    if len(matches) != 1:
        raise Refusal(
            "ambiguous-pair",
            f"{len(matches)} pairs target {recipient_unit}:{recipient_function}",
        )
    return matches[0]


def _validate_pair_policy(pair: Mapping[str, Any]) -> None:
    if pair.get("safe") is not True:
        raise Refusal("unsafe-pair", "atlas pair is not explicitly safe")
    if pair.get("donor_source_resolved") is not True:
        raise Refusal(
            "donor-source-unresolved",
            "atlas does not explicitly mark donor source resolved",
        )
    recipient = pair.get("recipient", {})
    if recipient.get("status") != "ABSENT":
        raise Refusal(
            "recipient-not-absent",
            f"recipient status is {recipient.get('status')!r}, not 'ABSENT'",
        )
    proof = pair.get("proof")
    if not isinstance(proof, dict):
        raise Refusal("incomplete-proof", "proof must be an object")
    missing = [key for key in REQUIRED_PROOFS if proof.get(key) is not True]
    false_or_unknown = [key for key, value in proof.items() if value is not True]
    if missing or false_or_unknown:
        names = sorted(set(missing + false_or_unknown))
        raise Refusal("incomplete-proof", f"proofs are not true: {', '.join(names)}")
    meaningful = pair.get("meaningful")
    if not isinstance(meaningful, int) or isinstance(meaningful, bool):
        raise Refusal("invalid-meaningful-size", "meaningful size must be an integer")
    if meaningful <= 16:
        raise Refusal("stub", f"meaningful size {meaningful} is at most 16 bytes")
    substitutions = pair.get("substitutions")
    if not isinstance(substitutions, list):
        raise Refusal("invalid-atlas", "substitutions must be an ordered list")
    for event in substitutions:
        if (
            not isinstance(event, list)
            or len(event) != 2
            or not all(isinstance(value, str) for value in event)
        ):
            raise Refusal("invalid-atlas", f"invalid substitution event: {event!r}")


def _function_symbol(obj: Mapping[str, Any], name: str) -> Mapping[str, Any]:
    matches = [
        symbol
        for symbol in obj["symbols"]
        if symbol["name"] == name
        and int(symbol["section"]) > 0
        and int(symbol["type"]) == CODE_SYMBOL_TYPE
    ]
    if len(matches) != 1:
        raise Refusal(
            "target-symbol-ambiguous",
            f"expected one defined code symbol {name!r}, found {len(matches)}",
        )
    return matches[0]


def _function_raw(
    obj: Mapping[str, Any], symbol: Mapping[str, Any]
) -> Tuple[bytes, Mapping[str, Any]]:
    section_number = int(symbol["section"])
    if section_number <= 0 or section_number > len(obj["sections"]):
        raise Refusal("invalid-target-object", "function section number is invalid")
    section = obj["sections"][section_number - 1]
    start = int(section["raw"])
    size = int(section["size"])
    end = start + size
    if start <= 0 or end > len(obj["data"]):
        raise Refusal("invalid-target-object", "function bytes are unavailable")
    return bytes(obj["data"][start:end]), section


def _raw_relocations(
    obj: Mapping[str, Any], section: Mapping[str, Any], raw: bytes
) -> List[Dict[str, Any]]:
    result: List[Dict[str, Any]] = []
    for index in range(int(section["reloc_count"])):
        offset = int(section["reloc"]) + index * RELOC_ENTRY_SIZE
        if offset + RELOC_ENTRY_SIZE > len(obj["data"]):
            raise Refusal("invalid-target-object", "relocation table is truncated")
        address, target_index, relocation_type = struct.unpack_from(
            "<LLH", obj["data"], offset
        )
        target = obj["by_index"].get(target_index)
        if target is None:
            raise Refusal(
                "invalid-target-object",
                f"relocation {index} references missing symbol {target_index}",
            )
        if address + 4 > len(raw):
            raise Refusal(
                "invalid-target-object",
                f"relocation {index} lies outside function bytes",
            )
        result.append(
            {
                "index": index,
                "address": int(address),
                "type": int(relocation_type),
                "addend": int(struct.unpack_from("<i", raw, address)[0]),
                "target": target["name"],
                "target_type": int(target["type"]),
                "target_section": int(target["section"]),
            }
        )
    return result


def _meaningful_size(raw: bytes) -> int:
    size = len(raw)
    while size and raw[size - 1] in (0x90, 0xCC):
        size -= 1
    return size


def _load_target_function(path: Path, symbol_name: str) -> Dict[str, Any]:
    if not path.is_file():
        raise Refusal("target-object-missing", f"object does not exist: {path}")
    try:
        obj = load(path)
        info = section_info(obj, symbol_name)
    except (OSError, CoffError) as error:
        raise Refusal(
            "invalid-target-object",
            f"cannot load {path} / {symbol_name}: {error}",
        ) from error
    symbol = _function_symbol(obj, symbol_name)
    raw, section = _function_raw(obj, symbol)
    relocs = _raw_relocations(obj, section, raw)
    hardened_shape = [
        (int(item["address"]), int(item["type"]))
        for item in info["relocations"]
    ]
    raw_shape = [(item["address"], item["type"]) for item in relocs]
    if hardened_shape != raw_shape:
        raise Refusal(
            "invalid-target-object",
            "raw relocation order differs from hardened resolver order",
        )
    return {
        "path": str(path.resolve()),
        "sha256": _sha256_bytes(path.read_bytes()),
        "info": info,
        "symbol": dict(symbol),
        "raw": raw,
        "relocations": relocs,
        "meaningful": _meaningful_size(raw),
    }


def _is_constant_symbol(name: str) -> bool:
    return name.startswith(FORBIDDEN_CONSTANT_PREFIXES) or name.startswith("$")


def _c_identifier(symbol_name: str) -> str:
    if _is_constant_symbol(symbol_name):
        raise Refusal(
            "constant-substitution-forbidden",
            f"symbol is a compiler constant: {symbol_name!r}",
        )
    name = symbol_name
    if name.startswith("_"):
        name = name[1:]
    name = re.sub(r"@\d+$", "", name)
    if not re.fullmatch(r"[A-Za-z_][A-Za-z0-9_]*", name):
        raise Refusal(
            "unsupported-symbol-spelling",
            f"cannot map COFF symbol to one C identifier: {symbol_name!r}",
        )
    return name


def _derive_substitutions(
    pair: Mapping[str, Any],
    donor_target: Mapping[str, Any],
    recipient_target: Mapping[str, Any],
) -> Dict[str, Any]:
    donor_info = donor_target["info"]
    recipient_info = recipient_target["info"]
    if donor_info["size"] != recipient_info["size"]:
        raise Refusal("size-mismatch", "donor/recipient padded sizes differ")
    if donor_info["normalized_sha256"] != recipient_info["normalized_sha256"]:
        raise Refusal(
            "normalized-bytes-mismatch",
            "donor/recipient masked function bytes differ",
        )
    if donor_target["meaningful"] != recipient_target["meaningful"]:
        raise Refusal("meaningful-size-mismatch", "measured meaningful sizes differ")
    if donor_target["meaningful"] != pair["meaningful"]:
        raise Refusal(
            "stale-atlas",
            "atlas meaningful size differs from target object measurement",
        )
    if donor_target["meaningful"] <= 16:
        raise Refusal("stub", "measured target function is a stub")
    if donor_info["size"] != pair.get("size"):
        raise Refusal("stale-atlas", "atlas padded size differs from target object")
    if donor_target["symbol"]["storage"] != recipient_target["symbol"]["storage"]:
        raise Refusal("storage-class-mismatch", "function storage classes differ")
    if donor_target["raw"][:4] != recipient_target["raw"][:4]:
        raise Refusal("prologue-mismatch", "first four target bytes differ")

    donor_relocs = donor_target["relocations"]
    recipient_relocs = recipient_target["relocations"]
    if len(donor_relocs) != len(recipient_relocs):
        raise Refusal("relocation-count-mismatch", "target relocation counts differ")

    events: List[List[str]] = []
    mapping: Dict[str, str] = {}
    reverse: Dict[str, str] = {}
    recipient_symbols: Dict[str, str] = {}
    for donor, recipient in zip(donor_relocs, recipient_relocs):
        if (
            donor["address"] != recipient["address"]
            or donor["type"] != recipient["type"]
        ):
            raise Refusal(
                "statement-order-mismatch",
                "ordered target relocation address/type sequence differs",
            )
        if donor["addend"] != recipient["addend"]:
            raise Refusal(
                "relocation-addend-mismatch",
                "relocation addends differ; constant substitution is forbidden",
            )
        donor_name = donor["target"]
        recipient_name = recipient["target"]
        kind = (
            "function"
            if recipient["type"] == 0x14
            or recipient["target_type"] == CODE_SYMBOL_TYPE
            else "data"
        )
        if not _is_constant_symbol(recipient_name):
            recipient_symbols[recipient_name] = kind
        if donor_name == recipient_name:
            continue
        donor_identifier = _c_identifier(donor_name)
        recipient_identifier = _c_identifier(recipient_name)
        if mapping.get(donor_identifier, recipient_identifier) != recipient_identifier:
            raise Refusal(
                "substitution-non-injective",
                f"{donor_name!r} maps to more than one recipient symbol",
            )
        if reverse.get(recipient_identifier, donor_identifier) != donor_identifier:
            raise Refusal(
                "substitution-non-injective",
                f"more than one donor symbol maps to {recipient_name!r}",
            )
        mapping[donor_identifier] = recipient_identifier
        reverse[recipient_identifier] = donor_identifier
        events.append([donor_name, recipient_name])

    if events != pair["substitutions"]:
        raise Refusal(
            "wrong-callee-or-stale-atlas",
            "atlas substitution events do not exactly match recipient target relocations",
        )
    if pair.get("class") == "IDENTICAL-INCLUDING-TARGETS" and events:
        raise Refusal(
            "invalid-zero-substitution-class",
            "zero-substitution class produced target-derived substitutions",
        )
    return {
        "events": events,
        "identifier_map": mapping,
        "recipient_symbols": recipient_symbols,
        "donor_relocation_counts": {
            identifier: count
            for identifier, count in sorted(
                {
                    _c_identifier(relocation["target"]): sum(
                        1
                        for candidate in donor_relocs
                        if not _is_constant_symbol(candidate["target"])
                        and _c_identifier(candidate["target"])
                        == _c_identifier(relocation["target"])
                    )
                    for relocation in donor_relocs
                    if not _is_constant_symbol(relocation["target"])
                }.items()
            )
        },
    }


def _mask_noncode(text: str) -> str:
    """Replace comments and string/character contents while preserving offsets."""
    chars = list(text)
    index = 0
    state = "normal"
    while index < len(chars):
        char = chars[index]
        following = chars[index + 1] if index + 1 < len(chars) else ""
        if state == "normal":
            if char == "/" and following == "/":
                chars[index] = chars[index + 1] = " "
                index += 2
                state = "line"
                continue
            if char == "/" and following == "*":
                chars[index] = chars[index + 1] = " "
                index += 2
                state = "block"
                continue
            if char == '"':
                chars[index] = " "
                index += 1
                state = "string"
                continue
            if char == "'":
                chars[index] = " "
                index += 1
                state = "character"
                continue
        elif state == "line":
            if char == "\n":
                state = "normal"
            else:
                chars[index] = " "
            index += 1
            continue
        elif state == "block":
            if char == "*" and following == "/":
                chars[index] = chars[index + 1] = " "
                index += 2
                state = "normal"
                continue
            if char != "\n":
                chars[index] = " "
            index += 1
            continue
        else:
            if char == "\\" and following:
                chars[index] = " "
                if following != "\n":
                    chars[index + 1] = " "
                index += 2
                continue
            quote = '"' if state == "string" else "'"
            if char == quote:
                chars[index] = " "
                state = "normal"
            elif char != "\n":
                chars[index] = " "
            index += 1
            continue
        index += 1
    return "".join(chars)


def _matching_delimiter(masked: str, start: int, opening: str, closing: str) -> int:
    depth = 0
    for index in range(start, len(masked)):
        if masked[index] == opening:
            depth += 1
        elif masked[index] == closing:
            depth -= 1
            if depth == 0:
                return index
    raise Refusal("donor-source-ambiguous", f"unclosed {opening!r} delimiter")


def _definition_start(masked: str, identifier_start: int) -> int:
    line_start = masked.rfind("\n", 0, identifier_start) + 1
    prefix = masked[line_start:identifier_start]
    if prefix.strip():
        return line_start
    previous_end = line_start - 1
    while previous_end > 0:
        previous_start = masked.rfind("\n", 0, previous_end) + 1
        if masked[previous_start:previous_end].strip():
            return previous_start
        previous_end = previous_start - 1
    return line_start


def _function_definitions(text: str, identifier: str) -> List[Tuple[int, int]]:
    masked = _mask_noncode(text)
    token = re.compile(r"\b" + re.escape(identifier) + r"\b")
    definitions: List[Tuple[int, int]] = []
    for match in token.finditer(masked):
        index = match.end()
        while index < len(masked) and masked[index].isspace():
            index += 1
        if index >= len(masked) or masked[index] != "(":
            continue
        close_parenthesis = _matching_delimiter(masked, index, "(", ")")
        after = close_parenthesis + 1
        while after < len(masked) and masked[after].isspace():
            after += 1
        if after >= len(masked) or masked[after] != "{":
            continue
        start = _definition_start(masked, match.start())
        prefix = masked[start:match.start()]
        if "#" in prefix or not prefix.strip():
            continue
        close_brace = _matching_delimiter(masked, after, "{", "}")
        definitions.append((start, close_brace + 1))
    return definitions


def _extract_donor_source(path: Path, symbol_name: str) -> Dict[str, Any]:
    if not path.is_file():
        raise Refusal("donor-source-missing", f"donor TU does not exist: {path}")
    try:
        raw = path.read_bytes()
        text = raw.decode("utf-8")
    except (OSError, UnicodeError) as error:
        raise Refusal("donor-source-missing", f"cannot read donor TU: {error}") from error
    identifier = _c_identifier(symbol_name)
    definitions = _function_definitions(text, identifier)
    if not definitions:
        raise Refusal(
            "donor-source-missing",
            f"{identifier!r} is not a function definition in {path}",
        )
    if len(definitions) != 1:
        raise Refusal(
            "donor-source-ambiguous",
            f"found {len(definitions)} definitions of {identifier!r}",
        )
    start, end = definitions[0]
    source = text[start:end]
    occurrences = len(
        re.findall(r"\b" + re.escape(identifier) + r"\b", _mask_noncode(source))
    )
    if occurrences != 1:
        raise Refusal(
            "donor-source-ambiguous",
            "recursive or multiply-mentioned donor function name is unsupported",
        )
    return {
        "path": str(path.resolve()),
        "sha256": _sha256_bytes(raw),
        "identifier": identifier,
        "source": source,
        "start_line": text.count("\n", 0, start) + 1,
        "end_line": text.count("\n", 0, end) + 1,
    }


def _canonical_c_type(text: str, *, parameter: bool) -> str:
    value = _mask_noncode(text).strip()
    if parameter and ("(" in value or ")" in value):
        raise Refusal(
            "prototype-proof-unsupported",
            f"function-pointer parameter cannot be proven safely: {text!r}",
        )
    value = re.sub(
        r"\b(static|extern|inline|__inline|__forceinline)\b",
        " ",
        value,
    )
    if parameter and "[" in value:
        raise Refusal(
            "prototype-proof-unsupported",
            f"array parameter cannot be proven safely: {text!r}",
        )
    type_words = {
        "void", "char", "short", "int", "long", "float", "double",
        "signed", "unsigned", "const", "volatile", "struct", "union",
        "enum", "boolean", "real",
    }
    if parameter and value != "...":
        matches = list(re.finditer(r"\b[A-Za-z_][A-Za-z0-9_]*\b", value))
        if len(matches) >= 2:
            last = matches[-1]
            suffix = value[last.end():].strip()
            if last.group() not in type_words and not suffix:
                value = value[:last.start()] + value[last.end():]
    value = re.sub(r"\s+", " ", value).strip()
    value = re.sub(r"\s*\*\s*", "*", value)
    value = re.sub(r"\s*,\s*", ",", value)
    if not value:
        raise Refusal("prototype-proof-unsupported", "empty canonical C type")
    return value


def _signature_records(
    text: str, identifier: str, source: str
) -> List[Dict[str, Any]]:
    masked = _mask_noncode(text)
    token = re.compile(r"\b" + re.escape(identifier) + r"\b")
    records: List[Dict[str, Any]] = []
    for match in token.finditer(masked):
        opening = match.end()
        while opening < len(masked) and masked[opening].isspace():
            opening += 1
        if opening >= len(masked) or masked[opening] != "(":
            continue
        closing = _matching_delimiter(masked, opening, "(", ")")
        after = closing + 1
        while after < len(masked) and masked[after].isspace():
            after += 1
        if after >= len(masked) or masked[after] not in ";{":
            continue
        start = _definition_start(masked, match.start())
        return_text = text[start:match.start()].strip()
        masked_return = masked[start:match.start()].strip()
        if (
            not masked_return
            or masked_return.startswith("#")
            or any(character in masked_return for character in "{}=")
            or re.search(
                r"\b(if|for|while|switch|return|sizeof)\s*$",
                masked_return,
            )
        ):
            continue
        parameter_text = text[opening + 1:closing]
        parameters: List[str] = []
        part_start = 0
        depth = 0
        for index, character in enumerate(_mask_noncode(parameter_text)):
            if character in "([":
                depth += 1
            elif character in ")]":
                depth -= 1
            elif character == "," and depth == 0:
                parameters.append(parameter_text[part_start:index])
                part_start = index + 1
        parameters.append(parameter_text[part_start:])
        canonical_parameters = [
            _canonical_c_type(parameter, parameter=True)
            for parameter in parameters
            if parameter.strip() and parameter.strip() != "void"
        ]
        records.append(
            {
                "source": source,
                "line": text.count("\n", 0, start) + 1,
                "definition": masked[after] == "{",
                "return_type": _canonical_c_type(return_text, parameter=False),
                "parameter_types": canonical_parameters,
                "text": text[start:closing + 1].strip(),
            }
        )
    return records


def _prove_prototype_compatibility(
    repo_root: Path,
    recipient_path: Path,
    donor_source: Mapping[str, Any],
    donor_function: str,
    recipient_function: str,
) -> Dict[str, Any]:
    donor_identifier = _c_identifier(donor_function)
    recipient_identifier = _c_identifier(recipient_function)
    donor_records = _signature_records(
        donor_source["source"],
        donor_identifier,
        donor_source["path"],
    )
    if len(donor_records) != 1:
        raise Refusal(
            "donor-prototype-ambiguous",
            f"expected one donor signature, found {len(donor_records)}",
        )
    recipient_records: List[Dict[str, Any]] = []
    for path in _quoted_include_closure(repo_root, recipient_path):
        try:
            text = path.read_text(encoding="utf-8")
        except (OSError, UnicodeError) as error:
            raise Refusal(
                "recipient-prototype-missing",
                f"cannot inspect signature context {path}: {error}",
            ) from error
        recipient_records.extend(
            _signature_records(
                text,
                recipient_identifier,
                str(path.relative_to(repo_root)).replace("\\", "/"),
            )
        )
    if not recipient_records:
        raise Refusal(
            "recipient-prototype-missing",
            f"no explicit declaration or inline definition of "
            f"{recipient_identifier!r} in recipient include context",
        )
    recipient_shapes = {
        (
            record["return_type"],
            tuple(record["parameter_types"]),
        )
        for record in recipient_records
    }
    if len(recipient_shapes) != 1:
        raise Refusal(
            "recipient-prototype-ambiguous",
            f"conflicting declarations of {recipient_identifier!r}",
        )
    donor_shape = (
        donor_records[0]["return_type"],
        tuple(donor_records[0]["parameter_types"]),
    )
    recipient_shape = next(iter(recipient_shapes))
    if donor_shape != recipient_shape:
        raise Refusal(
            "recipient-prototype-incompatible",
            f"donor {donor_shape[0]}{donor_shape[1]} is incompatible with "
            f"recipient {recipient_shape[0]}{recipient_shape[1]}",
        )
    return {
        "compatible": True,
        "donor": donor_records[0],
        "recipient": recipient_records,
        "canonical_return_type": donor_shape[0],
        "canonical_parameter_types": list(donor_shape[1]),
    }


def _replace_token(text: str, old: str, new: str) -> Tuple[str, int]:
    pattern = re.compile(r"\b" + re.escape(old) + r"\b")
    return pattern.subn(new, text)


def _transform_source(
    donor_source: Mapping[str, Any],
    donor_function: str,
    recipient_function: str,
    derived: Mapping[str, Any],
) -> str:
    source = donor_source["source"]
    masked_source = _mask_noncode(source)
    for identifier, expected in derived["donor_relocation_counts"].items():
        actual = len(
            re.findall(r"\b" + re.escape(identifier) + r"\b", masked_source)
        )
        if actual != expected:
            raise Refusal(
                "wrong-callee-or-source-mismatch",
                f"{identifier!r} occurs {actual} times in source but target "
                f"relocates it {expected} times",
            )
    donor_identifier = _c_identifier(donor_function)
    recipient_identifier = _c_identifier(recipient_function)
    source, count = _replace_token(source, donor_identifier, recipient_identifier)
    if count != 1:
        raise Refusal(
            "donor-source-ambiguous",
            f"donor function name replacement count is {count}, not 1",
        )
    for old, new in sorted(
        derived["identifier_map"].items(), key=lambda item: (-len(item[0]), item[0])
    ):
        source, count = _replace_token(source, old, new)
        if count != derived["donor_relocation_counts"][old]:
            raise Refusal("internal-error", f"validated token {old!r} changed count")
    return source


def _quoted_include_closure(repo_root: Path, source_path: Path) -> List[Path]:
    pending = [source_path.resolve()]
    visited: List[Path] = []
    seen = set()
    while pending:
        path = pending.pop()
        if path in seen:
            continue
        seen.add(path)
        if not path.is_file() or not _inside(path, repo_root):
            continue
        visited.append(path)
        if len(visited) > 512:
            raise Refusal("include-context-ambiguous", "quoted include closure exceeds 512 files")
        try:
            text = path.read_text(encoding="utf-8")
        except (OSError, UnicodeError) as error:
            raise Refusal("include-context-ambiguous", f"cannot read {path}: {error}") from error
        for include in re.findall(r'^\s*#\s*include\s*"([^"]+)"', text, re.MULTILINE):
            candidates = (
                path.parent / include,
                repo_root / include,
                repo_root / "source" / include,
            )
            resolved = next(
                (
                    candidate.resolve()
                    for candidate in candidates
                    if candidate.is_file() and _inside(candidate, repo_root)
                ),
                None,
            )
            if resolved is not None and resolved not in seen:
                pending.append(resolved)
    return sorted(visited)


def _has_function_declaration(text: str, identifier: str) -> bool:
    masked = _mask_noncode(text)
    token = re.compile(r"\b" + re.escape(identifier) + r"\b")
    for match in token.finditer(masked):
        index = match.end()
        while index < len(masked) and masked[index].isspace():
            index += 1
        if index >= len(masked) or masked[index] != "(":
            continue
        close = _matching_delimiter(masked, index, "(", ")")
        after = close + 1
        while after < len(masked) and masked[after].isspace():
            after += 1
        if after >= len(masked) or masked[after] not in ";{":
            continue
        line_start = masked.rfind("\n", 0, match.start()) + 1
        prefix = masked[line_start:match.start()].strip()
        if (
            prefix
            and not prefix.startswith("#")
            and not re.search(r"\b(if|for|while|switch|return|sizeof)\s*$", prefix)
        ):
            return True
    return False


def _has_data_declaration(text: str, identifier: str) -> bool:
    masked = _mask_noncode(text)
    for statement in masked.split(";")[:-1]:
        if not re.search(r"\b" + re.escape(identifier) + r"\b", statement):
            continue
        tail = statement.rsplit("}", 1)[-1].rsplit("{", 1)[-1].strip()
        if not tail or tail.startswith("#") or "(" in tail:
            continue
        if re.match(r"^(return|if|for|while|switch)\b", tail):
            continue
        before = tail.split(identifier, 1)[0]
        if re.search(
            r"\b(extern|static|const|volatile|struct|union|enum|void|char|short|"
            r"int|long|float|double|signed|unsigned|boolean|real)\b",
            before,
        ):
            return True
    return False


def _function_declaration_texts(text: str, identifier: str) -> List[str]:
    masked = _mask_noncode(text)
    token = re.compile(r"\b" + re.escape(identifier) + r"\b")
    declarations: List[str] = []
    for match in token.finditer(masked):
        index = match.end()
        while index < len(masked) and masked[index].isspace():
            index += 1
        if index >= len(masked) or masked[index] != "(":
            continue
        close = _matching_delimiter(masked, index, "(", ")")
        after = close + 1
        while after < len(masked) and masked[after].isspace():
            after += 1
        if after >= len(masked) or masked[after] != ";":
            continue
        start = _definition_start(masked, match.start())
        prefix = masked[start:match.start()].strip()
        if (
            not prefix
            or prefix.startswith("#")
            or any(character in prefix for character in "{}=")
            or re.search(r"\b(if|for|while|switch|return|sizeof)\s*$", prefix)
        ):
            continue
        declarations.append(text[start:after + 1].strip())
    return declarations


def _data_declaration_texts(text: str, identifier: str) -> List[str]:
    masked = _mask_noncode(text)
    declarations: List[str] = []
    statement_start = 0
    for match in re.finditer(";", masked):
        statement_end = match.end()
        masked_statement = masked[statement_start:statement_end]
        if re.search(r"\b" + re.escape(identifier) + r"\b", masked_statement):
            brace = max(
                masked_statement.rfind("{"),
                masked_statement.rfind("}"),
            )
            local_start = brace + 1 if brace >= 0 else 0
            candidate_mask = masked_statement[local_start:].strip()
            candidate_text = text[
                statement_start + local_start:statement_end
            ].strip()
            before = candidate_mask.split(identifier, 1)[0]
            if (
                candidate_mask
                and not candidate_mask.startswith("#")
                and "(" not in candidate_mask
                and "=" not in before
                and re.search(
                    r"\b(extern|static|const|volatile|struct|union|enum|void|"
                    r"char|short|int|long|float|double|signed|unsigned|boolean|"
                    r"real)\b",
                    before,
                )
            ):
                declarations.append(candidate_text)
        statement_start = statement_end
    return declarations


def _resolve_unique_header_declaration(
    repo_root: Path, symbol_name: str, kind: str
) -> Optional[Dict[str, str]]:
    identifier = _c_identifier(symbol_name)
    candidates: List[Dict[str, str]] = []
    source_root = repo_root / "source"
    if not source_root.is_dir():
        return None
    for header in sorted(source_root.rglob("*.h")):
        try:
            raw = header.read_bytes()
            text = raw.decode("utf-8")
        except (OSError, UnicodeError) as error:
            raise Refusal(
                "recipient-declaration-ambiguous",
                f"cannot inspect repository header {header}: {error}",
            ) from error
        if not re.search(r"\b" + re.escape(identifier) + r"\b", _mask_noncode(text)):
            continue
        declarations = (
            _function_declaration_texts(text, identifier)
            if kind == "function"
            else _data_declaration_texts(text, identifier)
        )
        for declaration in declarations:
            candidates.append(
                {
                    "symbol": symbol_name,
                    "text": declaration,
                    "source": str(header.relative_to(repo_root)).replace("\\", "/"),
                    "sha256": _sha256_bytes(raw),
                }
            )
    unique_texts = {item["text"] for item in candidates}
    if not candidates:
        return None
    if len(unique_texts) != 1:
        sources = ", ".join(sorted({item["source"] for item in candidates}))
        raise Refusal(
            "recipient-declaration-ambiguous",
            f"conflicting header declarations for {symbol_name!r}: {sources}",
        )
    return min(candidates, key=lambda item: item["source"])


def _load_verified_declarations(
    repo_root: Path,
    manifest_path: Optional[Path],
) -> Dict[str, Dict[str, str]]:
    if manifest_path is None:
        return {}
    value = _read_json(manifest_path)
    if not isinstance(value, dict) or not isinstance(value.get("declarations"), list):
        raise Refusal(
            "invalid-declaration-manifest",
            "manifest must contain a declarations list",
        )
    result: Dict[str, Dict[str, str]] = {}
    for item in value["declarations"]:
        if not isinstance(item, dict):
            raise Refusal("invalid-declaration-manifest", "declaration must be an object")
        symbol = item.get("symbol")
        text = item.get("text")
        relative_source = item.get("source")
        expected_hash = item.get("sha256")
        if not all(isinstance(value, str) for value in (
            symbol, text, relative_source, expected_hash
        )):
            raise Refusal(
                "invalid-declaration-manifest",
                "symbol, text, source, and sha256 must be strings",
            )
        if symbol in result:
            raise Refusal(
                "invalid-declaration-manifest",
                f"duplicate declaration for {symbol!r}",
            )
        source = (repo_root / relative_source).resolve()
        if not _inside(source, repo_root) or source.suffix.lower() != ".h":
            raise Refusal(
                "invalid-declaration-manifest",
                f"declaration source is not a repository header: {relative_source!r}",
            )
        try:
            raw = source.read_bytes()
            header_text = raw.decode("utf-8")
        except (OSError, UnicodeError) as error:
            raise Refusal(
                "invalid-declaration-manifest",
                f"cannot read declaration header: {error}",
            ) from error
        if _sha256_bytes(raw) != expected_hash:
            raise Refusal(
                "declaration-source-changed",
                f"header hash changed for {relative_source!r}",
            )
        if text not in header_text:
            raise Refusal(
                "declaration-not-verbatim",
                f"declaration is not verbatim in {relative_source!r}",
            )
        identifier = _c_identifier(symbol)
        if (
            not re.search(r"\b" + re.escape(identifier) + r"\b", _mask_noncode(text))
            or not text.strip().endswith(";")
            or "{" in _mask_noncode(text)
            or "}" in _mask_noncode(text)
            or "#" in _mask_noncode(text)
        ):
            raise Refusal(
                "invalid-declaration-manifest",
                f"unsafe declaration text for {symbol!r}",
            )
        result[symbol] = {
            "text": text.strip(),
            "source": relative_source.replace("\\", "/"),
            "sha256": expected_hash,
        }
    return result


def _recipient_context(
    repo_root: Path,
    recipient_path: Path,
    recipient_symbols: Mapping[str, str],
    declarations: Mapping[str, Mapping[str, str]],
) -> Tuple[List[Mapping[str, str]], List[str]]:
    closure = _quoted_include_closure(repo_root, recipient_path)
    context_parts = []
    for path in closure:
        try:
            context_parts.append(path.read_text(encoding="utf-8"))
        except (OSError, UnicodeError) as error:
            raise Refusal("include-context-ambiguous", f"cannot read {path}: {error}") from error
    context = "\n".join(context_parts)
    insertions: List[Mapping[str, str]] = []
    used_manifest_symbols: List[str] = []
    for symbol_name, kind in sorted(recipient_symbols.items()):
        identifier = _c_identifier(symbol_name)
        present = (
            _has_function_declaration(context, identifier)
            if kind == "function"
            else _has_data_declaration(context, identifier)
        )
        if present:
            continue
        declaration = declarations.get(symbol_name)
        if declaration is None:
            declaration = _resolve_unique_header_declaration(
                repo_root, symbol_name, kind
            )
        if declaration is None:
            raise Refusal(
                "recipient-declaration-missing",
                f"no explicit {kind} declaration for {symbol_name!r} in recipient "
                "quoted-include context or a unique repository header",
            )
        insertion = dict(declaration)
        insertion["symbol"] = symbol_name
        insertions.append(insertion)
        used_manifest_symbols.append(symbol_name)
    unused = sorted(set(declarations) - set(used_manifest_symbols))
    if unused:
        raise Refusal(
            "unused-declaration",
            f"manifest contains unnecessary declarations: {', '.join(unused)}",
        )
    return insertions, [str(path.relative_to(repo_root)).replace("\\", "/") for path in closure]


def _compose_fragment(
    transformed_source: str, declarations: Sequence[Mapping[str, str]]
) -> str:
    parts: List[str] = []
    if declarations:
        parts.append("/* Verified declarations copied from repository headers. */")
        for declaration in declarations:
            parts.append(
                f"/* {declaration['symbol']}: {declaration['source']} "
                f"sha256={declaration['sha256']} */\n{declaration['text']}"
            )
    parts.append(transformed_source.rstrip())
    return "\n\n".join(parts) + "\n"


def _parameter_identifiers(function_source: str) -> List[str]:
    masked = _mask_noncode(function_source)
    opening = masked.find("(")
    if opening < 0:
        return []
    closing = _matching_delimiter(masked, opening, "(", ")")
    parameters = masked[opening + 1:closing]
    parts: List[str] = []
    start = 0
    depth = 0
    for index, character in enumerate(parameters):
        if character in "([":
            depth += 1
        elif character in ")]":
            depth -= 1
        elif character == "," and depth == 0:
            parts.append(parameters[start:index])
            start = index + 1
    parts.append(parameters[start:])

    type_words = {
        "void", "char", "short", "int", "long", "float", "double",
        "signed", "unsigned", "const", "volatile", "struct", "union",
        "enum", "boolean", "real",
    }
    identifiers: List[str] = []
    for part in parts:
        tokens = re.findall(r"\b[A-Za-z_][A-Za-z0-9_]*\b", part)
        if not tokens or (len(tokens) == 1 and tokens[0] == "void"):
            continue
        candidate = tokens[-1]
        if candidate not in type_words:
            identifiers.append(candidate)
    return identifiers


def _review_lints(
    donor_source: str,
    donor_function: str,
    recipient_function: str,
    declarations: Sequence[Mapping[str, str]],
) -> List[Dict[str, str]]:
    lints: List[Dict[str, str]] = []
    donor_domain = _c_identifier(donor_function).split("_", 1)[0]
    recipient_domain = _c_identifier(recipient_function).split("_", 1)[0]
    if donor_domain != recipient_domain:
        for parameter in _parameter_identifiers(donor_source):
            if parameter == donor_domain or parameter.startswith(donor_domain + "_"):
                lints.append(
                    {
                        "code": "donor-domain-parameter-name",
                        "severity": "REVIEW_REQUIRED",
                        "detail": (
                            f"parameter {parameter!r} retains donor domain "
                            f"{donor_domain!r}; a human must choose any semantic "
                            "recipient name because relocations cannot prove it"
                        ),
                    }
                )
    for declaration in declarations:
        lints.append(
            {
                "code": "generated-declaration-redundancy-review",
                "severity": "REVIEW_REQUIRED",
                "detail": (
                    f"{declaration['symbol']!r} was copied from "
                    f"{declaration['source']}; verify the full compiler/forced-include "
                    "context does not already provide it and prefer the real header "
                    "include over a redundant local prototype"
                ),
            }
        )
    return lints


def _review_markdown(packet: Mapping[str, Any]) -> str:
    pair = packet["pair"]
    donor = pair["donor"]
    recipient = pair["recipient"]
    events = packet["derived_substitutions"]["events"]
    substitution_lines = (
        "\n".join(f"- `{old}` -> `{new}`" for old, new in events)
        if events
        else "- None (target relocation destinations are identical)."
    )
    lint_lines = (
        "\n".join(
            f"- **{item['severity']} / `{item['code']}`**: {item['detail']}"
            for item in packet["review_lints"]
        )
        if packet["review_lints"]
        else "- None."
    )
    return f"""# Exact-shape transfer review packet

Verdict: **CANDIDATE GENERATED — NOT ACCEPTED**

- Donor: `{donor['unit']}:{donor['function']}`
- Recipient: `{recipient['unit']}:{recipient['function']}`
- Candidate bytes claimed by atlas: `{pair['size']}`
- Candidate patch: `candidate.patch` (unapplied)
- Frozen pre-edit TU/object baseline: `frozen_tu_baseline.json`
- Production source modified: **no**
- Matching/config/ledger changes: **none**

## Target-derived substitutions

{substitution_lines}

These substitutions were independently derived from the ordered recipient
January target relocations. The atlas event list matched exactly. The generated
body is the donor source with token substitutions only.

## Review lints

{lint_lines}

Lints are never auto-fixed because parameter semantics and the complete compiler
include context are not derivable from recipient target relocations.

## Required acceptance work (not run by this executor)

1. Apply the patch only in a disposable human review worktree.
2. Compile the recipient translation unit with the campaign's pinned XDK flags.
3. Require hardened cross-name `section_infos_equal` for the candidate and
   recipient target symbols.
4. Require a zero-regression whole-TU manifest comparison.
5. Require unchanged whole-object data/BSS/ownership fingerprints.
6. Review the source diff and only then decide whether to commit in a human lane.

This packet grants no matching credit and cannot be integrated automatically.
"""


def _freeze_tu_baseline(
    unit: str,
    recipient_function: str,
    target_path: Path,
    base_path: Path,
) -> Dict[str, Any]:
    if not base_path.is_file():
        raise Refusal("base-object-missing", f"base object does not exist: {base_path}")
    try:
        target_fingerprint = _object_fingerprint(target_path, {}, {})
        base_fingerprint = _object_fingerprint(base_path, {}, {})
        target_obj = load(target_path)
        base_obj = load(base_path)
    except (OSError, CoffError, GateError) as error:
        raise Refusal(
            "frozen-tu-baseline-failed",
            f"cannot fingerprint {unit}: {error}",
        ) from error
    if recipient_function in base_fingerprint["functions"]:
        raise Refusal(
            "recipient-base-already-defines",
            f"pre-edit base object already defines {recipient_function!r}",
        )

    comparisons: Dict[str, str] = {}
    strict_exact: List[str] = []
    for name in sorted(target_fingerprint["functions"]):
        if name not in base_fingerprint["functions"]:
            comparisons[name] = "ABSENT_IN_BASE"
            continue
        try:
            exact = section_infos_equal(
                section_info(target_obj, name),
                section_info(base_obj, name),
            )
        except CoffError as error:
            raise Refusal(
                "frozen-tu-baseline-failed",
                f"cannot compare pre-edit sibling {unit}:{name}: {error}",
            ) from error
        state = "STRICT_EXACT" if exact else "NONEXACT"
        comparisons[name] = state
        if exact:
            strict_exact.append(name)
    base_only = sorted(
        set(base_fingerprint["functions"]) - set(target_fingerprint["functions"])
    )
    return {
        "schema_version": 1,
        "purpose": (
            "pre-edit frozen baseline only; does not accept the generated candidate"
        ),
        "unit": unit,
        "recipient_function": recipient_function,
        "recipient_absent_in_base": True,
        "target_path": str(target_path.resolve()),
        "base_path": str(base_path.resolve()),
        "target": target_fingerprint,
        "base": base_fingerprint,
        "pre_edit_function_states": comparisons,
        "pre_edit_strict_exact_functions": strict_exact,
        "base_only_functions": base_only,
        "post_edit_requirements": {
            "recipient_hardened_comparator": "REQUIRED",
            "pre_edit_strict_exact_siblings_unchanged": "REQUIRED_ZERO_REGRESSIONS",
            "base_non_code_sections_unchanged": "REQUIRED",
            "base_symbol_ownership_unchanged_except_reviewed_candidate": "REQUIRED",
        },
        "matching_credit_granted": False,
    }


def plan_candidate(
    *,
    repo_root: Path,
    atlas_path: Path,
    target_root: Path,
    base_root: Optional[Path] = None,
    recipient_unit: str,
    recipient_function: str,
    donor_unit: Optional[str] = None,
    donor_function: Optional[str] = None,
    declaration_manifest: Optional[Path] = None,
) -> Dict[str, Any]:
    """Build an in-memory candidate packet without writing any files."""
    repo_root = repo_root.resolve()
    if not (repo_root / ".git").exists():
        raise Refusal("invalid-repository", f"not a Git worktree: {repo_root}")
    atlas = _read_json(atlas_path)
    pair = _select_pair(
        atlas,
        recipient_unit,
        recipient_function,
        donor_unit,
        donor_function,
    )
    _validate_pair_policy(pair)

    donor = pair["donor"]
    recipient = pair["recipient"]
    donor_source_path = _checked_repo_path(repo_root, donor["unit"], ".c")
    recipient_source_path = _checked_repo_path(repo_root, recipient["unit"], ".c")
    if not recipient_source_path.is_file():
        raise Refusal(
            "recipient-source-missing",
            f"recipient TU does not exist: {recipient_source_path}",
        )
    try:
        recipient_raw = recipient_source_path.read_bytes()
        recipient_text = recipient_raw.decode("utf-8")
    except (OSError, UnicodeError) as error:
        raise Refusal(
            "recipient-source-missing",
            f"cannot read recipient TU: {error}",
        ) from error
    recipient_identifier = _c_identifier(recipient["function"])
    if _function_definitions(recipient_text, recipient_identifier):
        raise Refusal(
            "recipient-source-already-defines",
            f"recipient TU already defines {recipient_identifier!r}",
        )

    donor_target_path = _checked_repo_path(
        target_root.resolve(), donor["unit"], ".obj"
    )
    recipient_target_path = _checked_repo_path(
        target_root.resolve(), recipient["unit"], ".obj"
    )
    donor_target = _load_target_function(donor_target_path, donor["function"])
    recipient_target = _load_target_function(
        recipient_target_path, recipient["function"]
    )
    frozen_baseline = None
    if base_root is not None:
        recipient_base_path = _checked_repo_path(
            base_root.resolve(), recipient["unit"], ".obj"
        )
        frozen_baseline = _freeze_tu_baseline(
            recipient["unit"],
            recipient["function"],
            recipient_target_path,
            recipient_base_path,
        )
    derived = _derive_substitutions(pair, donor_target, recipient_target)
    donor_source = _extract_donor_source(donor_source_path, donor["function"])
    prototype_proof = _prove_prototype_compatibility(
        repo_root,
        recipient_source_path,
        donor_source,
        donor["function"],
        recipient["function"],
    )
    transformed = _transform_source(
        donor_source,
        donor["function"],
        recipient["function"],
        derived,
    )

    declarations = _load_verified_declarations(repo_root, declaration_manifest)
    insertions, include_context = _recipient_context(
        repo_root,
        recipient_source_path,
        derived["recipient_symbols"],
        declarations,
    )
    fragment = _compose_fragment(transformed, insertions)
    review_lints = _review_lints(
        donor_source["source"],
        donor["function"],
        recipient["function"],
        insertions,
    )
    old_lines = recipient_text.splitlines(keepends=True)
    separator = "" if not recipient_text or recipient_text.endswith("\n") else "\n"
    candidate_text = recipient_text + separator + "\n" + fragment
    patch = "".join(
        difflib.unified_diff(
            old_lines,
            candidate_text.splitlines(keepends=True),
            fromfile=f"a/{recipient['unit']}.c",
            tofile=f"b/{recipient['unit']}.c",
        )
    )
    if not patch:
        raise Refusal("empty-candidate", "candidate patch is unexpectedly empty")

    pair_snapshot = json.loads(json.dumps(pair))
    packet: Dict[str, Any] = {
        "schema_version": 1,
        "verdict": "CANDIDATE_GENERATED_NOT_ACCEPTED",
        "pair": pair_snapshot,
        "repository": {
            "root": str(repo_root),
            "recipient_source": {
                "path": str(recipient_source_path),
                "sha256": _sha256_bytes(recipient_raw),
            },
        },
        "donor_source": {
            key: value for key, value in donor_source.items() if key != "source"
        },
        "target_objects": {
            "donor": {
                "path": donor_target["path"],
                "sha256": donor_target["sha256"],
            },
            "recipient": {
                "path": recipient_target["path"],
                "sha256": recipient_target["sha256"],
            },
        },
        "frozen_tu_baseline": (
            {
                "sha256": _sha256_bytes(_json_bytes(frozen_baseline)),
                "file": "frozen_tu_baseline.json",
                "pre_edit_strict_exact_function_count": len(
                    frozen_baseline["pre_edit_strict_exact_functions"]
                ),
                "pre_edit_non_code_section_count": len(
                    frozen_baseline["base"]["non_code_sections"]
                ),
            }
            if frozen_baseline is not None
            else None
        ),
        "derived_substitutions": {
            "events": derived["events"],
            "identifier_map": derived["identifier_map"],
            "source": "ordered donor/recipient January target COFF relocations",
        },
        "declarations_inserted": insertions,
        "prototype_compatibility": prototype_proof,
        "review_lints": review_lints,
        "recipient_include_context": include_context,
        "generation_checks": {
            "atlas_proofs_all_true": True,
            "meaningful_size_over_16": True,
            "padded_and_meaningful_sizes_equal": True,
            "normalized_target_bytes_equal": True,
            "relocation_address_type_order_equal": True,
            "relocation_addends_equal": True,
            "storage_classes_equal": True,
            "prologues_equal": True,
            "substitutions_injective": True,
            "atlas_events_match_derived_events": True,
            "donor_source_definition_unique": True,
            "recipient_prototype_return_abi_compatible": True,
            "statement_order_changed": False,
            "production_source_modified": False,
            "review_lint_count": len(review_lints),
        },
        "acceptance_gates": {
            "hardened_cross_name_section_infos_equal": "NOT_RUN_REQUIRED",
            "whole_tu_zero_regression": (
                "NOT_RUN_REQUIRED_BASELINE_FROZEN"
                if frozen_baseline is not None
                else "NOT_RUN_REQUIRED_BASELINE_MISSING"
            ),
            "whole_object_data_bss_ownership_unchanged": (
                "NOT_RUN_REQUIRED_BASELINE_FROZEN"
                if frozen_baseline is not None
                else "NOT_RUN_REQUIRED_BASELINE_MISSING"
            ),
        },
        "matching_credit_granted": False,
        "automatic_integration_performed": False,
        "candidate": {
            "fragment_sha256": _sha256_bytes(fragment.encode("utf-8")),
            "patch_sha256": _sha256_bytes(patch.encode("utf-8")),
        },
    }
    return {
        "packet": packet,
        "fragment": fragment,
        "patch": patch,
        "frozen_baseline": frozen_baseline,
        "review": _review_markdown(packet),
    }


def emit_candidate(result: Mapping[str, Any], output_dir: Path, repo_root: Path) -> None:
    """Write one new out-of-repository review directory."""
    output_dir = output_dir.resolve()
    repo_root = repo_root.resolve()
    if _inside(output_dir, repo_root):
        raise Refusal(
            "output-inside-repository",
            "review output must be outside the repository",
        )
    if output_dir.exists():
        raise Refusal(
            "output-already-exists",
            "review output directory must not already exist",
        )
    if result.get("frozen_baseline") is None:
        raise Refusal(
            "frozen-tu-baseline-missing",
            "candidate emission requires a pre-edit recipient base object",
        )
    hashed_inputs = [
        result["packet"]["repository"]["recipient_source"],
        result["packet"]["donor_source"],
        result["packet"]["target_objects"]["donor"],
        result["packet"]["target_objects"]["recipient"],
        {
            "path": result["frozen_baseline"]["base_path"],
            "sha256": result["frozen_baseline"]["base"]["object_sha256"],
        },
    ]
    for item in hashed_inputs:
        path = Path(item["path"])
        try:
            current_hash = _sha256_bytes(path.read_bytes())
        except OSError as error:
            raise Refusal(
                "input-changed-before-emission",
                f"cannot re-read {path}: {error}",
            ) from error
        if current_hash != item["sha256"]:
            raise Refusal(
                "input-changed-before-emission",
                f"input changed after planning: {path}",
            )
    staging: Optional[Path] = None
    try:
        output_dir.parent.mkdir(parents=True, exist_ok=True)
        staging = Path(
            tempfile.mkdtemp(
                prefix=f".{output_dir.name}.tmp-",
                dir=str(output_dir.parent),
            )
        )
        (staging / "candidate.c").write_text(
            result["fragment"], encoding="utf-8", newline="\n"
        )
        (staging / "candidate.patch").write_text(
            result["patch"], encoding="utf-8", newline="\n"
        )
        (staging / "review_packet.json").write_bytes(
            _json_bytes(result["packet"])
        )
        (staging / "frozen_tu_baseline.json").write_bytes(
            _json_bytes(result["frozen_baseline"])
        )
        (staging / "REVIEW.md").write_text(
            result["review"], encoding="utf-8", newline="\n"
        )
        staging.replace(output_dir)
        staging = None
    except OSError as error:
        raise Refusal("output-write-failed", str(error)) from error
    finally:
        if staging is not None:
            shutil.rmtree(staging, ignore_errors=True)


def _parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description=(
            "Generate an unapplied exact-shape transfer candidate and review "
            "packet; never modifies the repository or grants matching credit."
        )
    )
    parser.add_argument("--repo-root", type=Path, required=True)
    parser.add_argument("--atlas", type=Path, required=True)
    parser.add_argument(
        "--target-root",
        type=Path,
        required=True,
        help="root containing <unit>.obj January target objects",
    )
    parser.add_argument(
        "--base-root",
        type=Path,
        required=True,
        help="root containing untouched pre-edit <unit>.obj base objects",
    )
    parser.add_argument("--recipient-unit", required=True)
    parser.add_argument("--recipient-function", required=True)
    parser.add_argument("--donor-unit")
    parser.add_argument("--donor-function")
    parser.add_argument("--declaration-manifest", type=Path)
    parser.add_argument(
        "--output-dir",
        type=Path,
        required=True,
        help="new directory outside repo; candidate is never applied",
    )
    return parser


def main(argv: Optional[Sequence[str]] = None) -> int:
    arguments = _parser().parse_args(argv)
    try:
        result = plan_candidate(
            repo_root=arguments.repo_root,
            atlas_path=arguments.atlas,
            target_root=arguments.target_root,
            base_root=arguments.base_root,
            recipient_unit=arguments.recipient_unit,
            recipient_function=arguments.recipient_function,
            donor_unit=arguments.donor_unit,
            donor_function=arguments.donor_function,
            declaration_manifest=arguments.declaration_manifest,
        )
        emit_candidate(result, arguments.output_dir, arguments.repo_root)
    except Refusal as refusal:
        print(
            json.dumps(
                {
                    "verdict": "REFUSED",
                    "refusal": refusal.as_dict(),
                    "candidate_emitted": False,
                    "production_source_modified": False,
                    "matching_credit_granted": False,
                },
                indent=2,
                sort_keys=True,
            ),
            file=sys.stderr,
        )
        return 2
    print(
        json.dumps(
            {
                "verdict": result["packet"]["verdict"],
                "output_dir": str(arguments.output_dir.resolve()),
                "production_source_modified": False,
                "matching_credit_granted": False,
            },
            indent=2,
            sort_keys=True,
        )
    )
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
