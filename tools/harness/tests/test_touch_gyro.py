"""Android gyro modes preserve saved input and discard unzoomed samples."""
import re
import sys
import xml.etree.ElementTree as ET
from pathlib import Path

import pytest

sys.path.insert(0, str(Path(__file__).resolve().parents[2]))
from harness import build, read, run


@pytest.mark.parametrize("case", ["legacy", "modes", "axes"])
def test_gyro(case):
    source = read("port/linux/src/touch_gyro.h") + read("port/linux/src/touch_gyro.c")
    source = re.sub(r'^#include.*\n', '', source, flags=re.M)
    status, output = run(build("touch_gyro", (("under_test.inc", source),)), case)
    assert status == 0, output


def test_android_menu():
    import port_settings
    spec = port_settings.SCREENS["gamepad_setup/gyro"]
    assert [value for _, value in spec["rows"][0][2]] == ["legacy", "false", "true", "zoom"]
    assert all(row[4] == "android" for row in spec["rows"])
    setup = port_settings.SCREENS["gamepad_setup"]
    xml = ET.fromstring("<menus>" + "\n".join(port_settings._setting_screen("gamepad_setup", setup)) + "</menus>")
    children = next(w for w in xml if w.get("name", "").endswith("/options_menu")).findall("child")
    for platform, count, spacing in [("desktop", 10, 27), ("web", 10, 27), ("android", 11, 24)]:
        rows = [c for c in children if "/op_" in c.get("widget", "") and
                (not c.get("platform") or platform in c.get("platform").split())]
        assert len(rows) == count
        assert [int(c.get("y")) for c in rows] == [73 + i * spacing for i in range(count)]
        assert int(rows[-1].get("y")) + 28 <= 350
