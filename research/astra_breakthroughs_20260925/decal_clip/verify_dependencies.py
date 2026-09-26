"""Fail closed on unexpected debugger/compiler/helper versions.

Only source helpers are snapshotted. Set HALO_C2_DEBUGGER to an alternate
location of the exact recorded debugger executable if the old lane moved.
No compiler or debugger binary belongs in this research packet.
"""
from pathlib import Path
import hashlib
import json
import os
import probe

HERE = Path(__file__).resolve().parent

def verify():
    manifest = json.loads((HERE / "dependencies/manifest.json").read_text())
    for item in manifest["source_snapshots"]:
        path = HERE / "dependencies" / item["name"]
        actual = hashlib.sha256(path.read_bytes()).hexdigest()
        if actual != item["snapshot_sha256"]:
            raise RuntimeError("Snapshot hash mismatch: " + str(path))
    debugger = Path(os.environ.get("HALO_C2_DEBUGGER", manifest["debugger"]["default_path"]))
    if not debugger.is_file():
        raise RuntimeError("Pinned debugger missing. Set HALO_C2_DEBUGGER to the recorded executable: " + str(debugger))
    if hashlib.sha256(debugger.read_bytes()).hexdigest() != manifest["debugger"]["sha256"]:
        raise RuntimeError("Debugger hash mismatch: " + str(debugger))
    for name, expected in manifest["compiler_sha256"].items():
        path = probe.ROOT / "xbox/bin/vc7" / name
        if hashlib.sha256(path.read_bytes()).hexdigest() != expected:
            raise RuntimeError("Compiler hash mismatch (breakpoints invalid): " + str(path))
    return debugger

if __name__ == "__main__":
    print("Dependency hashes verified; debugger:", verify())
