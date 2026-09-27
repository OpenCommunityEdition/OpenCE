"""Derive the V1 form of a pinned extent-model entry: add its surplus list.

Reads an existing grouped entry (B1's hs entry, fc53d5f6's actions entry),
keeps every key and member byte-for-byte in meaning, and appends a
``surplus`` list naming each rebuilt-only data section in the report sections
the entry touches, with the base snapshot the V1 verifier pins.  It never
decides admissibility: it only records what is there, and the V1 verifier then
checks every surplus rule independently.  Dry run unless --write.

Run from a repository root whose tools/ holds the V1 verifier:
  python -B <this> --entries in.json --unit source/hs/hs --out out.json [--write]
"""
import argparse
import json
import sys
from pathlib import Path

ROOT = Path.cwd()
sys.path.insert(0, str(ROOT))

from tools import semantic_progress as sp  # noqa: E402
from tools.coff_compare import load, section_info  # noqa: E402


def derive(entry, config_unit):
    target = load(ROOT / config_unit["target_path"])
    base = load(ROOT / config_unit["base_path"])
    target_numbers = set()
    base_numbers = set()
    for member in entry["members"]:
        target_numbers.add(int(sp._unique_defined_symbol(
            target, member["symbol"], "target owner")["section"]))
        base_numbers.add(int(sp._unique_defined_symbol(
            base, member["symbol"], "base owner")["section"]))
    touched = sp._objdiff_group_coverage(target, target_numbers, entry["unit"])
    touched_names = {
        sp._objdiff_base_name(section)
        for sections in touched.values() for section in sections}
    surplus = []
    for section in base["sections"]:
        number = int(section["index"])
        if not sp._objdiff_data_section(section) \
                or sp._objdiff_base_name(section) not in touched_names \
                or number in base_numbers:
            continue
        owners = [item for item in base["symbols"]
                  if int(item["section"]) == number
                  and item["name"] != section["name"]]
        if len(owners) != 1:
            raise SystemExit(f"section {number} has {len(owners)} owners")
        owner = owners[0]
        info = section_info(base, owner["name"])
        surplus.append({
            "symbol": owner["name"],
            "measurements": {
                "base": sp._semantic_data_member_snapshot(owner, section, info)},
        })
    out = dict(entry)
    out["reason"] = entry["reason"].rstrip() + (
        f" V1 verifier form: the rebuilt object adds {len(surplus)} select-any"
        f" COMDAT section(s) to these report sections; each is listed in"
        f" 'surplus' with a pinned snapshot and earns no credit.  Every member"
        f" also has an identical section symbol table, COMDAT selection and"
        f" image-address-resolved relocation list in both objects.")
    out["surplus"] = surplus
    return out, touched


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--entries", type=Path, required=True)
    parser.add_argument("--unit", required=True)
    parser.add_argument("--out", type=Path, required=True)
    parser.add_argument("--write", action="store_true")
    args = parser.parse_args()
    value = json.loads(args.entries.read_text(encoding="utf-8"))
    entries = value if isinstance(value, list) else [value]
    matches = [item for item in entries if item["unit"] == args.unit]
    if len(matches) != 1:
        raise SystemExit(f"expected one {args.unit} entry, found {len(matches)}")
    config = json.loads((ROOT / "objdiff.json").read_text(encoding="utf-8"))
    config_unit = {u["name"]: u for u in config["units"]}[args.unit]
    derived, touched = derive(matches[0], config_unit)
    summary = {
        "unit": args.unit,
        "members": len(derived["members"]),
        "touched_report_sections": {
            name: sp._objdiff_report_extent(sections)
            for name, sections in touched.items()},
        "surplus": [item["symbol"] for item in derived["surplus"]],
    }
    print(json.dumps(summary, indent=1))
    if args.write:
        args.out.write_text(
            json.dumps([derived], indent=1) + "\n", encoding="utf-8",
            newline="\n")
        print(f"wrote {args.out}")


if __name__ == "__main__":
    main()
