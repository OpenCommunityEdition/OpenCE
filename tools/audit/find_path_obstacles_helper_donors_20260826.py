from pathlib import Path
import json
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from tools.coff_compare import load, section_info, section_infos_equal


functions = (
    "_set_real_point2d",
    "_rotate_vector2d",
    "_project_point3d",
    "_point_in_circle",
    "_point_in_sphere",
)
target_obj = load("build/split/source/ai/path_obstacles.obj")

for function in functions:
    target = section_info(target_obj, function)
    print(function, json.dumps(target, sort_keys=True))
    for path in Path("build/base").rglob("*.obj"):
        obj = load(str(path))
        if not any(
            item["name"] == function and item["section"] > 0
            for item in obj["symbols"]
        ):
            continue

        print(
            " ",
            section_infos_equal(target, section_info(obj, function)),
            path)
