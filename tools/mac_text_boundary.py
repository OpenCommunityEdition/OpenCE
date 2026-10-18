#!/usr/bin/env python
"""Generate actual CPU font services and endian-neutral font byte arrays.
Runs on Python 2.6 and 3: usage mac_text_boundary.py OUTPUT_DIR
(writes mac_text_body.h, mac_text_gl_body.h and mac_text_fonts.c there)
"""
import json
import os
import sys
from mac_gen_io import read_bytes, read_text, write_text

output = sys.argv[1]
source = read_text('port/linux/src/text_hires.c')
marker = 'unsigned int text_hires_atlas_texture(unsigned long data)'
if source.count(marker) != 1:
    raise SystemExit('Expected one GL atlas upload boundary')
upload = source[source.index(marker):].replace('text_hires_atlas_texture(', 'mac_text_gl_upload_shared(', 1).replace('xgpu_gl_state_invalidate()', 'mac_text_gl_invalidate()')
write_text(os.path.join(output, 'mac_text_gl_body.h'), '/* Shared atlas upload function; native invalidation callback only. */\n' + upload)
source = source[:source.index(marker)]
for before, after in [('#include "platform.h"', '#include "engine_text.h"'), ('#include "xgpu.h"', ''), ('static GLuint atlas_texture;', ''), ('#include "../../third_party/stb/stb_truetype.h"', '#include "port/third_party/stb/stb_truetype.h"'), ('platform_log(', 'mac_text_log('), ('halo_screen_pixel_scale()', 'mac_text_pixel_scale()')]:
    if before not in source:
        raise SystemExit('Missing expected CPU text boundary: ' + before)
    source = source.replace(before, after)
write_text(os.path.join(output, 'mac_text_body.h'), '#line 1 "port/linux/src/text_hires.c"\n' + source)
root = 'port/assets/fonts'
fonts = json.loads(read_text(os.path.join(root, 'fonts.json')))['fonts']
files = []
for f in fonts:
    if f['file'] not in files:
        files.append(f['file'])
out = ['#include "text_hires.h"']
for i, name in enumerate(files):
    data = read_bytes(os.path.join(root, name))
    out.append('static const unsigned char mac_font%d[] __attribute__((aligned(4))) = {' % i)
    for offset in range(0, len(data), 20):
        out.append(','.join('0x%02x' % b for b in data[offset:offset + 20]) + ',')
    out.append('};')
out.append('const struct text_hires_embedded text_hires_embedded[] = {')
for f in fonts:
    i = files.index(f['file'])
    size = len(read_bytes(os.path.join(root, f['file'])))
    out.append('{%s,%s,(const unsigned int *)mac_font%d,%d},' % (json.dumps(f['tag']), json.dumps(f['file']), i, size))
out += ['};', 'const unsigned int text_hires_embedded_count = %d;' % len(fonts)]
write_text(os.path.join(output, 'mac_text_fonts.c'), '\n'.join(out) + '\n')
