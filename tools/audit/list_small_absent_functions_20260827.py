import json
import sys
from pathlib import Path


sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from tools.coff_compare import (
    CoffError,
    IMAGE_SCN_CNT_CODE,
    load,
    section_info,
    symbol,
)


root = Path(__file__).resolve().parents[2]
target_root = root / "build" / "split" / "source"
base_root = root / "build" / "base" / "source"
rows = []

for target_path in target_root.rglob("*.obj"):
    relative = target_path.relative_to(target_root)
    base_path = base_root / relative
    if not base_path.exists():
        continue
    target = load(target_path)
    base = load(base_path)
    names = sorted({
        item["name"]
        for item in target["symbols"]
        if item["section"] > 0
        and item["value"] == 0
        and target["sections"][item["section"] - 1]["flags"] & IMAGE_SCN_CNT_CODE
        and item["name"].startswith("_")
        and not item["name"].startswith("_code_")
    })
    for name in names:
        try:
            symbol(base, name)
            continue
        except CoffError:
            pass
        info = section_info(target, name)
        if info["size"] > 64:
            continue
        rows.append({
            "unit": "source/" + relative.with_suffix("").as_posix(),
            "function": name,
            "size": info["size"],
            "relocations": info["relocation_count"],
            "sha256": info["normalized_sha256"],
            "targets": [item["target"] for item in info["relocations"]],
        })

rows.sort(
    key=lambda row: (
        -row["size"],
        row["relocations"],
        row["unit"],
        row["function"],
    )
)
print(json.dumps(rows, indent=2))
