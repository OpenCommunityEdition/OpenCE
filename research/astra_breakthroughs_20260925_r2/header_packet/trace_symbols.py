"""Map corresponding colour decisions to the symbols they actually colour.

Observation only. This supplements, not replaces, trace_canary.py: positional
register equality by itself does not identify the values occupying registers.
"""
import json
from pathlib import Path
import struct
import subprocess

import trace_canary as C


def run(label):
    root = C.OUT / label
    source = root / (C.UNIT + ".c")
    work = C.OUT / ("symbols_" + label)
    C.T.cflags = lambda unit: ["/I" + str(root / "source"), "/I" + str(root / "source/math")] + C.ORIGINAL_FLAGS(unit)
    C.T.CHAINS = "0 24 0;0 16 0 0;0 24 0 0 4;0 24 28"
    seg, obj = C.T.run_gated(str(source), str(work), C.UNIT, C.FN,
                             raw_bps=[C.T.CHOOSE, C.T.POP])
    rows = [{"event": "choose" if bp == 0 else "pop", "regs": regs,
             "web": record, "symbol": chains.get(0), "descriptor": chains.get(1),
             "name": b"".join(struct.pack("<I", w) for w in (chains.get(2) or [])).split(b"\0")[0].decode("latin1"),
             "first_ref": chains.get(3)}
            for kind, bp, regs, record, chains in seg]
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
    print(label, len(rows), "events, stock equality", l == r)
    print("Named chosen symbols:", [r["name"] for r in rows if r["event"] == "choose" and r["name"]])
    return result


if __name__ == "__main__":
    run("baseline")
    run("owner")
