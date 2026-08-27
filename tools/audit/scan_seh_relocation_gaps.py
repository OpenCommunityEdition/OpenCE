from pathlib import Path
import json
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))

from tools.coff_compare import IMAGE_SCN_CNT_CODE, load, section_info


def relocation_symbol(relocation):
    symbolic = relocation.get("symbolic_target")
    target = symbolic if symbolic is not None else relocation["target"]
    if target[0] == "symbol":
        return target[1]
    return None


config = json.loads((ROOT / "objdiff.json").read_text(encoding="utf-8"))
for unit in config["units"]:
    if "base_path" not in unit:
        continue
    target_path = ROOT / unit["target_path"]
    base_path = ROOT / unit["base_path"]
    if not target_path.exists() or not base_path.exists():
        continue

    target = load(target_path)
    base = load(base_path)
    target_names = {
        item["name"]
        for item in target["symbols"]
        if item["section"] > 0 and item["type"] == 0x20
    }
    for item in base["symbols"]:
        if item["section"] <= 0 or item["type"] != 0x20:
            continue
        if item["name"] not in target_names:
            continue
        section = base["sections"][item["section"] - 1]
        if not section["flags"] & IMAGE_SCN_CNT_CODE:
            continue

        base_info = section_info(base, item["name"])
        extra = [
            relocation
            for relocation in base_info["relocations"]
            if relocation_symbol(relocation) == "__except_list"
        ]
        if not extra:
            continue

        target_info = section_info(target, item["name"])
        if (
            base_info["size"] == target_info["size"]
            and base_info["normalized_sha256"] == target_info["normalized_sha256"]
        ):
            print(
                unit["name"],
                item["name"],
                f"target={target_info['relocation_count']}",
                f"base={base_info['relocation_count']}",
                "except_list=" + ",".join(str(reloc["address"]) for reloc in extra),
            )
