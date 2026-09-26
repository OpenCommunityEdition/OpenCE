# Halo compiler breakthroughs — research round 2

Research baseline: `012f7283230d13be8427ef7e93f73f3ef686111f`.
Production source/config/tools remain identical to `20d0563d`.
**Zero new matching bytes, zero new object admissions, no holds lifted.**

This round replaces three vague "compiler tie" descriptions with narrower,
reproducible mechanisms. Only the first currently has an independent predictive
source control; none is a ready production landing.

## 1. Stack allocation has a measurable reference-ranking threshold

[Scout investigation](scout/README.md): a scalar/aggregate projection changes
computed-left's memory-reference weight from six to eight. That crosses another
12-byte local's unchanged weight of seven, reverses their placement order, and
changes which stack group grows. The affected groups occupy 24 versus 20 bytes.
Storage flags and the four relevant interference sets remain unchanged.

A separate small, cast-free C control predicted and reproduced the same
mechanism: weight four to six crosses five, and frame 32 becomes 28 bytes.
A const-correct consumer-declaration negative control leaves counts, ordering,
slots and function bytes unchanged. This is a predictive model, not another
declaration-order sweep.

Use it to diagnose the R1 census's 28 allocation-mismatch functions (59,512
overlapping meaningful bytes), **not** to claim that all 28 share this cause or
are now closable. Adding artificial reads or casts to cross a threshold remains
inadmissible. The scout's separate x87 operand-order residual remains unresolved.

The new parameterized `scout/trace_frame.py` captures counts, ordering, group
choices and final slots for a supplied source/unit/function. It rejects unsafe
or concurrently shared output paths and inconsistent identities/counts. Beyond
the toy, a fresh canonical `_biped_update_physics` run passed: 73 frame records,
73 packing decisions, unchanged production function, complete stock-object
equality. Its 44,980-byte frame versus January's 44,972 remains an unexplained
eight-byte gap; this is a validated next investigation, not a closure. See
`frame_probe_smoke.json` for its compact receipt.

## 2. The decal counter reload is identified end to end

[Decal investigation](decal_clip/README.md): the original spill pass converts
the counter operands to memory but does not create the disputed ECX load. A
later registerization path inserts that load directly before the increment.
The same node is followed into the encoder and produces the actual `8b4dec`
bytes at `+0x4e5`; the already-present edge load precedes it at `+0x4e1`.

The later scheduler preserves that adjacency. The next probe is now precisely
the scheduler's dependency/priority record for these two identified nodes,
rather than more blind source-order or register-force attempts. January's own
IR is unavailable; we have not proved which earlier difference produced its
opposite ordering.

## 3. The shared-header regression now has operand-level anchors

[Header experiment](header_packet/README.md): the genuine plane-helper owner
repair freshly closes `_collision_move_point` in scratch but loses two existing
exact functions. It stays unlanded.

Forty integer-choice summaries agree positionally in the frame-statistics
canary. Twelve named values are identified, but that is not proof of identical
assignment for every temporary. Actual differing global-field loads are now
joined to both compiler-symbol offsets and final named relocations, giving a
reliable starting point for tracing their earlier construction order.

The [independent header evidence audit](header_evidence/README.md) also catches
stale scheduling suggestions and preserves a qualified first-party stack-walk
include clue. It supplies no new authority to choose the held declaration fix.

## Verification and safety

- Private copied debuggers/configs; pinned compiler/debugger/source hashes.
- Every successful observation replayed with stock CL, requiring whole-object
  equality except the four-byte COFF timestamp.
- No production source, headers, tools, symbols, parks, scorer or admission
  changes. No production patches, compiler binaries or objects bundled.
- New parser tests cover truncated debugger hit counters, incorrect load
  identity, unequal trace lengths, and frame-mechanism predictions.
- Independent cross-reviews reside in each packet; limitations remain explicit.
- Original-source evidence, diagnostic controls and admission authority are
  separate. A mechanism control is not automatically authentic Halo source.

Run offline validation:

```powershell
python -m pytest -q tools research/astra_breakthroughs_20260925 research/astra_breakthroughs_20260925_r2
```

See [NEXT_MODEL_TASKS.md](NEXT_MODEL_TASKS.md) for bounded continuations. Reuse
the existing R1 `coupled_probe.py` for justified BASE/A/B/AB source hypotheses;
two interacting changes are not ruled out by a failed one-factor test.
