import sys, json, os
sys.path.insert(0, 'tools')
import coff_compare as cc

pj = json.load(open('config/parked.json'))
parked = set((e['unit'], e['function']) for e in pj['entries'])
# An asm transcription matches by construction, so "it gates EXACT" is not
# evidence that it was reconstructed in C. These stay PARKED even when exact;
# every other class is unparked once it matches (tools/campaign/unpark.py).
asm_parked = set((e['unit'], e['function']) for e in pj['entries']
                 if e.get('class') in ('asm-implemented', 'vendored-assembly'))

parked_total = [0]
asm_total = [0]
d = json.load(open('objdiff.json'))
rows = []
tot_bytes = done_bytes = 0
tot_fn = done_fn = 0
objs_total = objs_done = 0
for u in d['units']:
    tp, bp = u.get('target_path'), u.get('base_path')
    if not tp or not os.path.exists(tp):
        continue
    target = cc.load(open(tp,'rb').read())
    base = cc.load(open(bp,'rb').read()) if (bp and os.path.exists(bp)) else None
    seen = set()
    unit_rows = []
    secs = target['sections']
    for sym in target['symbols']:
        name = sym['name']
        if not name.startswith('_') or name in seen:
            continue
        if sym['section'] <= 0 or sym['storage'] not in (2,3) or sym['value'] != 0:
            continue
        if secs[sym['section']-1]['name'] != '.text':
            continue
        if sym['section'] in seen:
            continue
        seen.add(sym['section'])
        try:
            ti = cc.section_info(target, name)
        except Exception:
            continue
        if ti is None:
            continue
        seen.add(name)
        size = ti['size']
        status = 'UNWRITTEN'
        if base is not None:
            try:
                bi = cc.section_info(base, name)
            except Exception:
                bi = None
            if bi is not None:
                status = 'EXACT' if cc.section_infos_equal(ti, bi) else 'residual'
        if (u['name'], name) in asm_parked:
            status = 'ASM'
        elif (u['name'], name) in parked and status != 'EXACT':
            status = 'PARKED'
        unit_rows.append((name, size, status))
    if not unit_rows:
        continue
    ub = sum(r[1] for r in unit_rows)
    db = sum(r[1] for r in unit_rows if r[2] == 'EXACT')
    fn = len(unit_rows)
    dn = sum(1 for r in unit_rows if r[2] == 'EXACT')
    npark = sum(1 for r in unit_rows if r[2] == 'PARKED')
    nasm = sum(1 for r in unit_rows if r[2] == 'ASM')
    parked_total[0] += npark
    asm_total[0] += nasm
    tot_bytes += ub; done_bytes += db; tot_fn += fn; done_fn += dn
    objs_total += 1
    if dn == fn: objs_done += 1
    rows.append((u['name'], ub, db, fn, dn, unit_rows))

rows.sort(key=lambda r: -(r[1]-r[2]))
print(f"objects {objs_done}/{objs_total} fully exact; bytes {done_bytes}/{tot_bytes}; fns {done_fn}/{tot_fn}"
      f"   [STRICT section_infos_equal only; {parked_total[0]} parked + "
      f"{asm_total[0]} asm-implemented functions NOT counted]")
sel = sys.argv[1] if len(sys.argv)>1 else None
for name, ub, db, fn, dn, ur in rows:
    if sel:
        if sel in name:
            print(f"\n{name}: {db}/{ub} B, {dn}/{fn} fn")
            for n,s,st in sorted(ur, key=lambda r:-r[1]):
                if st != 'EXACT':
                    print(f"  {st:9s} {s:6d}  {n}")
    elif fn-dn>0 and (ub-db)<=8000 and dn>0:
        print(f"{name:60s} {db:6d}/{ub:6d} B  {dn:3d}/{fn:3d} fn  remaining {ub-db:5d} B / {fn-dn} fn")
