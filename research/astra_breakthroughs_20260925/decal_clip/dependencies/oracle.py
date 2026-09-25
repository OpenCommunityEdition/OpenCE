"""Allocator oracle: per-function trace of VC7's global integer colouring.

  python oracle.py <src.c> <unit> <function> [--force CHOOSE:<k>:<reg>] [--out obj]

Prints every coloured web of <function> in pop order with:
  id, chosen reg, priority (and its refs x weight terms), [W+0x44] position,
  initial allowed set (at first pricing), allowed set at decision time,
  interfering web ids (neighbour set iterated by the chooser), cost table,
  the MODEL prediction (argmin cost over allowed, class order, ties to earlier)
and checks the model rules:
  R1 chooser rule   chosen == model prediction from (allowed, cost)
  R2 allowed rule   allowed(decision) == initial - regs of already-coloured neighbours
  R3 order rule     pops sorted by priority desc within each pricing batch
"""
import os, sys, json
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import trace as T
from objfns import fns_in_order, status
WT = T.WT
sys.path.insert(0, WT)
from tools import coff_compare as cc

CLASS = ['eax', 'ecx', 'edx', 'esi', 'edi', 'ebx', 'ebp']
IDX = {'eax': 1, 'ecx': 2, 'edx': 3, 'ebx': 4, 'ebp': 6, 'esi': 7, 'edi': 8}


def s32(v):
    return v - (1 << 32) if v is not None and v >= (1 << 31) else v


def chain_set(ch):
    if not ch or ch[0] != 0: return None
    return [T.REGN[r] for r in (1, 2, 3, 7, 8, 4, 6) if (ch[2] >> r) & 1]


def analyse(seg):
    """Return (decisions, batches) with initial allowed sets and model checks."""
    init = {}      # W -> first allowed set seen at pricing
    ids = {}
    dec = []
    batch = 0
    last_kind = None
    nb = []
    gen = {}
    for e in seg:
        if e[0] == 'N':
            nb.append(e[1]); continue
        if e[0] == 'P':
            continue
        if e[0] in 'SF':
            if e[0] == 'F':
                init.pop(e[1], None)
                gen[e[2]] = gen.get(e[2], 0) + 1
            continue
        if e[0] == 'A':
            if e[1] not in init and e[5] is not None:
                init[e[1]] = chain_set(e[5])
            if last_kind == 'C': batch += 1
            last_kind = 'A'
            continue
        _, W, reg, mem, rm, ch, chs = e
        last_kind = 'C'
        d = dict(W=W, id=rm[7], reg=T.REGN.get(reg, reg), pri=s32(rm[3]), pos=rm[0x11],
                 f40=rm[0x10], cost={r: mem[IDX[r]] for r in CLASS[:6]},
                 allowed=chain_set(ch), nbrs=sorted(set(nb)), batch=batch, chs=chs)
        d['init'] = init.get(W)
        d['gen'] = gen.get(d['id'], 0)
        d['gen_of'] = dict(gen)
        nb = []
        dec.append(d)
        init.pop(W, None)
    return dec


def attach_terms(seg, dec):
    acc = {}; sub = {}
    seen = set()
    done = []
    for e in seg:
        if e[0] == 'F':
            if e[1] in acc or e[1] in sub:
                pass
            acc.pop(e[1], None); sub.pop(e[1], None); seen.discard(e[1])
            continue
        if e[0] == 'C':
            seen.add(e[1])
            done.append((e[1], list(acc.get(e[1], [])), list(sub.get(e[1], []))))
            continue
        if e[0] in 'NP' or (e[1] in seen and e[0] in 'AS'):
            continue   # after the web was coloured (record may be recycled)
        if e[0] == 'A':
            acc.setdefault(e[1], []).append((e[2], e[3]))
        elif e[0] == 'S':
            sub.setdefault(e[1], []).append(e[2])
    for d, (W, a, sb) in zip(dec, done):
        assert W == d['W']
        d['terms'] = a
        d['subs'] = sb
        d['pri_model'] = sum(a * b for a, b in d['terms']) - sum(d['subs'])


def model_choice(d):
    best = None
    for r in CLASS[:6]:
        if d['allowed'] is None or r not in d['allowed']: continue
        if best is None or d['cost'][r] < d['cost'][best]:
            best = r
    return best


def check(dec):
    out = {'R0': [0, 0], 'R1': [0, 0], 'R2': [0, 0], 'R3': [0, 0]}
    for d in dec:
        out['R0'][0] += 1
        if d['pri_model'] == d['pri']: out['R0'][1] += 1
        else: d['R0fail'] = d['pri_model']
    reg_of = {}
    # R1
    for d in dec:
        p = model_choice(d)
        d['model'] = p
        out['R1'][0] += 1
        if p == d['reg']: out['R1'][1] += 1
    # R2: interference. Web B is a neighbour of A iff B is in A's uncoloured-neighbour
    # set when A is coloured (the chooser iterates only uncoloured neighbours), so an
    # edge between two coloured webs is visible at the EARLIER one's decision.
    # Weak form (falsifiable): no earlier-coloured neighbour's register is allowed.
    # The remaining exclusions are 'static' (class/call-crossing/byte/frame):
    # reported so the static families can be checked.
    earlier = []
    for d in dec:
        taken = {e['reg'] for e in earlier if d['id'] in e['nbrs'] and e['gen_of'].get(d['id'], 0) == d['gen']}
        d['taken'] = sorted(taken)
        out['R2'][0] += 1
        if d['allowed'] is not None and not (taken & set(d['allowed'])):
            out['R2'][1] += 1
        else:
            d['R2fail'] = sorted(taken & set(d['allowed'] or []))
        d['static_excl'] = sorted(set(CLASS) - set(d['allowed'] or []) - taken)
        earlier.append(d)
    # R3: within a batch, priorities non-increasing; ties broken by pos
    for i in range(1, len(dec)):
        a, b = dec[i - 1], dec[i]
        if a['batch'] != b['batch']: continue
        out['R3'][0] += 1
        if (a['pri'], a['pos'], a['id']) > (b['pri'], b['pos'], b['id']):
            out['R3'][1] += 1
        else:
            b['R3fail'] = 'after id=%d pri=%d pos=%x' % (a['id'], a['pri'], a['pos'])
    ties = [(dec[i - 1]['pos'], dec[i]['pos']) for i in range(1, len(dec))
            if dec[i - 1]['batch'] == dec[i]['batch'] and dec[i - 1]['pri'] == dec[i]['pri']]
    out['ties_pos_pairs'] = ties
    return out


def run_fn(src, unit, fn, work=None, force=()):
    work = os.path.abspath(work or os.path.join(T.HERE, 'work'))
    seg, obj = T.run_gated(src, work, unit, fn, force=force)
    dec = analyse(seg); attach_terms(seg, dec)
    return dec, obj, 0, None


def exact_vs_january(obj, unit, fn):
    t = cc.load(open(os.path.join(WT, 'build', 'split', unit + '.obj'), 'rb').read())
    o = cc.load(open(obj, 'rb').read())
    ti = cc.section_info(t, fn); oi = cc.section_info(o, fn)
    return cc.section_infos_equal(ti, oi), oi['size'], ti['size']


def fmt(d):
    return '#%-2d id=%-3d %-4s model=%-4s pri=%5d pos=%4x b%d allowed=%-28s nbrs=%s cost=%s terms=%s%s%s' % (
        d['k'], d['id'], d['reg'], d['model'], d['pri'], d['pos'], d['batch'], ','.join(d['allowed'] or []),
        d['nbrs'], {r: c for r, c in d['cost'].items() if c}, '+'.join('%dx%d' % t for t in d['terms']),
        ' R2FAIL%s' % (d['R2fail'],) if 'R2fail' in d else '', ' R3FAIL(%s)' % d['R3fail'] if d.get('R3fail') else '') + (
        ' -sub%d' % sum(d['subs']) if d['subs'] else '') + (' R0FAIL(model %d)' % d['R0fail'] if 'R0fail' in d else '')


if __name__ == '__main__':
    import argparse
    ap = argparse.ArgumentParser()
    ap.add_argument('src'); ap.add_argument('unit'); ap.add_argument('fn')
    ap.add_argument('--force', action='append', default=[],
                    help='k:reg  force this function\'s k-th decision (0-based) to reg')
    ap.add_argument('--out')
    ap.add_argument('--json')
    a = ap.parse_args()
    dec0, obj, base, order = run_fn(a.src, a.unit, a.fn)
    force = []
    for f in a.force:
        k, r = f.split(':')
        hit = base + int(k) + 1
        v = IDX[r]
        # ebx holds the choice; [esp+0x10] holds its copy used to update neighbours
        force.append(('CHOOSE', hit, 0, 1, 0, v))
        force.append(('CHOOSE', hit, 1, 6, 0x10, v))
    if force:
        dec, obj, base, order = run_fn(a.src, a.unit, a.fn, force=force)
    else:
        dec = dec0
    for i, d in enumerate(dec): d['k'] = i
    res = check(dec)
    for d in dec: print(fmt(d))
    ex, osz, tsz = exact_vs_january(obj, a.unit, a.fn)
    print('checks: R0 priority %d/%d  R1 chooser %d/%d  R2 allowed %d/%d  R3 order %d/%d  tie pos pairs %s' % (
        res['R0'][1], res['R0'][0], res['R1'][1], res['R1'][0], res['R2'][1], res['R2'][0], res['R3'][1], res['R3'][0], res['ties_pos_pairs']))
    print('EXACT vs January: %s  (ours %d / January %d)' % (ex, osz, tsz))
    if a.out:
        import shutil; shutil.copy(obj, a.out)
    if a.json:
        json.dump(dict(dec=dec, res=res, exact=ex), open(a.json, 'w'), default=str, indent=1)


def uncoloured(seg):
    """Webs that were priced (ACC/SUB) but whose record was freed or never chosen.
    Returns list of dicts {id, W, pri_model, terms, subs, freed}."""
    live = {}
    out = []
    for e in seg:
        if e[0] == 'A':
            W = e[1]; rm = e[6] if len(e) > 6 else None
            r = live.setdefault(W, dict(W=W, id=(rm[7] if rm else None), terms=[], subs=[], freed=False))
            r['terms'].append((e[2], e[3]))
            if rm and r['id'] is None: r['id'] = rm[7]
        elif e[0] == 'S':
            if e[1] in live: live[e[1]]['subs'].append(e[2])
        elif e[0] == 'C':
            live.pop(e[1], None)
        elif e[0] == 'F':
            r = live.pop(e[1], None)
            if r:
                r['freed'] = True; r['id'] = e[2]
                r['pri_model'] = sum(a * b for a, b in r['terms']) - sum(r['subs'])
                out.append(r)
    for r in live.values():
        r['pri_model'] = sum(a * b for a, b in r['terms']) - sum(r['subs'])
        out.append(r)
    return out


def pops(seg):
    """Every popped web in pop order with its outcome.
    Returns list of dicts {W, id, pri_model, outcome} where outcome is the register or
    'SPILL/SKIP' (popped, never reached the chooser)."""
    info = {}
    out = []
    pending = None
    rnd = 0; last = None
    for e in seg:
        if e[0] in 'ASF' and last in ('P', 'C'):
            rnd += 1
        if e[0] in 'ASFPC':
            last = e[0]
        if e[0] == 'A':
            W = e[1]; rm = e[6] if len(e) > 6 else None
            r = info.get(W)
            if r is None or r.get('closed'):
                r = info[W] = dict(W=W, id=(rm[7] if rm else None), terms=[], subs=[])
            r['terms'].append((e[2], e[3]))
        elif e[0] == 'S':
            if e[1] in info: info[e[1]]['subs'].append(e[2])
        elif e[0] == 'F':
            if e[1] in info: info[e[1]]['closed'] = True; info[e[1]]['id'] = e[2]
        elif e[0] == 'P':
            if pending is not None: out.append(pending)
            r = info.get(e[1], dict(W=e[1], id=None, terms=[], subs=[]))
            rm = e[2]; ch = e[3]
            pending = dict(W=e[1], id=(rm[7] if rm else r['id']), pri_model=sum(a * b for a, b in r['terms']) - sum(r['subs']),
                           refs=sum(a for a, b in r['terms']), outcome='SPILL/SKIP',
                           pri=s32(rm[3]) if rm else None, f40=s32(rm[0x10]) if rm else None,
                           flags=(rm[1] if rm else None), allowed=chain_set(ch), pos=rm[0x11] if rm else None, rm=rm,
                           round=rnd)
        elif e[0] == 'C':
            if pending is not None and pending['W'] == e[1]:
                pending['outcome'] = T.REGN.get(e[2]); pending['id'] = e[4][7]
                out.append(pending); pending = None
            info.get(e[1], {})['closed'] = True
    if pending is not None: out.append(pending)
    return out


def gate_model(p):
    """Predicted pop outcome: True = coloured (if allowed non-empty), False = skipped,
    None = the undecoded 0x10795bb6 check (benefit > 0 but no def-cost flag 0x400)."""
    f40 = p['f40']; rm = p['rm']; fl = p['flags'] or 0
    if not p['allowed']:
        return False
    if f40 < 0:
        return False
    if f40 == 0:
        single = rm[10] == rm[11]
        const = bool(fl & 0x10000)
        return single and not const and rm[9] > 2
    if not (fl & 0x400):
        return None
    return True


def replay(seg):
    """End-to-end replay of the allocator from its INPUTS only.

    Inputs taken from the trace: per pop batch, the set of webs popped, each web's
    priority/position/id (R0 gives the priority from refs x pressure terms), benefit
    [W+0x40] and flags, its STATIC register class (allowed set at decision time plus the
    registers held by already-coloured neighbours), its interference edges, and the
    captured cost table.  Output: predicted pop order and register for every web,
    compared with what C2.Dll actually did.  Returns (n_webs, n_order_ok, n_reg_ok, diffs)."""
    pl = pops(seg)
    dec = analyse(seg); attach_terms(seg, dec)
    # map coloured pops to their decision records (same order)
    di = 0
    for p in pl:
        if p['outcome'] != 'SPILL/SKIP':
            p['dec'] = dec[di]; di += 1
    # batches: consecutive pops between pricing phases -> use decision batch ids; skipped
    # pops inherit the batch of the previous pop
    for p in pl:
        p['batch'] = p['round']
    # static class = allowed at decision + regs of coloured neighbours at that time
    edges = {}
    for p in pl:
        if 'dec' in p:
            d = p['dec']
            p['static'] = set(d['allowed'] or []) | set(d.get('taken', []))
        else:
            p['static'] = set(p['allowed'] or [])
    check(dec)   # fills d['taken']
    for p in pl:
        if 'dec' in p:
            p['static'] = set(p['dec']['allowed'] or []) | set(p['dec']['taken'])
    n = len(pl); order_ok = reg_ok = 0; diffs = []
    # predicted order within batch: sort by (pri, pos, id) desc (stable on actual order)
    from itertools import groupby
    pred_seq = []
    for bb, grp in groupby(pl, key=lambda p: p['batch']):
        g = list(grp)
        seen = set(); first = []; again = []
        for p in g:
            (again if p['W'] in seen else first).append(p); seen.add(p['W'])
        # a web skipped with benefit < 0 is re-appended at the list TAIL (0x1072f411 ->
        # 0x1072f6c2: it inherits the tail's priority), so re-pops follow in skip order
        pred_seq += sorted(first, key=lambda p: (p['pri'], p['pos'] or 0, p['id'] or 0), reverse=True) + again
    for a, p in zip(pl, pred_seq):
        if a is p: order_ok += 1
    # predicted registers: simulate in ACTUAL pop order (order checked separately)
    held = {}   # (id, gen-less W) -> reg for coloured-so-far
    coloured = []
    for p in pl:
        g = gate_model(p)
        if 'dec' in p:
            d = p['dec']
            taken = {e['dec']['reg'] for e in coloured if d['id'] in e['dec']['nbrs']
                     and e['dec']['gen_of'].get(d['id'], 0) == d['gen']}
            allowed = [r for r in CLASS[:6] if r in p['static'] and r not in taken]
            best = None
            for r in allowed:
                if best is None or d['cost'][r] < d['cost'][best]: best = r
            pr = best if g is not False else 'SPILL/SKIP'
            if g is None: pr = best  # undecoded branch: assume colour
            coloured.append(p)
        else:
            pr = 'SPILL/SKIP' if g is not True else '(colour)'
        if pr == p['outcome']: reg_ok += 1
        else: diffs.append((p['id'], pr, p['outcome']))
    return n, order_ok, reg_ok, diffs

