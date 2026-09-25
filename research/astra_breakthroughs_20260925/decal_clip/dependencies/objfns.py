"""List code functions of an object in SECTION order, with strict-exact status vs January."""
import os, sys
WT = r'C:\halo-worktrees\claude-lane-a-ai-core-20260920'
sys.path.insert(0, WT)
from tools import coff_compare as cc


def fns_in_order(o):
    secs = o['sections']
    seen = {}
    for s in o['symbols']:
        if (s['name'].startswith('_') and s['section'] > 0 and s['storage'] in (2, 3)
                and s['value'] == 0 and secs[s['section'] - 1]['name'] == '.text'):
            seen.setdefault(s['section'], s['name'])
    return [seen[k] for k in sorted(seen)]


def status(objpath, unit):
    target = cc.load(open(os.path.join(WT, 'build', 'split', unit + '.obj'), 'rb').read())
    ours = cc.load(open(objpath, 'rb').read())
    tn = set(fns_in_order(target))
    out = []
    for name in fns_in_order(ours):
        oi = cc.section_info(ours, name)
        if name in tn:
            ti = cc.section_info(target, name)
            st = 'EXACT' if cc.section_infos_equal(ti, oi) else 'resid'
        else:
            st = 'notJ'
        out.append((name, oi['size'], st))
    return out


if __name__ == '__main__':
    for r in status(sys.argv[1], sys.argv[2]):
        print('%-6s %5d %s' % (r[2], r[1], r[0]))

