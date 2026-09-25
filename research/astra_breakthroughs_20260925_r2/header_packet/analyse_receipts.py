"""Validate and compact the header experiment's independently anchored records.

Pure helpers are unit-testable without a compiler/debugger. The command reads
existing scratch receipts and objects; it never recompiles or edits game source.
"""
from pathlib import Path
import hashlib
import json
import struct
import sys

ROOT = Path(__file__).resolve().parents[3]
OUT = ROOT / "scratch/astra_header_r2_20260925"
HERE = Path(__file__).resolve().parent
PROJECTION = ("reg", "pri", "pos", "cost", "allowed", "init", "terms", "subs", "batch")


def compare_choices(left, right):
    if len(left) != len(right):
        raise ValueError("Unequal choice counts: no positional correspondence")
    return [{"position": i, "fields": {k: [a.get(k), b.get(k)]
             for k in PROJECTION if a.get(k) != b.get(k)}}
            for i, (a, b) in enumerate(zip(left, right))
            if any(a.get(k) != b.get(k) for k in PROJECTION)]


def decode_absolute_load(row, code, relocations):
    """Only decode the proven absolute dword MOV form, fail closed otherwise."""
    offset = row["offset"]
    data = bytes.fromhex(row["bytes"])
    if len(data) != 6 or data[0] != 0x8b or (data[1] & 0xc7) != 5:
        raise ValueError("Not the supported absolute dword MOV")
    if code[offset:offset + 6] != data:
        raise ValueError("Emitted instruction does not equal object bytes")
    matches = [r for r in relocations if r["address"] == offset + 2]
    if len(matches) != 1 or matches[0]["target"][:2] != ["symbol", "_rasterizer_frame_statistics"]:
        raise ValueError("No unique named-global relocation anchor")
    field = struct.unpack_from("<I", data, 2)[0]
    symbol = row.get("symbol")
    if not symbol or len(symbol) < 10 or symbol[9] != field:
        raise ValueError("Compiler symbol displacement does not match emitted load")
    if matches[0]["target"][2] != field:
        raise ValueError("Relocation addend does not match compiler symbol")
    return {"offset": offset, "register": ["eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi"][(data[1] >> 3) & 7],
            "global": "_rasterizer_frame_statistics", "member_offset": field,
            "symbol_type": symbol[4], "symbol_ordinal": symbol[7],
            "bytes": data.hex()}


def read(path):
    return json.loads(path.read_text())


def main():
    sys.path.insert(0, str(ROOT))
    from tools import coff_compare as cc
    probes = read(OUT / "results.json")
    traces = {label: read(OUT / ("trace_" + label) / "receipt.json") for label in ("baseline", "owner")}
    result = {"credit": 0, "head": probes["baseline"], "six_unit_probe": {},
              "chooser_count": len(traces["baseline"]["decisions"]),
              "chooser_projection_changes": compare_choices(traces["baseline"]["decisions"], traces["owner"]["decisions"]),
              "named_choices": {}, "anchored_loads": {}, "receipts": {}}
    for unit, cells in probes["cells"].items():
        result["six_unit_probe"][unit] = {label: {k: row[k] for k in
            ("source_sha256", "header_sha256", "object_sha256", "real_math_includes",
             "shadow_header_confirmed", "gained", "lost", "changed", "added", "removed")}
            for label, row in cells.items()}
    for label in ("baseline", "owner"):
        for prefix in ("trace_", "symbols_", "emission_", "emission_identity_"):
            path = OUT / (prefix + label) / "receipt.json"
            receipt = read(path)
            if not receipt["timestamp_only_equal"]:
                raise ValueError("Missing stock/instrumented equality")
            result["receipts"][prefix + label] = {
                "path": str(path.relative_to(ROOT)).replace("\\", "/"),
                "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
                **{k: receipt[k] for k in ("instrumented_sha256", "stock_sha256", "timestamp_only_equal")}}
        symbols = read(OUT / ("symbols_" + label) / "receipt.json")
        result["named_choices"][label] = [{"position": i, "name": r["name"],
            "register_index": r["regs"]["ebx"], "web_id": r["web"][7], "symbol_ordinal": r["symbol"][7]}
            for i, r in enumerate(r for r in symbols["rows"] if r["event"] == "choose") if r["name"]]
        identity = read(OUT / ("emission_identity_" + label) / "receipt.json")
        obj = cc.load((OUT / ("emission_identity_" + label) / "stock.obj").read_bytes())
        symbol = cc.symbol(obj, "_rasterizer_frame_statistics_draw")
        section = obj["sections"][symbol["section"] - 1]
        code = obj["data"][section["raw"]:section["raw"] + section["size"]]
        relocs = cc.section_info(obj, "_rasterizer_frame_statistics_draw")["relocations"]
        result["anchored_loads"][label] = [decode_absolute_load(r, code, relocs)
            for r in identity["rows"] if r["offset"] in (0x1f, 0x25, 0x2b, 0x31, 0x3a)]
        if len(result["anchored_loads"][label]) != 5:
            raise ValueError("Missing or repeated emission anchors")
    result["limits"] = [
        "40 choice projections are equal positionally, not proof of equal value assignments.",
        "Twelve named choices are identified; unnamed temporaries are not semantically mapped.",
        "The swapped global members have no directly corresponding named CHOOSE event.",
        "Emission proves their symbol-to-byte mapping, not which earlier pass first reordered them.",
        "Six TUs are a mechanism probe, not a full-board admission sweep."]
    (HERE / "measurements.json").write_text(json.dumps(result, indent=2) + "\n")
    print("Verified 10 emitted loads against object bytes, named relocations and compiler symbols")
    print("Choice projections:", result["chooser_count"], "changes:", len(result["chooser_projection_changes"]))
    for label, rows in result["anchored_loads"].items():
        print(label, [(hex(r["offset"]), r["register"], r["member_offset"]) for r in rows])


if __name__ == "__main__":
    main()
