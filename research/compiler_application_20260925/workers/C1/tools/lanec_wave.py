"""READ-ONLY. Where the EXCLUDED Lane C wave (commit 7f2768d4, 2026-09-22) put each pooled
tentative definition.  Context only: canonical reconciliation excluded the wave because pool
adjacency cannot prove ownership.  Runs decl_scan's lexer over an archive of that commit
(scratch/campaign/workers/C1/lanec_7f2768d4) and writes data/lanec_wave.json."""
import json
import os
import sys
from collections import defaultdict

sys.path.insert(0, os.path.dirname(__file__))
import c1_common as C  # noqa: E402
import decl_scan as D  # noqa: E402

ROOTC = C.ROOT / "scratch/campaign/workers/C1/lanec_7f2768d4"


def main():
    recs, _ = C.pool_records()
    cn = {C.c_name(r["name"]): r["name"] for r in recs if r["section"] == ".bss"}
    out = defaultdict(list)
    for base in ("source", "libs"):
        for dp, dn, fn in os.walk(ROOTC / base):
            for f in fn:
                if os.path.splitext(f)[1].lower() not in D.EXTS:
                    continue
                p = os.path.join(dp, f)
                raw = open(p, encoding="latin-1").read()
                present = [n for n in cn if n in raw]
                if not present:
                    continue
                rel = os.path.relpath(p, ROOTC).replace("\\", "/")
                for row in D.scan_file(p, set(present)):
                    if row["kind"] in ("tentative", "initialised"):
                        out[cn[row["name"]]].append({"file": rel, "line": row["line"], "kind": row["kind"],
                                                     "text": row["text"][:160]})
    json.dump(out, open(C.DATA / "lanec_wave.json", "w"), indent=1)
    print("pooled names with a Lane-C-wave definition:", len(out))


if __name__ == "__main__":
    main()
