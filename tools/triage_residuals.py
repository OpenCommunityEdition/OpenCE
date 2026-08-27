# -*- coding: utf-8 -*-
"""Triage a unit's residual functions: register+slot-wildcarded structural diff count.
Usage: triage.py <unit-rel-path-no-ext>
Lists shared .text functions that are non-exact, ranked by structural diffs (asc).
"""
import sys, re
sys.path.insert(0, r"C:\Users\isabe\Documents\Codex\2026-07-13\i-w\work\libcmt-stream\tools")
import coff_compare as cc
import capstone

ROOT = r"C:\Users\isabe\Documents\Codex\2026-07-13\i-w\work\libcmt-stream"
UNIT = sys.argv[1]
TGT = "%s\\build\\split\\%s.obj" % (ROOT, UNIT.replace("/", "\\"))
OURS = "%s\\build\\base\\%s.obj" % (ROOT, UNIT.replace("/", "\\"))
md = capstone.Cs(capstone.CS_ARCH_X86, capstone.CS_MODE_32)
SLOT = re.compile(r"ebp [-+] (0x[0-9a-f]+|\d+)")
REG = re.compile(r"\b(eax|ecx|edx|ebx|esi|edi|ax|cx|dx|bx|si|di|al|cl|dl|bl)\b")

t = cc.load(TGT); o = cc.load(OURS)
names = sorted({s["name"] for s in t["by_index"].values()
                if s["section"] > 0 and s["storage"] in (2, 3) and s["value"] == 0
                and not s["name"].startswith((".", "$"))})


def masked(obj, name):
    info = cc.section_info(obj, name)
    sec = obj["sections"][cc.symbol(obj, name)["section"] - 1]
    raw = bytearray(cc._section_bytes(obj, sec))
    for r in info["relocations"]:
        raw[r["address"]:r["address"] + 4] = b"\0\0\0\0"
    return bytes(raw), info["size"]


def t2(x):
    return REG.sub("R", SLOT.sub("ebp-S", x))


rows = []
for n in names:
    try:
        oi = cc.section_info(o, n)
    except Exception:
        continue
    try:
        if cc.section_infos_equal(cc.section_info(t, n), oi):
            continue
    except Exception:
        continue
    try:
        jr, js = masked(t, n); orr, os = masked(o, n)
    except Exception:
        continue
    ji = ["%s %s" % (x.mnemonic, x.op_str) for x in md.disasm(jr, 0)]
    oo = ["%s %s" % (x.mnemonic, x.op_str) for x in md.disasm(orr, 0)]
    struct = 0
    for k in range(min(len(ji), len(oo))):
        a, b = ji[k], oo[k]
        if a == b:
            continue
        if a.split()[0][0] == "j" and b.split()[0][0] == "j":
            continue
        if t2(a) != t2(b):
            struct += 1
    rows.append((struct, abs(len(ji) - len(oo)), js, n, len(ji)))
rows.sort()
print("%-34s %6s %6s %6s %6s" % ("function", "struct", "|dlen|", "bytes", "jinstr"))
for struct, dl, sz, n, ni in rows:
    print("%-34s %6d %6d %6d %6d" % (n, struct, dl, sz, ni))
