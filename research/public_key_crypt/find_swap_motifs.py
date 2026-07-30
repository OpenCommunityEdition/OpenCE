"""Find January byte-swap motifs and identify strict-exact source donors."""

from pathlib import Path
import sys


ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))

from tools.coff_compare import CoffError, load, section_info, section_infos_equal


PATTERNS = {
    "target_low_half": bytes.fromhex(
        "8b c8 25 00 ff 00 00 c1 e1 10 0b c8"
    ),
    "target_low_core": bytes.fromhex(
        "25 00 ff 00 00 c1 e1 10 0b c8"
    ),
    "target_full_pair": bytes.fromhex(
        "8b d0 8b c8 c1 e9 10 81 e2 00 00 ff 00 0b d1 "
        "8b c8 25 00 ff 00 00 c1 e1 10 0b c8"
    ),
}


def code_symbols(obj, section_index):
    return [
        symbol
        for symbol in obj["symbols"]
        if symbol["section"] == section_index
        and symbol["value"] == 0
        and symbol["name"].startswith("_")
    ]


def main():
    split_root = ROOT / "build" / "split"
    base_root = ROOT / "build" / "base"
    hits = []

    for target_path in split_root.rglob("*.obj"):
        try:
            target_obj = load(target_path)
        except CoffError:
            continue
        relative = target_path.relative_to(split_root)
        base_path = base_root / relative
        try:
            base_obj = load(base_path) if base_path.exists() else None
        except CoffError:
            base_obj = None

        for section in target_obj["sections"]:
            if not (section["flags"] & 0x20) or section["raw"] == 0:
                continue
            raw = target_obj["data"][
                section["raw"]:section["raw"] + section["size"]
            ]
            matched = [
                (name, raw.find(pattern))
                for name, pattern in PATTERNS.items()
                if raw.find(pattern) >= 0
            ]
            if not matched:
                continue

            symbols = code_symbols(target_obj, section["index"])
            if not symbols:
                continue
            for symbol in symbols:
                exact = False
                if base_obj is not None:
                    try:
                        exact = section_infos_equal(
                            section_info(target_obj, symbol["name"]),
                            section_info(base_obj, symbol["name"]),
                        )
                    except CoffError:
                        pass
                hits.append(
                    (
                        str(relative),
                        symbol["name"],
                        section["size"],
                        exact,
                        ",".join(f"{name}@0x{offset:x}" for name, offset in matched),
                    )
                )

    print("object|symbol|size|strict_exact|motifs")
    for hit in sorted(hits):
        print("|".join(map(str, hit)))
    print(f"hits={len(hits)}")


if __name__ == "__main__":
    main()
