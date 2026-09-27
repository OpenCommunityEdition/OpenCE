"""READ-ONLY. Render the per-record summary table (data/table.md) from common_evidence.json."""
import json
import os
import sys

sys.path.insert(0, os.path.dirname(__file__))
import c1_common as C  # noqa: E402


def short(s, n):
    return s if len(s) <= n else s[:n - 1] + "~"


def main():
    d = json.load(open(C.OUT / "common_evidence.json"))
    lines = ["| # | pool off | symbol | size | our tree | decl ext/tent | declaring header(s) | Jan refs obj/rel |"
             " HCEX 2011 | pool-order candidate (H_order+H_ref, NOT evidence) | verdict | established owner / classes |",
             "|---:|---:|---|---:|---|---|---|---|---|---|---|---|"]
    for p in d["packets"]:
        ot = p["our_tree"]
        emit = ot["emits"]
        if emit == "COMMON":
            defs = ",".join(x["object"].split("/")[-1].replace(".obj", "") for x in ot["common_definers"])
            emit = "COMMON %s%s" % (short(defs, 40), "" if ot["size_equal"] else " (SIZE %d)" % ot["common_definers"][0]["size"])
        ne = sum(1 for x in p["declarations"] if x["kind"] == "extern")
        nt = sum(1 for x in p["declarations"] if x["kind"] == "tentative")
        hd = ", ".join(h.split("/")[-1] for h in p["declaring_headers"]) or "-"
        jr = p["january_reference_totals"]
        hx = p.get("hcex_2011", {})
        if hx.get("found") is True:
            st = hx["storage"]
            st = ("pooled" if st.startswith("pooled") else "discarded" if st.startswith("discarded") else
                  "DEF " + st.split("\\")[-1] if st.startswith("defined") else st)
            hcx = "%s; %s" % (short(hx["type"], 34), st)
        elif hx.get("found") is False:
            hcx = "absent"
        else:
            hcx = "-"
        nc = p["module_cluster_negative_constraint"]
        if "order_feasible_referencer_modules" in nc:
            fe = nc["order_feasible_referencer_modules"]
            if nc["no_january_referencer"]:
                oc = "no referencer; interval %s" % nc["min_violation_module_interval"]
            elif nc["h_ref_fails_in_every_min_violation_assignment"]:
                oc = "H_ref FAILS; interval %s" % nc["min_violation_module_interval"]
            else:
                oc = ", ".join("%d %s" % (m, n.split("\\")[-1].replace(".obj", "")) for m, n in fe)
        else:
            oc = "n/a (vendor/linker)"
        owner = p["established_owner"] or ""
        cls = "+".join(p["establishing_classes"])
        vo = ("%s [%s]" % (short(owner, 44), cls)) if owner else (cls or "-")
        lines.append("| %d | %s | `%s` | %d | %s | %d/%d | %s | %d/%d | %s | %s | **%s** | %s |" % (
            p["pool_index"], "-" if p["pool_offset"] is None else p["pool_offset"], short(p["symbol"], 46),
            p["size"], emit, ne, nt, short(hd, 40), jr["objects"], jr["relocations"], hcx, short(oc, 60),
            p["verdict"], vo))
    open(C.DATA / "table.md", "w").write("\n".join(lines) + "\n")
    print("rows", len(lines) - 2)


if __name__ == "__main__":
    main()
