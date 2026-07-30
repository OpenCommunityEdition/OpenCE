"""Exhaustively test legal SWAP4 term orderings under the campaign compiler."""

from itertools import permutations
import json
from pathlib import Path
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))

from tools.coff_compare import load, section_info, section_infos_equal


SOURCE = ROOT / "source" / "bungie_net" / "common" / "public_key_crypt.c"
TARGET = ROOT / "build" / "split" / "source" / "bungie_net" / "common" / "public_key_crypt.obj"
BASE = ROOT / "build" / "base" / "source" / "bungie_net" / "common" / "public_key_crypt.obj"
NINJA = Path(
    r"C:\Users\isabe\AppData\Local\Microsoft\WinGet\Packages"
    r"\Ninja-build.Ninja_Microsoft.Winget.Source_8wekyb3d8bbwe\ninja.exe"
)

FUNCTIONS = [
    "_code_0006f630",
    "_code_0006f700",
    "_code_0006f780",
    "_generate_key_parameters",
    "_generate_public_key",
    "_generate_private_key",
]

BASELINE = """\
\t\tprivate_key->dwords[i] =
\t\t\t((value & 0xFF000000) >> 24) |
\t\t\t((value & 0x00FF0000) >> 8) |
\t\t\t((value & 0x0000FF00) << 8) |
\t\t\t((value & 0x000000FF) << 24);"""

TERMS = {
    "A": "(value >> 24)",
    "B": "((value >> 8) & 0x0000FF00)",
    "C": "((value << 8) & 0x00FF0000)",
    "D": "(value << 24)",
}


def build():
    completed = subprocess.run(
        [str(NINJA), str(BASE.relative_to(ROOT))],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if completed.returncode:
        raise RuntimeError(completed.stdout)


def replacement(order):
    lines = ["\t\tprivate_key->dwords[i] ="]
    for index, name in enumerate(order):
        suffix = ";" if index == len(order) - 1 else " |"
        lines.append(f"\t\t\t{TERMS[name]}{suffix}")
    return "\n".join(lines)


def measure(order):
    target = load(TARGET)
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
    return {
        "order": "".join(order),
        "all_equal": all(row["equal"] for row in rows.values()),
        "functions": rows,
    }


def main():
    original_bytes = SOURCE.read_bytes()
    original = original_bytes.decode("ascii")
    if original.count(BASELINE) != 1:
        raise RuntimeError("baseline byte-swap block is absent or ambiguous")

    results = []
    try:
        for order in permutations(TERMS):
            SOURCE.write_bytes(
                original.replace(BASELINE, replacement(order)).encode("ascii")
            )
            build()
            result = measure(order)
            results.append(result)
            private = result["functions"]["_generate_private_key"]
            print(
                result["order"],
                private["size"],
                private["relocations"],
                private["sha256"],
                private["equal"],
                "siblings=",
                all(
                    result["functions"][name]["equal"]
                    for name in FUNCTIONS[:-1]
                ),
                flush=True,
            )
            if result["all_equal"]:
                break
    finally:
        SOURCE.write_bytes(original_bytes)
        build()

    output = Path(__file__).with_name("swap_term_permutations.json")
    output.write_text(json.dumps(results, indent=2) + "\n")
    print(f"wrote {output}")


if __name__ == "__main__":
    main()
