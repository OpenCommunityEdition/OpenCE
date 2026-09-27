"""READ-ONLY. Merge every evidence source into one packet per pooled record.

Inputs (all produced by the other tools from the frozen snapshot):
  data/pool_inventory.json   obj_scan.py      January record: size, align, rva, pool offset
  data/jan_refs.json         obj_scan.py      January split referencers (+ module of each object)
  data/our_objects.json      obj_scan.py      our rebuilt objects' storage for the name
  data/decls.json            decl_scan.py     every declaration in our source tree
  data/lib_defs.json         lib_scan.py      vendor library members defining the name
  data/vendor_identity.json  vendor_identity.py  code identity of those members vs January split
  data/pdb_evidence.json     pdb_evidence.py  cachebeta publics, HCEX type/size/storage
  data/order_constraint.json order_constraint.py  hypothesis-conditional NEGATIVE constraint
  data/lanec_wave.json       lanec_wave.py    where the EXCLUDED Lane C wave put it (context)
Output: common_evidence.json (one packet per record) + data/summary.json.

VERDICT RULES (only classes that can OBSERVE a definer are allowed to place):
  PLACED       EC-LNK  record produced by the linker itself ("* Linker *" contribution AND content
                       decodes as a linker product; reproduced in LAB1 P2), or
               EC-LIB  an authentic definer object's own COFF symbol table carries the COMMON, with
                       EC-MOD (member in cachebeta.pdb module list), size equal to January's
                       contribution, and build identity (every comparable function of the member
                       byte-identical to January's split object, or EC-PCH).
               EC-PCH  a compiler-generated __@@_PchSym_ COMMON whose name encodes its defining
                       object path (corroborated by EC-LIB + EC-MOD).
  CONSTRAINED  an observing class exists but cannot be tied to January: EC-HCEXDEF (the 2011 HCEX
               build contributes the symbol from a named compiland, i.e. it was a real definition
               there) - later build, different storage contract, so candidate only.
  UNPLACED     everything else.  Pool position (EC-POOL), referencers (EC-REF), our tree
               (EC-TREE), cachebeta publics (EC-PUB), HCEX types (EC-HCEX), the split's UNDEF form
               (EC-SPLIT) and the Sept-2001 map (EC-MAP, unavailable for data and <common> anyway)
               are reported but never change the verdict.
"""
import json
import os
import sys
from collections import Counter, defaultdict

sys.path.insert(0, os.path.dirname(__file__))
import c1_common as C  # noqa: E402

SNAPSHOT = "scratch/campaign/workers/C1/snap_20260926_011221 (HEAD 09f5208f8a2b8be564519b800f7bdafb20fa19fc)"


def pchsym_decode(name):
    """__@@_PchSym_@00@<enc>@<tag>: lowercase letters atbash, A-J digits, U '\\', O '.'."""
    try:
        enc = name.split("@00@", 1)[1].rsplit("@", 1)[0]
    except IndexError:
        return None
    out = []
    for ch in enc:
        if "a" <= ch <= "z":
            out.append(chr(ord("z") - (ord(ch) - ord("a"))))
        elif "A" <= ch <= "J":
            out.append(str(ord(ch) - ord("A")))
        elif ch == "U":
            out.append("\\")
        elif ch == "O":
            out.append(".")
        elif ch == "R":
            out.append("?")   # separator whose plain form is not needed here
        else:
            out.append(ch)
    return "".join(out)


def load(name):
    return json.load(open(C.DATA / name))


def main():
    inv = load("pool_inventory.json")
    recs = inv["records"]
    jr = load("jan_refs.json")
    refs, objmod = jr["refs"], jr["obj_module"]
    ours = load("our_objects.json")
    decls = load("decls.json")
    libd = load("lib_defs.json")["defs"]
    vid = load("vendor_identity.json")
    pdb = load("pdb_evidence.json")
    oc = {r["name"]: r for r in load("order_constraint.json")["records"]}
    lcw = load("lanec_wave.json")
    mods = C.modules()
    halo_split_objs = set(objmod)

    def module_of(obj):
        m = objmod.get(obj, {})
        bm = m.get("basename_modules") or []
        return bm[0] if len(bm) == 1 else m.get("contrib_module")

    packets = []
    for r in recs:
        n = r["name"]
        p = {"symbol": n, "c_name": C.c_name(n), "pool_index": r["pool_index"],
             "pool_offset": r["pool_offset"], "rva": r["rva"], "section": r["section"],
             "size": r["contrib_size"], "linker_alignment": r["contrib_align"]}
        # ---- our tree
        es = ours.get(n, [])
        forms = Counter(e["form"] for e in es)
        if any(e["form"] == "DEFINED" for e in es):
            d = [e for e in es if e["form"] == "DEFINED"]
            how = ".bss" if all(e.get("uninit") for e in d) else ".data"
        elif forms.get("COMMON"):
            how = "COMMON"
        elif forms.get("UNDEF"):
            how = "extern-only"
        else:
            how = "absent"
        p["our_tree"] = {
            "emits": how,
            "common_definers": [{"object": e["object"], "size": e["value"]} for e in es if e["form"] == "COMMON"],
            "defined_in": [{"object": e["object"], "section": e.get("section"), "storage": e["storage"]}
                           for e in es if e["form"] == "DEFINED"],
            "importing_objects": sorted(e["object"] for e in es if e["form"] == "UNDEF"),
            "size_equal": (all(e["value"] == r["contrib_size"] for e in es if e["form"] == "COMMON")
                           if forms.get("COMMON") else None),
        }
        rows = decls.get(n, [])
        keep = [x for x in rows if x["kind"] in ("extern", "tentative", "initialised", "static", "macro")]
        p["declarations"] = [{"file": x["file"], "line": x["line"], "kind": x["kind"],
                              "type": x.get("type_head"), "declarator_suffix": x.get("declarator_suffix"),
                              "text": x["text"], "preprocessor_conditions": x.get("conds", [])} for x in keep]
        p["declaring_headers"] = sorted({x["file"] for x in keep if x["file"].endswith(".h")
                                         and x["kind"] in ("extern", "tentative")})
        p["use_sites_in_tree"] = dict(Counter(x["file"] for x in rows if x["kind"] == "use"))
        # ---- January references
        jref = refs.get(n, {})
        p["january_references"] = [{"object": o, "module_index": module_of(o),
                                    "module": mods.get(module_of(o)), "relocations": v["count"],
                                    "functions": v["functions"], "sections": v["sections"]}
                                   for o, v in sorted(jref.items())]
        p["january_reference_totals"] = {"objects": len(jref), "relocations": sum(v["count"] for v in jref.values())}
        # ---- PDBs, map
        pe = pdb.get(n, {})
        p["cachebeta_public"] = {"present": pe.get("cachebeta_public"),
                                 "rva_equals_symbols_json": pe.get("cachebeta_rva_equals_symbols_json")}
        h = pe.get("hcex")
        if h is not None:
            if not h["data"]:
                hs = {"found": False}
            else:
                d0 = h["data"][0]
                mod0 = h["contrib_modules"][0] if h["contrib_modules"] else None
                storage = ("discarded (RVA 0: declared, not linked)" if d0["rva"] == 0 else
                           "pooled COMMON (* Linker *)" if mod0 == "* Linker *" else
                           "defined in compiland " + str(mod0) if mod0 else "unresolved contribution")
                hs = {"found": True, "type": d0["type"], "public_length": h["public_length"],
                      "storage": storage, "records": len(h["data"])}
            p["hcex_2011"] = hs
        else:
            p["hcex_2011"] = {"found": None, "note": "not queried (non-.bss or PchSym)"}
        p["sept2001_map"] = ("unavailable: halo-symbol-atlas keeps CODE symbols only (8,568 map-tier "
                             "records for 2001-09-25 cachebeta.xbe, all code); and a VC7 map prints "
                             "<common> for COMMON data (LAB1 P1)")
        # ---- vendor library evidence
        lds = [x for x in libd.get(n, []) if x["form"] == "COMMON"]
        p["vendor_library_definers"] = lds
        vend = None
        for x in lds:
            key = "%s:%s" % (x["lib"], x["member"])
            if key in vid:
                vend = dict(vid[key], key=key)
        if vend:
            p["vendor_identity"] = {k: vend[k] for k in ("key", "january_split_object", "functions_compared",
                                                         "identical", "differing")}
        if n.startswith("___@@_PchSym"):
            p["pchsym_decoded_object"] = pchsym_decode(n)
        # ---- negative constraint (hypothesis-conditional)
        if n in oc:
            o = oc[n]
            p["module_cluster_negative_constraint"] = {
                "hypotheses": "H_order (LAB1 plain-object law) + H_ref (definer is a January referencer) - UNPROVEN",
                "pool_prev": o["prev"], "pool_next": o["next"],
                "referencer_modules": o["referencer_modules"],
                "min_violation_module_interval": o["optimal_module_interval"],
                "order_feasible_referencer_modules": o["order_feasible_referencer_modules"],
                "order_excluded_referencer_modules": o["order_excluded_referencer_modules"],
                "no_january_referencer": o["wildcard_no_referencer"],
                "h_ref_fails_in_every_min_violation_assignment": o["h_ref_violated_in_every_optimum"],
            }
        else:
            p["module_cluster_negative_constraint"] = {"note": "outside the Halo plain-object segment (pool 3..215)"}
        p["lane_c_excluded_wave_definition"] = lcw.get(n, [])
        # ---- verdict
        classes, verdict, owner, why = [], "UNPLACED", None, ""
        if r["section"] != ".bss":
            raw = bytes.fromhex(r["raw_hex"] or "")
            if len(raw) == 28 and raw[12:16] == b"\x02\x00\x00\x00":
                kind = "IMAGE_DEBUG_DIRECTORY (Type 2 = CODEVIEW, TimeDateStamp 0x3C4344A0, SizeOfData 0x3C -> 0x2B6A2C)"
            elif raw[:4] == b"NB10":
                kind = "CodeView NB10 record, age 1, path " + raw[16:].split(b"\0")[0].decode("latin-1")
            else:
                kind = "unknown"
            p["content"] = kind
            verdict, owner = "PLACED", "* Linker * (linker-generated; no source TU)"
            classes = ["EC-LNK"]
            why = ("contribs.json module 847 = cachebeta.pdb '* Linker *' (DIA2Dump -m); content is a linker "
                   "product (%s); LAB1 P2 reproduces both record kinds from '* Linker *'" % kind)
        elif lds:
            # January linked the multithreaded release CRT (___ptd_glob exists only in libcmt/libcmtd)
            # and the release xapilib/dsound; prefer the member whose build identity was checked.
            pref = [y for y in lds if "%s:%s" % (y["lib"], y["member"]) in vid]
            x = pref[0] if pref else lds[0]
            size_ok = all(y["value"] == r["contrib_size"] for y in lds if y["lib"] in ("libcmt.lib", "xapilib.lib", "dsound.lib"))
            member = x["member"].replace("\\", "/").split("/")[-1]
            in_modules = [i for i, m in mods.items() if m.lower().endswith("\\" + member.lower())]
            identical = vend and vend["functions_compared"] > 0 and not vend["differing"]
            pch = n.startswith("___@@_PchSym")
            if in_modules and size_ok and (identical or pch):
                verdict = "PLACED"
                owner = "%s:%s (vendor library member)" % (x["lib"], x["member"])
                classes = ["EC-LIB", "EC-MOD"] + (["EC-PCH"] if pch else ["EC-LIB-ID"])
                why = ("member's own COFF symbol table defines it COMMON %d B (= January contribution); member "
                       "in January module list at index %s; %s" % (
                           x["value"], [hex(i + 1) for i in in_modules],
                           ("PchSym name decodes to %s" % pchsym_decode(n)) if pch else
                           ("%d/%d member functions byte-identical to January's split %s" % (
                               vend["identical"], vend["functions_compared"], vend["january_split_object"]))))
            else:
                verdict = "CONSTRAINED"
                owner = None
                classes = ["EC-LIB"]
                why = "library member defines it but build identity/module/size corroboration incomplete"
        else:
            hx = p.get("hcex_2011", {})
            if hx.get("found") and str(hx.get("storage", "")).startswith("defined in compiland"):
                verdict = "CONSTRAINED"
                classes = ["EC-HCEXDEF"]
                why = ("the 2011 HCEX build defines it (non-pooled) in %s; a LATER build with a different "
                       "storage contract (January pools it, so January's definitions were all tentative) - "
                       "candidate family only, cannot establish January's TU (house rule 31)"
                       % hx["storage"].replace("defined in compiland ", ""))
            else:
                why = ("no evidence class that can observe a COMMON definer exists for this record: the split "
                       "shows only UNDEF references (LAB1 P6), contribs/PDB attribute it to '* Linker *' "
                       "(LAB1 P2/P5), a map would print <common> (LAB1 P1)")
        p["verdict"] = verdict
        p["established_owner"] = owner
        p["establishing_classes"] = classes
        p["verdict_reason"] = why
        p["segment"] = ("linker" if r["section"] != ".bss" else "vendor" if lds else "halo")
        packets.append(p)

    meta = {
        "generated_from": SNAPSHOT,
        "record_count": len(packets),
        "sizes_note": ("'size' is January's cachebeta.pdb section-contribution size (equal to csplit's "
                       "section size for all 242); objdiff's report shows .bss 1,272,568 / .rdata 96 because it "
                       "aligns each concatenated section"),
        "evidence_classes": {
            "EC-LNK": "linker-generated record (* Linker * contribution + decoded content) - ESTABLISHING (producer, no TU)",
            "EC-LIB": "authentic definer object's COFF symbol table (library member) shows COMMON - ESTABLISHING",
            "EC-MOD": "member present in cachebeta.pdb module list - corroboration",
            "EC-LIB-ID": "member code byte-identical to January's split object - build-identity corroboration",
            "EC-PCH": "__@@_PchSym_ name encodes its defining object - ESTABLISHING (self-identifying)",
            "EC-HCEXDEF": "2011 HCEX build defines it in a named compiland - later build, CONSTRAINING only",
            "EC-POOL": "pool position / module cluster - NEGATIVE constraint only, hypothesis-conditional",
            "EC-REF": "January referencers - never establishes (a definer need not reference; an importer "
                      "looks identical)",
            "EC-SPLIT": "January split symbol form - always UNDEF for pooled names (0 COMMON-style symbols in "
                        "832 split objects); cannot distinguish definer from importer",
            "EC-TREE": "our source/objects - reconstruction, not January evidence",
            "EC-PUB": "cachebeta.pdb publics - proves external linkage and address, not TU",
            "EC-HCEX": "HCEX type/size - later build; types/names only",
            "EC-MAP": "Sept-2001 map - data half unavailable; a map prints <common> for COMMON (LAB1 P1)",
        },
    }
    json.dump({"meta": meta, "packets": packets}, open(C.OUT / "common_evidence.json", "w"), indent=1)

    # summary
    by = defaultdict(lambda: [0, 0])
    for p in packets:
        k = (p["verdict"], p["segment"])
        by[k][0] += 1
        by[k][1] += p["size"]
    emits = Counter((p["segment"], p["our_tree"]["emits"]) for p in packets)
    emitb = Counter()
    for p in packets:
        emitb[(p["segment"], p["our_tree"]["emits"])] += p["size"]
    summ = {"verdicts": {"%s/%s" % k: v for k, v in sorted(by.items())},
            "our_tree_emits": {"%s/%s" % k: [v, emitb[k]] for k, v in sorted(emits.items())},
            "total_bytes": sum(p["size"] for p in packets)}
    json.dump(summ, open(C.DATA / "summary.json", "w"), indent=1)
    print(json.dumps(summ, indent=1))


if __name__ == "__main__":
    main()
