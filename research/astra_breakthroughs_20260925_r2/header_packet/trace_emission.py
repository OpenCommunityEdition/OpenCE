"""Correlate canary IR nodes with actual emitted function bytes.

The encoder bridge was independently established by the R2 decal investigation.
This observes two stock compiles; it does not override any compiler decisions.
"""
import argparse
import json
from pathlib import Path
import struct
import subprocess

import trace_canary as C


def run(label, identity=False):
    root = C.OUT / label
    source = root / (C.UNIT + ".c")
    work = C.OUT / (("emission_identity_" if identity else "emission_") + label)
    C.T.cflags = lambda unit: ["/I" + str(root / "source"), "/I" + str(root / "source/math")] + C.ORIGINAL_FLAGS(unit)
    C.T.CHAINS = ("1 24 28 18;1 24 28 18 0;1 24 28 18 0 4;2 24" if identity
                  else "1 24;2 24;1 24 28;1 24 2c")
    seg, obj = C.T.run_gated(str(source), str(work), C.UNIT, C.FN,
                             raw_bps=[0x10751347])
    rows = []
    for kind, bp, regs, record, chains in seg:
        data = b"".join(struct.pack("<I", v) for v in (chains.get(3 if identity else 1) or []))[:regs["edx"]]
        row = {"offset": regs["esi"], "size": regs["edx"],
               "bytes": data.hex(), "node_address": regs["ebx"]}
        if identity:
            row.update(symbol=chains.get(0), descriptor=chains.get(1),
                       name=b"".join(struct.pack("<I", v) for v in (chains.get(2) or [])).split(b"\0")[0].decode("latin1"))
        else:
            row.update(node=chains.get(0), input=chains.get(2), output=chains.get(3))
        rows.append(row)
    instrumented = Path(obj).read_bytes()
    (work / "instrumented.obj").write_bytes(instrumented)
    cmd = (work / "dbg_cmd.u16").read_text(encoding="utf-16-le").rstrip("\0")
    cwd = (work / "dbg_dir.u16").read_text(encoding="utf-16-le").rstrip("\0")
    subprocess.run(cmd, cwd=cwd, capture_output=True, check=True, timeout=60)
    stock = Path(obj).read_bytes()
    (work / "stock.obj").write_bytes(stock)
    l, r = bytearray(instrumented), bytearray(stock)
    l[4:8] = r[4:8] = b"\0" * 4
    if l != r:
        raise RuntimeError("Instrumentation changed compiler output")
    result = {"label": label, "command": cmd, "cwd": cwd,
              "source_sha256": C.sha(source.read_bytes()),
              "instrumented_sha256": C.sha(instrumented), "stock_sha256": C.sha(stock),
              "timestamp_only_equal": l == r, "rows": rows}
    (work / "receipt.json").write_text(json.dumps(result, indent=2) + "\n")
    print(label, len(rows), "emission events, stock equality", l == r)
    for row in rows:
        if 0x1f <= row["offset"] < 0x40:
            print(json.dumps(row))
    return result


if __name__ == "__main__":
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--identity", action="store_true")
    args = ap.parse_args()
    run("baseline", args.identity)
    run("owner", args.identity)
