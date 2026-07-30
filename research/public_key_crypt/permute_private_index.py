"""Exhaust plausible C89 index types for generate_private_key."""

import json
from pathlib import Path
import re
import subprocess
import sys


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))

from tools.coff_compare import load, section_info, section_infos_equal


SOURCE = ROOT / "source/bungie_net/common/public_key_crypt.c"
TARGET = ROOT / "build/split/source/bungie_net/common/public_key_crypt.obj"
BASE = ROOT / "build/base/source/bungie_net/common/public_key_crypt.obj"
NINJA = Path(
    r"C:\Users\isabe\AppData\Local\Microsoft\WinGet\Packages"
    r"\Ninja-build.Ninja_Microsoft.Winget.Source_8wekyb3d8bbwe\ninja.exe"
)
OBJECT_TARGET = "build/base/source/bungie_net/common/public_key_crypt.obj"
TYPES = ("long", "unsigned long", "int", "unsigned int", "short", "unsigned short")


def build():
    subprocess.run(
        [str(NINJA), OBJECT_TARGET],
        cwd=ROOT,
        check=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
    )


def main():
    original = SOURCE.read_bytes()
    pattern = re.compile(
        rb"(void generate_private_key\([\s\S]*?\)\r?\n\{\r?\n\t)"
        rb"long i;"
    )
    if not pattern.search(original):
        raise RuntimeError("private-key index declaration not found")
    target = load(TARGET)
    results = []
    try:
        for type_name in TYPES:
            candidate = pattern.sub(
                lambda match: match.group(1) + type_name.encode() + b" i;",
                original,
                count=1,
            )
            SOURCE.write_bytes(candidate)
            build()
            base = load(BASE)
            target_info = section_info(target, "_generate_private_key")
            base_info = section_info(base, "_generate_private_key")
            row = {
                "type": type_name,
                "equal": section_infos_equal(target_info, base_info),
                "size": base_info["size"],
                "relocations": len(base_info["relocations"]),
                "sha256": base_info["normalized_sha256"],
            }
            results.append(row)
            print(row)
    finally:
        SOURCE.write_bytes(original)
        build()
    output = Path(__file__).with_name("private_index_types.json")
    output.write_text(json.dumps(results, indent=2) + "\n")
    print(f"wrote {output}")


if __name__ == "__main__":
    main()
