"""Run VC7 (C2.Dll) under dbg32c and capture the global integer allocator's inputs
and decisions for every function in one translation unit.

Breakpoints (all LIGHT, uncapped):
  bp0 MARK   per-function backend marker (first backend pass)
  bp1 ACC1   0x1072a527  priority accumulate: eax=web, ecx=refs-in-block, edx=block weight
  bp2 ACC2   0x1072a54c  same, second site
  bp3 CHOOSE 0x1072da96  register chosen: eax=web, ebx=register index, cost table @0x10864600

Output: list of events as tuples, and the compiled object (for byte comparison).
"""
import os, re, subprocess, sys, json, shutil
HERE = os.path.dirname(os.path.abspath(__file__))
DEBUGGER = os.path.join(HERE, 'dbg32c.exe')
WT = r'C:\halo-worktrees\claude-lane-a-ai-core-20260920'
CL = os.path.join(WT, 'xbox', 'bin', 'vc7', 'CL.Exe')
REGN = {1: 'eax', 2: 'ecx', 3: 'edx', 4: 'ebx', 5: 'esp', 6: 'ebp', 7: 'esi', 8: 'edi'}
MARK = 0x10720825
CHAINS = '0 4 20 0;0 12 28;0 12 2c'
ACC1, ACC2, NEIGH, CHOOSE, SUB, FREE, POP = 0x1072a527, 0x1072a54c, 0x1072d9c0, 0x1072da96, 0x1072ab30, 0x10719244, 0x10727cd4


def cflags(unit='source/ai/actors'):
    bn = re.sub(r'\s*\$\r?\n\s*', ' ', open(os.path.join(WT, 'build.ninja')).read())
    key = 'build' + chr(92) + 'base' + chr(92) + unit.replace('/', chr(92)) + '.obj:'
    i = bn.index(key); j = bn.index('cflags = ', i); k = bn.index('\nbuild ', j)
    cf = re.sub(r'\s+', ' ', bn[j + 9:k]).strip()
    out = []
    for t in re.findall(r'/I"[^"]+"|\S+', cf):
        if t.startswith('/I'):
            p = t[2:].strip('"')
            out.append('/I' + (p if os.path.isabs(p) else os.path.join(WT, p.replace('/', os.sep))))
        else:
            out.append(t)
    return out


def run(src, workdir, unit='source/ai/actors', marks=(MARK,), extra_cflags=(), force=(), dwp=(), gate=None, raw_bps=None):
    """Compile src under the debugger in workdir. Returns (events, objpath)."""
    os.makedirs(workdir, exist_ok=True)
    exe = os.path.join(workdir, 'dbg32c.exe')
    shutil.copy(DEBUGGER, exe)
    obj = os.path.join(workdir, 'trace.obj')
    if os.path.exists(obj): os.remove(obj)
    args = [CL, '/nologo', '/c'] + cflags(unit) + list(extra_cflags) + \
           ['/I' + os.path.dirname(os.path.join(WT, unit)), '/Fo' + obj, os.path.abspath(src)]
    cmd = ' '.join(('"%s"' % a if ' ' in a else a) for a in args)
    w = lambda n, b: open(os.path.join(workdir, n), 'wb').write(b)
    w('dbg_cmd.u16', (cmd + '\0').encode('utf-16-le'))
    w('dbg_dir.u16', (WT + '\0').encode('utf-16-le'))
    bps = list(marks) + (list(raw_bps) if raw_bps else [ACC1, ACC2, NEIGH, CHOOSE, SUB, FREE, POP])
    w('dbg_bp.txt', (' '.join('0x%x' % b for b in bps) + '\n').encode())
    # force entries: (which, hit, kind, reg, off, val); which in {'CHOOSE','ACC1',...} or bp index
    names = {'MARK': 0, 'ACC1': len(marks), 'ACC2': len(marks) + 1, 'NEIGH': len(marks) + 2, 'CHOOSE': len(marks) + 3, 'SUB': len(marks) + 4, 'FREE': len(marks) + 5, 'POP': len(marks) + 6}
    fl = ''.join('%d %d %d %d %x %x\n' % (names.get(f[0], f[0]), f[1], f[2], f[3], f[4], f[5] & 0xffffffff) for f in force)
    w('dbg_force.txt', fl.encode())
    for f in ('dbg_wp.txt', 'dbg_scan.txt', 'dbg_dwp.txt', 'dbg_gate.txt', 'dbg_swp.txt'):
        p = os.path.join(workdir, f)
        if os.path.exists(p): os.remove(p)
    if gate is not None:
        lo, hi = gate if isinstance(gate, tuple) else (gate, gate)
        w('dbg_gate.txt', ('%d %d' % (lo, hi)).encode())
    if dwp:
        w('dbg_dwp.txt', ''.join('%d %d %d %x %d %d NL' % d for d in dwp).replace('NL', chr(10)).encode())
    w('dbg_light.txt', ('900000 %d' % ((1 << len(bps)) - 1)).encode())
    w('dbg_rmem.txt', b'0 24')
    w('dbg_mem.txt', b'0x10864600 9')
    w('dbg_chain.txt', CHAINS.encode())
    subprocess.run([exe], cwd=workdir, capture_output=True, timeout=1800)
    nm = len(marks)
    ev = []
    sg = lambda v: v - 0x100000000 if v & 0x80000000 else v
    for l in open(os.path.join(workdir, 'dbg_result.txt')):
        if l.startswith('GATE'):
            ev.append(('M', 0)); continue
        if not l.startswith('LT'): continue
        t = l.split()
        bi = int(t[1][2:], 16)
        regs = {k: int(v, 16) for k, v in (x.split('=') for x in t[3:10])}
        mem = rm = ch = None
        chs = {}
        for ci in range(4):
            tag = 'chain%d:' % ci
            if tag in t:
                i = t.index(tag); vals = []
                for x in t[i + 1:]:
                    if not x.startswith('0x'): break
                    vals.append(int(x, 16))
                chs[ci] = vals or None
        ch = chs.get(0)
        if 'mem:' in t:
            i = t.index('mem:'); mem = [sg(int(x, 16)) for x in t[i + 1:i + 10]]
        if 'rmem:' in t:
            i = t.index('rmem:'); rm = [int(x, 16) for x in t[i + 1:i + 25]]
        if raw_bps and bi >= nm:
            ev.append(('RAW', bi - nm, regs, rm, chs)); continue
        if bi < nm:
            ev.append(('M', bi))
        elif bi in (nm, nm + 1):
            ev.append(('A', regs['eax'], regs['ecx'], regs['edx'], bi, ch, rm))
        elif bi == nm + 2:
            ev.append(('N', regs['eax']))
        elif bi == nm + 3:
            ev.append(('C', regs['eax'], regs['ebx'], mem, rm, ch, chs))
        elif bi == nm + 4:
            ev.append(('S', regs['eax'], regs['edx']))
        elif bi == nm + 5:
            ev.append(('F', regs['ebx'], regs['edx']))
        else:
            ev.append(('P', regs['eax'], rm, ch))
    return ev, (obj if os.path.exists(obj) else None)


def segment(ev):
    """Split events into per-function lists at each marker."""
    segs = [[]]
    for e in ev:
        if e[0] == 'M':
            segs.append([])
        else:
            segs[-1].append(e)
    return segs


def summarize(seg):
    """Per web: priority accumulation terms, and colouring decisions in pop order."""
    acc = {}
    dec = []
    nb = []
    for e in seg:
        if e[0] == 'N':
            nb.append(e[1]); continue
        if e[0] == 'A':
            _, W, refs, wt = e[:4]
            acc.setdefault(W, []).append((refs, wt))
        else:
            _, W, reg, mem, rm, ch = e[:6]
            allowed = [REGN[r] for r in (1, 2, 3, 7, 8, 4, 6) if ch and ch[0] == 0 and (ch[2] >> r) & 1] if ch else None
            dec.append(dict(W=W, reg=REGN.get(reg, reg), pri=rm[3] if rm else None,
                            pos=rm[0x11] if rm else None, f40=rm[0x10] if rm else None,
                            cost={REGN[r]: mem[r] for r in (1, 2, 3, 7, 8, 4)} if mem else None,
                            terms=list(acc.get(W, [])), rec=rm, allowed=allowed, chain=ch, id=rm[7] if rm else None, nbrs=nb))
            nb = []
    return dec


if __name__ == '__main__':
    import argparse
    ap = argparse.ArgumentParser()
    ap.add_argument('src'); ap.add_argument('--unit', default='source/ai/actors')
    ap.add_argument('--work', default=os.path.join(HERE, 'work'))
    a = ap.parse_args()
    ev, obj = run(a.src, a.work, a.unit)
    segs = segment(ev)
    print('markers', len(segs) - 1, 'obj', obj)
    for k, s in enumerate(segs):
        d = summarize(s)
        print('--- seg', k, 'decisions', len(d))
        for x in d:
            print('  W=%08x %-4s pri=%5d pos=%5x f40=%4d terms=%s cost=%s' % (
                x['W'], x['reg'], x['pri'], x['pos'], x['f40'],
                '+'.join('%dx%d' % t for t in x['terms']), x['cost']))


_LOC = {}


def locate(src, unit, fn, workdir):
    """1-based function index of fn in the object's section order (= marker count)."""
    key = (os.path.abspath(src), os.path.getmtime(src), unit, fn)
    if key not in _LOC:
        ev, obj = run(src, workdir, unit, gate=10 ** 6)
        sys.path.insert(0, WT)
        from tools import coff_compare as cc
        sys.path.insert(0, HERE)
        from objfns import fns_in_order
        order = fns_in_order(cc.load(open(obj, 'rb').read()))
        _LOC[key] = (order.index(fn) + 1, len(order))
    return _LOC[key]


def run_gated(src, workdir, unit, fn, **kw):
    """Trace ONLY fn. Returns (seg, obj). Hit numbers for force= are 1-based within fn."""
    k, n = locate(src, unit, fn, workdir)
    ev, obj = run(src, workdir, unit, gate=k, **kw)
    segs = segment(ev)
    return segs[k], obj
