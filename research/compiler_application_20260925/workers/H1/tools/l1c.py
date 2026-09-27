import bsslab, random, json
rng = random.Random(777)
pool = []
while len(pool) < 150:
    n = bsslab.rand_name(rng)
    if n not in pool: pool.append(n)
ents = [(n, "boolean {n}", None) for n in pool]
r = bsslab.compile_tu(ents, "L1c_pool")
order = [n for n in r["order"]]
print("pool order first 20:", order[:20])
pos = {n: i for i, n in enumerate(order)}
# consistency: random subsets in shuffled declaration order
bad = 0
for t in range(12):
    sub = rng.sample(pool, 12)
    rng.shuffle(sub)
    r2 = bsslab.compile_tu([(n, "boolean {n}", None) for n in sub], "L1c_sub%02d" % t)
    exp = sorted(sub, key=lambda n: pos[n])
    ok = r2["order"] == exp
    bad += not ok
    print("subset", t, "consistent" if ok else "INCONSISTENT", r2["order"][:6], exp[:6])
json.dump(dict(pool_order=order), open(r"C:\halo-worktrees\claude-compiler-application-20260925\scratch\campaign\workers\H1\lab\L1c_pool.json", "w"), indent=1)
print("inconsistent subsets", bad)
