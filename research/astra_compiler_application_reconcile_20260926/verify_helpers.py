"""Independent, fail-closed bounded helper checks for this reconciliation.

Run from the canonical root after the full build. No production compiler or
comparator changes. Link probes establish duplicate compatibility, not a game link.
"""
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import tempfile

ROOT = Path.cwd()
sys.path.insert(0, str(ROOT / "tools"))
import coff_compare as cc

OUT = ROOT / "scratch/reconcile_compiler_application_20260926/helper_checks"
BEFORE = ROOT / "scratch/reconcile_compiler_application_20260926/before_base"


def load(path):
    return cc.load(path.read_bytes())


def definitions(obj):
    return {s["name"]: s for s in obj["symbols"]
            if s["storage"] == 2 and s["section"] > 0}


def link(first, second, label, duplicate_control=False):
    with tempfile.TemporaryDirectory(prefix="halo_provider_") as tmp:
        command_second = second
        if duplicate_control:
            # The linker ignores the same pathname twice (LNK4042); a distinct
            # byte-identical file is required to exercise duplicate definitions.
            command_second = Path(tmp) / "duplicate.obj"
            command_second.write_bytes(second.read_bytes())
        cmd = [str(ROOT / "xbox/bin/vc7/Link.Exe"), "/NOLOGO", "/MACHINE:X86",
               "/SUBSYSTEM:CONSOLE", "/NODEFAULTLIB", "/ENTRY:probe_entry",
               "/OUT:" + str(Path(tmp) / "probe.exe"), str(first), str(command_second)]
        result = subprocess.run(cmd, capture_output=True, text=True, cwd=ROOT)
    output = result.stdout + result.stderr
    diagnostics = set(re.findall(r"(?:fatal error|error|warning) (LNK\d+)", output))
    allowed = {"LNK2001", "LNK2019", "LNK1120"}
    if duplicate_control:
        allowed |= {"LNK2005", "LNK1169"}
        passed = "LNK2005" in diagnostics and result.returncode in (1120, 1169)
    else:
        passed = result.returncode in (0, 1120)
    passed = passed and not (diagnostics - allowed)
    pins = {str(p.relative_to(ROOT)): hashlib.sha256(p.read_bytes()).hexdigest()
            for p in (first, second)}
    receipt = {"command": cmd, "returncode": result.returncode,
               "diagnostics": sorted(diagnostics), "pins": pins,
               "negative_control": duplicate_control, "passed": passed}
    (OUT / (label + ".txt")).write_text(json.dumps(receipt, indent=2) + "\n" + output)
    return receipt


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    index = {}
    for path in sorted((ROOT / "build/split").rglob("*.obj")):
        obj = load(path)
        for name, sym in definitions(obj).items():
            index.setdefault(name, []).append((path, obj, sym))
    new_symbols = []
    checks = {}
    for path in sorted((ROOT / "build/base").rglob("*.obj")):
        rel = path.relative_to(ROOT / "build/base")
        old_path = BEFORE / rel
        if not old_path.exists():
            raise RuntimeError("Missing baseline object: " + str(rel))
        obj, old = load(path), load(old_path)
        old_defs, now_defs = definitions(old), definitions(obj)
        target_path = ROOT / "build/split" / rel
        target_defs = definitions(load(target_path))
        for name in sorted(now_defs.keys() - old_defs.keys()):
            sym = now_defs[name]
            sec = obj["sections"][sym["section"] - 1]
            new_symbols.append({"unit": str(rel), "symbol": name,
                                "section": sec["name"], "target_owned": name in target_defs})
            if sec["name"] == ".text" and name not in target_defs:
                checks[(str(rel), name)] = (path, obj, sym)
        if str(rel).replace("\\", "/") == "source/math/periodic_functions.obj":
            for name in ("_fast_ftol", "_real_random"):
                checks[(str(rel), name)] = (path, obj, now_defs[name])
    identity, pairs = [], set()
    for (unit, name), (path, obj, sym) in sorted(checks.items()):
        providers = index.get(name, [])
        if len(providers) != 1:
            raise RuntimeError("Nonunique/missing selected provider: " + name)
        target_path, target, tsym = providers[0]
        rel = target_path.relative_to(ROOT / "build/split")
        provider_path = ROOT / "build/base" / rel
        provider = load(provider_path)
        psym = definitions(provider).get(name)
        if psym is None:
            raise RuntimeError("Current provider no longer defines " + name)
        ti = cc.section_info_by_number(target, tsym["section"])
        oi = cc.section_info_by_number(obj, sym["section"])
        pi = cc.section_info_by_number(provider, psym["section"])
        passed = cc.section_infos_equal(ti, oi) and cc.section_infos_equal(ti, pi)
        identity.append({"unit": unit, "symbol": name, "provider": str(rel),
                         "size": ti["size"], "relocations": ti["relocation_count"],
                         "sha256": ti["normalized_sha256"], "passed": passed})
        pairs.add((path, provider_path))
    receipts = []
    for i, (candidate, provider) in enumerate(sorted(pairs)):
        receipts.append(link(candidate, provider, f"pair_{i}_unit_first"))
        receipts.append(link(provider, candidate, f"pair_{i}_provider_first"))
    for i, candidate in enumerate(sorted({pair[0] for pair in pairs})):
        receipts.append(link(candidate, candidate, f"control_{i}", duplicate_control=True))
    result = {"new_external_definitions": new_symbols, "identity": identity,
              "links": receipts, "passed": all(x["passed"] for x in identity + receipts)}
    (OUT / "summary.json").write_text(json.dumps(result, indent=2) + "\n")
    print(json.dumps(result, indent=2))
    return 0 if result["passed"] else 1


if __name__ == "__main__":
    raise SystemExit(main())
