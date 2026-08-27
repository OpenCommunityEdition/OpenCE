from pathlib import Path
import json
import sys

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from tools.coff_compare import load, section_info, section_infos_equal


functions = (
    "_projection_from_vector3d",
    "_projection_sign_from_vector3d",
    "_project_point2d",
    "_triple_product3d",
    "_plane2d_from_points",
    "_plane3d_from_point_and_normal",
    "_plane3d_negate",
    "_plane3d_distance_to_point",
)
target_path = Path("build/split/source/effects/decals.obj")
target_obj = load(str(target_path))
base_objects = []

for path in Path("build/base").rglob("*.obj"):
    obj = load(str(path))
    base_objects.append((path, obj))

for function in functions:
    target = section_info(target_obj, function)
    print("target", function, json.dumps(target, sort_keys=True))

    for path, obj in base_objects:
        if not any(
            item["name"] == function and item["section"] > 0
            for item in obj["symbols"]
        ):
            continue

        candidate = section_info(obj, function)
        print(
            " ",
            section_infos_equal(target, candidate),
            path,
            json.dumps(candidate, sort_keys=True))
