"""Snapshot and compare strict function verdicts by stable target section.

The older name-keyed verdict map is useful when symbol identities do not
change.  A semantic rename, however, can make an exact function look lost and
re-created.  This tool keys each owner by unit and target section index while
retaining the display name and measurements for review.

Run from the repository root::

    python -m tools.campaign.stable_verdicts snapshot scratch/before.json
    python -m tools.campaign.stable_verdicts snapshot scratch/after.json
    python -m tools.campaign.stable_verdicts diff scratch/before.json scratch/after.json
"""

from __future__ import annotations

import argparse
import json
from pathlib import Path
from typing import Any

from tools import coff_compare


def build_snapshot(project_root: Path, config_path: Path) -> dict[str, dict[str, Any]]:
    config = json.loads(config_path.read_text(encoding="utf-8"))
    result: dict[str, dict[str, Any]] = {}

    for unit in config["units"]:
        target_value = unit.get("target_path")
        base_value = unit.get("base_path")
        if not target_value:
            continue

        target_path = project_root / target_value
        base_path = project_root / base_value if base_value else None
        if not target_path.is_file():
            continue

        target = coff_compare.load(target_path)
        base = coff_compare.load(base_path) if base_path and base_path.is_file() else None
        seen_sections: set[int] = set()

        for symbol in target["symbols"]:
            section = symbol["section"]
            if (
                not symbol["name"].startswith("_")
                or section <= 0
                or section in seen_sections
                or symbol["storage"] not in (2, 3)
                or symbol["value"] != 0
                or target["sections"][section - 1]["name"] != ".text"
            ):
                continue

            seen_sections.add(section)
            name = symbol["name"]
            target_info = coff_compare.section_info(target, name)
            status = "U"
            if base is not None:
                try:
                    base_info = coff_compare.section_info(base, name)
                except (KeyError, ValueError):
                    base_info = None
                if base_info is not None:
                    status = (
                        "E"
                        if coff_compare.section_infos_equal(target_info, base_info)
                        else "R"
                    )

            result[f'{unit["name"]}::section:{section}'] = {
                "status": status,
                "name": name,
                "size": target_info["size"],
                "sha256": target_info["normalized_sha256"],
            }

    return result


def write_snapshot(
    output_path: Path,
    project_root: Path = Path("."),
    config_path: Path = Path("objdiff.json"),
) -> None:
    project_root = project_root.resolve()
    if not config_path.is_absolute():
        config_path = project_root / config_path
    result = build_snapshot(project_root, config_path)
    output_path.parent.mkdir(parents=True, exist_ok=True)
    output_path.write_text(
        json.dumps(result, indent=2, sort_keys=True) + "\n",
        encoding="utf-8",
    )
    exact_count = sum(item["status"] == "E" for item in result.values())
    print(f"wrote {output_path}: {len(result)} functions, {exact_count} exact")


def compare_snapshots(
    before: dict[str, dict[str, Any]],
    after: dict[str, dict[str, Any]],
) -> tuple[list[str], list[str]]:
    lost = sorted(
        key
        for key, item in before.items()
        if item["status"] == "E" and after.get(key, {"status": "U"})["status"] != "E"
    )
    gained = sorted(
        key
        for key, item in after.items()
        if item["status"] == "E" and before.get(key, {"status": "U"})["status"] != "E"
    )
    return lost, gained


def diff_snapshots(before_path: Path, after_path: Path) -> int:
    before = json.loads(before_path.read_text(encoding="utf-8"))
    after = json.loads(after_path.read_text(encoding="utf-8"))
    lost, gained = compare_snapshots(before, after)

    for key in lost:
        after_name = after.get(key, {}).get("name", "GONE")
        print(f'REGRESSION {key} {before[key]["name"]} -> {after_name}')
    for key in gained:
        before_name = before.get(key, {}).get("name", "NEW")
        print(f'gained {key} {before_name} -> {after[key]["name"]} {after[key]["size"]}')

    gained_bytes = sum(after[key]["size"] for key in gained)
    print(f"gained {len(gained)} {gained_bytes} bytes; regressions {len(lost)}")
    return 1 if lost else 0


def main(argv: list[str] | None = None) -> int:
    parser = argparse.ArgumentParser(
        description="Snapshot and diff strict function verdicts by target section identity."
    )
    subparsers = parser.add_subparsers(dest="command", required=True)

    snapshot_parser = subparsers.add_parser("snapshot")
    snapshot_parser.add_argument("output", type=Path)
    snapshot_parser.add_argument("--project", type=Path, default=Path("."))
    snapshot_parser.add_argument("--config", type=Path, default=Path("objdiff.json"))

    diff_parser = subparsers.add_parser("diff")
    diff_parser.add_argument("before", type=Path)
    diff_parser.add_argument("after", type=Path)

    args = parser.parse_args(argv)
    if args.command == "snapshot":
        write_snapshot(args.output, args.project, args.config)
        return 0
    return diff_snapshots(args.before, args.after)


if __name__ == "__main__":
    raise SystemExit(main())
