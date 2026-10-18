#!/usr/bin/env python
"""Reuse exact shared CPU resource declarations/layout code without GL headers."""
import os
import sys
outdir = sys.argv[1] if len(sys.argv) > 1 else 'build/mac-audit'
if not os.path.isdir(outdir): os.makedirs(outdir)
header = open('port/linux/src/xgpu.h').read()
start = header.index('struct xgpu_texture_description\n')
end = header.index('/* the GL texture for an Xbox texture header', start)
with open(outdir + '/mac_resource_declarations.h', 'w') as out:
    out.write('/* Generated from the shared xgpu.h; do not edit. */\n')
    out.write(header[start:end])
source = open('port/linux/src/xbox_textures.c').read()
start = source.index('enum texel_kind\n')
end = source.index('/* ---------- swizzling */', start)
with open(outdir + '/mac_texture_layout.inc', 'w') as out:
    out.write('/* Exact shared CPU texture layout section; do not edit. */\n')
    out.write(source[start:end])
