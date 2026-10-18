#!/usr/bin/env python3
"""Research tool: read a pixel GLSL 1.20 shader that port/mac generated from
NV2A combiner state (as dumped by tools/mac_gl_shader_dump_interpose.c),
rebuild its dataflow with constant folding, dead-code elimination and range
analysis, and write

  <name>.opt.frag   simplified GLSL 1.20 (same inputs and uniforms)
  <name>.arb        ARB_fragment_program 1.0 (inputs: fragment.texcoord[n],
                    program.local[0..24], see LOCAL_* below)
  <name>.stats      instruction estimates

It understands only the generator's own text (nv2a_pixel_shader_to_glsl for
texture modes 0, 1 and 3); anything else is refused.  Not part of the build.
"""
import re, sys, os

# program.local[] layout shared with tools/mac_fragment_cost_probe.c
LOCAL_C0, LOCAL_C1, LOCAL_FC0, LOCAL_FC1 = 0, 8, 16, 17
LOCAL_SCALE, LOCAL_FOGC, LOCAL_FOGP, LOCAL_ALPHA = 18, 22, 23, 24
TEXCOORD = {'xB0': 0, 'xB1': 1, 'xT0': 2, 'xT1': 3, 'xT2': 4, 'xT3': 5, 'xD0': 6, 'xD1': 7}
REGS = ['v0', 'v1', 't0', 't1', 't2', 't3', 'r0', 'r1']


class Refuse(Exception):
    pass


# ---------------------------------------------------------------- nodes
class Node(object):
    __slots__ = ('kind', 'args', 'vec', 'id', 'rng')

    def __init__(self, kind, args, vec):
        self.kind, self.args, self.vec, self.id, self.rng = kind, args, vec, None, None


_table = {}


def mk(kind, args, vec):
    key = (kind, tuple(a.id if isinstance(a, Node) else a for a in args), vec)
    n = _table.get(key)
    if n is None:
        n = Node(kind, args, vec)
        n.id = len(_table)
        _table[key] = n
        n.rng = compute_range(n)
    return n


def cs(x):
    return mk('c', (float(x),), False)


def cv(x, y, z):
    return mk('c', (float(x), float(y), float(z)), True)


def isconst(n):
    return n.kind == 'c'


def cval(n):
    return n.args


def allsame(n, v):
    return isconst(n) and all(a == v for a in n.args)


def compute_range(n):
    k = n.kind
    if k == 'c':
        return (min(n.args), max(n.args))
    if k == 'in':
        return n.args[2]
    if k in ('splat', 'lane'):
        return n.args[0].rng
    if k == 'neg':
        a = n.args[0].rng
        return (-a[1], -a[0])
    if k == 'add':
        a, b = n.args[0].rng, n.args[1].rng
        return (a[0] + b[0], a[1] + b[1])
    if k == 'mul':
        a, b = n.args[0].rng, n.args[1].rng
        c = [a[0] * b[0], a[0] * b[1], a[1] * b[0], a[1] * b[1]]
        return (min(c), max(c))
    if k == 'dp3':
        a, b = n.args[0].rng, n.args[1].rng
        c = [a[0] * b[0], a[0] * b[1], a[1] * b[0], a[1] * b[1]]
        return (3 * min(c), 3 * max(c))
    if k == 'max':
        a, b = n.args[0].rng, n.args[1].rng
        return (max(a[0], b[0]), max(a[1], b[1]))
    if k == 'min':
        a, b = n.args[0].rng, n.args[1].rng
        return (min(a[0], b[0]), min(a[1], b[1]))
    if k == 'clamp':
        a = n.args[0].rng
        lo, hi = n.args[1], n.args[2]
        return (max(a[0], lo), min(a[1], hi))
    if k == 'sel':
        a, b = n.args[1].rng, n.args[2].rng
        return (min(a[0], b[0]), max(a[1], b[1]))
    if k == 'lrp':
        a, b, c = n.args[0].rng, n.args[1].rng, n.args[2].rng
        if a[0] >= 0 and a[1] <= 1:
            return (min(b[0], c[0]), max(b[1], c[1]))
        return (-1e9, 1e9)
    if k == 'ge':
        return (0.0, 1.0)
    raise Refuse('range ' + k)


INF = 1e9


def splat(s):
    assert not s.vec
    if isconst(s):
        return cv(s.args[0], s.args[0], s.args[0])
    return mk('splat', (s,), True)


def lane(v, i):
    assert v.vec
    if isconst(v):
        return cs(v.args[i])
    if v.kind == 'splat':
        return v.args[0]
    return mk('lane', (v, i), False) if False else _lane(v, i)


def _lane(v, i):
    key = ('lane', v.id, i)
    n = _table.get(key)
    if n is None:
        n = Node('lane', (v, i), False)
        n.id = len(_table)
        _table[key] = n
        n.rng = v.rng
    return n


def neg(a):
    if isconst(a):
        return mk('c', tuple(-x for x in a.args), a.vec)
    if a.kind == 'neg':
        return a.args[0]
    return mk('neg', (a,), a.vec)


def add(a, b):
    assert a.vec == b.vec
    if allsame(a, 0.0):
        return b
    if allsame(b, 0.0):
        return a
    if isconst(a) and isconst(b):
        return mk('c', tuple(x + y for x, y in zip(a.args, b.args)), a.vec)
    if a.id > b.id:
        a, b = b, a
    return mk('add', (a, b), a.vec)


def mul(a, b):
    assert a.vec == b.vec
    if allsame(a, 0.0) or allsame(b, 0.0):
        return mk('c', (0.0,) * len(a.args if isconst(a) else b.args), a.vec) if True else None
    if allsame(a, 1.0):
        return b
    if allsame(b, 1.0):
        return a
    if allsame(a, -1.0):
        return neg(b)
    if allsame(b, -1.0):
        return neg(a)
    if isconst(a) and isconst(b):
        return mk('c', tuple(x * y for x, y in zip(a.args, b.args)), a.vec)
    if a.id > b.id:
        a, b = b, a
    return mk('mul', (a, b), a.vec)


def dp3(a, b):
    assert a.vec and b.vec
    if allsame(a, 0.0) or allsame(b, 0.0):
        return cs(0.0)
    if isconst(a) and isconst(b):
        return cs(sum(x * y for x, y in zip(a.args, b.args)))
    if a.id > b.id:
        a, b = b, a
    return mk('dp3', (a, b), False)


def vmax(a, b):
    assert a.vec == b.vec
    if isconst(a) and isconst(b):
        return mk('c', tuple(max(x, y) for x, y in zip(a.args, b.args)), a.vec)
    if isconst(a) and not isconst(b):
        a, b = b, a
    if isconst(b) and a.rng[0] >= max(b.args):
        return a
    if isconst(b) and a.rng[1] <= min(b.args):
        return b
    return mk('max', (a, b), a.vec)


def clamp(x, lo, hi):
    """clamp x to [lo, hi]; lo/hi may be -INF/INF"""
    if isconst(x):
        return mk('c', tuple(min(max(v, lo), hi) for v in x.args), x.vec)
    if x.rng[0] >= lo:
        lo = -INF
    if x.rng[1] <= hi:
        hi = INF
    if lo == -INF and hi == INF:
        return x
    if x.kind == 'clamp':
        # merge nested clamps
        ilo, ihi = x.args[1], x.args[2]
        nlo, nhi = max(lo, ilo), min(hi, ihi)
        return clamp(x.args[0], nlo, nhi)
    return mk('clamp', (x, lo, hi), x.vec)


def sel(c, a, b):
    """c ? a : b with c a 'ge' node"""
    if a is b:
        return a
    return mk('sel', (c, a, b), a.vec)


def ge_half(s):
    return mk('ge', (s,), False)


def lrp(a, b, c):
    """a*b + (1-a)*c"""
    if allsame(c, 0.0):
        return mul(a, b)
    return mk('lrp', (a, b, c), True)


def inp(name, vec, rng):
    return mk('in', (name, vec, rng), vec)


# --------------------------------------------------------------- parsing
class Prog(object):
    pass


UNIT = (0.0, 1.0)


def texval(stage):
    return (inp('tex%d' % stage, True, UNIT), inp('tex%d.a' % stage, False, UNIT))


def parse(src):
    p = Prog()
    p.samplers = {}
    for m in re.finditer(r'uniform (sampler2D|sampler3D|samplerCube) tex(\d);', src):
        p.samplers[int(m.group(2))] = m.group(1)
    m = re.search(r'#define xFog (.*)\n', src)
    p.fog_define = m.group(1).strip() if m else '1.0'
    p.fog_in_use = False
    body = src[src.index('void main()'):]
    lines = [l.strip() for l in body.split('\n')]
    reg = {}
    zero3, zero1 = cv(0, 0, 0), cs(0)
    # inputs
    reg['v0'] = [inp('v0', True, UNIT), inp('v0.a', False, UNIT)]
    reg['v1'] = [inp('v1', True, UNIT), inp('v1.a', False, UNIT)]
    for i in range(4):
        reg['t%d' % i] = [zero3, zero1]
    p.tex_used = {}
    p.tex_mode = {}
    p.alpha_test = None
    p.unique_c0 = p.unique_c1 = None

    def getreg(name, final=False):
        if name == 'vec4(0.0)':
            return [zero3, zero1]
        m = re.match(r'ps_c([01])\[(\d+)\]$', name)
        if m:
            k = int(m.group(1)) * 8 + int(m.group(2))
            return [inp('c%d_%s' % (int(m.group(1)), m.group(2)), True, UNIT),
                    inp('c%d_%s.a' % (int(m.group(1)), m.group(2)), False, UNIT)]
        m = re.match(r'ps_final_c([01])$', name)
        if m:
            return [inp('fc%s' % m.group(1), True, UNIT), inp('fc%s.a' % m.group(1), False, UNIT)]
        if name in reg:
            return reg[name]
        raise Refuse('register ' + name)

    def value(v, alpha_portion):
        # v is a register-ish text with a component selector
        m = re.match(r'^(vec4\(0\.0\)|ps_c[01]\[\d+\]|ps_final_c[01]|\w+)\.(rgb|a|b)$', v)
        if m:
            r = getreg(m.group(1))
            sel_ = m.group(2)
            if sel_ == 'rgb':
                return r[0]
            if sel_ == 'a':
                return r[1]
            return lane(r[0], 2)
        m = re.match(r'^vec3\((.*)\.a\)$', v)
        if m:
            return splat(getreg(m.group(1))[1])
        raise Refuse('value ' + v)

    def mapped_input(text, alpha_portion):
        t = text
        # one of the mapping forms around a value V
        for pat, fn in (
            (r'^max\((.*), 0\.0\)$', lambda x: vmax(x, zeroof(x))),
            (r'^\(1\.0 - clamp\((.*), 0\.0, 1\.0\)\)$', lambda x: sub1(clamp(x, 0.0, 1.0))),
            (r'^\(2\.0 \* max\((.*), 0\.0\) - 1\.0\)$', lambda x: add(mul(two(x), vmax(x, zeroof(x))), minus1(x))),
            (r'^\(1\.0 - 2\.0 \* max\((.*), 0\.0\)\)$', lambda x: add(one(x), neg(mul(two(x), vmax(x, zeroof(x)))))),
            (r'^\(max\((.*), 0\.0\) - 0\.5\)$', lambda x: add(vmax(x, zeroof(x)), half(x, -0.5))),
            (r'^\(0\.5 - max\((.*), 0\.0\)\)$', lambda x: add(half(x, 0.5), neg(vmax(x, zeroof(x))))),
            (r'^\(-(.*)\)$', lambda x: neg(x)),
            (r'^\((.*)\)$', lambda x: x),
        ):
            m = re.match(pat, t)
            if m:
                return fn(value(m.group(1), alpha_portion))
        raise Refuse('input mapping ' + text)

    def mk_const_like(x, c):
        return cv(c, c, c) if x.vec else cs(c)

    def zeroof(x):
        return mk_const_like(x, 0.0)

    def one(x):
        return mk_const_like(x, 1.0)

    def two(x):
        return mk_const_like(x, 2.0)

    def minus1(x):
        return mk_const_like(x, -1.0)

    def half(x, c):
        return mk_const_like(x, c)

    def sub1(x):
        return add(one(x), neg(x))

    def out_map(expr, name):
        # expr is the argument of clamp(..., -1.0, 1.0) in terms of name
        n = re.escape(name)
        return expr

    # state machine over lines
    i = 0
    stage_tmp = {}
    stages_done = 0
    final_seen = False
    result = None
    while i < len(lines):
        l = lines[i]
        i += 1
        m = re.match(r'/\* texture stage (\d), mode (\d+) \*/$', l)
        if m:
            st, mode = int(m.group(1)), int(m.group(2))
            l2 = lines[i]
            i += 1
            p.tex_mode[st] = mode
            if mode == 0:
                continue
            if mode == 1 and re.match(r'^t%d = texture2D\(tex%d, \(vec4\(xT%d\.xyz / \(xT%d\.w != 0\.0 \? xT%d\.w : 1\.0\), 1\.0\)\)\.xy \* texture_scale\[%d\]\.xy\);$' % ((st,) * 6), l2):
                reg['t%d' % st] = list(texval(st))
                p.tex_used[st] = 'proj2d'
                continue
            if mode == 3 and l2 == 't%d = textureCube(tex%d, (xT%d).xyz);' % (st, st, st):
                reg['t%d' % st] = list(texval(st))
                p.tex_used[st] = 'cube'
                continue
            raise Refuse('texture stage %d mode %d: %s' % (st, mode, l2))
        m = re.match(r'float fog_factor = (.*);$', l)
        if m:
            p.fog_expr = m.group(1)
            fogs = clamp(inp('xFog', False, (-INF, INF)) if p.fog_define != '1.0' else cs(1.0), 0.0, 1.0)
            if m.group(1) != 'xFog':
                raise Refuse('fog mode ' + m.group(1))
            reg['fog'] = [inp('fogc', True, UNIT), fogs]
            continue
        if l.startswith('vec4 r0 = vec4(0.0, 0.0, 0.0, t0.a);'):
            reg['r0'] = [zero3, reg['t0'][1]]
            continue
        if l.startswith('vec4 r1 = vec4(0.0);'):
            reg['r1'] = [zero3, zero1]
            continue
        m = re.match(r'/\* combiner stage (\d) \*/$', l)
        if m:
            i += 1  # '{'
            vals = {}
            writes = []
            while lines[i] != '}':
                s = lines[i]
                i += 1
                m = re.match(r'^(vec3|float) ([ac])([ABCD]) = (.*);$', s)
                if m:
                    alpha = m.group(2) == 'a'
                    vals[m.group(2) + m.group(3)] = mapped_input(m.group(4), alpha)
                    continue
                m = re.match(r'^vec3 cAB = vec3\(dot\(cA, cB\)\);$', s)
                if m:
                    vals['cAB'] = splat(dp3(vals['cA'], vals['cB']))
                    continue
                m = re.match(r'^vec3 cCD = vec3\(dot\(cC, cD\)\);$', s)
                if m:
                    vals['cCD'] = splat(dp3(vals['cC'], vals['cD']))
                    continue
                m = re.match(r'^(vec3|float) ([ac])(AB|CD) = ([ac])([AC]) \* ([ac])([BD]);$', s)
                if m:
                    vals[m.group(2) + m.group(3)] = mul(vals[m.group(4) + m.group(5)], vals[m.group(6) + m.group(7)])
                    continue
                m = re.match(r'^(vec3|float) ([ac])SUM = ([ac])AB \+ ([ac])CD;$', s)
                if m:
                    vals[m.group(2) + 'SUM'] = add(vals[m.group(2) + 'AB'], vals[m.group(2) + 'CD'])
                    continue
                m = re.match(r'^(vec3|float) ([ac])SUM = r0\.a >= 0\.5 \? ([ac])CD : ([ac])AB;$', s)
                if m:
                    x = m.group(2)
                    vals[x + 'SUM'] = sel(ge_half(reg['r0'][1]), vals[x + 'CD'], vals[x + 'AB'])
                    continue
                m = re.match(r'^([ac])(AB|CD|SUM) = clamp\((.*), -1\.0, 1\.0\);$', s)
                if m:
                    x, w, e = m.group(1), m.group(2), m.group(3)
                    base = vals[x + w]
                    nm = x + w
                    mm = re.match(r'^\(%s( - 0\.5| \* 2\.0| \* 4\.0| \* 0\.5)?\)$' % nm, e)
                    mm2 = re.match(r'^\(\(%s - 0\.5\) \* 2\.0\)$' % nm, e)
                    if mm2:
                        v = mul(add(base, half(base, -0.5)), two(base))
                    elif mm:
                        suf = mm.group(1)
                        if suf is None:
                            v = base
                        elif suf == ' - 0.5':
                            v = add(base, half(base, -0.5))
                        elif suf == ' * 2.0':
                            v = mul(base, two(base))
                        elif suf == ' * 4.0':
                            v = mul(base, mk_const_like(base, 4.0))
                        else:
                            v = mul(base, mk_const_like(base, 0.5))
                    else:
                        raise Refuse('output map ' + e)
                    vals[nm] = clamp(v, -1.0, 1.0)
                    continue
                m = re.match(r'^(\w+)\.(rgb|a) = ([ac])(AB|CD|SUM)(\.b)?;$', s)
                if m:
                    writes.append((m.group(1), m.group(2), m.group(3) + m.group(4), bool(m.group(5))))
                    continue
                raise Refuse('combiner line ' + s)
            i += 1  # '}'
            for dst, comp, nm, blane in writes:
                v = vals[nm]
                if blane:
                    v = lane(v, 2)
                if comp == 'rgb':
                    reg[dst][0] = v
                else:
                    reg[dst][1] = v
            stages_done += 1
            continue
        m = re.match(r'^vec4 ef_product = vec4\((.*) \* (.*), 0\.0\);$', l)
        if m:
            ef_l, ef_r = m.group(1), m.group(2)
            reg['ef_product'] = [mul(final_in(ef_l, getreg, False), final_in(ef_r, getreg, False)), zero1]
            continue
        m = re.match(r'^vec4 v1r0_sum = vec4\((.*) \+ (.*), 0\.0\);$', l)
        if m:
            a = final_in(m.group(1), getreg, False)
            b = final_in(m.group(2), getreg, False)
            reg['v1r0_sum'] = [add(a, b), zero1]
            continue
        m = re.match(r'^v1r0_sum = clamp\(v1r0_sum, 0\.0, 1\.0\);$', l)
        if m:
            reg['v1r0_sum'][0] = clamp(reg['v1r0_sum'][0], 0.0, 1.0)
            continue
        m = re.match(r'^vec3 f([ABCD]) = (.*);$', l)
        if m:
            reg['f' + m.group(1)] = final_in(m.group(2), getreg, False)
            continue
        m = re.match(r'^float fG = (.*);$', l)
        if m:
            reg['fG'] = final_in(m.group(1), getreg, True)
            continue
        if l == 'vec4 result = r0;':
            result = [reg['r0'][0], reg['r0'][1]]
            continue
        if l == 'vec4 result = vec4(fA * fB + (1.0 - fA) * fC + fD, fG);':
            rgb = add(lrp(reg['fA'], reg['fB'], reg['fC']), reg['fD'])
            result = [rgb, reg['fG']]
            continue
        m = re.match(r'^if \(!\(floor\(clamp\(result\.a, 0\.0, 1\.0\) \* 255\.0 \+ 0\.5\) (>|>=|<|<=) alpha_reference\)\) discard;$', l)
        if m:
            p.alpha_test = m.group(1)
            continue
        if l == 'gl_FragColor = clamp(result, 0.0, 1.0);':
            p.out_rgb = clamp(result[0], 0.0, 1.0)
            p.out_a = clamp(result[1], 0.0, 1.0)
            p.alpha_node = p.out_a
            return p
        if l.startswith('result.a = mix'):
            raise Refuse('coverage alpha')
    raise Refuse('no output')


def final_in(text, getreg, alpha):
    m = re.match(r'^\(1\.0 - clamp\((.*), 0\.0, 1\.0\)\)$', text)
    inv = bool(m)
    if not m:
        m = re.match(r'^clamp\((.*), 0\.0, 1\.0\)$', text)
    if not m:
        raise Refuse('final input ' + text)
    v = m.group(1)
    mm = re.match(r'^(vec4\(0\.0\)|ps_c[01]\[\d+\]|ps_final_c[01]|\w+)\.(rgb|a|b)$', v)
    if mm:
        r = getreg(mm.group(1))
        x = r[0] if mm.group(2) == 'rgb' else (r[1] if mm.group(2) == 'a' else lane(r[0], 2))
    else:
        mm = re.match(r'^vec3\((.*)\.a\)$', v)
        if not mm:
            raise Refuse('final value ' + v)
        x = splat(getreg(mm.group(1))[1])
    x = clamp(x, 0.0, 1.0)
    if inv:
        one = cv(1, 1, 1) if x.vec else cs(1)
        x = add(one, neg(x))
    return x


# ------------------------------------------------- traversal / statistics
def topo(roots):
    order, seen = [], set()

    def visit(n):
        if n.id in seen:
            return
        seen.add(n.id)
        for a in n.args:
            if isinstance(a, Node):
                visit(a)
        order.append(n)
    for r in roots:
        visit(r)
    return order


def uses(order):
    cnt = {}
    for n in order:
        for a in n.args:
            if isinstance(a, Node):
                cnt[a.id] = cnt.get(a.id, 0) + 1
    return cnt


# --------------------------------------------------------------- GLSL out
def fmt(x):
    s = '%.9g' % x
    if '.' not in s and 'e' not in s and 'n' not in s:
        s += '.0'
    return s


def emit_glsl(p, mode):
    """mode 'orig': keep the original texture coordinate handling;
    'proj': texture2DProj on the varying (projective divide, scale folded
    into the vertex stage, so the scale uniform is not applied)."""
    roots = [p.out_rgb, p.out_a]
    order = topo(roots)
    cnt = uses(order)
    out = []
    names = {}
    head = ['#version 120']
    for k, v in sorted(TEXCOORD.items()):
        head.append('#define %s gl_TexCoord[%d]' % (k, v))
    head += ['uniform vec4 ps_c0[8];', 'uniform vec4 ps_c1[8];', 'uniform vec4 ps_final_c0;', 'uniform vec4 ps_final_c1;',
             'uniform vec4 fog_color;', 'uniform float alpha_reference;', 'uniform vec4 texture_scale[4];']
    for st, t in sorted(p.samplers.items()):
        head.append('uniform %s tex%d;' % (t, st))
    head.append('void main()\n{')
    # texture fetches first
    for st in sorted(p.tex_used):
        if p.tex_used[st] == 'proj2d':
            if mode == 'orig':
                out.append('\tvec4 T%d = texture2D(tex%d, (vec4(xT%d.xyz / (xT%d.w != 0.0 ? xT%d.w : 1.0), 1.0)).xy * texture_scale[%d].xy);' % ((st,) * 6))
            else:
                out.append('\tvec4 T%d = texture2DProj(tex%d, xT%d);' % (st, st, st))
        else:
            out.append('\tvec4 T%d = textureCube(tex%d, xT%d.xyz);' % (st, st, st))

    def ref(n):
        if n.kind == 'c':
            if n.vec:
                if n.args[0] == n.args[1] == n.args[2]:
                    return 'vec3(%s)' % fmt(n.args[0])
                return 'vec3(%s, %s, %s)' % tuple(fmt(a) for a in n.args)
            return fmt(n.args[0]) if n.args[0] >= 0 else '(%s)' % fmt(n.args[0])
        if n.kind == 'in':
            nm, vec = n.args[0], n.args[1]
            return glsl_input(nm, vec)
        if n.kind == 'neg':
            return '(-%s)' % ref(n.args[0])
        if n.kind == 'splat':
            return 'vec3(%s)' % ref(n.args[0])
        if n.kind == 'lane':
            return '%s.%s' % (ref(n.args[0]), 'xyz'[n.args[1]]) if n.args[0].id in names else '%s.%s' % (ref(n.args[0]), 'xyz'[n.args[1]])
        if n.id in names:
            return names[n.id]
        return expr(n)

    def glsl_input(nm, vec):
        m = re.match(r'^(v[01])(\.a)?$', nm)
        if m:
            return '%s%s' % (('xD0' if m.group(1) == 'v0' else 'xD1'), '.a' if m.group(2) else '.rgb')
        m = re.match(r'^tex(\d)(\.a)?$', nm)
        if m:
            return 'T%s%s' % (m.group(1), '.a' if m.group(2) else '.rgb')
        m = re.match(r'^c([01])_(\d)(\.a)?$', nm)
        if m:
            return 'ps_c%s[%s]%s' % (m.group(1), m.group(2), '.a' if m.group(3) else '.rgb')
        m = re.match(r'^fc([01])(\.a)?$', nm)
        if m:
            return 'ps_final_c%s%s' % (m.group(1), '.a' if m.group(2) else '.rgb')
        if nm == 'fogc':
            return 'fog_color.rgb'
        raise Refuse('glsl input ' + nm)

    def expr(n):
        k = n.kind
        a = n.args
        if k == 'add':
            return '(%s + %s)' % (ref(a[0]), ref(a[1]))
        if k == 'mul':
            return '(%s * %s)' % (ref(a[0]), ref(a[1]))
        if k == 'dp3':
            return 'dot(%s, %s)' % (ref(a[0]), ref(a[1]))
        if k == 'max':
            return 'max(%s, %s)' % (ref(a[0]), ref(a[1]))
        if k == 'min':
            return 'min(%s, %s)' % (ref(a[0]), ref(a[1]))
        if k == 'clamp':
            lo, hi = a[1], a[2]
            if lo == 0.0 and hi == 1.0:
                return 'clamp(%s, 0.0, 1.0)' % ref(a[0])
            if lo == -INF:
                return 'min(%s, %s)' % (ref(a[0]), fmt(hi))
            if hi == INF:
                return 'max(%s, %s)' % (ref(a[0]), fmt(lo))
            return 'clamp(%s, %s, %s)' % (ref(a[0]), fmt(lo), fmt(hi))
        if k == 'ge':
            return '(%s >= 0.5)' % ref(a[0])
        if k == 'sel':
            return '(%s ? %s : %s)' % (ref(a[0]), ref(a[1]), ref(a[2]))
        if k == 'lrp':
            return 'mix(%s, %s, %s)' % (ref(a[2]), ref(a[1]), ref(a[0]))
        raise Refuse('glsl expr ' + k)

    n = 0
    for nd in order:
        if nd.kind in ('c', 'in', 'neg', 'splat', 'lane'):
            continue
        if nd.kind == 'ge':
            names[nd.id] = '(%s >= 0.5)' % ref(nd.args[0])
            continue
        # name multiply-used or compound nodes
        if cnt.get(nd.id, 0) > 1 or nd is p.out_rgb or nd is p.out_a:
            nm = 'n%d' % n
            n += 1
            out.append('\t%s %s = %s;' % ('vec3' if nd.vec else 'float', nm, expr(nd)))
            names[nd.id] = nm
    if p.alpha_test:
        out.append('\tif (!(floor(%s * 255.0 + 0.5) %s alpha_reference)) discard;' % (ref(p.out_a), p.alpha_test))
    out.append('\tgl_FragColor = vec4(%s, %s);' % (ref(p.out_rgb), ref(p.out_a)))
    out.append('}')
    return '\n'.join(head + out) + '\n'


# ----------------------------------------------------------------- ARB out
def emit_arb(p, scale_mode):
    """scale_mode 'vp': TXP straight from the interpolant (scale folded into
    the vertex stage); 'mul': one MUL by program.local[18+n] first."""
    roots = [p.out_rgb, p.out_a]
    order = topo(roots)
    cnt = uses(order)
    ins = []
    temps = {}
    ntemp = [0]
    stats = {'alu': 0, 'tex': 0, 'valu': 0, 'salu': 0}

    def newtemp():
        t = 'R%d' % ntemp[0]
        ntemp[0] += 1
        return t

    def lit(x):
        return '{%s, %s, %s, %s}' % ((fmt(x),) * 4)

    def src(n, sw):
        """sw: 'xyz' for a vector use or a single lane char/'xxx' for scalar use."""
        k = n.kind
        if len(sw) == 3:
            sw = sw + sw[-1]
        if k == 'neg':
            return '-' + src(n.args[0], sw)
        if k == 'c':
            if n.vec:
                v = n.args
                return '{%s, %s, %s, 0.0}.%s' % (fmt(v[0]), fmt(v[1]), fmt(v[2]), sw) if sw == 'xyz' else '{%s, %s, %s, 0.0}.%s' % (fmt(v[0]), fmt(v[1]), fmt(v[2]), sw)
            return '{%s, %s, %s, %s}.%s' % ((fmt(n.args[0]),) * 4 + (sw,))
        if k == 'splat':
            return src(n.args[0], 'x' * len(sw))
        if k == 'lane':
            return src(n.args[0], 'xyz'[n.args[1]] * len(sw))
        if k == 'in':
            return input_src(n.args[0], n.args[1], sw)
        t = temps[n.id]
        if n.vec:
            return '%s.%s' % (t, sw)
        return '%s.%s' % (t, 'x' * len(sw))

    def input_src(nm, vec, sw):
        comp_rgb = sw if len(sw) == 3 and sw == 'xyz' else None

        def pick(base, is_alpha):
            if is_alpha:
                return '%s.%s' % (base, 'w' * len(sw))
            # rgb register: sw uses x/y/z lanes of rgb; map to same lanes
            return '%s.%s' % (base, sw)
        m = re.match(r'^(v[01])(\.a)?$', nm)
        if m:
            return pick('fragment.texcoord[%d]' % (6 if m.group(1) == 'v0' else 7), bool(m.group(2)))
        m = re.match(r'^tex(\d)(\.a)?$', nm)
        if m:
            return pick(texreg[int(m.group(1))], bool(m.group(2)))
        m = re.match(r'^c([01])_(\d)(\.a)?$', nm)
        if m:
            return pick('program.local[%d]' % ((LOCAL_C0 if m.group(1) == '0' else LOCAL_C1) + int(m.group(2))), bool(m.group(3)))
        m = re.match(r'^fc([01])(\.a)?$', nm)
        if m:
            return pick('program.local[%d]' % (LOCAL_FC0 + int(m.group(1))), bool(m.group(2)))
        if nm == 'fogc':
            return pick('program.local[%d]' % LOCAL_FOGC, False)
        raise Refuse('arb input ' + nm)

    texreg = {}
    for st in sorted(p.tex_used):
        t = newtemp()
        texreg[st] = t
        stats['tex'] += 1
        if p.tex_used[st] == 'proj2d':
            if scale_mode == 'mul':
                c = newtemp()
                ins.append('MUL %s, fragment.texcoord[%d], program.local[%d];' % (c, 2 + st, LOCAL_SCALE + st))
                stats['alu'] += 1
                ins.append('TXP %s, %s, texture[%d], 2D;' % (t, c, st))
            else:
                ins.append('TXP %s, fragment.texcoord[%d], texture[%d], 2D;' % (t, 2 + st, st))
        else:
            ins.append('TEX %s, fragment.texcoord[%d], texture[%d], CUBE;' % (t, 2 + st, st))

    def producing(n):
        return n.kind not in ('c', 'in', 'neg', 'splat', 'lane', 'ge')

    # nodes folded into a consumer: 'ge' (inlined as sub), mul consumed by add (mad)
    folded = set()
    alias = {}
    sat_users = {}  # node id -> sat flag for the clamp(0,1) folded into producer
    for n in order:
        if n.kind == 'add' and True:
            for a in n.args:
                if a.kind == 'mul' and cnt.get(a.id, 0) == 1 and a.vec == n.vec:
                    # only one of the operands can be folded
                    if not any(x.id in folded for x in n.args if x is not a):
                        pass
        if n.kind == 'clamp' and n.args[1] == 0.0 and n.args[2] == 1.0 and n.id not in (p.out_rgb.id, p.out_a.id):
            a = n.args[0]
            if producing(a) and cnt.get(a.id, 0) == 1 and a.kind in ('add', 'mul', 'max', 'min', 'dp3', 'lrp', 'sel'):
                folded.add(n.id)
                sat_users[a.id] = True
                alias[n.id] = a
    mad_of = {}
    for n in order:
        if n.kind == 'add':
            for a in n.args:
                inner = a.args[0] if a.kind == 'neg' else a
                if inner.kind == 'mul' and cnt.get(a.id, 0) == 1 and cnt.get(inner.id, 0) == 1 and inner.id not in folded and a.id not in mad_of.values():
                    mad_of[n.id] = a
                    folded.add(a.id)
                    folded.add(inner.id)
                    break
    root_ids = {p.out_rgb.id, p.out_a.id}
    root_sat = {}
    for r, nm in ((p.out_rgb, 'rgb'), (p.out_a, 'a')):
        pass
    out_lines = []

    def dest_mask(n):
        return 'xyz' if n.vec else 'x'

    def emit_op(n, dest, mask, sat):
        k, a = n.kind, n.args
        sw = 'xyz' if n.vec else 'x'
        s = '_SAT' if sat else ''
        if k == 'add':
            if n.id in mad_of:
                m = mad_of[n.id]
                neg_m = m.kind == 'neg'
                mm = m.args[0] if neg_m else m
                other = [x for x in a if x is not m][0]
                ins.append('MAD%s %s.%s, %s%s, %s, %s;' % (s, dest, mask, '-' if neg_m else '', src(mm.args[0], sw), src(mm.args[1], sw), src(other, sw)))
                stats['valu' if n.vec else 'salu'] += 1
                return
            ins.append('ADD%s %s.%s, %s, %s;' % (s, dest, mask, src(a[0], sw), src(a[1], sw)))
        elif k == 'mul':
            ins.append('MUL%s %s.%s, %s, %s;' % (s, dest, mask, src(a[0], sw), src(a[1], sw)))
        elif k == 'dp3':
            ins.append('DP3%s %s.%s, %s, %s;' % (s, dest, mask, src(a[0], 'xyz'), src(a[1], 'xyz')))
        elif k == 'max':
            ins.append('MAX%s %s.%s, %s, %s;' % (s, dest, mask, src(a[0], sw), src(a[1], sw)))
        elif k == 'min':
            ins.append('MIN%s %s.%s, %s, %s;' % (s, dest, mask, src(a[0], sw), src(a[1], sw)))
        elif k == 'clamp':
            lo, hi = a[1], a[2]
            x = src(a[0], sw)
            if lo == 0.0 and hi == 1.0:
                ins.append('MOV_SAT %s.%s, %s;' % (dest, mask, x))
            elif lo != -INF and hi != INF:
                ins.append('MAX %s.%s, %s, %s;' % (dest, mask, x, lit(lo) + '.x'))
                stats['valu' if n.vec else 'salu'] += 1
                ins.append('MIN%s %s.%s, %s.%s, %s;' % (s, dest, mask, dest, sw + sw[-1] if len(sw) == 3 else sw, lit(hi) + '.x'))
            elif hi == INF:
                ins.append('MAX%s %s.%s, %s, %s;' % (s, dest, mask, x, lit(lo) + '.x'))
            else:
                ins.append('MIN%s %s.%s, %s, %s;' % (s, dest, mask, x, lit(hi) + '.x'))
        elif k == 'lrp':
            ins.append('LRP%s %s.%s, %s, %s, %s;' % (s, dest, mask, src(a[0], sw), src(a[1], sw), src(a[2], sw)))
        elif k == 'sel':
            c = a[0]
            t = newtemp()
            ins.append('ADD %s.x, %s, {-0.5, -0.5, -0.5, -0.5}.x;' % (t, src(c.args[0], 'x')))
            stats['salu'] += 1
            ins.append('CMP%s %s.%s, %s.%s, %s, %s;' % (s, dest, mask, t, 'xxxx' if len(sw) > 1 else 'x', src(a[2], sw), src(a[1], sw)))
        else:
            raise Refuse('arb op ' + k)
        stats['valu' if n.vec else 'salu'] += 1

    for n in order:
        if not producing(n) or n.id in folded:
            continue
        if n.id in root_ids:
            continue
        t = newtemp()
        temps[n.id] = t
        emit_op(n, t, dest_mask(n), sat_users.get(n.id, False))
        for cid, prod in alias.items():
            if prod is n:
                temps[cid] = t
    # roots: the final clamp(0,1) writes the output with saturation
    rrgb, ra = p.out_rgb, p.out_a
    alpha_src = None
    if p.alpha_test:
        pass
    # evaluate alpha first when the kill needs it
    def root(n, mask, outname):
        sw = 'xyz' if n.vec else 'x'
        inner = n
        sat = False
        if n.kind == 'clamp' and n.args[1] == 0.0 and n.args[2] == 1.0:
            inner = n.args[0]
            sat = True
        if inner.id in temps or not producing(inner) or inner.id in folded and inner.id not in temps:
            # leaf or computed: MOV
            if inner.id in temps or not producing(inner):
                ins.append('MOV%s %s.%s, %s;' % ('_SAT' if sat or True else '', outname, mask, src(inner, sw if mask != 'w' else 'x')))
                stats['valu' if n.vec else 'salu'] += 1
                return
        # produce into the output directly
        if mask == 'w':
            t = newtemp()
            temps[inner.id] = t
            emit_op_w(inner, t, sat)
            return t
        emit_op(inner, outname, mask, sat)
        return None

    def emit_op_w(inner, t, sat):
        emit_op(inner, t, 'x', sat)

    # alpha: compute into a temp .x (needed for KIL and for result.color.w)
    ia = ra.args[0] if (ra.kind == 'clamp' and ra.args[1] == 0.0 and ra.args[2] == 1.0) else ra
    asat = ra.kind == 'clamp' and ra.args[1] == 0.0 and ra.args[2] == 1.0
    alpha_ref = None
    if producing(ia) and ia.id not in temps:
        ta = newtemp()
        temps[ia.id] = ta
        emit_op(ia, ta, 'x', asat)
        alpha_ref = '%s.x' % ta
    else:
        alpha_ref = src(ia, 'x')
        if asat:
            ta = newtemp()
            ins.append('MOV_SAT %s.x, %s;' % (ta, alpha_ref))
            stats['salu'] += 1
            alpha_ref = '%s.x' % ta
    if p.alpha_test:
        k = newtemp()
        if p.alpha_test in ('>', '>='):
            ins.append('SUB %s.x, %s, program.local[%d].x;' % (k, alpha_ref, LOCAL_ALPHA))
        else:
            ins.append('SUB %s.x, program.local[%d].x, %s;' % (k, LOCAL_ALPHA, alpha_ref))
        ins.append('KIL %s.x;' % k)
        stats['salu'] += 1
    # rgb
    irgb = rrgb.args[0] if (rrgb.kind == 'clamp' and rrgb.args[1] == 0.0 and rrgb.args[2] == 1.0) else rrgb
    rsat = rrgb.kind == 'clamp' and rrgb.args[1] == 0.0 and rrgb.args[2] == 1.0
    if producing(irgb) and irgb.id not in temps:
        emit_op(irgb, 'result.color', 'xyz', rsat)
    else:
        ins.append('MOV_SAT result.color.xyz, %s;' % src(irgb, 'xyz'))
        stats['valu'] += 1
    ins.append('MOV result.color.w, %s;' % alpha_ref)
    stats['salu'] += 1

    text = ['!!ARBfp1.0', 'OPTION ARB_precision_hint_fastest;'] if False else ['!!ARBfp1.0']
    if ntemp[0]:
        text.append('TEMP ' + ', '.join('R%d' % i for i in range(ntemp[0])) + ';')
    text += ins
    text.append('END')
    stats['alu'] += stats['valu'] + stats['salu']
    stats['instr_est'] = max(stats['valu'], stats['salu'])  # perfect vector/scalar pairing bound
    return '\n'.join(text) + '\n', stats


def stats_of(p):
    order = topo([p.out_rgb, p.out_a])
    kinds = {}
    for n in order:
        kinds[n.kind] = kinds.get(n.kind, 0) + 1
    return kinds


def main():
    src_path = sys.argv[1]
    outbase = sys.argv[2]
    src = open(src_path).read()
    try:
        p = parse(src)
    except Refuse as e:
        sys.stderr.write('REFUSED %s: %s\n' % (src_path, e))
        sys.exit(3)
    open(outbase + '.opt.frag', 'w').write(emit_glsl(p, 'orig'))
    open(outbase + '.optproj.frag', 'w').write(emit_glsl(p, 'proj'))
    arb, st = emit_arb(p, 'vp')
    open(outbase + '.arb', 'w').write(arb)
    arb2, st2 = emit_arb(p, 'mul')
    open(outbase + '.mul.arb', 'w').write(arb2)
    with open(outbase + '.stats', 'w') as f:
        f.write('tex_stages %s\nalpha_test %s\n' % (sorted(p.tex_used.items()), p.alpha_test))
        f.write('dag %s\n' % sorted(stats_of(p).items()))
        f.write('arb_vp %s\n' % sorted(st.items()))
        f.write('arb_mul %s\n' % sorted(st2.items()))


if __name__ == '__main__':
    main()
