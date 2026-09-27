"""R-B: owner declarations for three consumer-local items in rasterizer_xbox.c (applied to an R-A tree, in place).
    python mk_RB.py <root> [items]     items = comma list of debug,pixel,palette (default: all)"""
import sys

root = sys.argv[1].replace(chr(92), '/').rstrip('/')
items = sys.argv[2].split(',') if len(sys.argv) > 2 else ['debug', 'pixel', 'palette']
p = root + '/source/rasterizer/xbox/rasterizer_xbox.c'
s = open(p, 'rb').read().decode('latin-1')
assert '\r\n' in s
s = s.replace('\r\n', '\n')


def rep(s, old, new, count=1):
    n = s.count(old)
    assert n == count, (n, old[:80])
    return s.replace(old, new)


if 'debug' in items:
    s = rep(s, '#include "rasterizer/rasterizer_debug.h"\n',
            '#include "rasterizer/rasterizer_debug.h"\n#include "rasterizer/rasterizer_debug_options.h"\n')
    start = s.index('/* The debug-option field names and offsets are the ones the scripting engine\n')
    end = s.index('};\n\n', s.index('struct rasterizer_debug_options\n{', start)) + len('};\n\n')
    block = s[start:end]
    assert block.count('struct rasterizer_debug_options') == 1 and 'secondary_render_target_debug' in block
    s = s[:start] + s[end:]
    s = rep(s, 'extern struct rasterizer_debug_options rasterizer_debug_options;\n', '')
if 'pixel' in items:
    s = rep(s, '\nextern struct pixel_shader_definition pixel_shader;\n', '\n')
if 'palette' in items:
    s = rep(s, '#include "bitmaps/bitmaps.h"\n', '#include "bitmaps/bitmaps.h"\n#include "bitmaps/bitmaps_internal.h"\n')
    s = rep(s, '/* owned by source/bitmaps/bitmaps.c */\nextern pixel32 global_vector_palette[NUMBER_OF_ENTRIES_IN_PALETTE];\n\n', '')
open(p, 'wb').write(s.replace('\n', '\r\n').encode('latin-1'))
print('R-B items applied:', items)
