"""READ-ONLY, HYPOTHESIS-CONDITIONAL. Module-cluster position as a NEGATIVE constraint only.

Hypotheses (neither is proven; the owner ruled pool order can constrain, never place):
  H_order  for plain command-line objects, VC7 Link pools COMMON records in ascending PDB
           module index of the FIRST-PROCESSED (= highest-module-index) tentative definer
           (LAB1: 2 links, 7 symbols; LAB2 shows library members follow a DIFFERENT rule,
           so the 27-record vendor tail is excluded from this analysis).
  H_ref    that definer is one of the January objects that reference the record.
Under both, the definer module sequence is non-decreasing along the pool.  For every
Halo record (pool 3..215) this computes the referencer modules that are ORDER-INFEASIBLE
(excluded) and the feasible remainder, with records that have no January referencer
treated as wildcards.  Where no monotone assignment exists, the pass reports the
conflict instead of forcing one (a direct measurement that H_order+H_ref fail there).

Writes data/order_constraint.json.  Nothing here can yield PLACED.
"""
import json
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
import c1_common as C  # noqa: E402


def main():
    inv = json.load(open(C.DATA / "pool_inventory.json"))["records"]
    jr = json.load(open(C.DATA / "jan_refs.json"))
    refs, objmod = jr["refs"], jr["obj_module"]
    mods = C.modules()

    def module_of(obj):
        m = objmod.get(obj, {})
        bm = m.get("basename_modules") or []
        if len(bm) == 1:
            return bm[0]
        return m.get("contrib_module")

    halo = [r for r in inv if r["section"] == ".bss" and 3 <= r["pool_index"] <= 215]
    cand = []
    for r in halo:
        ms = sorted({module_of(o) for o in refs.get(r["name"], {}) if module_of(o) is not None})
        cand.append(ms)
    n = len(halo)
    V = 848                                   # every cachebeta module index is a possible value
    INF = 1 << 20
    def cost(i, v):
        return 0 if (not cand[i] or v in cand[i]) else 1
    # F[i][v]: min violations over records 0..i with record i's definer module = v (monotone)
    F = []
    prev = None
    for i in range(n):
        row = [0] * V
        run = INF
        for v in range(V):
            if prev is not None:
                run = min(run, prev[v])
                base = run
            else:
                base = 0
            row[v] = base + cost(i, v)
        F.append(row)
        prev = row
    B = [None] * n
    nxt = None
    for i in range(n - 1, -1, -1):
        row = [0] * V
        run = INF
        for v in range(V - 1, -1, -1):
            if nxt is not None:
                run = min(run, nxt[v])
                base = run
            else:
                base = 0
            row[v] = base + cost(i, v)
        B[i] = row
        nxt = row
    K = min(F[-1])
    out = []
    for i, r in enumerate(halo):
        tot = [F[i][v] + B[i][v] - cost(i, v) for v in range(V)]
        opt = {v for v in range(V) if tot[v] == K}
        ms = cand[i]
        feas = [c for c in ms if c in opt]
        excl = [c for c in ms if c not in opt]
        lo_v, hi_v = min(opt), max(opt)
        out.append({
            "name": r["name"], "pool_index": r["pool_index"],
            "prev": halo[i - 1]["name"] if i > 0 else None,
            "next": halo[i + 1]["name"] if i + 1 < n else None,
            "referencer_modules": [[c, mods.get(c)] for c in ms],
            "optimal_module_interval": [lo_v, hi_v],
            "order_feasible_referencer_modules": [[c, mods.get(c)] for c in feas],
            "order_excluded_referencer_modules": [[c, mods.get(c)] for c in excl],
            "wildcard_no_referencer": not ms,
            "h_ref_violated_in_every_optimum": bool(ms) and not feas,
        })
    conflicts_f = [i for i, o in enumerate(out) if o["h_ref_violated_in_every_optimum"]]
    conflicts_b = []
    json.dump({"hypotheses": ["H_order (LAB1, plain objects only)", "H_ref (unproven)"],
               "method": "min-violation monotone assignment over module indexes 0..847; a violation = a record "
                         "whose assigned definer module is not one of its January referencer modules",
               "minimum_violations": K,
               "h_ref_violated_in_every_optimum": [halo[i]["name"] for i in conflicts_f],
               "records": out}, open(C.DATA / "order_constraint.json", "w"), indent=1)
    single = sum(1 for o in out if len(o["order_feasible_referencer_modules"]) == 1)
    empty = sum(1 for o in out if not o["wildcard_no_referencer"] and not o["order_feasible_referencer_modules"])
    print("halo records", n, "wildcards", sum(o["wildcard_no_referencer"] for o in out))
    print("minimum violations K* =", K)
    print("records whose every optimum puts the definer OUTSIDE its referencers:", len(conflicts_f),
          [halo[i]["name"] for i in conflicts_f][:30])
    print("records with exactly one order-feasible referencer module:", single, "| with none:", empty)
    print("records with >=1 excluded referencer module:", sum(1 for o in out if o["order_excluded_referencer_modules"]))


if __name__ == "__main__":
    main()
