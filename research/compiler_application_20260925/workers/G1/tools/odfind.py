"""G1 read-only helper: locate the first-party /Od body of a January function by its float-constant and callee-count
fingerprint (halo_cache_symbols.exe + Ghidra's function table; data only).

usage: python -B odfind.py <unit> <fn> [<fn> ...]
Scores every /Od function by the weighted overlap of the float literals it references with the January function's
__real@ relocation targets, and prints the best five with sizes. A candidate must still be confirmed by reading it.
"""
import collections
import importlib.util
import json
import os
import pickle
import re
import struct
import sys
from pathlib import Path

HERE = Path(__file__).resolve()
OD = HERE.parents[2] / 'W1' / 'tools' / 'odbuild.py'
spec = importlib.util.spec_from_file_location('odbuild', OD)
ob = importlib.util.module_from_spec(spec)
spec.loader.exec_module(ob)
sys.path.insert(0, 'tools')
import coff_compare as cc  # noqa: E402

TSV = Path(ob.EXE).with_suffix('.exe.functions.tsv')
CACHE = Path('scratch/campaign/workers/G1/odfind_cache.pkl')
RDATA = (0x93c000, 0x93c000 + 0xb2e00)


def od_index():
    if CACHE.exists():
        return pickle.loads(CACHE.read_bytes())
    idx = {}
    for line in open(TSV, encoding='utf-8').read().splitlines()[1:]:
        entry, name, src, ranges = line.split('\t')
        rng = json.loads(ranges.replace('[', '["').replace(']', '"]').replace(', ', '", "').replace('""', '"')
                         .replace('["[', '[[').replace(']"]', ']]')) if False else re.findall(r'([0-9a-f]{8}), ([0-9a-f]{8})', ranges)
        lo = min(int(a, 16) for a, b in rng)
        hi = max(int(b, 16) for a, b in rng)
        if hi - lo < 16 or hi - lo > 0x10000:
            continue
        o = ob.va2off(lo)
        if o is None:
            continue
        blob = bytes(ob.DATA[o:o + (hi - lo + 1)])
        consts = collections.Counter()
        calls = 0
        for m in re.finditer(rb'[\x00-\xff]', b''):
            pass
        for i in range(len(blob) - 4):
            v = struct.unpack_from('<I', blob, i)[0]
            if RDATA[0] <= v < RDATA[1]:
                fo = ob.va2off(v)
                f = struct.unpack_from('<f', ob.DATA, fo)[0]
                if f == f and 1e-7 < abs(f) < 1e7 and blob[i - 1:i] in (b'\x05', b'\x0d', b'\x15', b'\x1d', b'\x25', b'\x2d', b'\x35', b'\x3d'):
                    consts[round(f, 6)] += 1
            if blob[i] == 0xe8:
                calls += 1
        idx[lo] = (hi - lo + 1, consts, calls)
    CACHE.write_bytes(pickle.dumps(idx))
    return idx


def jan_consts(unit, fn):
    obj = cc.load(open('build/split/' + unit + '.obj', 'rb').read())
    info = cc.section_info(obj, fn)
    c = collections.Counter()
    ncall = 0
    for r in info['relocations']:
        t = r.get('symbolic_target') or r['target']
        name = t[1] if isinstance(t[1], str) else ''
        m = re.search(r'__real@([0-9a-f]{8})$', name)
        if m:
            f = struct.unpack('<f', bytes.fromhex(m.group(1))[::-1])[0]
            if 1e-7 < abs(f) < 1e7:
                c[round(f, 6)] += 1
        if r['type'] == 0x14:
            ncall += 1
    return info['size'], c, ncall


def main():
    unit = sys.argv[1]
    idx = od_index()
    for fn in sys.argv[2:]:
        size, jc, ncall = jan_consts(unit, fn)
        scores = []
        for va, (osz, oc, ocalls) in idx.items():
            if not oc:
                continue
            inter = sum((jc & oc).values())
            union = sum((jc | oc).values())
            if not inter:
                continue
            s = inter / union - 0.1 * abs(ocalls - ncall) / max(ncall, 1)
            scores.append((s, va, osz, ocalls, inter, union))
        scores.sort(reverse=True)
        print('## %s (J %d B, %d calls, consts %s)' % (fn, size, ncall, dict(jc)))
        for s, va, osz, ocalls, inter, union in scores[:5]:
            print('   %.3f  od 0x%x  %5d B  calls %3d  const %d/%d' % (s, va, osz, ocalls, inter, union))


if __name__ == '__main__':
    main()
