from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from tools.coff_compare import load, section_info, section_infos_equal


target = section_info(
    load("build/split/source/ai/action_alert.obj"),
    "_distance_squared3d")

for path in Path("build/base").rglob("*.obj"):
    obj = load(str(path))
    if not any(
        item["name"] == "_distance_squared3d" and item["section"] > 0
        for item in obj["symbols"]
    ):
        continue

    print(
        section_infos_equal(target, section_info(obj, "_distance_squared3d")),
        path)
