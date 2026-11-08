#!/usr/bin/env python3
"""Embeds the MCC import's data (port/assets/mcc: each disc map's values as
operations, <map>.p2, and the fingerprint of its simulation data, <map>.fp)
in the game as C data, each file deflated, for the importer
(port/linux/game/mcc_import_patch.c, mcc_import_data_read):

    python tools/mcc_patches_embed.py OUTPUT.c

The file defines its own structure, as mcc_import_patch.c declares it. The
data are 32-bit words, as tools/embed_assets.py writes them. The Linux and
Windows builds generate it; the Android and web builds have no importer yet.
"""

import struct
import sys
import zlib
from pathlib import Path
from typing import Any, List

ROOT = Path(__file__).resolve().parent.parent
DATA = Path("port/assets/mcc")


def inputs() -> List[Path]:
    """the files the generated C is made from"""
    return sorted(DATA / path.name for path in (ROOT / DATA).glob("*") if path.suffix in (".p2", ".fp"))


def mcc_patches_configure_inputs() -> List[Path]:
    """What configure.py is run again for: the folder (a file added or
    removed), not each file, which the build itself depends on."""
    return [DATA] if (ROOT / DATA).is_dir() else []


def mcc_patches_build(n: Any, prefix: str, output: Path) -> List[Path]:
    """Emits the rule that generates output; returns [output]."""
    n.rule(
        name=f"{prefix}_mcc_patches",
        command="$python tools/mcc_patches_embed.py $out",
        description=f"{prefix.upper()} EMBED $out",
    )
    n.build(outputs=output, rule=f"{prefix}_mcc_patches", implicit=[Path("tools/mcc_patches_embed.py"), *inputs()])
    return [output]


def main() -> None:
    if len(sys.argv) != 2:
        sys.exit("usage: mcc_patches_embed.py OUTPUT.c")
    lines = [
        "/* made by tools/mcc_patches_embed.py from port/assets/mcc: do not edit */",
        "",
        "struct mcc_patch_embedded",
        "{",
        "\tconst char *name;",
        "\tconst unsigned int *data;",
        "\tunsigned int size;",
        "\tunsigned int inflated_size;",
        "};",
        "",
    ]
    files = inputs()
    entries = []
    for index, path in enumerate(files):
        # (as committed: a Windows checkout may have CRLF line endings)
        text = (ROOT / path).read_bytes().replace(b"\r\n", b"\n")
        deflated = zlib.compress(text, 9)
        padded = deflated + b"\0" * (-len(deflated) % 4)
        values = struct.unpack(f"<{len(padded) // 4}I", padded)
        lines.append(f"static const unsigned int mcc_patch{index}[] = {{")
        lines += ["\t" + ", ".join(f"0x{word:08x}" for word in values[start:start + 8]) + ","
                  for start in range(0, len(values), 8)]
        lines.append("};")
        entries.append(f'\t{{ "{path.name}", mcc_patch{index}, {len(deflated)}u, {len(text)}u }},')
    lines.append("const struct mcc_patch_embedded mcc_patches_embedded[] =")
    lines.append("{")
    lines += entries or ["\t{ 0, 0, 0u, 0u },"]
    lines.append("};")
    lines.append(f"const unsigned int mcc_patches_embedded_count = {len(entries)}u;")
    Path(sys.argv[1]).parent.mkdir(parents=True, exist_ok=True)
    Path(sys.argv[1]).write_text("\n".join(lines) + "\n", encoding="ascii", newline="\n")


if __name__ == "__main__":
    main()
