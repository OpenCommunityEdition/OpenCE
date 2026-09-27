"""Shared READ-ONLY loaders for worker C1's COMMON-pool evidence packets.

Nothing here writes outside research/compiler_application_20260925/workers/C1/ or
scratch/campaign/workers/C1/.  Every input is read from a frozen snapshot taken at
2026-09-26 01:12:21 -0700 (HEAD 09f5208f8a2b8be564519b800f7bdafb20fa19fc):

    scratch/campaign/workers/C1/snap_20260926_011221/{base,split,source,config}

so a lead rebuild mid-run cannot mix two builds into one packet.
"""
import json
import os
import re
import struct
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[5]          # worktree root
SNAP = ROOT / "scratch/campaign/workers/C1/snap_20260926_011221"
OUT = ROOT / "research/compiler_application_20260925/workers/C1"
DATA = OUT / "data"
sys.path.insert(0, str(ROOT / "tools/campaign"))
import coffio  # noqa: E402

JAN_POOL = SNAP / "split/source/linker_common.obj"
MODULE_LIST = DATA / "cachebeta_modules.txt"      # DIA2Dump -m cachebeta.pdb


def align_of(chars):
    a = (chars >> 20) & 0xF
    return 0 if a == 0 else 1 << (a - 1)


def load_json(p):
    with open(p, encoding="utf-8") as f:
        return json.load(f)


def symbols_json():
    return load_json(SNAP / "config/symbols.json")


def contribs():
    return load_json(SNAP / "config/contribs.json")


def modules():
    """0-based module_index (as used in contribs.json) -> DIA module name."""
    mods = {}
    for line in open(MODULE_LIST, encoding="latin-1"):
        m = re.match(r"^([0-9A-F]{4}) (.*)$", line.rstrip("\r\n"))
        if m:
            mods[int(m.group(1), 16) - 1] = m.group(2)
    return mods


def pool_records():
    """The 242 csplit records of January's pooled block, in section order."""
    o = coffio.load(str(JAN_POOL))
    owner = {}
    for sy in o.symbols:
        if sy["sec"] > 0 and not sy["name"].startswith("."):
            owner.setdefault(sy["sec"], sy)
    syms = {}
    for e in symbols_json():
        syms.setdefault(e["name"], []).append(e["file_offset"])
    con = {c["file_offset"]: c for c in contribs() if c["module_index"] == 847}
    recs = []
    for s in o.sections:
        sy = owner[s["index"]]
        name = sy["name"]
        rvas = syms.get(name, [])
        rva = rvas[0] if len(rvas) == 1 else None
        c = con.get(rva) if rva is not None else None
        recs.append(dict(
            pool_index=s["index"],
            name=name,
            section=s["name"],
            csplit_size=s["size"],
            csplit_align=align_of(s["chars"]),
            chars=s["chars"],
            storage=sy["storage"],
            rva=rva,
            symbols_json_hits=len(rvas),
            contrib_size=c["size"] if c else None,
            contrib_align=align_of(c["flags"]) if c else None,
            contrib_flags=c["flags"] if c else None,
            raw_hex=s["data"].hex() if s["name"] != ".bss" else None,
        ))
    bss = [r for r in recs if r["section"] == ".bss" and r["rva"] is not None]
    base = min(r["rva"] for r in bss)
    for r in recs:
        r["pool_offset"] = (r["rva"] - base) if (r["section"] == ".bss" and r["rva"] is not None) else None
    return recs, base


def c_name(sym):
    """COFF name -> C identifier (cdecl data symbols carry one leading underscore)."""
    return sym[1:] if sym.startswith("_") else sym


def iter_objs(root):
    for p in sorted(Path(root).rglob("*.obj")):
        yield p


def load_coff(p):
    try:
        return coffio.load(str(p))
    except Exception:
        return None
