"""Normalised relocation-target multiset of one function: January (build/split) vs candidate objects.
symbol:X, defined-noncode:.rdata:X and .rdata+X are the same destination (name after the last section tag).
usage: relocms.py FUNCTION obj1 [obj2 ...]"""
import sys, subprocess, re, collections
fn = sys.argv[1]
def ms(obj):
    args = [sys.executable, '-B', 'tools/campaign/tinfo.py', '--fn', fn]
    args += (['source/ai/ai_communication'] if obj == 'JAN' else ['--object', obj])
    out = subprocess.run(args, capture_output=True, text=True).stdout
    c = collections.Counter()
    for ln in out.split('\n'):
        m = re.search(r'type=(0x[0-9a-f]+) target=(.*)$', ln.strip())
        if not m: continue
        t = m.group(2)
        t = re.sub(r'^(symbol|defined-noncode|defined-code):', '', t)
        t = re.sub(r'^\.(rdata|data|bss|text)[:+]', '', t)
        c[(m.group(1), t)] += 1
    return c
base = ms('JAN')
print('JAN', sum(base.values()), 'relocations')
for o in sys.argv[2:]:
    c = ms(o)
    miss = base - c; extra = c - base
    print('%s: %d relocations; missing %s; extra %s' % (o, sum(c.values()), dict(miss), dict(extra)))
