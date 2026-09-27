"""READ-ONLY. Scan January's split objects and our rebuilt objects for every pooled record.

Writes data/pool_inventory.json, data/jan_refs.json, data/our_objects.json.

January side (snapshot split/):
  * every relocation whose target symbol names a pooled record, per object, per
    containing function (the defined symbol at or below the relocation offset);
  * the storage form of every symbol-table entry for a pooled name (a split object
    can only show UNDEF/COMMON/defined - csplit decides that, not the original TU);
  * the object's January module (basename match against the cachebeta.pdb module list
    AND the contribs.json module of its defined symbols; disagreements are flagged).
Our side (snapshot base/): COMMON (sec 0, value>0), defined (section + storage) or
UNDEF (sec 0, value 0) for every pooled name.
"""
import json
import os
import sys
from collections import Counter, defaultdict
from pathlib import Path

sys.path.insert(0, os.path.dirname(__file__))
import c1_common as C  # noqa: E402


def module_maps():
    mods = C.modules()
    by_base = defaultdict(list)
    for i, n in mods.items():
        by_base[n.replace("\\", "/").split("/")[-1].lower()].append(i)
    # contribution intervals for code/data: rva -> module
    cons = sorted((c["file_offset"], c["size"], c["module_index"]) for c in C.contribs())
    return mods, by_base, cons


def module_of_rva(cons, rva):
    import bisect
    i = bisect.bisect_right(cons, (rva, 1 << 40, 1 << 40)) - 1
    if i >= 0:
        off, size, m = cons[i]
        if off <= rva < off + max(size, 1):
            return m
    return None


def main():
    C.DATA.mkdir(parents=True, exist_ok=True)
    recs, base = C.pool_records()
    pooled = {r["name"] for r in recs}
    mods, by_base, cons = module_maps()
    symrva = defaultdict(list)
    for e in C.symbols_json():
        symrva[e["name"]].append(e["file_offset"])

    # ---------------- January split objects ----------------
    jan_refs = defaultdict(lambda: defaultdict(lambda: {"count": 0, "functions": Counter(), "sections": Counter()}))
    jan_symforms = defaultdict(list)
    obj_module = {}
    common_style = []
    suspects = []
    split_root = C.SNAP / "split"
    for p in C.iter_objs(split_root):
        rel = p.relative_to(split_root).as_posix()
        if rel == "source/linker_common.obj":
            continue
        o = C.load_coff(p)
        if o is None:
            continue
        # module attribution
        bm = by_base.get(p.name.lower(), [])
        cm = Counter()
        for sy in o.symbols:
            if sy["sec"] > 0 and not sy["name"].startswith(".") and sy["storage"] in (2, 3):
                for rva in symrva.get(sy["name"], [])[:1]:
                    m = module_of_rva(cons, rva)
                    if m is not None:
                        cm[m] += 1
        cmod = cm.most_common(1)[0][0] if cm else None
        obj_module[rel] = {"basename_modules": bm, "contrib_module": cmod,
                           "contrib_votes": dict(cm.most_common(3)),
                           "agree": (cmod in bm) if (bm and cmod is not None) else None}
        # symbol forms for pooled names
        idx = {sy["index"]: sy for sy in o.symbols}
        for sy in o.symbols:
            if sy["sec"] == 0 and sy["value"] > 0 and sy["storage"] == 2:
                common_style.append((rel, sy["name"], sy["value"]))
            if sy["name"] in pooled:
                jan_symforms[sy["name"]].append({"object": rel, "sec": sy["sec"], "value": sy["value"],
                                                 "storage": sy["storage"]})
        # relocations
        secsyms = defaultdict(list)
        for sy in o.symbols:
            if sy["sec"] > 0 and not sy["name"].startswith(".") and not sy["name"].startswith("$"):
                secsyms[sy["sec"]].append((sy["value"], sy["name"]))
        for k in secsyms:
            secsyms[k].sort()
        for s in o.sections:
            for va, symidx, typ in s["relocs"]:
                t = idx.get(symidx)
                if t is None or t["name"] not in pooled:
                    continue
                if typ != 6:
                    # only DIR32 can be a genuine data reference; the 3 REL32 hits (libcmt fflush/getws/
                    # putws -> _ai_debug) are csplit address attributions, kept aside as suspects
                    suspects.append({"object": rel, "symbol": t["name"], "type": typ, "offset": va,
                                     "section": s["name"]})
                    continue
                fn = None
                for v, n in secsyms.get(s["index"], []):
                    if v <= va:
                        fn = n
                    else:
                        break
                e = jan_refs[t["name"]][rel]
                e["count"] += 1
                e["functions"][fn or "?"] += 1
                e["sections"][s["name"]] += 1
    jan_out = {}
    for name, per in jan_refs.items():
        jan_out[name] = {obj: {"count": e["count"], "functions": dict(e["functions"]),
                               "sections": dict(e["sections"])} for obj, e in sorted(per.items())}

    # ---------------- our rebuilt objects ----------------
    ours = defaultdict(list)
    base_root = C.SNAP / "base"
    for p in C.iter_objs(base_root):
        rel = p.relative_to(base_root).as_posix()
        o = C.load_coff(p)
        if o is None:
            continue
        secname = {s["index"]: s for s in o.sections}
        for sy in o.symbols:
            if sy["name"] not in pooled:
                continue
            if sy["sec"] == 0 and sy["value"] > 0:
                form = "COMMON"
                sec = None
            elif sy["sec"] == 0:
                form = "UNDEF"
                sec = None
            elif sy["sec"] > 0:
                s = secname[sy["sec"]]
                sec = s["name"]
                form = "DEFINED"
            else:
                form = "OTHER"
                sec = None
            ent = {"object": rel, "form": form, "value": sy["value"], "storage": sy["storage"]}
            if form == "DEFINED":
                s = secname[sy["sec"]]
                ent.update(section=sec, section_size=s["size"], section_chars=s["chars"],
                           uninit=bool(s["chars"] & 0x80))
            ours[sy["name"]].append(ent)

    json.dump({"base_rva": base, "records": recs}, open(C.DATA / "pool_inventory.json", "w"), indent=1)
    json.dump({"refs": jan_out, "symforms": jan_symforms, "obj_module": obj_module,
               "common_style_symbols_in_split": common_style,
               "suspect_non_dir32_references": suspects},
              open(C.DATA / "jan_refs.json", "w"), indent=1)
    json.dump(ours, open(C.DATA / "our_objects.json", "w"), indent=1)

    # summary
    print("pool records", len(recs), "base rva", hex(base))
    print("split objects with a COMMON-style (sec 0, value>0) external symbol:", len(common_style))
    print("pooled names referenced by >=1 January object:", len(jan_out))
    forms = Counter()
    for r in recs:
        fs = {e["form"] for e in ours.get(r["name"], [])}
        forms[tuple(sorted(fs))] += 1
    print("our-tree form sets:", dict(forms))
    dis = [k for k, v in obj_module.items() if v["agree"] is False]
    print("split objects whose basename module != contrib module:", len(dis), dis[:10])


if __name__ == "__main__":
    main()
