"""Join observed allocator records to post-layout frame symbols by pointer.

This is observation-only and emits an ordinary stock-compiler object.
"""
import json
from pathlib import Path
import allocator_trace as A
import probe

T = A.T
T.CHAINS = "0 24 0;3 24"
src = str(probe.OUT / "C_canonical_context.c")
work = probe.OUT / "web_frame_bridge"
work.mkdir(exist_ok=True)
listing = work / "listing.cod"
seg, obj = T.run_gated(src, str(work), probe.UNIT, probe.NAME,
    raw_bps=[T.CHOOSE, T.POP, 0x10814408], extra_cflags=["/FAsc", "/Fa" + str(listing)])
chooses, pops, frames = [], [], []
for kind, bp, regs, record, chains in seg:
    if bp == 0:
        chooses.append({"regs": regs, "web": record, "symbol": chains.get(0)})
    elif bp == 1:
        pops.append({"regs": regs, "web": record, "symbol": chains.get(0)})
    else:
        rec = chains.get(1)
        if rec and rec[2] == regs["edx"] and (rec[1] >> 8) & 0x20:
            off = rec[9] - (1 << 32) if rec[9] & (1 << 31) else rec[9]
            frames.append({"address": regs["edx"], "offset": off, "record": rec})
matches = []
for k, d in enumerate(pops):
    for f in frames:
        if f["address"] == d["web"][0]:
            matches.append({"pop": k, "web_id": d["web"][7], "frame_offset": f["offset"], "frame_type": hex(f["record"][4]), "chosen": any(c["web"][0] == f["address"] for c in chooses), "symbol_address": f["address"]})
stock = probe.cc.load((probe.OUT / "C_canonical_context.obj").read_bytes())
traced = probe.cc.load(Path(obj).read_bytes())
code_equal = probe.cc.section_infos_equal(probe.cc.section_info(stock, probe.NAME), probe.cc.section_info(traced, probe.NAME))
result = {"stock_body_equal": code_equal, "chooses": chooses, "pops": pops, "frames": frames, "matches": matches}
(work / "bridge.json").write_text(json.dumps(result, indent=2) + "\n")
print("choices", len(chooses), "pops", len(pops), "frame locals", len(frames), "direct matches", len(matches))
print("stock body equal", code_equal)
for m in matches:
    print(m)
