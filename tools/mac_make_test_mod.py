"""Writes a small synthetic mod into a data root's mods folder for the Mac build's mods check: a sound, a bitmap and a
unicode string list over Blood Gulch's own tags, made by tools/test_mods.py's writers (no mod is downloaded).

usage: python3 tools/mac_make_test_mod.py DATA_ROOT [FOLDER]"""

import sys
import types
from pathlib import Path

# (test_mods.py imports pytest for its tests; the writers need none of it)
stub = types.ModuleType("pytest")
stub.fixture = lambda *args, **kwargs: (lambda function: function)
sys.modules.setdefault("pytest", stub)
sys.path.insert(0, str(Path(__file__).resolve().parent))
import test_mods  # noqa: E402


def main() -> int:
    if len(sys.argv) not in (2, 3):
        print(__doc__)
        return 2
    data = Path(sys.argv[1])
    folder = sys.argv[2] if len(sys.argv) == 3 else "test"
    test_mods.write_mod(data, {
        f"{test_mods.SOUND}.sound": test_mods.sound_file(),
        f"{test_mods.BITMAP}.bitmap": test_mods.bitmap_file(),
        f"{test_mods.STRINGS}.unicode_string_list": test_mods.strings_file(["MOD TEXT"] * 4),
    }, folder)
    return 0


if __name__ == "__main__":
    sys.exit(main())
