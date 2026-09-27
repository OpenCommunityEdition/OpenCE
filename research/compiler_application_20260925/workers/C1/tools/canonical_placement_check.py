"""READ-ONLY, HYPOTHESIS-CONDITIONAL. Check our tree's existing COMMON definers (the 67 records
canonical already defines tentatively) against the pool-order negative constraint.

For each record our tree emits as COMMON, take the defining object's January module (basename
match) and ask: is it inside the min-violation module interval computed by order_constraint.py
(H_order + H_ref)?  LAB1 P3: a record sits in the run of its highest-module definer, so a definer
ABOVE the interval is excluded and one BELOW can only be an additional, shadowed definer.  Under H_order ALONE (no H_ref), a definer module can be excluded only if it is
outside the interval spanned by the neighbouring records' definers - which needs anchors we do not
have - so this check is reported as H_order+H_ref-conditional.  Writes data/canonical_placement_check.json.
"""
import json, os, sys
sys.path.insert(0, os.path.dirname(__file__))
import c1_common as C  # noqa: E402


def main():
    pk = {p["symbol"]: p for p in json.load(open(C.OUT / "common_evidence.json"))["packets"]}
    mods = C.modules()
    base = {}
    for i, m in mods.items():
        base.setdefault(m.replace("\\", "/").split("/")[-1].lower(), []).append(i)
    rows = []
    for n, p in pk.items():
        for d in p["our_tree"]["common_definers"]:
            b = d["object"].split("/")[-1].lower()
            ms = base.get(b, [])
            m = ms[0] if len(ms) == 1 else None
            nc = p["module_cluster_negative_constraint"]
            iv = nc.get("min_violation_module_interval")
            feas = [x[0] for x in nc.get("order_feasible_referencer_modules", [])]
            rows.append({"symbol": n, "pool_index": p["pool_index"], "our_definer": d["object"],
                         "module": m, "interval": iv,
                         "inside_interval": (iv[0] <= m <= iv[1]) if (iv and m is not None) else None,
                         # LAB1 P3: a record sits in the run of its HIGHEST-module definer, so a definer
                         # above the interval is excluded; one below can only be an extra, shadowed definer
                         "h_order_status": (None if (iv is None or m is None) else
                                            "consistent" if iv[0] <= m <= iv[1] else
                                            "EXCLUDED (module above the record's cluster)" if m > iv[1] else
                                            "shadowed-only (module below the cluster; cannot be the sole definer)"),
                         "is_order_feasible_referencer": (m in feas) if m is not None else None,
                         "january_split_form_in_that_object":
                             ("UNDEF" if any(r["object"] == d["object"] for r in p["january_references"])
                              else "no reference")})
    json.dump(rows, open(C.DATA / "canonical_placement_check.json", "w"), indent=1)
    out = [r for r in rows if r["inside_interval"] is False]
    print("our COMMON definers:", len(rows), "| outside H_order+H_ref interval:", len(out))
    for r in out:
        print("  ", r["symbol"], r["our_definer"], "module", r["module"], "interval", r["interval"],
              r["h_order_status"], "| Jan:", r["january_split_form_in_that_object"])
    print("definers whose January object shows UNDEF:", sum(r["january_split_form_in_that_object"] == "UNDEF" for r in rows))


if __name__ == "__main__":
    main()
