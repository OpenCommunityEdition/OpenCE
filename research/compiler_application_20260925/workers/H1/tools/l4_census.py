"""H1 L4: January order vs Sapien (VC7.1) order of every object's .bss owners that crossmap locates uniquely,
classified by OUR source's storage form (static/external, initialised/uninitialised). Names need not be authentic.
Writes scratch/campaign/workers/H1/L4_census.json."""
import glob, json, os, re
import crossmap, exeindex, janfn

ROOT = r"C:\halo-worktrees\claude-compiler-application-20260925"
H1 = os.path.join(ROOT, "scratch", "campaign", "workers", "H1")
sidx = exeindex.load(os.path.join(H1, "sapien_index.json"))


def form(src, name):
    """(linkage, init) from our source's file-scope definition of name, or None."""
    for m in re.finditer(r"^([^\n;{}()]*?)\b" + re.escape(name) + r"\b\s*(\[[^\]\n]*\]\s*)*(=[^;]*)?;", src, re.M):
        head = m.group(1)
        if head.strip().startswith(("extern", "typedef", "return", "#")) or "(" in head:
            continue
        if not re.match(r"^\s*(static\s+)?(const\s+)?(unsigned\s+|signed\s+)?(struct\s+|union\s+|enum\s+)?\w+[\s\*]+(const\s+)?$", head):
            continue
        linkage = "S" if head.lstrip().startswith("static") else "E"
        init = m.group(3)
        if init is None:
            kind = "U"
        else:
            v = init[1:].strip().replace(" ", "")
            kind = "Z" if v in ("0", "NULL", "{0}", "FALSE", "0L", "{NULL}", "{FALSE}") else ("F" if re.match(r"^-?0?\.0*f?$|^0\.f$|^0\.0f$", v) else "I")
        return linkage + kind
    return None


rows = []
for path in sorted(glob.glob(os.path.join(ROOT, "build", "split", "source", "**", "*.obj"), recursive=True)):
    try:
        o, jf, dsyms, secname = janfn.facts(path)
    except Exception:
        continue
    bss = sorted((v, n) for s, lst in dsyms.items() if secname[s].startswith(".bss") for v, n in lst)
    bss = [(v, n) for v, n in bss if n.startswith("_") and not n.startswith("_bss_")]
    if len(bss) < 2:
        continue
    fmap, res, _, _ = crossmap.run(path, {n for v, n in bss}, verbose=False, idx=sidx)
    sap = {n: res[n][0] for v, n in bss if n in res and len(res[n]) == 1}
    # keep only owners with distinct Sapien addresses
    if len(set(sap.values())) != len(sap) or len(sap) < 2:
        continue
    srcp = os.path.join(ROOT, os.path.relpath(path, os.path.join(ROOT, "build", "split")))[:-4] + ".c"
    if not os.path.exists(srcp):
        continue
    src = open(srcp, encoding="latin-1").read()
    jan = [n for v, n in bss if n in sap]
    forms = {n: form(src, n[1:]) for n in jan}
    sap_order = sorted(jan, key=lambda n: sap[n])
    row = dict(obj=os.path.relpath(path, ROOT), jan=jan, sapien=sap_order, forms=forms,
               sapien_addr={n: hex(sap[n]) for n in jan}, same=jan == sap_order)
    rows.append(row)
    print("%-52s n=%d same=%-5s forms=%s" % (row["obj"][19:], len(jan), row["same"], "".join((forms[n] or "??") + " " for n in jan)))
json.dump(rows, open(os.path.join(H1, "L4_census.json"), "w"), indent=1)
