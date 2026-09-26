"""A3: build path_obstacle_avoidance scratch copies (CRLF preserved) for cards PAO-1, PAO-2, PAO-12.

    python -B research/compiler_application_20260925/workers/A3/tools/mk_pao.py

Reads the worktree source (never writes it) and writes copies under workers/A3/copies/.
"""
import os

WT = os.path.abspath(os.path.join(os.path.dirname(__file__), '..', '..', '..', '..', '..'))
SRC = os.path.join(WT, 'source', 'ai', 'path_obstacle_avoidance.c')
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'copies')
os.makedirs(OUT, exist_ok=True)

raw = open(SRC, 'rb').read()
assert b'\r\n' in raw
text = raw.decode('latin-1')
NL = '\r\n'


def sub(t, old, new):
    old = old.replace('\n', NL)
    new = new.replace('\n', NL)
    assert t.count(old) == 1, (old[:80], t.count(old))
    return t.replace(old, new)


# PAO-1: genuine header inline valid_real_point2d (drop rename, TU hand body, memcpy plumbing)
A_OLD_INC = ('#define valid_real_point2d valid_real_point2d_inline\n'
             '#define normalize2d normalize2d_inline\n'
             '#include "math/real_math.h"\n'
             '#undef normalize2d\n'
             '#undef valid_real_point2d\n')
A_NEW_INC = ('#define normalize2d normalize2d_inline\n'
             '#include "math/real_math.h"\n'
             '#undef normalize2d\n')
MEMCPY_OLD = '#undef memcpy\n#include <stddef.h>\n#include <string.h>\n'
MEMCPY_NEW = '#include <stddef.h>\n'
BODY_OLD = ('boolean valid_real_point2d(\n'
            '\treal_point2d const *point)\n'
            '{\n'
            '\treal x_value = point->x;\n'
            '\treal y_value;\n'
            '\tunsigned long x_bits;\n'
            '\tunsigned long y_bits;\n'
            '\n'
            '\tmemcpy(&x_bits, &x_value, sizeof(x_bits));\n'
            '\n'
            '\treturn (x_bits & 0x7F800000) != 0x7F800000 &&\n'
            '\t\t(y_value = point->y,\n'
            '\t\tmemcpy(&y_bits, &y_value, sizeof(y_bits)),\n'
            '\t\t(y_bits & 0x7F800000) != 0x7F800000);\n'
            '}\n'
            '\n')

# PAO-2: genuine header inline normalize2d (drop rename + consumer-local prototype)
B_OLD_INC = ('#define valid_real_point2d valid_real_point2d_inline\n'
             '#define normalize2d normalize2d_inline\n'
             '#include "math/real_math.h"\n'
             '#undef normalize2d\n'
             '#undef valid_real_point2d\n')
B_NEW_INC = ('#define valid_real_point2d valid_real_point2d_inline\n'
             '#include "math/real_math.h"\n'
             '#undef valid_real_point2d\n')
PROTO_OLD = ('/* The owner declaration in real_math.h is macro-renamed while importing the\n'
             ' * January inline set; restore the external name after that schedule ends. */\n'
             'real normalize2d(\n'
             '\treal_vector2d *vector);\n'
             '\n')

AB_NEW_INC = '#include "math/real_math.h"\n'


def pao1(t):
    t = sub(t, A_OLD_INC, A_NEW_INC)
    t = sub(t, MEMCPY_OLD, MEMCPY_NEW)
    t = sub(t, BODY_OLD, '')
    return t


def pao2(t):
    t = sub(t, B_OLD_INC, B_NEW_INC)
    t = sub(t, PROTO_OLD, '')
    return t


def pao12(t):
    t = sub(t, A_OLD_INC, AB_NEW_INC)
    t = sub(t, MEMCPY_OLD, MEMCPY_NEW)
    t = sub(t, BODY_OLD, '')
    t = sub(t, PROTO_OLD, '')
    return t


for name, fn in (('pao1', pao1), ('pao2', pao2), ('pao12', pao12)):
    d = os.path.join(OUT, name, 'source', 'ai')
    os.makedirs(d, exist_ok=True)
    p = os.path.join(d, 'path_obstacle_avoidance.c')
    open(p, 'wb').write(fn(text).encode('latin-1'))
    print('wrote', p)
