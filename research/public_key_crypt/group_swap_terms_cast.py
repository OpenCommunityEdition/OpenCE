"""Search SWAP4 order/grouping with the one proven low-register cast control."""

from itertools import permutations
import json
from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
sys.path.insert(0, str(Path(__file__).resolve().parent))

from tools.coff_compare import load, section_info, section_infos_equal
from permute_swap_terms import BASE, BASELINE, FUNCTIONS, SOURCE, TARGET, build


TERMS = {
    "A": "(value >> 24)",
    "B": "((value >> 8) & 0x0000FF00)",
    "C": "((((unsigned short)value) << 8) & 0x00FF0000)",
    "D": "(value << 24)",
}


def trees(names):
    if len(names) == 1:
        name = names[0]
        return [(name, TERMS[name])]
    result = []
    for split in range(1, len(names)):
        for left_shape, left_expr in trees(names[:split]):
            for right_shape, right_expr in trees(names[split:]):
                result.append(
                    (
                        f"({left_shape}|{right_shape})",
                        f"({left_expr} | {right_expr})",
                    )
                )
    return result


def replacement(expression):
    return (
        "\t\tprivate_key->dwords[i] =\n"
        f"\t\t\t{expression};"
    )


def main():
    original_bytes = SOURCE.read_bytes()
    original = original_bytes.decode("ascii")
    if original.count(BASELINE) != 1:
        raise RuntimeError("baseline byte-swap block is absent or ambiguous")

    target = load(TARGET)
    results = []
    seen = set()
    exact = None
    try:
        for order in permutations(TERMS):
            for shape, expression in trees(order):
                SOURCE.write_bytes(
                    original.replace(BASELINE, replacement(expression)).encode("ascii")
                )
                build()
                base = load(BASE)
                rows = {}
                for name in FUNCTIONS:
                    target_info = section_info(target, name)
                    base_info = section_info(base, name)
                    rows[name] = {
                        "equal": section_infos_equal(target_info, base_info),
                        "size": base_info["size"],
                        "relocations": base_info["relocation_count"],
                        "sha256": base_info["normalized_sha256"],
                    }
                private = rows["_generate_private_key"]
                result = {
                    "order": "".join(order),
                    "shape": shape,
                    "functions": rows,
                    "all_equal": all(row["equal"] for row in rows.values()),
                }
                results.append(result)
                signature = (
                    private["size"],
                    private["relocations"],
                    private["sha256"],
                )
                if signature not in seen:
                    seen.add(signature)
                    print(
                        result["order"],
                        shape,
                        *signature,
                        private["equal"],
                        flush=True,
                    )
                if result["all_equal"]:
                    exact = result
                    print(f"STRICT EXACT: {result['order']} {shape}", flush=True)
                    return
    finally:
        SOURCE.write_bytes(original_bytes)
        build()
        output = Path(__file__).with_name("swap_cast_groupings.json")
        output.write_text(json.dumps(results, indent=2) + "\n")
        print(f"variants={len(results)} unique_results={len(seen)} exact={exact}")
        print(f"wrote {output}")


if __name__ == "__main__":
    main()
