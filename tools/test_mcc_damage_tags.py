"""The real metadata reader rejects non-finite MCC extension acceleration."""
import struct
import pytest
from tools.test_mcc_cameras import camera_tool, convert


@pytest.mark.parametrize("flags,values,accepted", [
    (0x30000, (0, -0.1, 2.45), 1),
    (0x10000, (-1, -1, 0), 1),
    (0x20000, (-1, float("nan"), float("inf")), 1),
    (0x30000, (0, float("nan"), 2), 0),
    (0x30000, (0, 0, float("inf")), 0),
    (0x20000, (float("-inf"), 0, 0), 0),
    (0, (1, float("nan"), float("inf")), 1),
])
def test_mcc_damage_components(camera_tool, tmp_path, flags, values, accepted):
    data = bytearray(0x2C0)
    struct.pack_into("<I", data, 0, int.from_bytes(b"jpt!", "big"))
    struct.pack_into("<I", data, 20, 0x50000020)
    struct.pack_into("<I", data, 0x20 + 0x1C8, flags)
    struct.pack_into("<3f", data, 0x20 + 0x1F4, *values)
    assert convert(camera_tool, tmp_path, data) == (accepted, data)


def test_mcc_damage_truncated_root(camera_tool, tmp_path):
    data = bytearray(0x2BF)
    struct.pack_into("<I", data, 0, int.from_bytes(b"jpt!", "big"))
    struct.pack_into("<I", data, 20, 0x50000020)
    assert convert(camera_tool, tmp_path, data)[0] == 0
