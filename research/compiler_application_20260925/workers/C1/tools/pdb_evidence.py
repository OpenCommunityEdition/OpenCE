"""READ-ONLY. PDB evidence per pooled record.

1. cachebeta.pdb PUBLIC membership (scratch/tools/cachebeta_publics.txt, DIA2Dump -p):
   present?  RVA equal to config/symbols.json file_offset?
2. HCEX.pdb (2011 Xbox 360 SHIP build, VC16; a LATER build - names/types only, never
   January layout or ownership): DIA2Dump -sym <name> -> the global's type string and its
   public Length; plus the HCEX section-contribution module covering its RVA (DIA2Dump -c),
   which says whether HCEX ALSO pooled it ("* Linker *") or placed it in a compiland.

Raw DIA outputs are cached under scratch/campaign/workers/C1/hcex_sym/.  Writes
data/pdb_evidence.json.
"""
import bisect
import json
import os
import re
import subprocess
import sys

sys.path.insert(0, os.path.dirname(__file__))
import c1_common as C  # noqa: E402

DIA = r"C:\Users\isabe\Documents\Codex\2026-07-13\i-w\research\tools\DIA2Dump\x64\Release\DIA2Dump.exe"
HCEX = (r"C:\Users\isabe\Documents\Codex\2026-07-13\i-w\research\hcea_jun2011_prototype\payload"
        r"\Halo CE Anniversary (Jun 24 2011)\HCEX.pdb")
PUBLICS = C.ROOT / "scratch/tools/cachebeta_publics.txt"
CACHE = C.ROOT / "scratch/campaign/workers/C1/hcex_sym"


def cachebeta_publics():
    pub = {}
    for line in open(PUBLICS, encoding="latin-1"):
        m = re.match(r"PublicSymbol: \[([0-9A-F]{8})\]\[([0-9A-F]{4}):([0-9A-F]{8})\] ([^(]+)", line)
        if m:
            pub.setdefault(m.group(4).strip(), []).append(int(m.group(1), 16))
    return pub


def hcex_contribs():
    p = CACHE / "_contribs.txt"
    if not p.exists():
        CACHE.mkdir(parents=True, exist_ok=True)
        p.write_bytes(subprocess.run([DIA, "-c", HCEX], capture_output=True).stdout)
    rows = []
    for line in open(p, encoding="latin-1"):
        m = re.match(r"\s+([0-9A-F]{8})\s+[0-9A-F]{4}:[0-9A-F]{8}\s+([0-9A-F]{8})\s+(.*)$", line)
        if m:
            rows.append((int(m.group(1), 16), int(m.group(2), 16), m.group(3).strip()))
    rows.sort()
    return rows


def hcex_sym(cname):
    CACHE.mkdir(parents=True, exist_ok=True)
    p = CACHE / (cname + ".txt")
    if not p.exists():
        p.write_bytes(subprocess.run([DIA, "-sym", cname, HCEX], capture_output=True).stdout)
    txt = open(p, encoding="latin-1").read()
    datas = re.findall(r"Data\s+: static, \[([0-9A-F]{8})\]\[[0-9A-F]{4}:[0-9A-F]{8}\], (\w+), Type: (.*), " +
                       re.escape(cname) + r"\s*$", txt, re.M)
    # public Length (first PublicSymbol block after the data record)
    length = None
    m = re.search(r"PublicSymbol: \[[0-9A-F]{8}\]\[[0-9A-F]{4}:[0-9A-F]{8}\] " + re.escape(cname) + r"\b", txt)
    blk = txt
    lm = re.findall(r"SymTag:\s+0xA\s*\n\s+Name:\s+" + re.escape(cname) + r"\s*\n(?:.*\n){0,12}?\s+Length:\s+0x([0-9A-F]+)", txt)
    if lm:
        length = int(lm[0], 16)
    return {"data": [{"rva": int(r, 16), "scope": s, "type": t.strip()} for r, s, t in datas],
            "public_length": length, "public": bool(m)}


def main():
    recs, _ = C.pool_records()
    pub = cachebeta_publics()
    con = hcex_contribs()
    out = {}
    for r in recs:
        n = r["name"]
        e = {"cachebeta_public": n in pub, "cachebeta_public_rvas": pub.get(n, []),
             "cachebeta_rva_equals_symbols_json": (r["rva"] in pub.get(n, [])) if r["rva"] is not None else None}
        if r["section"] == ".bss" and not n.startswith("___@@_PchSym"):
            h = hcex_sym(C.c_name(n))
            mods = []
            for d in h["data"]:
                i = bisect.bisect_right(con, (d["rva"], 1 << 40, "")) - 1
                if i >= 0 and con[i][0] <= d["rva"] < con[i][0] + max(con[i][1], 1):
                    mods.append(con[i][2])
                else:
                    mods.append(None)
            h["contrib_modules"] = mods
            e["hcex"] = h
        out[n] = e
    json.dump(out, open(C.DATA / "pdb_evidence.json", "w"), indent=1)
    from collections import Counter
    print("cachebeta public:", sum(v["cachebeta_public"] for v in out.values()), "/", len(out))
    print("rva equal:", Counter(v["cachebeta_rva_equals_symbols_json"] for v in out.values()))
    hx = [v["hcex"] for v in out.values() if "hcex" in v]
    print("HCEX data found:", sum(bool(h["data"]) for h in hx), "/", len(hx))
    print("HCEX contrib modules:", Counter((h["contrib_modules"][0] if h["contrib_modules"] else "-") if h["data"] else "absent" for h in hx).most_common(12))


if __name__ == "__main__":
    main()
