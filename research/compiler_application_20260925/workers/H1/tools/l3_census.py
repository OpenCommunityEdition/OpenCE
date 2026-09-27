"""H1 L3: for every January split object, the .bss statics whose names are AUTHENTIC (HCEX.pdb SymTagData), their
January order, their VC7 uninitialised-bucket (name-hash) order (lab compile, neutral long type) and their order
in the HEK Sapien build (VC7.1), located by crossmap. Writes scratch/campaign/workers/H1/L3_census.json."""
import glob, json, os, re, sys
import bsslab, crossmap, exeindex, janfn

ROOT = r"C:\halo-worktrees\claude-compiler-application-20260925"
H1 = os.path.join(ROOT, "scratch", "campaign", "workers", "H1")
auth = set()
for line in open(os.path.join(H1, "hcex_globals.txt"), encoding="latin-1"):
    m = re.match(r"Data: \[[0-9A-F]+\]\[[0-9A-F]+:[0-9A-F]+\] (\S+)$", line.strip())
    if m:
        auth.add(m.group(1))
sidx = exeindex.load(os.path.join(H1, "sapien_index.json"))
rows = []
for path in sorted(glob.glob(os.path.join(ROOT, "build", "split", "source", "**", "*.obj"), recursive=True)):
    try:
        o, jf, dsyms, secname = janfn.facts(path)
    except Exception as e:
        continue
    bss = [(v, n) for s, lst in dsyms.items() if secname[s].startswith(".bss") for v, n in lst]
    names = [(v, n) for v, n in bss if n.startswith("_") and n[1:] in auth]
    if len(names) < 2:
        continue
    fmap, res, _, _ = crossmap.run(path, {n for v, n in names}, verbose=False, idx=sidx)
    sap = {n: res[n][0] for v, n in names if n in res and len(res[n]) == 1}
    if len(sap) < 2:
        continue
    jan_order = [n for v, n in sorted(names) if n in sap]
    tag = "L3h_" + re.sub(r"[^A-Za-z0-9]", "_", os.path.relpath(path, os.path.join(ROOT, "build", "split")))[:60]
    r = bsslab.compile_tu([(n[1:], "long {n}", None) for n in jan_order], tag)
    hash_order = ["_" + n for n in r["order"]]
    sap_order = sorted(jan_order, key=lambda n: sap[n])
    row = dict(obj=os.path.relpath(path, ROOT), jan=jan_order, hash=hash_order, sapien=sap_order,
               sapien_addr={n: hex(sap[n]) for n in jan_order},
               jan_is_hash=jan_order == hash_order, sapien_is_hash=sap_order == hash_order,
               sapien_is_jan=sap_order == jan_order)
    rows.append(row)
    print("%-55s n=%d jan==hash %-5s sapien==hash %-5s sapien==jan %-5s" % (row["obj"][13:], len(jan_order),
          row["jan_is_hash"], row["sapien_is_hash"], row["sapien_is_jan"]))
json.dump(rows, open(os.path.join(H1, "L3_census.json"), "w"), indent=1)
