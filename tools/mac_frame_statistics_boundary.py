#!/usr/bin/env python3
"""Preserve the two explicit MS-ABI private counter offsets on native Macs."""
from pathlib import Path
import argparse
p = argparse.ArgumentParser()
p.add_argument('output', type=Path)
a = p.parse_args()
source = Path('source/rasterizer/rasterizer_frame_statistics.c').read_text()
for name in ('fps_accumulation_time', 'fps_accumulation_frame_index'):
    before = 'unsigned __int64 ' + name + ';'
    if source.count(before) != 1:
        raise SystemExit('Expected one private counter declaration: ' + name)
    source = source.replace(before, 'unsigned __int64 ' + name + ' __attribute__((aligned(8)));')
a.output.parent.mkdir(parents=True, exist_ok=True)
a.output.write_text('#line 1 "source/rasterizer/rasterizer_frame_statistics.c"\n' + source)
