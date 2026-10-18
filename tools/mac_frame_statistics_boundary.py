#!/usr/bin/env python
"""Preserve the two explicit MS-ABI private counter offsets on native Macs.
Runs on Python 2.6 and 3: usage mac_frame_statistics_boundary.py OUTPUT
"""
import sys
from mac_gen_io import read_text, write_text

source = read_text('source/rasterizer/rasterizer_frame_statistics.c')
for name in ('fps_accumulation_time', 'fps_accumulation_frame_index'):
    before = 'unsigned __int64 ' + name + ';'
    if source.count(before) != 1:
        raise SystemExit('Expected one private counter declaration: ' + name)
    source = source.replace(before, 'unsigned __int64 ' + name + ' __attribute__((aligned(8)));')
write_text(sys.argv[1], '#line 1 "source/rasterizer/rasterizer_frame_statistics.c"\n' + source)
