#!/usr/bin/env python3
"""Research tool: recover the pixel shader keys (NV2A combiner words and
texture/fog/alpha-test state) of the pixel shaders in a directory of dumped
GLSL 1.20 files (tools/mac_gl_shader_dump_interpose.c output, the form
port/mac/mac_glsl120.c produces from port/linux/src/nv2a_psh.c), and write
them as tools/mac_nv2a_arbfp_keys.txt for tools/mac_nv2a_arbfp_probe.c and
tools/mac_nv2a_arbfp_gpu_probe.c.

The recovery is checked, not trusted: the probe regenerates each shader from
its recovered key with the real generator and requires the text to equal the
dumped file (the color constants and the sampler/uniform declarations that do
not depend on the key aside).

usage: mac_arbfp_keys_from_glsl.py DUMP_DIR [SUMMARY.txt] > keys.txt
"""
import re
import sys
import os

REGS = {'vec4(0.0)': 0, 'fog': 3, 'v0': 4, 'v1': 5, 't0': 8, 't1': 9, 't2': 10, 't3': 11, 'r0': 12, 'r1': 13}
DEST = {'v0': 4, 'v1': 5, 't0': 8, 't1': 9, 't2': 10, 't3': 11, 'r0': 12, 'r1': 13}
MAPS = [
    (0x00, 'max(%s, 0.0)'), (0x20, '(1.0 - clamp(%s, 0.0, 1.0))'), (0x40, '(2.0 * max(%s, 0.0) - 1.0)'),
    (0x60, '(1.0 - 2.0 * max(%s, 0.0))'), (0x80, '(max(%s, 0.0) - 0.5)'), (0xa0, '(0.5 - max(%s, 0.0))'),
    (0xc0, '(%s)'), (0xe0, '(-%s)'),
]
OUTMAPS = [(0x08, '(%s - 0.5)'), (0x10, '(%s * 2.0)'), (0x18, '((%s - 0.5) * 2.0)'), (0x20, '(%s * 4.0)'),
           (0x30, '(%s * 0.5)'), (0x00, '(%s)')]
CMP = {'<': 513, '==': 514, '<=': 515, '>': 516, '!=': 517, '>=': 518}


class Bad(Exception):
    pass


def reg_of(source, final):
    if source.startswith('ps_c0['):
        return 1, int(source[6:-1])
    if source.startswith('ps_c1['):
        return 2, int(source[6:-1])
    if source == 'ps_final_c0':
        return 1, 0
    if source == 'ps_final_c1':
        return 2, 0
    if source == 'v1r0_sum':
        return 14, 0
    if source == 'ef_product':
        return 15, 0
    if source in REGS:
        return REGS[source], 0
    raise Bad('register ' + source)


def value_of(text, alpha_portion):
    """'X.rgb' / 'vec3(X.a)' / 'X.a' / 'X.b' -> (source, alpha_channel)"""
    m = re.fullmatch(r'vec3\((.+)\.a\)', text)
    if m:
        return m.group(1), True
    m = re.fullmatch(r'(.+)\.(rgb|a|b)', text)
    if not m:
        raise Bad('value ' + text)
    return m.group(1), m.group(2) == 'a'


def parse_input(expr, alpha_portion):
    for code, fmt in MAPS:
        head, tail = fmt.split('%s')
        if expr.startswith(head) and expr.endswith(tail):
            inner = expr[len(head):len(expr) - len(tail)]
            if code == 0xc0 and inner.startswith('-'):
                continue
            try:
                source, achan = value_of(inner, alpha_portion)
                reg, index = reg_of(source, False)
            except Bad:
                continue
            return (reg | (0x10 if achan else 0) | code), index
    raise Bad('input ' + expr)


def parse_final_input(expr, alpha_portion):
    inv = 0
    m = re.fullmatch(r'\(1\.0 - clamp\((.+), 0\.0, 1\.0\)\)', expr)
    if m:
        inv = 0x20
    else:
        m = re.fullmatch(r'clamp\((.+), 0\.0, 1\.0\)', expr)
        if not m:
            raise Bad('final input ' + expr)
    source, achan = value_of(m.group(1), alpha_portion)
    reg, _ = reg_of(source, True)
    return reg | (0x10 if achan else 0) | inv


def parse_dot_input(expr):
    for mapping, pat in ((0, r'(t\d)\.rgb'), (1, r'\(\((t\d)\.rgb \* 255\.0 - 128\.0\) / 127\.0\)'),
                         (3, r'signed_bytes\((t\d)\.rgb\)'), (2, r'\((t\d)\.rgb \* 2\.0 - 1\.0\)')):
        m = re.fullmatch(pat, expr)
        if m:
            return mapping, int(m.group(1)[1])
    raise Bad('dot input ' + expr)


def parse(path):
    text = open(path).read()
    key = {}
    m = re.search(r'#define xFog \(gl_TexCoord\[(\d)\]\.(\w)\)', text)
    key['fog_tex'], key['fog_comp'] = (int(m.group(1)), 'xyzw'.index(m.group(2))) if m else (-1, 0)
    # sampler declarations
    decl = dict((int(a), b) for b, a in re.findall(r'uniform (sampler2D|sampler3D|samplerCube) tex(\d);', text))
    # texture stages
    stage_text = {}
    modes = 0
    for st in range(4):
        m = re.search(r'/\* texture stage %d, mode (\d+) \*/\n(.*?)(?=\t/\* texture stage|\tfloat fog_factor)' % st, text, re.S)
        mode = int(m.group(1))
        stage_text[st] = m.group(2)
        modes |= mode << (5 * st)
    key['modes'] = modes
    samplers = []
    for st in range(4):
        d = decl.get(st, 'sampler2D')
        body = stage_text[st]
        if d == 'samplerCube':
            samplers.append(3)
        elif d == 'sampler3D':
            samplers.append(2)
        else:
            samplers.append(1 if ('texture2D(tex%d' % st) in body else 0)
    key['samplers'] = samplers
    # dot mapping / input texture / compare
    dotmap, inputtex, compare = 0, 0, 0
    for st in range(4):
        body = stage_text[st]
        mm = re.search(r'dot%d = dot\(xT%d\.xyz, (.*)\);' % (st, st), body)
        if mm and st >= 1:
            mapping, inp = parse_dot_input(mm.group(1))
            dotmap |= mapping << ((st - 1) * 4)
            if st == 2:
                inputtex |= inp << 16
            elif st == 3:
                inputtex |= inp << 20
            elif inp != 0:
                raise Bad('stage 1 input')
        if st == 2 and 'dot3 = dot(xT3.xyz' in body:
            mm = re.search(r'dot3 = dot\(xT3\.xyz, (.*)\);', body)
            mapping, inp = parse_dot_input(mm.group(1))
            dotmap |= mapping << 8
            inputtex |= inp << 20
        mm = re.search(r't%d = texture\w+\(tex%d, \(vec4\((t\d)\.(\w), (t\d)\.(\w), 0\.0, 1\.0\)\)' % (st, st), body)
        if mm:
            inp = int(mm.group(1)[1])
            if st == 2:
                inputtex |= inp << 16
            elif st == 3:
                inputtex |= inp << 20
        nibble = 0
        for ci, c in enumerate('xyzw'):
            mm = re.search(r'if \(xT%d\.%s (>=|<) 0\.0\) discard;' % (st, c), body)
            if mm and mm.group(1) == '>=':
                nibble |= 1 << ci
        compare |= nibble << (4 * st)
    key['dotmap'], key['inputtex'], key['compare'] = dotmap, inputtex, compare
    # combiner stages
    blocks = re.findall(r'/\* combiner stage (\d+) \*/\n\t\{\n(.*?)\n\t\}', text, re.S)
    key['count'] = len(blocks)
    unique0 = unique1 = 0
    msb = 'r0.a >= 0.5' in text
    rgbin, alin, rgbout, alout = [0] * 8, [0] * 8, [0] * 8, [0] * 8
    for num, block in blocks:
        st = int(num)
        for portion in range(2):
            prefix = 'a' if portion else 'c'
            word = 0
            for shift, letter in ((24, 'A'), (16, 'B'), (8, 'C'), (0, 'D')):
                mm = re.search(r'\t\t(?:float|vec3) %s%s = (.*);\n' % (prefix, letter), block)
                code, index = parse_input(mm.group(1), portion == 1)
                if (code & 0xf) == 1 and index:
                    unique0 = 1
                if (code & 0xf) == 2 and index:
                    unique1 = 1
                word |= code << shift
            (alin if portion else rgbin)[st] = word
            flags = 0
            if portion == 0:
                if 'vec3 cAB = vec3(dot(cA, cB));' in block:
                    flags |= 0x02
                if 'vec3 cCD = vec3(dot(cC, cD));' in block:
                    flags |= 0x01
            if re.search(r'%sSUM = (r0\.a >= 0\.5|\(int\(r0\.a \* 255\.0 \+ 0\.5\) & 1\) != 0) \?' % prefix, block):
                flags |= 0x04
            mm = re.search(r'\t\t%sAB = clamp\((.*), -1\.0, 1\.0\);' % prefix, block)
            inner = mm.group(1)
            outmap = None
            for code, fmt in OUTMAPS:
                if inner == fmt % (prefix + 'AB'):
                    outmap = code
                    break
            if outmap is None:
                raise Bad('output mapping ' + inner)
            flags |= outmap
            dab = dcd = dsum = 0
            comp = 'a' if portion else 'rgb'
            for dname, value in re.findall(r'\t\t(\w+)\.%s = (%sAB|%sCD|%sSUM);' % (comp, prefix, prefix, prefix), block):
                if value.endswith('AB'):
                    dab = DEST[dname]
                elif value.endswith('CD'):
                    dcd = DEST[dname]
                else:
                    dsum = DEST[dname]
            if portion == 0:
                if re.search(r'\t\t\w+\.a = cAB\.b;', block):
                    flags |= 0x80
                if re.search(r'\t\t\w+\.a = cCD\.b;', block):
                    flags |= 0x40
            (alout if portion else rgbout)[st] = dcd | (dab << 4) | (dsum << 8) | (flags << 12)
    key['count_word'] = len(blocks) | (0x1000 if unique0 else 0) | (0x10000 if unique1 else 0) | (0x100 if msb else 0)
    key['rgbin'], key['alin'], key['rgbout'], key['alout'] = rgbin, alin, rgbout, alout
    # final combiner
    if 'vec4 result = r0;' in text:
        key['abcd'] = key['efg'] = 0
    else:
        # the two factors are separated by " * " at the top level: split carefully
        line = re.search(r'vec4 ef_product = vec4\((.*), 0\.0\);', text).group(1)
        parts = split_product(line)
        e = parse_final_input(parts[0], False)
        f = parse_final_input(parts[1], False)
        settings = 0
        mm = re.search(r'vec4 v1r0_sum = vec4\((.*) \+ (.*), 0\.0\);', text)
        if mm.group(1).startswith('(1.0 -'):
            settings |= 0x40
        if mm.group(2).startswith('(1.0 -'):
            settings |= 0x20
        if 'v1r0_sum = clamp(v1r0_sum, 0.0, 1.0);' in text:
            settings |= 0x80
        fin = []
        for name in 'ABCD':
            mm = re.search(r'vec3 f%s = (.*);' % name, text)
            fin.append(parse_final_input(mm.group(1), False))
        mm = re.search(r'float fG = (.*);', text)
        g = parse_final_input(mm.group(1), True)
        key['abcd'] = (fin[0] << 24) | (fin[1] << 16) | (fin[2] << 8) | fin[3]
        key['efg'] = (e << 24) | (f << 16) | (g << 8) | settings
    if 'result.a = mix(1.0, result.a, t0.g);' in text:
        raise Bad('coverage_alpha')
    mm = re.search(r'if \(!\(floor\(clamp\(result\.a, 0\.0, 1\.0\) \* 255\.0 \+ 0\.5\) (\S+) alpha_reference\)\) discard;', text)
    key['alpha_func'] = CMP[mm.group(1)] if mm else 0
    if re.search(r'^\tdiscard;$', text, re.M):
        key['alpha_func'] = 512
    mm = re.search(r'float fog_factor = (.*);', text)
    expr = mm.group(1)
    if expr == '1.0':
        key['fog'] = (0, 0)
    elif expr == 'xFog':
        key['fog'] = (1, 0)
    elif expr.startswith('exp(-fog_parameters.z'):
        key['fog'] = (1, 1)
    elif expr.startswith('exp(-(fog_parameters.z'):
        key['fog'] = (1, 2)
    elif expr.startswith('(fog_parameters.y'):
        key['fog'] = (1, 3)
    else:
        raise Bad('fog ' + expr)
    return key


def split_product(line):
    depth = 0
    for i in range(len(line)):
        c = line[i]
        if c == '(':
            depth += 1
        elif c == ')':
            depth -= 1
        elif depth == 0 and line[i:i + 3] == ' * ':
            return line[:i], line[i + 3:]
    raise Bad('product ' + line)


def main():
    dump = sys.argv[1]
    weights = {}
    if len(sys.argv) > 2:
        for line in open(sys.argv[2]):
            m = re.match(r'program (\d+) fragment_serial (\d+) draws (\d+) vertices (\d+) samples (\d+)', line)
            if m:
                weights[int(m.group(2))] = (int(m.group(3)), int(m.group(5)))
    print('# pixel shader keys recovered from %s by tools/mac_arbfp_keys_from_glsl.py' % os.path.basename(dump.rstrip('/')))
    print('# name draws samples fog_texcoord fog_component count_word final_abcd final_efg compare dotmap inputtex '
          'texture_modes sampler0..3 alpha_func fog_enable fog_mode | rgb_in[8] alpha_in[8] rgb_out[8] alpha_out[8]')
    for name in sorted(os.listdir(dump)):
        if not name.endswith('.frag'):
            continue
        serial = int(re.search(r'(\d+)', name).group(1))
        try:
            k = parse(os.path.join(dump, name))
        except Bad as e:
            sys.stderr.write('%s: %s\n' % (name, e))
            continue
        draws, samples = weights.get(serial, (0, 0))
        fields = [name[:-5], str(draws), str(samples), str(k['fog_tex']), str(k['fog_comp']),
                  '%x' % k['count_word'], '%x' % k['abcd'], '%x' % k['efg'], '%x' % k['compare'],
                  '%x' % k['dotmap'], '%x' % k['inputtex'], '%x' % k['modes']]
        fields += [str(s) for s in k['samplers']]
        fields += [str(k['alpha_func']), str(k['fog'][0]), str(k['fog'][1])]
        fields += ['%x' % v for v in k['rgbin'] + k['alin'] + k['rgbout'] + k['alout']]
        print(' '.join(fields))


if __name__ == '__main__':
    main()
