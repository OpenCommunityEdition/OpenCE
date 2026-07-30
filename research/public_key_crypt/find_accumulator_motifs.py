"""Find January functions using AND-EAX / SHL-ECX / OR-ECX,EAX motifs."""

from pathlib import Path
import re
import sys


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))

from tools.coff_compare import CoffError, load


# 25 imm32       and eax, imm32
# c1 e1 imm8     shl ecx, imm8
# 0b c8          or  ecx, eax
PATTERN = re.compile(rb"\x25(.{4})\xc1\xe1(.)\x0b\xc8", re.DOTALL)


def main():
    split_root = ROOT / "build" / "split"
    rows = []
    for path in split_root.rglob("*.obj"):
        try:
            obj = load(path)
        except CoffError:
            continue
        for section in obj["sections"]:
            if not (section["flags"] & 0x20) or not section["raw"]:
                continue
            raw = obj["data"][section["raw"]:section["raw"] + section["size"]]
            matches = list(PATTERN.finditer(raw))
            if not matches:
                continue
            symbols = [
                symbol["name"]
                for symbol in obj["symbols"]
                if symbol["section"] == section["index"]
                and symbol["value"] == 0
                and symbol["name"].startswith("_")
            ]
            for match in matches:
                rows.append(
                    (
                        str(path.relative_to(split_root)),
                        ",".join(symbols),
                        section["size"],
                        match.start(),
                        match.group(1).hex(),
                        match.group(2).hex(),
                    )
                )
    print("object|symbols|size|offset|and_imm_le|shift")
    for row in sorted(rows):
        print("|".join(map(str, row)))
    print(f"hits={len(rows)}")


if __name__ == "__main__":
    main()
