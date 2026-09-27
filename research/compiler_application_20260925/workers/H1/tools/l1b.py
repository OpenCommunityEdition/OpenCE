import bsslab, random, json
rng = random.Random(4242)
SHAPE = [("A", "char {n}[1024]"), ("B", "struct lab_entry {n}[512]"), ("C", "short {n}"), ("D", "short {n}"),
         ("E", "short {n}"), ("F", "boolean {n}"), ("G", "boolean {n}")]
viol = 0; n = 0; nsec = set()
log = []
for t in range(60):
    mask = "".join(rng.choice("uz") for _ in SHAPE)
    names = [bsslab.rand_name(rng) for _ in SHAPE]
    ents = [(nm, decl, None if m == "u" else ("{ 0 }" if "[" in decl else "0")) for (role, decl), nm, m in zip(SHAPE, names, mask)]
    r = bsslab.compile_tu(ents, "L1b_%02d" % t)
    nsec.add(len([s for s in r["sections"] if s[1] == ".bss"]))
    off = {role: r["syms"][nm][2] for (role, _), nm in zip(SHAPE, names)}
    order = sorted(off, key=lambda k: off[k])
    ms = {role: m for (role, _), m in zip(SHAPE, mask)}
    seq = "".join(ms[k] for k in order)
    zorder = [k for k in order if ms[k] == "z"]
    decl_ok = zorder == sorted(zorder)
    ok = ("zu" not in seq)
    n += 1; viol += (not ok) or (not decl_ok)
    log.append(dict(t=t, mask=mask, names=names, off=off, order="".join(order), bucketseq=seq, uninit_first=ok, zero_decl_order=decl_ok, sections=r["sections"]))
    print(t, mask, "".join(order), seq, "uninit-first" if ok else "VIOLATION", "zero-decl-order" if decl_ok else "ZERO-ORDER-VIOLATION", r["sections"])
print("trials", n, "violations", viol, "bss section counts seen", nsec)
json.dump(log, open(r"C:\halo-worktrees\claude-compiler-application-20260925\scratch\campaign\workers\H1\lab\L1b_log.json", "w"), indent=1)
