"""Identify tail webs by allowed-register counterfactuals, NOT candidates.

Only five decisions in the relevant source-position range are instrumented.
The existing debugger modifies live compiler process state for the diagnostic;
the compiler binary and production sources are never modified.
"""
import json
import allocator_trace as A
import mapweb as M
import probe

src = str(probe.OUT / "C_canonical_context.c")
work = str(probe.OUT / "tail_web_map_diagnostic")
dec, mapped, asm = M.map_webs(src, probe.UNIT, probe.NAME, ks={1, 26, 33, 44, 45}, work=work)
for k, (d, alt, occurrences) in mapped.items():
    print("decision", k, "id", d["id"], d["reg"], "->", alt, "IL pos", hex(d["pos"]))
    for offset, text in occurrences:
        print(hex(offset) if isinstance(offset, int) else offset, text)
(probe.OUT / "tail_web_map_diagnostic.json").write_text(json.dumps(mapped, indent=2) + "\n")
