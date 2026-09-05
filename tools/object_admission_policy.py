"""Validate reviewed whole-object admission rejections.

Function exactness alone cannot prove that a rebuilt translation unit owns
the same COFF sections as its target or has an authenticated complete source
layout. This policy records reviewed ownership and source-layout failures
that must veto whole-object completion without debiting valid target function
progress.
"""

from pathlib import Path
from typing import Any
import json


ALLOWED_CLASSES = {
    "bss-symbol-layout-mismatch",
    "candidate-only-comdat-owner",
    "source-layout-incomplete",
}


class ObjectAdmissionPolicyError(RuntimeError):
    pass


def rejection_index(
    policy: dict[str, Any] | None,
    known_units: set[str] | None = None,
) -> dict[str, list[dict[str, str]]]:
    """Return validated rejection records indexed by unit name."""
    if policy is None:
        return {}
    if policy.get("version") != 1 or not isinstance(policy.get("entries"), list):
        raise ObjectAdmissionPolicyError(
            "object admission policy must have version 1 and an entries list"
        )

    result: dict[str, list[dict[str, str]]] = {}
    seen: set[tuple[str, str]] = set()
    for entry in policy["entries"]:
        if not isinstance(entry, dict):
            raise ObjectAdmissionPolicyError("object admission entry must be an object")
        normalized: dict[str, str] = {}
        for field in ("unit", "class", "symbol", "reason", "evidence", "reopen"):
            value = entry.get(field)
            if not isinstance(value, str) or not value.strip():
                raise ObjectAdmissionPolicyError(
                    f"object admission entry requires non-empty {field}"
                )
            normalized[field] = value.strip()
        if normalized["class"] not in ALLOWED_CLASSES:
            raise ObjectAdmissionPolicyError(
                f"unknown object admission blocker class: {normalized['class']}"
            )
        if known_units is not None and normalized["unit"] not in known_units:
            raise ObjectAdmissionPolicyError(
                f"object admission rejection unit not found: {normalized['unit']}"
            )
        key = (normalized["unit"], normalized["symbol"])
        if key in seen:
            raise ObjectAdmissionPolicyError(
                f"duplicate object admission rejection: {key[0]}:{key[1]}"
            )
        seen.add(key)
        result.setdefault(normalized["unit"], []).append(normalized)
    return result


def load_rejection_index(
    path: Path,
    known_units: set[str] | None = None,
) -> dict[str, list[dict[str, str]]]:
    if not path.is_file():
        return {}
    try:
        policy = json.loads(path.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError) as error:
        raise ObjectAdmissionPolicyError(
            f"cannot load object admission policy {path}: {error}"
        ) from error
    if not isinstance(policy, dict):
        raise ObjectAdmissionPolicyError("object admission policy root must be an object")
    return rejection_index(policy, known_units)
