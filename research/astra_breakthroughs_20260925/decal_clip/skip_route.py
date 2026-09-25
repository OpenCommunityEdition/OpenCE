"""Observe the counter's exact skip path; no process-state forcing."""
import json
from pathlib import Path
import allocator_trace as A
import probe

T = A.T
T.CHAINS = "2 18;1 18"
work = probe.OUT / "skip_route"
sites = [0x10728140, 0x1072f561, 0x1072f470, 0x1072f41f]
seg, obj = T.run_gated(str(probe.OUT / "C_canonical_context.c"), str(work), probe.UNIT, probe.NAME, raw_bps=sites)
events = []
for kind, bp, regs, record, chains in seg:
    web = chains.get(0 if bp == 0 else 1)
    if not web or len(web) < 18:
        continue
    if web[7] not in (50, 99) or web[17] != 0x1a0:
        continue
    benefit = web[16] - (1 << 32) if web[16] & (1 << 31) else web[16]
    events.append({"site": hex(sites[bp]), "web_id": web[7], "position": hex(web[17]), "benefit": benefit, "flags": hex(web[1]), "symbol_address": web[0]})
stock = probe.cc.load((probe.OUT / "C_canonical_context.obj").read_bytes())
traced = probe.cc.load(Path(obj).read_bytes())
equal = probe.cc.section_infos_equal(probe.cc.section_info(stock, probe.NAME), probe.cc.section_info(traced, probe.NAME))
result = {"stock_body_equal": equal, "counter_route_events": events}
(work / "route.json").write_text(json.dumps(result, indent=2) + "\n")
print(json.dumps(result, indent=2))
