"""READ-ONLY. Which vendor LIBRARY MEMBER defines each pooled name, and how.

Scans every *.lib under the local XDK library directory (Aug-2001, XDK 3911 era; NOT
proven to be January's exact library build) and reports, per pooled name, every member
whose COFF symbol table holds it as COMMON (sec 0, value = size), as a real definition
(sec > 0) or as an import.  Writes data/lib_defs.json.

A member's COMMON entry is DIRECT evidence of which vendor TU declared the tentative
definition in THAT library build.  Whether January linked the same build is checked
separately (build_packets.py compares member names against cachebeta.pdb modules and
sizes against January's contribution).
"""
import json
import os
import struct
import sys
from collections import defaultdict

sys.path.insert(0, os.path.dirname(__file__))
import c1_common as C  # noqa: E402

LIBDIR = r"C:\Users\isabe\Documents\Codex\2026-07-13\i-w\work\data-encoding-final\xbox\lib"


def members(data):
    assert data[:8] == b"!<arch>\n"
    off = 8
    longnames = b""
    while off + 60 <= len(data):
        hdr = data[off:off + 60]
        name = hdr[:16].decode("latin-1").rstrip()
        size = int(hdr[48:58].decode("latin-1").strip() or 0)
        body = data[off + 60:off + 60 + size]
        if name == "//":
            longnames = body
        elif name == "/":
            pass
        else:
            if name.startswith("/") and name[1:].isdigit():
                k = int(name[1:])
                e = longnames.find(b"\0", k)
                if e < 0:
                    e = longnames.find(b"\n", k)
                name = longnames[k:e].decode("latin-1")
            yield name.rstrip("/"), body
        off += 60 + size
        if off & 1:
            off += 1


def coff_symbols(b):
    if len(b) < 20:
        return None
    machine, nsec, ts, symoff, nsym = struct.unpack_from("<HHLLL", b, 0)
    if machine == 0 and nsec == 0xFFFF:
        return None  # import object / anon object
    if machine != 0x14C or symoff == 0 or symoff + nsym * 18 > len(b):
        return None
    stroff = symoff + nsym * 18
    out = []
    i = 0
    while i < nsym:
        e = symoff + i * 18
        z, no = struct.unpack_from("<LL", b, e)
        if z == 0:
            k = stroff + no
            name = b[k:b.index(b"\0", k)].decode("latin-1")
        else:
            name = b[e:e + 8].rstrip(b"\0").decode("latin-1")
        value, sec, typ, storage, naux = struct.unpack_from("<LhHBB", b, e + 8)
        out.append((name, value, sec, storage))
        i += 1 + naux
    return out


def main():
    recs, _ = C.pool_records()
    pooled = {r["name"] for r in recs}
    defs = defaultdict(list)
    nlib = nmem = 0
    for f in sorted(os.listdir(LIBDIR)):
        if not f.lower().endswith(".lib"):
            continue
        data = open(os.path.join(LIBDIR, f), "rb").read()
        if data[:8] != b"!<arch>\n":
            continue
        nlib += 1
        for mname, body in members(data):
            syms = coff_symbols(body)
            if syms is None:
                continue
            nmem += 1
            for name, value, sec, storage in syms:
                if name not in pooled or storage != 2:
                    continue
                if sec == 0 and value > 0:
                    form = "COMMON"
                elif sec == 0:
                    form = "UNDEF"
                elif sec > 0:
                    form = "DEFINED"
                else:
                    form = "OTHER"
                defs[name].append({"lib": f, "member": mname, "form": form, "value": value})
    json.dump({"libdir": LIBDIR, "defs": defs}, open(C.DATA / "lib_defs.json", "w"), indent=1)
    print("libs", nlib, "members", nmem, "pooled names seen", len(defs))
    for name in sorted(defs):
        forms = defs[name]
        cm = [(d["lib"], d["member"], d["value"]) for d in forms if d["form"] in ("COMMON", "DEFINED")]
        if cm:
            print(" ", name, cm[:6], "(+%d importers)" % sum(d["form"] == "UNDEF" for d in forms))


if __name__ == "__main__":
    main()
