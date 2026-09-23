"""Minimal full-fidelity COFF (i386 obj) reader/writer for probe experiments."""
import struct

SEC_HDR = 40
SYM = 18
NUL = bytes([0])


def cstr(b, off):
    e = b.index(NUL, off)
    return b[off:e].decode("ascii", "replace")


class Coff:
    pass


def load(path):
    data = open(path, "rb").read()
    o = Coff()
    o.machine, nsec, o.timestamp, symoff, nsym, optsize, o.flags = struct.unpack_from("<HHLLLHH", data, 0)
    stroff = symoff + nsym * SYM
    strlen = struct.unpack_from("<L", data, stroff)[0] if stroff < len(data) else 4
    strtab = data[stroff:stroff + strlen]
    o.sections = []
    off = 20
    for i in range(nsec):
        (raw_name, vsize, vaddr, size, praw, preloc, plnum, nreloc, nln, chars) = \
            struct.unpack_from("<8sLLLLLLHHL", data, off)
        nm = raw_name.rstrip(NUL).decode("ascii", "replace")
        if nm.startswith("/"):
            nm = cstr(strtab, int(nm[1:]))
        s = dict(index=i + 1, name=nm, vsize=vsize, vaddr=vaddr, size=size,
                 chars=chars, nln=nln, plnum=plnum)
        s["data"] = b"" if praw == 0 else data[praw:praw + size]
        relocs = []
        for r in range(nreloc):
            va, symidx, typ = struct.unpack_from("<LLH", data, preloc + r * 10)
            relocs.append([va, symidx, typ])
        s["relocs"] = relocs
        s["linenums"] = data[plnum:plnum + nln * 6] if plnum else b""
        o.sections.append(s)
        off += SEC_HDR
    o.symbols = []
    i = 0
    while i < nsym:
        eo = symoff + i * SYM
        zeroes, name_off = struct.unpack_from("<LL", data, eo)
        if zeroes == 0:
            name = cstr(strtab, name_off)
        else:
            name = data[eo:eo + 8].rstrip(NUL).decode("ascii", "replace")
        value, secno, typ, storage, naux = struct.unpack_from("<LhHBB", data, eo + 8)
        aux = data[eo + SYM: eo + SYM + naux * SYM]
        o.symbols.append(dict(index=i, name=name, value=value, sec=secno, type=typ,
                              storage=storage, naux=naux, aux=aux))
        i += 1 + naux
    return o


def build(o):
    nsec = len(o.sections)
    strparts = []
    strmap = {}

    def add_str(s):
        if s in strmap:
            return strmap[s]
        off = 4 + sum(len(x) for x in strparts)
        strparts.append(s.encode("ascii") + NUL)
        strmap[s] = off
        return off

    sec_name_field = []
    for s in o.sections:
        if len(s["name"]) <= 8:
            sec_name_field.append(s["name"].encode("ascii").ljust(8, NUL))
        else:
            sec_name_field.append(None)
    symbytes = []
    for sy in o.symbols:
        if len(sy["name"]) <= 8:
            nf = sy["name"].encode("ascii").ljust(8, NUL)
        else:
            nf = struct.pack("<LL", 0, add_str(sy["name"]))
        aux = sy["aux"]
        need = sy["naux"] * SYM
        if len(aux) < need:
            aux = aux + NUL * (need - len(aux))
        elif len(aux) > need:
            aux = aux[:need]
        symbytes.append(struct.pack("<8sLhHBB", nf, sy["value"], sy["sec"], sy["type"],
                                    sy["storage"], sy["naux"]) + aux)
    for i, s in enumerate(o.sections):
        if sec_name_field[i] is None:
            sec_name_field[i] = ("/%d" % add_str(s["name"])).encode("ascii").ljust(8, NUL)
    nsym = sum(1 + sy["naux"] for sy in o.symbols)
    hdr_end = 20 + nsec * SEC_HDR
    cur = hdr_end
    payload = bytearray()
    for s in o.sections:
        uninit = bool(s["chars"] & 0x00000080)
        if uninit or s["size"] == 0 or not s["data"]:
            praw = 0
        else:
            praw = cur + len(payload)
            payload += s["data"]
        s["_praw"] = praw
    for s in o.sections:
        if s["relocs"]:
            s["_preloc"] = cur + len(payload)
            for (va, si, ty) in s["relocs"]:
                payload += struct.pack("<LLH", va, si, ty)
        else:
            s["_preloc"] = 0
        if s["linenums"]:
            s["_plnum"] = cur + len(payload)
            payload += s["linenums"]
        else:
            s["_plnum"] = 0
    symoff = cur + len(payload)
    strtab = b"".join(strparts)
    strlen = 4 + len(strtab)
    out = bytearray()
    out += struct.pack("<HHLLLHH", o.machine, nsec, o.timestamp, symoff, nsym, 0, o.flags)
    for i, s in enumerate(o.sections):
        out += struct.pack("<8sLLLLLLHHL", sec_name_field[i], s["vsize"], s["vaddr"],
                           s["size"], s["_praw"], s["_preloc"], s["_plnum"],
                           len(s["relocs"]), s["nln"], s["chars"])
    assert len(out) == hdr_end
    out += payload
    assert len(out) == symoff
    for sb in symbytes:
        out += sb
    out += struct.pack("<L", strlen) + strtab
    return bytes(out)


def save(o, path):
    open(path, "wb").write(build(o))
