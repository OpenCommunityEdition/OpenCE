"""Admission rejects unsupported movement values instead of silently ignoring them."""
import math
import struct
import pytest
from test_mcc_cameras import camera_tool, convert

@pytest.mark.parametrize('mode,forward,sideways,accepted', [
    (0,0,0,1), (0,.333,.5,1), (1,1,2,1), (2,2,1,1),
    (3,0,0,0), (65535,0,0,0), (0,-.1,0,0), (0,0,2.1,0),
    (0,math.nan,0,0), (0,0,math.inf,0), (0,-math.inf,0,0)])
def test_mcc_weapon_penalty_admission(camera_tool,tmp_path,mode,forward,sideways,accepted):
    data=bytearray(0x528)
    struct.pack_into('<I',data,0,int.from_bytes(b'weap','big'))
    struct.pack_into('<I',data,20,0x50000020)
    struct.pack_into('<H2xff',data,0x20+0x3fc,mode,forward,sideways)
    assert convert(camera_tool,tmp_path,data)==(accepted,data)
