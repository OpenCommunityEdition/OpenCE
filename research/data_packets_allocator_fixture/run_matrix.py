"""Compile and measure the bounded data-packet allocator fixture matrix."""

from __future__ import annotations

import hashlib
import json
import re
import subprocess
import sys
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
HERE = Path(__file__).resolve().parent
CL = ROOT / "xbox" / "bin" / "vc7" / "CL.Exe"
OBJDUMP = Path(r"C:\Program Files\LLVM\bin\llvm-objdump.exe")
SOURCE = HERE / "fixture.c"
OUT = HERE / "objects"
sys.path.insert(0, str(ROOT))

from tools import coff_compare  # noqa: E402

CFLAGS = [
    "/nologo",
    "/c",
    "/O2",
    "/Oy-",
    "/DDEBUG",
    "/Dxbox",
]

VARIANTS = {
    0: "baseline: element then count; direct data pointer",
    1: "bind element assignment to verifier arg2 with comma expression",
    2: "bind element assignment to verifier arg1 with comma expression",
    3: "bind element assignment to verifier arg3 with comma expression",
    4: "load count before assigning element",
    5: "assign element after verifier call",
    6: "late packet-definition alias after element/count",
    7: "late packet alias initialized through comma expression",
    8: "data pointer local computed before validation/calls",
    9: "data pointer local computed between integer and memory calls",
    10: "bind data pointer assignment to encode_integer arg1",
    11: "array element as decoded_data then += 2",
    12: "typed short-pointer +1 spelling for both data and array",
    13: "array element routed through one-field cursor aggregate",
    14: "preserve array base; derive element after verifier",
    15: "block-local short raw_count",
    16: "element declaration before count (baseline expression order)",
    17: "load count through element-2 dependency",
    18: "load count through element, then increment element",
    19: "function-scope packet-definition alias",
}


def run(args: list[str]) -> subprocess.CompletedProcess[str]:
    return subprocess.run(
        args,
        cwd=ROOT,
        check=True,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )


def main() -> None:
    OUT.mkdir(exist_ok=True)
    results: list[dict[str, object]] = []

    for variant, description in VARIANTS.items():
        obj = OUT / f"fixture_{variant:02d}.obj"
        compile_result = run(
            [
                str(CL),
                *CFLAGS,
                f"/DVARIANT={variant}",
                f"/Fo{obj}",
                str(SOURCE),
            ]
        )
        dump = run([str(OBJDUMP), "-dr", str(obj)]).stdout
        (OUT / f"fixture_{variant:02d}.disasm.txt").write_text(dump, encoding="utf-8")

        loaded = coff_compare.load(obj)
        section = coff_compare.section_info(loaded, "_fixture")

        code_lines = [
            line.strip()
            for line in dump.splitlines()
            if re.match(r"^\s*[0-9a-f]+:\s+[0-9a-f ]+\s+", line)
        ]
        normalized = "\n".join(
            re.sub(r"(?<=calll\s)0x[0-9a-f]+", "CALL", line) for line in code_lines
        )
        calls = []
        lines = dump.splitlines()
        for index, line in enumerate(lines):
            if "IMAGE_REL_I386_REL32" in line:
                calls.append(
                    {
                        "target": line.split()[-1],
                        "window": [entry.strip() for entry in lines[max(0, index - 9) : index + 2]],
                    }
                )

        results.append(
            {
                "variant": variant,
                "description": description,
                "object_sha256": hashlib.sha256(obj.read_bytes()).hexdigest(),
                "disassembly_sha256": hashlib.sha256(normalized.encode()).hexdigest(),
                "normalized_sha256": section["normalized_sha256"],
                "code_size": section["size"],
                "relocation_count": section["relocation_count"],
                "instruction_count": len(code_lines),
                "object_size": obj.stat().st_size,
                "calls": calls,
                "compiler_output": compile_result.stdout.strip(),
            }
        )

    (HERE / "results.json").write_text(json.dumps(results, indent=2), encoding="utf-8")
    print(json.dumps(results, indent=2))


if __name__ == "__main__":
    main()
