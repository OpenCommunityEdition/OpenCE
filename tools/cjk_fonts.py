#!/usr/bin/env python3
"""
Subsets of Noto Sans CJK (SIL Open Font License 1.1) for the subtitles of the
CJK languages (port/assets/fonts/README.md, port/linux/src/text_hires.c).

    python3 tools/cjk_fonts.py <noto-cjk checkout>/Sans/SubsetOTF port/assets/fonts

Each language gets the glyphs of its region's own forms, for the characters
that the region's legacy character set holds as common: GB 2312 for Chinese
(Simplified), Big5's frequently used characters for Chinese (Traditional),
JIS X 0208's first level for Japanese (and all its kana) and KS X 1001's
Hangul for Korean, each with its set's punctuation and symbols.
"""
import sys
from pathlib import Path

from fontTools import subset

WEIGHT = "Medium"
# language: (the font's region folder, codec, byte ranges of the common characters)
SETS = {
    "SC": ("SC", "gb2312", [((0xA1, 0xA1), (0xA9, 0xFE)), ((0xB0, 0xA1), (0xD7, 0xF9))]),
    "TC": ("TC", "big5", [((0xA1, 0x40), (0xC6, 0x7E))]),
    "JP": ("JP", "shift_jis", None),
    "KR": ("KR", "euc_kr", [((0xA1, 0xA1), (0xAC, 0xFE)), ((0xB0, 0xA1), (0xC8, 0xFE))]),
}


def codepoints(codec: str, ranges):
    found = set()
    if codec == "shift_jis":
        # JIS X 0208 levels 1 (rows 1 to 48 of the two-byte area) and the symbols
        for lead in range(0x81, 0x89):
            for trail in range(0x40, 0xFD):
                if trail == 0x7F:
                    continue
                try:
                    found.add(ord(bytes([lead, trail]).decode(codec)))
                except (UnicodeDecodeError, TypeError):
                    pass
        for lead in range(0x88, 0x99):  # (level 1 ends at 0x889E-0x9872)
            for trail in range(0x40, 0xFD):
                if trail == 0x7F or (lead, trail) < (0x88, 0x9F) or (lead, trail) > (0x98, 0x72):
                    continue
                try:
                    found.add(ord(bytes([lead, trail]).decode(codec)))
                except (UnicodeDecodeError, TypeError):
                    pass
    else:
        for (lead0, trail0), (lead1, trail1) in ranges:
            for lead in range(lead0, lead1 + 1):
                for trail in range(0x40 if codec == "big5" else 0xA1, 0xFF):
                    if codec != "big5" and not (0xA1 <= trail <= 0xFE):
                        continue
                    if (lead, trail) < (lead0, trail0) or (lead, trail) > (lead1, trail1):
                        continue
                    try:
                        found.add(ord(bytes([lead, trail]).decode(codec)))
                    except (UnicodeDecodeError, TypeError):
                        pass
    # kana and the CJK symbols and full-width forms of every language
    found.update(range(0x3000, 0x3040))
    found.update(range(0x3041, 0x3100))
    found.update(range(0xFF01, 0xFF5F))
    found.update({0x2026, 0x2014, 0x2018, 0x2019, 0x201C, 0x201D, 0x00B7})
    return sorted(c for c in found if c >= 0x80)


def main() -> None:
    source, target = Path(sys.argv[1]), Path(sys.argv[2])
    for region, (folder, codec, ranges) in SETS.items():
        options = subset.Options()
        options.layout_features = ["vert"]
        options.name_IDs = [0, 1, 2, 3, 4, 5, 6, 13, 14]
        options.notdef_outline = False
        options.hinting = False
        options.drop_tables += ["DSIG", "GSUB", "GPOS", "GDEF", "BASE", "vhea", "vmtx", "VORG"]
        font = subset.load_font(source / folder / f"NotoSans{region}-{WEIGHT}.otf", options)
        subsetter = subset.Subsetter(options)
        subsetter.populate(unicodes=codepoints(codec, ranges))
        subsetter.subset(font)
        out = target / f"NotoSansCJK-{region}-{WEIGHT}-subset.otf"
        subset.save_font(font, out, options)
        print(out, out.stat().st_size)


if __name__ == "__main__":
    main()
