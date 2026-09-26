"""Replay a debugger command with STOCK CL and compare whole object bytes.

Run immediately after the corresponding observation script. The exact source,
output path, command line, and working directory are reused. Both objects are
saved; only the COFF TimeDateStamp is ignored in the secondary comparison.
"""
from pathlib import Path
import hashlib
import json
import subprocess
import sys
import probe

def digest(data):
    return hashlib.sha256(data).hexdigest()

def check(label):
    work = probe.OUT / label
    path = work / "trace.obj"
    instrumented = path.read_bytes()
    (work / "instrumented.obj").write_bytes(instrumented)
    command = (work / "dbg_cmd.u16").read_text(encoding="utf-16-le").rstrip("\0")
    cwd = (work / "dbg_dir.u16").read_text(encoding="utf-16-le").rstrip("\0")
    compiler_before = {n: digest((probe.ROOT / "xbox/bin/vc7" / n).read_bytes()) for n in ("CL.Exe", "C1.Dll", "C2.Dll")}
    result = subprocess.run(command, cwd=cwd, capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    stock = path.read_bytes()
    (work / "stock.obj").write_bytes(stock)
    left, right = bytearray(instrumented), bytearray(stock)
    left[4:8] = right[4:8] = b"\0\0\0\0"
    compiler_after = {n: digest((probe.ROOT / "xbox/bin/vc7" / n).read_bytes()) for n in compiler_before}
    io, so = probe.cc.load(instrumented), probe.cc.load(stock)
    names = [s["name"] for s in io["symbols"] if s["section"] > 0 and s["value"] == 0 and s["type"] == 0x20 and io["sections"][s["section"] - 1]["name"] == ".text"]
    unequal = [name for name in set(names) if not probe.cc.section_infos_equal(probe.cc.section_info(io, name), probe.cc.section_info(so, name))]
    info = {
        "label": label, "exact_command": command, "cwd": cwd,
        "instrumented_sha256": digest(instrumented), "stock_sha256": digest(stock),
        "raw_equal": instrumented == stock,
        "instrumented_timestamp_masked_sha256": digest(left), "stock_timestamp_masked_sha256": digest(right),
        "equal_ignoring_only_coff_timestamp": left == right,
        "raw_differing_offsets": [i for i, (a,b) in enumerate(zip(instrumented, stock)) if a != b][:100],
        "all_code_sections_equal": not unequal, "unequal_code_symbols": unequal,
        "code_symbol_count": len(set(names)),
        "compiler_disk_hashes_unchanged": compiler_before == compiler_after,
        "compiler_sha256": compiler_after,
    }
    (work / "instrumentation_check.json").write_text(json.dumps(info, indent=2) + "\n")
    print(label, "raw equal", info["raw_equal"], "stamp-masked equal", info["equal_ignoring_only_coff_timestamp"], "all code sections equal", info["all_code_sections_equal"], "changed offsets", info["raw_differing_offsets"])

if __name__ == "__main__":
    for label in sys.argv[1:] or ["web_frame_bridge", "spill_operand_probe", "skip_route"]:
        check(label)
