# Independent review of decal_clip R2

Reviewer: scout worker. Scope: README/FINDINGS, all observation scripts, bounded C2 sites, archived records, parser tests, and existing scratch objects/logs. No production edits or new compiler probes.

Outcome: **no blocker in the central INC -> counter reload -> actual ECX bytes claim.** The packet correctly declines to claim a first January-versus-S4 compiler divergence or a proven scheduler tie-break cause.

Verified independently:

- All nine instrumented and stock object pairs exist and match byte-for-byte after masking only COFF timestamp bytes4..7. Recomputed both binary hashes and raw-log hashes against every archived receipt; all match. The deliberately wrong-hit pilot is labeled excluded, not silently discarded or used as counter evidence.
- Four parser tests pass, including interleaving, wraparound past32, and missing-record rejection. Full hit22, not its displayed low nibble6, identifies the counter. The remaining exact16-record-loss limitation is accurately disclosed.
- Within the emission run, INC is `0x00e84a24`, new reload is `0x00e6a7f0`, and both the spill-time INC predecessor and encoder-time reload predecessor are `0x00e85198`. Actual encoder records identify those nodes as INC `41` at4ee, reload `8b4dec` at4e5, and edge load `8b749608` at4e1. These are within-run pointer joins, not cross-process heap-address matching.
- In the separate operand run, the spilled operand and constructed reload share the source-symbol pointer `0x02445b2c`. Thus reload identity does not depend on the reused -20 stack displacement.
- The decision run observes opcode0x21/table0x442, live return register ID2, then arguments `[INC, counter operand, 2, 1]`. The bounded call-site disassembly supports bit0x400 eligibility, output-list mode1, and memory operand checks.
- Scheduler watch records update the reload's previous pointer at1074efdf to the same edge node within that run. The final encoder order corroborates that adjacency. Record zeroing terminates the retained watch prefix; recycled-address events are excluded.

Two non-blocking evidence-completeness nits were sent to the clip worker:

1. The original `c2_sites.txt` reload-constructor excerpt stopped at10758333, just before the cited10758341 call and mode/store branch. Extend that bounded excerpt so the central constructor claim is auditable directly in the text packet.
2. The final4f5 store is labeled COUNTER_STORE in FINDINGS, while the original emission script/archive names it OTHER_4f5 and does not join the store constructor's pointer in the encoder run. Either label that attribution as operand/byte inference or add the10758283 constructor-return site to the same-run encoder bridge. The three main node identities above are unaffected.

No source promotion, source-control authorization, exact close, or credit follows from this review. The next proposed scheduler-record mapping is appropriately bounded and explicitly stops short of forcing register assignments or inventing source dependencies.

## Resolution check

Both nits are now resolved. Independently re-read the refreshed packet: the constructor excerpt includes10758341 and its mode branch; the emission run now captures10758283 and joins the created store through the original counter operand to the encoder's `894dec` at4f5. The normalized chronology names that exact node COUNTER_STORE. Recomputed the refreshed emission binary hashes and full-object timestamp-only equality; all pass. No remaining review blocker or open evidence-completeness nit.
