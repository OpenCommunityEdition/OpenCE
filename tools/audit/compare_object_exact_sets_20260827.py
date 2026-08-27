import json
import sys
from pathlib import Path


sys.path.insert(0, str(Path(__file__).resolve().parents[2]))

from tools.coff_compare import (
    CoffError,
    IMAGE_SCN_CNT_CODE,
    load,
    section_info,
    section_infos_equal,
)


def function_names(obj):
    return sorted({
        item["name"]
        for item in obj["symbols"]
        if item["section"] > 0
        and item["value"] == 0
        and obj["sections"][item["section"] - 1]["flags"] & IMAGE_SCN_CNT_CODE
    })


def exact_names(target, candidate):
    result = set()
    for name in function_names(target):
        try:
            if section_infos_equal(
                section_info(target, name), section_info(candidate, name)
            ):
                result.add(name)
        except CoffError:
            pass
    return result


target = load(sys.argv[1])
baseline = load(sys.argv[2])
candidate = load(sys.argv[3])
baseline_exact = exact_names(target, baseline)
candidate_exact = exact_names(target, candidate)

print(json.dumps({
    "baseline_exact": sorted(baseline_exact),
    "candidate_exact": sorted(candidate_exact),
    "lost_exact": sorted(baseline_exact - candidate_exact),
    "newly_exact": sorted(candidate_exact - baseline_exact),
}, indent=2))
