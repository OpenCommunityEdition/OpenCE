#!/usr/bin/env python
"""Generate a checked CPU-only Xbox texture layout/decode slice for Mac.

The view retains the source-defined format table, dimension and face/mip
layout, Morton addressing, endian-safe texel conversion, and level decoder.
It stops before the shared renderer's GL cache and upload code.
"""
import os
import sys

SOURCE = 'port/linux/src/xbox_textures.c'
LAYOUT = 'port/linux/src/xgpu.h'
DEFAULT_OUTPUT = 'build/mac-audit/mac_xgpu_texture_decode_view.inc'
START = 'enum texel_kind\n'
END = '#ifdef HALO_GLES\n/* ---------- DXT decoding, for ES drivers without S3TC (Mali) */'
LAYOUT_START = 'struct xgpu_texture_description\n{'
LAYOUT_END = '\n};'

def require_once(text, anchor, message):
    if text.count(anchor) != 1:
        raise ValueError(message)

def between(text, first, last):
    start = text.index(first)
    return text[start:text.index(last, start)]

source = open(SOURCE, 'r').read()
layout = open(LAYOUT, 'r').read()
require_once(source, START, 'shared texture decoder start anchor changed')
require_once(source, END, 'shared texture decoder end anchor changed')
require_once(layout, LAYOUT_START, 'shared texture description anchor changed')
body = between(source, START, END)
description = between(layout, LAYOUT_START, LAYOUT_END) + LAYOUT_END
for name in ('xgpu_texture_describe', 'xgpu_texture_level_offset',
             'xgpu_texture_face_size', 'xgpu_texture_level_pitch'):
    if body.count(' ' + name + '(') < 1:
        raise ValueError('missing shared texture decoder function: ' + name)
    body = body.replace(name, 'mac_' + name)
if body.count('static BOOL decode_level(') != 1:
    raise ValueError('shared per-level decoder anchor changed')
body = body.replace('static BOOL decode_level(', 'static BOOL mac_decode_level(', 1)
output = ('/* Generated from port/linux/src/xgpu.h and xbox_textures.c; do not edit. */\n'
          'typedef char mac_texture_decode_ilp32[(sizeof(unsigned long) == 4) ? 1 : -1];\n'
          + description + '\n' + body)
destination = sys.argv[1] if len(sys.argv) > 1 else DEFAULT_OUTPUT
parent = os.path.dirname(destination)
if parent and not os.path.isdir(parent):
    os.makedirs(parent)
open(destination, 'w').write(output)
print(destination)
