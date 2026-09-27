import bsslab, random, json, sys
rng = random.Random(20260926)
SHAPE = [("A", "char {n}[1024]"), ("B", "struct lab_entry {n}[512]"), ("C", "short {n}"), ("D", "short {n}"),
         ("E", "short {n}"), ("F", "boolean {n}"), ("G", "boolean {n}")]
JAN = {"A": 0, "B": 0x400, "C": 0x7400, "D": 0x7404, "E": 0x7408, "F": 0x740A, "G": 0x740B}
log = []
def run(mask, trial, names):
    ents = []
    for (role, decl), nm, m in zip(SHAPE, names, mask):
        init = None if m == "u" else ("{ 0 }" if "[" in decl else "0")
        ents.append((nm, decl, init))
    tag = "L1_%s_%02d" % (mask, trial)
    r = bsslab.compile_tu(ents, tag)
    off = {role: r["syms"][nm][2] for (role, _), nm in zip(SHAPE, names)}
    secidx = {role: r["syms"][nm][1] for (role, _), nm in zip(SHAPE, names)}
    order = "".join(sorted(off, key=lambda k: (secidx[k], off[k])))
    return r, off, order, secidx
# P1/P3/P5: all zero-init, 20 name draws
for mask in ("zzzzzzz", "uuuuuuu"):
    orders = {}
    for t in range(20):
        names = [bsslab.rand_name(rng) for _ in SHAPE]
        r, off, order, secidx = run(mask, t, names)
        key = order
        orders.setdefault(key, []).append(t)
        size = r["sections"]
        jan = all(off[k] == JAN[k] for k in JAN)
        log.append(dict(mask=mask, trial=t, names=names, off=off, order=order, sections=r["sections"], jan=jan))
        print(mask, t, order, r["sections"], "JAN" if jan else "", {k: hex(v) for k, v in off.items()})
    print(mask, "distinct orders:", len(orders))
json.dump(log, open(r"C:\halo-worktrees\claude-compiler-application-20260925\scratch\campaign\workers\H1\lab\L1_log.json", "w"), indent=1)
