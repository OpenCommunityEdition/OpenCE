import bsslab, json
pool = json.load(open(r"C:\halo-worktrees\claude-compiler-application-20260925\scratch\campaign\workers\H1\lab\L1c_pool.json"))["pool_order"]
# pick 7 names spread across the pool order, keep key order, assign to January roles A..G
idx = [5, 25, 45, 65, 85, 105, 125]
names = [pool[i] for i in idx]
SHAPE = [("A", "char {n}[1024]"), ("B", "struct lab_entry {n}[512]"), ("C", "short {n}"), ("D", "short {n}"),
         ("E", "short {n}"), ("F", "boolean {n}"), ("G", "boolean {n}")]
JAN = {"A": 0, "B": 0x400, "C": 0x7400, "D": 0x7404, "E": 0x7408, "F": 0x740A, "G": 0x740B}
import itertools, random
for trial, decl_perm in enumerate([list(range(7)), [6,5,4,3,2,1,0], [3,0,6,1,5,2,4]]):
    ents = [(names[i], SHAPE[i][1], None) for i in decl_perm]
    r = bsslab.compile_tu(ents, "L1d_uninit_janorder_%d" % trial)
    off = {SHAPE[i][0]: r["syms"][names[i]][2] for i in range(7)}
    ok = all(off[k] == JAN[k] for k in JAN)
    print("decl order", decl_perm, {k: hex(v) for k, v in off.items()}, r["sections"], "== JANUARY" if ok else "differs")
print("names used (A..G):", names)
