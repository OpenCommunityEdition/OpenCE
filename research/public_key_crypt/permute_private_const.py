"""Exhaust the ABI-neutral pointee-const variants of generate_private_key."""

from itertools import product
import json
from pathlib import Path
import re
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))

from tools.coff_compare import load, section_info, section_infos_equal


SOURCE = ROOT / "source/bungie_net/common/public_key_crypt.c"
HEADER = ROOT / "source/bungie_net/common/public_key_crypt.h"
TARGET = ROOT / "build/split/source/bungie_net/common/public_key_crypt.obj"
BASE = ROOT / "build/base/source/bungie_net/common/public_key_crypt.obj"
NINJA = Path(
    r"C:\Users\isabe\AppData\Local\Microsoft\WinGet\Packages"
    r"\Ninja-build.Ninja_Microsoft.Winget.Source_8wekyb3d8bbwe\ninja.exe"
)
OBJECT_TARGET = "build/base/source/bungie_net/common/public_key_crypt.obj"
FUNCTIONS = [
    "_code_0006f630",
    "_code_0006f700",
    "_code_0006f780",
    "_generate_key_parameters",
    "_generate_public_key",
    "_generate_private_key",
]


def replace_function_block(data, definition, qualifiers):
    ending = rb"\)\r?\n\{" if definition else rb"\);"
    pattern = re.compile(
        rb"void generate_private_key\([\s\S]*?" + ending
    )
    match = pattern.search(data)
    if not match:
        raise RuntimeError("generate_private_key block not found")
    block = match.group()
    for name, keep_const in zip((b"public_key", b"p", b"x"), qualifiers):
        old = b"struct public_key const *" + name
        new = old if keep_const else b"struct public_key *" + name
        if old not in block:
            raise RuntimeError(f"parameter not found: {name!r}")
        block = block.replace(old, new, 1)
    return data[:match.start()] + block + data[match.end():]


def build():
    subprocess.run(
        [str(NINJA), OBJECT_TARGET],
        cwd=ROOT,
        check=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )


def measure():
    target = load(TARGET)
    base = load(BASE)
    rows = {}
    for function in FUNCTIONS:
        target_info = section_info(target, function)
        base_info = section_info(base, function)
        rows[function] = {
            "equal": section_infos_equal(target_info, base_info),
            "size": base_info["size"],
            "relocations": len(base_info["relocations"]),
            "sha256": base_info["normalized_sha256"],
        }
    return rows


def main():
    source_original = SOURCE.read_bytes()
    header_original = HEADER.read_bytes()
    results = []
    try:
        for qualifiers in product((False, True), repeat=3):
            SOURCE.write_bytes(
                replace_function_block(source_original, True, qualifiers)
            )
            HEADER.write_bytes(
                replace_function_block(header_original, False, qualifiers)
            )
            build()
            rows = measure()
            results.append(
                {
                    "const": dict(
                        zip(("public_key", "p", "x"), qualifiers)
                    ),
                    "functions": rows,
                }
            )
            private = rows["_generate_private_key"]
            print(
                qualifiers,
                private["equal"],
                private["size"],
                private["relocations"],
                private["sha256"],
            )
    finally:
        SOURCE.write_bytes(source_original)
        HEADER.write_bytes(header_original)
        build()
    output = Path(__file__).with_name("private_const_permutations.json")
    output.write_text(json.dumps(results, indent=2) + "\n")
    print(f"wrote {output}")


if __name__ == "__main__":
    main()
