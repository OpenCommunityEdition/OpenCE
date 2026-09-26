"""W1 increment on a W0 tree: one owner declaration in rasterizer.h, every consumer-local extern removed.
    python mk_W1inc.py <root>"""
import glob
import sys

root = sys.argv[1].replace(chr(92), '/').rstrip('/') + '/source/'
EXT = 'extern struct rasterizer_window_begin_parameters global_window_parameters;\r\n'
h = open(root + 'rasterizer/rasterizer.h', 'rb').read().decode('latin-1')
old = 'extern struct rasterizer_frame_begin_parameters global_frame_parameters;\r\n'
assert h.count(old) == 1 and EXT not in h
h = h.replace(old, old + EXT)
open(root + 'rasterizer/rasterizer.h', 'wb').write(h.encode('latin-1'))
n = 0
for p in sorted(glob.glob(root + 'rasterizer/**/*.c', recursive=True)):
    s = open(p, 'rb').read().decode('latin-1')
    c = s.count(EXT)
    if c:
        assert c == 1, p
        s = s.replace(EXT, '')
        open(p, 'wb').write(s.encode('latin-1'))
        n += 1
        print('removed local extern:', p[len(root):])
print('consumers:', n)
