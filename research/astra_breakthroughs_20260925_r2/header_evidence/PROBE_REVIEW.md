# Independent review of the fresh shared-header probe

Reviewed 2026-09-25 at `012f7283`. Inputs: `header_packet/probe.py`,
`trace_canary.py`, `trace_symbols.py`; scratch `results.json`, both `trace_*`
receipts, both `symbols_*` receipts, and the pinned trace/oracle implementations.
This review reads existing receipts; it does not claim another compilation.

## Supported result

All six scratch controls reproduce production **code-owner section verdicts**.
The genuine broad `valid_real_plane3d` move freshly produces:

- `_collision_move_point` strict exact, plus its helper COMDAT;
- `_bitmap_copy` and `_rasterizer_frame_statistics_draw` exact-to-residual;
- matrix_math unchanged code rows, render_cameras helper emission only;
- units unchanged code rows in this probe.

The two observation-only compiler traces reproduce ordinary stock compilation
over the **entire object except COFF timestamp**, and their target sections equal
the ungated probe sections. The force files are empty. These are good controls
against debugger-induced output changes.

The selected projections of 40 CHOOSE events are equal by sequence position:
register, priority, position, costs, allowed/initial sets, priority terms,
subtractions, and batch. Both records also contain 91 POP events. The oracle
checks pass at their stated scope; notably its R2 interference test is explicitly
weak, not a full independently reconstructed interference graph.

**Valid headline:** the measured regression occurs despite equal positional
integer-chooser summaries. **Invalid headline:** the allocator assigns the same
registers to the same values, so allocation is ruled out.

## Limits / small hardening items

1. **Operand identity remains unproved.** First CHOOSE id changes 39 to 85;
   neighbor IDs and generation maps differ. These could be renumberings, changed
   memberships, or changed value associations. The new symbol snapshots match
   by positional scalar fingerprint (flags, type, ordinal, width, displacement,
   tag) at all 40 CHOOSE positions, but matching ordinals are not yet semantic
   names. `first_ref` / `last_ref` currently contain only raw pointer-rich
   records; they do not map source operands to final instructions.
2. **Shadow-header inclusion needs a receipt.** Early `/I` search paths do not
   necessarily override a quoted include found beside an original included
   header. Capture `/showIncludes` or preprocessing provenance and require that
   each intended consumer uses the shadow header, not the canonical copy first.
   Baseline reproduction is necessary but does not establish which candidate
   header an unchanged consumer consumed. The actual changed callers prove some
   consumers use it; they do not prove all consumers do.
3. **The baseline check is code-only.** `changed/added/removed` checks named
   code-owner section infos, not all data, storage classes, COMDAT selection or
   the whole object. That is appropriate for this mechanism probe, but not a
   production admission or full ownership-sweep claim.
4. **Untouched-source receipt omits the header.** `production_unchanged` records
   only the six C inputs. Add the canonical header hash, build.ninja and tool
   hashes to the receipt if this is to demonstrate reproducibility independently
   of the live workspace. The trace verifies compiler/debugger versions already;
   the initial nongated probe does not itself pin them.
5. **Unequal trace lengths should fail correspondence explicitly.** `zip`
   truncates comparison when one side has more choices. Current counts are both
   40, so it does not invalidate these results; make unequal counts a recorded
   failure rather than an empty-difference opportunity.
6. **Retain the traced symbol object.** `trace_symbols.py` hashes the observed
   bytes then overwrites trace.obj during stock replay without saving a separate
   observed object. Equality is tested live, but preserving the observed copy
   would make later independent examination as good as `trace_canary.py`.
7. **No full consumer sweep has happened.** Six selected TUs establish this
   local causal regression, not total blast radius. No new matching credit.

## Smallest next semantic-identity probe

Reuse the observation-only descriptor/name bridge the scout researcher just
validated, before attempting any forced-register counterfactuals:

1. At CHOOSE (`0x1072da96`), `EAX` is W and `W[0]` is the associated symbol.
2. For the scout's named frame-symbol class, `symbol[0]` is its descriptor and
   `descriptor+4` points at a C string. Try independent read chains
   `0 24 0;0 24 0 0;0 24 0 0 4`, then the existing `cstring` decoder. Treat this
   relation as unconfirmed for any different symbol kind until the decoded
   values agree with the ordinary `/FAsc` listing or named-global relocations.
3. Join baseline/owner decisions by verified name + type + offset / relevant
   use-site identity + generation, not heap addresses or numeric IDs. Preserve
   ambiguous unnamed temporaries as ambiguous.
4. Start only with webs involved in the final differing relocation/load window,
   plus an unchanged named control. If those globals are not globally colored,
   that itself redirects work to local lowering/materialization; do not infer
   their identity from the first three high-priority choices.
5. If a name bridge is unavailable, the existing `mapweb.py` is a bounded
   diagnostic fallback. Its register-forced objects are never candidates; it
   may perturb other allocation decisions, and its disassembly alignment is
   heuristic. Require relocation-aware anchoring and report collateral changes.

The key output is a labeled value-to-register/use-site mapping for the two
different compilations. Equal cost summaries alone do not provide it.

## Final update: hardening and semantic load anchors reviewed

The updated `probe.py`, `trace_symbols.py`, `trace_emission.py`,
`analyse_receipts.py`, and compact `header_packet/measurements.json` address
items 2, 4, 5 and 6 above: all twelve baseline/candidate compilations record the
intended shadow `real_math.h` under `/showIncludes`; canonical header, source,
build.ninja and compiler hashes are recorded; unequal choice counts fail; and
the symbol-traced objects are retained before stock replay. The controlled
source for frame_statistics is unchanged between the two header cases.

The descriptor/name bridge identifies twelve CHOOSE events with matching names
and registers. It does **not** identify all forty values, and the two changed
global-member operands are not among those named choices. This closes only the
named subset of item 1, not its general correspondence requirement.

The emission anchor supplies independent operand identity for the relevant
machine-code window. Each of five observations per compilation agrees with:
the emitted six-byte instruction, the final object's raw bytes, its unique
named `_rasterizer_frame_statistics` relocation/addend, and the memory-symbol
record's `+0x24` member displacement. At function offsets `0x1f` / `0x25`, the
baseline loads member offsets 172 / 156 into ECX / EDX; the owner-header case
loads 156 / 172 into those registers. Member offsets 144, 132 and 120 remain
unchanged controls at `0x2b`, `0x31` and `0x3a`. The instrumented objects agree
with stock output except for the COFF timestamp.

I independently reran `test_analyse_receipts.py`: **9 passed**. Its negative
fixtures reject unequal trace lengths and forged load bytes, symbol offsets,
relocations, duplicate relocation anchors and unsupported instruction shapes.

**Conclusion:** the updated bounded claims and `measurements.json` are accurate.
We now know which semantic global fields exchange their final load order while
the selected integer-chooser projections remain equal. We do not know the
earliest pass that changes that order, nor that all forty allocations preserve
value identity. Items 3 and 7 still delimit scope: six code-owner controls are
not a full data/ownership/consumer admission sweep. Credit remains **zero**.

The next useful research step is to trace these same two identified memory
symbols backward from encoder hook `0x10751347` to the first list/order
divergence. No declaration-count sweep or forced register result is justified
as a source fix by these measurements.
