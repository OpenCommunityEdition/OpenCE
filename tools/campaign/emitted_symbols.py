"""Read-only checks over every candidate COFF symbol, independent of targets."""

from collections.abc import Iterable
from typing import Any


IMAGE_SCN_CNT_CODE = 0x00000020


def forbidden_emitted_symbols(
    obj: dict[str, Any],
    names: Iterable[str],
) -> list[dict[str, Any]]:
    """Find each external/static code definition with an exact forbidden name.

    Do not collapse symbols by name: an undefined reference or data symbol can
    precede a code definition with the same name. Section flags identify code,
    including named code subsections and owners at nonzero offsets. Undefined,
    common, absolute, debug, and data symbols are not emitted code owners.
    """

    forbidden = set(names)
    findings = []
    for symbol in obj["symbols"]:
        if symbol["name"] not in forbidden or symbol["storage"] not in (2, 3):
            continue
        section_number = symbol["section"]
        if section_number <= 0:
            continue
        if section_number > len(obj["sections"]):
            raise ValueError(
                "symbol %r refers to missing COFF section %d"
                % (symbol["name"], section_number)
            )
        section = obj["sections"][section_number - 1]
        if not section["flags"] & IMAGE_SCN_CNT_CODE:
            continue
        findings.append({
            "name": symbol["name"],
            "symbol_index": symbol["index"],
            "section_number": section_number,
            "section_name": section["name"],
            "offset": symbol["value"],
            "section_size": section["size"],
        })
    return findings
