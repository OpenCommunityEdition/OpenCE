"""READ-ONLY. Corroborate that the local XDK-3911-era library members which define the
pooled vendor COMMONs are the SAME build January linked.

For each (lib, member) that lib_scan.py found defining a pooled record, extract the member
to scratch/campaign/workers/C1/libmembers/, then compare every function present in both
that member and January's split object of the same basename (build/split/libs/<lib>/):
raw bytes over the function extent with every relocation field of EITHER side masked
(4 bytes, DIR32 / REL32; csplit relocates intra-object calls the member pre-resolves).  Also compares the member's COMMON value with January's contribution size.

Writes data/vendor_identity.json.  Identity is corroboration of the library build, not a
proof on its own (a later XDK could carry an unchanged member).
"""
import json
import os
import sys
from pathlib import Path

sys.path.insert(0, os.path.dirname(__file__))
import c1_common as C  # noqa: E402
import lib_scan  # noqa: E402

SPLIT_DIR = {"libcmt.lib": "libcmt", "xapilib.lib": "xapilib", "dsound.lib": "dsound"}
EXTRACT = C.ROOT / "scratch/campaign/workers/C1/libmembers"


def fn_extents(o):
    out = {}
    bysec = {}
    for sy in o.symbols:
        if sy["sec"] > 0 and not sy["name"].startswith(".") and not sy["name"].startswith("$"):
            bysec.setdefault(sy["sec"], []).append(sy)
    for sec, syms in bysec.items():
        s = o.sections[sec - 1]
        if not (s["chars"] & 0x20):  # code only
            continue
        syms = sorted(syms, key=lambda x: x["value"])
        for i, sy in enumerate(syms):
            if sy["type"] != 0x20:
                continue
            end = s["size"]
            for t in syms[i + 1:]:
                if t["value"] > sy["value"]:
                    end = t["value"]
                    break
            data = bytes(s["data"][sy["value"]:end])
            relocs = [va - sy["value"] for va, _, typ in s["relocs"] if sy["value"] <= va < end]
            out[sy["name"]] = (data, relocs)
    return out


def main():
    lib_defs = json.load(open(C.DATA / "lib_defs.json"))["defs"]
    recs, _ = C.pool_records()
    size = {r["name"]: r["contrib_size"] for r in recs}
    wanted = {}
    for name, forms in lib_defs.items():
        for d in forms:
            if d["form"] == "COMMON" and d["lib"] in SPLIT_DIR:
                wanted.setdefault((d["lib"], d["member"]), []).append((name, d["value"]))
    EXTRACT.mkdir(parents=True, exist_ok=True)
    result = {}
    for (lib, member), names in sorted(wanted.items()):
        data = open(os.path.join(lib_scan.LIBDIR, lib), "rb").read()
        body = None
        for mname, b in lib_scan.members(data):
            if mname == member:
                body = b
                break
        base = member.replace("\\", "/").split("/")[-1]
        outp = EXTRACT / lib.replace(".lib", "") / base
        outp.parent.mkdir(parents=True, exist_ok=True)
        outp.write_bytes(body)
        mo = C.load_coff(outp)
        jp = C.SNAP / "split/libs" / SPLIT_DIR[lib] / base
        jo = C.load_coff(jp) if jp.exists() else None
        mf = fn_extents(mo) if mo else {}
        jf = fn_extents(jo) if jo else {}
        common = sorted(set(mf) & set(jf))
        def eq(n):
            (a, ra), (b, rb) = mf[n], jf[n]
            if len(a) != len(b):
                return False
            a, b = bytearray(a), bytearray(b)
            for k in set(ra) | set(rb):   # mask relocation fields of EITHER side (intra-object
                a[k:k + 4] = bytes(4)    # calls are pre-resolved in one, relocated in the other)
                b[k:k + 4] = bytes(4)
            return a == b
        same = [n for n in common if eq(n)]
        diff = [n for n in common if not eq(n)]
        result["%s:%s" % (lib, member)] = {
            "january_split_object": jp.relative_to(C.SNAP).as_posix() if jo else None,
            "pooled_commons": [{"name": n, "member_value": v, "january_contrib_size": size.get(n),
                                "size_equal": v == size.get(n)} for n, v in names],
            "functions_member": len(mf), "functions_january": len(jf),
            "functions_compared": len(common), "identical": len(same), "differing": diff,
            "january_only": sorted(set(jf) - set(mf))[:20], "member_only": sorted(set(mf) - set(jf))[:20],
        }
        print("%-32s jan=%s fns m/j/cmp=%d/%d/%d identical=%d differing=%d sizes_equal=%s" % (
            lib + ":" + base, "yes" if jo else "NO", len(mf), len(jf), len(common), len(same), len(diff),
            all(v == size.get(n) for n, v in names)))
    json.dump(result, open(C.DATA / "vendor_identity.json", "w"), indent=1)


if __name__ == "__main__":
    main()
