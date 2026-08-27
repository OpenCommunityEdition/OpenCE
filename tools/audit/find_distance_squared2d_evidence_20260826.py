from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from tools.coff_compare import load, section_info, section_infos_equal


function = "_distance_squared2d"
targets = []
bases = []

for path in Path("build/split").rglob("*.obj"):
    obj = load(str(path))
    if any(
        item["name"] == function and item["section"] > 0
        for item in obj["symbols"]
    ):
        targets.append((path, obj))

for path in Path("build/base").rglob("*.obj"):
    obj = load(str(path))
    if any(
        item["name"] == function and item["section"] > 0
        for item in obj["symbols"]
    ):
        bases.append((path, obj))

for target_path, target_obj in targets:
    target = section_info(target_obj, function)
    print("target", target_path, target)
    for base_path, base_obj in bases:
        print(
            " ",
            section_infos_equal(target, section_info(base_obj, function)),
            base_path)
