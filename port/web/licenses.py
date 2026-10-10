"""Writes the web site's licenses.txt (tools/web_build.py): the notices of
the code and fonts that halo.wasm and the page carry, which their licences
ask copies to include, as the other builds' releases carry them
(tools/ci_build.py).

    python port/web/licenses.py OUTPUT SDL3-LICENSE
"""

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent

# what each notice is for, and its file
NOTICES = [
    ("Expat (the menus' XML)", "port/third_party/expat/COPYING"),
    ("Opus (voice chat's codec)", "port/third_party/opus/COPYING"),
    ("zlib", "port/third_party/zlib/LICENSE"),
    ("KCP (internet play's streams)", "port/third_party/kcp/LICENSE"),
    ("Monocypher (internet play's signatures)", "port/third_party/monocypher/LICENCE.md"),
    ("tomlc17 (config.toml)", "port/third_party/tomlc17/LICENSE"),
    ("musl (the maths functions)", "port/third_party/musl-math/COPYRIGHT"),
    ("stb_vorbis (loose sounds)", "port/third_party/stb/LICENSE"),
    ("extract-xiso (how the disc image is read)", "port/third_party/extract-xiso/LICENSE.TXT"),
    ("Overpass (the menus' text and the page's)", "port/assets/fonts/Overpass-OFL.txt"),
    ("OpenCE (the title font, from Newtown)", "port/assets/fonts/OpenCE-OFL.txt"),
    ("Newtown", "port/assets/fonts/Newtown-LICENSE.txt"),
    ("Lucide (voice chat's speaker icons)", "port/assets/icons/lucide/LICENSE"),
]


def main() -> None:
    output, sdl_license = Path(sys.argv[1]), Path(sys.argv[2])
    parts = [
        "OpenCE for the web\n\n"
        "OpenCE's own code is CC0 (https://github.com/OpenCommunityEdition/OpenCE). The game data\n"
        "comes from the player's own disc. It carries the following, under their licences:\n",
        f"\n==================== SDL 3 (windows, input and audio)\n\n{sdl_license.read_text(encoding='utf-8')}",
    ]
    for title, path in NOTICES:
        parts.append(f"\n==================== {title}\n\n{(ROOT / path).read_text(encoding='utf-8')}")
    output.write_text("".join(parts), encoding="utf-8")


if __name__ == "__main__":
    main()
