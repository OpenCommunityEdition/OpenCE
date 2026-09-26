"""Observe counter operand rewrite at the proven reachable spill path.

Pre: 0x1072e92a has EBP=operand, EDI=instruction, [EBP+1c]=web.
Post: 0x1072eb4c follows clearing +1c and setting operand kind +8 to 2.
No bytes, registers, operands, or decisions are forced.
"""
import json
from pathlib import Path
import allocator_trace as A
import probe

T = A.T
T.CHAINS = "6 24;6 18 1c;5 24;5 24 0"
work = probe.OUT / "spill_operand_probe"
sites = [0x1072e92a, 0x1072eb4c]
seg, obj = T.run_gated(str(probe.OUT / "C_canonical_context.c"), str(work), probe.UNIT, probe.NAME, raw_bps=sites)
events, operands = [], set()
for kind, bp, regs, record, chains in seg:
    if bp == 0:
        web = chains.get(1)
        if not web or len(web) < 18 or web[7] != 99 or web[17] != 0x1a0:
            continue
        operands.add(regs["ebp"])
    elif regs["ebp"] not in operands:
        continue
    events.append({"site": hex(sites[bp]), "operand_address": regs["ebp"], "instruction_address": regs["edi"], "operand": chains.get(0), "instruction": chains.get(2), "next_instruction": chains.get(3)})
stock = probe.cc.load((probe.OUT / "C_canonical_context.obj").read_bytes())
traced = probe.cc.load(Path(obj).read_bytes())
equal = probe.cc.section_infos_equal(probe.cc.section_info(stock, probe.NAME), probe.cc.section_info(traced, probe.NAME))
result = {"stock_body_equal": equal, "events": events}
(work / "operands.json").write_text(json.dumps(result, indent=2) + "\n")
print("ordinary stock body equal", equal, "counter operand events", len(events))
for e in events:
    print(e["site"], "operand", hex(e["operand_address"]), "kind", hex(e["operand"][2]), "web ptr", hex(e["operand"][7]), "instruction", hex(e["instruction_address"]), "opcode", hex(e["instruction"][1]), "next", hex(e["instruction"][0]))
