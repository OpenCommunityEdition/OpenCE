# `hs_runtime.obj` park re-investigation, Opus5 150K lane wave w1 (2026-09-14)

## Scope and baseline

- Unit `source/hs/hs_runtime`. Worker-owned file: `source/hs/hs_runtime.c`.
- Baseline real-file gate (with the `_point_from_line3d` guard): **60 exact / 6 residual / 0 unwritten**. The guard passed.
- All six residuals are parked in `config/parked.json`:
  - `_hs_inspect_enum` (144, csplit-relocation-alias)
  - `_hs_evaluate_begin_random` (528, private-register-convention)
  - `_hs_global_reconcile_read` (1,408, instruction-scheduling)
  - `_hs_global_reconcile_write` (400 vs 416, instruction-scheduling)
  - `_hs_evaluate_inequality` (528, instruction-scheduling)
  - `_render_debug_trigger_volumes` (1,088, unclassified)
- Final real-file gate: unchanged at 60 / 6 / 0. `git diff -- source/hs/hs_runtime.c` is empty, and park drift is 0.

Ledgers read before work:
- every `hs_runtime_obj_*` ledger;
- `hs_runtime_inequality_reconciliation_20260908`;
- `hs_runtime_interface_leaf_reconciliation_20260908`;
- `claude_remaining_donor_batch_20260908`;
- `fable_bink_ui_network_hs_reconciliation_20260908`;
- `small_unwritten_threshold_wave_main_hs_ai_20260913`;
- the 100K handoff.

`branch_sweep` found no blob better than the current file.

## Outcome summary

| function | outcome |
| --- | --- |
| `_hs_evaluate_inequality` | **PARK-REOPEN-PROPOSED**: strict EXACT in a scratch candidate |
| `_hs_global_reconcile_read` | not landed; prologue global-load scheduling tie |
| `_hs_global_reconcile_write` | not landed; the same prologue tie, and the 16-byte size gap is one byte crossing jump-table alignment |
| `_hs_evaluate_begin_random` | not landed; loop-invariant sign-extension register tie (4 ModRM bytes) |
| `_render_debug_trigger_volumes` | not landed; frame-allocation priority fixed point; new PDB local evidence recorded |
| `_hs_inspect_enum` | skipped; csplit relocation alias (target-recovery limit) |

## `_hs_evaluate_inequality`: the macro-line proof

**Residual.** January and the current body differ in exactly one place: the real-typed arm.
- January loads the second operand through the x87: `fld [esi+4]` ... `fstp [ebp+8]`.
- VC7 copies it as an integer: `mov edx,[esi+4]` ... `mov [ebp+8],edx`.
- The other 185 instructions are equal.

**New evidence.** The three default-case assertions of the per-type comparison switches carry January lines:
- `0x16b` (363);
- `0x16e` (366);
- `0x172` (370), with the operand-type assertion at `0x171` (369) immediately before it.

Three lines per outer case (`case`, one statement, `break`) cannot hold two assignments plus a four-case switch. Each comparison switch was therefore a **single-line macro invocation** in `hs_library_internal_runtime.h`, the file named by the assertions.

**Mechanism.** A function-like macro parenthesises its parameters. The measured PAREN/FPU law then routes the real operand through the x87. A mechanism probe that only parenthesised the two real assignments became exact. That probe is not admissible on its own, because parentheses without a genuine macro are prohibited.

**Candidate.** `scratch/workers/hs_runtime/reopen_inequality.c` is the current real file plus two changes:
- A TU-local statement macro in the macros section. It takes `(first, second, line)`, assigns `value0 = (first)` and `value1 = (second)`, then runs the ordered-comparison switch with `comparison = FALSE` and the per-expansion `match_vassert` default.
- The three case bodies of `hs_evaluate_inequality` replaced by `HS_EVALUATE_INEQUALITY(..., 0x16b|0x16e|0x172);`.

The `line` parameter follows this file's existing `match_hs_assert(file, line, ...)` convention.

**Checks.**
- Whole-TU gate: 61 / 5 / 0. `_hs_evaluate_inequality` is strict EXACT at 528 bytes, 40 relocations, sha `63ee22c1451726f6`.
- All 60 baseline-exact rows are preserved.
- The point guard passes.
- `parkcheck`: the other five parks are OK; inequality drifts only to the target hash.
- Owner census against `build/base` and the target: no new or dropped owners.
- `fake_match_scan`: 0 leads.

**Rejected shapes.**
- Block-scoped initialised locals: inert, plus a register ripple.
- A macro that compares `(first)`/`(second)` without named locals: 688 bytes.
- The earlier real-to-double-to-real round trip stays rejected and was not retried.

**Park criterion.** The park premise was that only a redundant double round trip reproduces January's x87 lifetime. That premise is refuted:
- The exact form has no widening, narrowing or redundant store.
- The x87 route comes from ordinary macro-parameter parenthesisation.
- January's own assertion line numbers prove the macro.

## `_hs_global_reconcile_read` / `_hs_global_reconcile_write`

**Residual.** Both functions differ only in the if-block prologue:
- January computes `param & 0x7fff` and pushes it, then loads `_hs_global_data` into EAX.
- VC7 hoists the global load into ECX at block entry, and the second masked index then uses EDX.

For write, the extra byte (`mov ecx,[m]` is 6 bytes, `mov eax,[m]` is 5) pushes the jump table across a 4-byte alignment boundary. That single byte is the entire 416-vs-400 size gap.

The decoded write jump table maps all 39 cases onto the same 8 cross-jumped blocks in both builds, so the case bodies already reproduce January.

**Evidence.** HCEX `HCEX.pdb` gives `static void hs_global_reconcile_read(short)` and `...write(short)` with the parameter `short global_designator`.

**Shapes tried (all scratch).** Nine shapes; none is exact:
1. A `long` index local: the index is enregistered, which is worse.
2. `short` parameter with `TEST_FLAG(short)`: January's load order, but an extra `movsx` / `test ah,ah` that January does not have.
3. `short` parameter with a `(word)` test: baseline schedule.
4. Block-scoped initialised declarations: inert.
5. `short` parameter with the `hs.c` idiom `& 0x8000` / `& 0x7fff`: baseline schedule.
6. `word` parameter with an unparenthesised mask: baseline schedule.
7. `short` parameter, `(word)` test and an unparenthesised mask: baseline schedule.
8. `datum_get((hs_global_data), ...)` paren probe: inert.
9. `short` parameter with `designator<0`: `test di,di / jge`, not January.

**Reopen criterion.** A same-compiler donor showing the non-hoisted `datum_get(global, param & mask)` argument order.

## `_hs_evaluate_begin_random`

**Residual.** Instructions match 191/191. The only difference is the loop-preheader invariant sign extensions:
- January: `movsx eax,ax` / `movsx ecx,cx`.
- Ours: EDX / EAX.

**Shapes tried.** The `index+random` operand order and a function-scope `choice` were both inert.

**Reopen criterion.** An authoritative local record or a same-compiler donor.

## `_render_debug_trigger_volumes`

**New evidence.** `HCEX_Release.pdb` (`DIA2Dump -sym`) lists these locals:
- `real_matrix4x3 matrix`
- `real_vector3d local_extent`
- `real_vector3d world_extent`
- `real_vector3d sides[2]`
- `collision_result result`
- `real_vector3d ray`

`sides[2]` explains January's contiguous 24-byte zero-init: five register stores plus one immediate first-member store.

**Candidate `tv1`.** PDB names plus `real_vector3d sides[2] = { 0 }` in the edge loop.
- The zero-init pattern now matches January.
- The frame layout still follows descending emitted reference density exactly (`local_extent` 1.167 > `sides` 1.125 > ...).
- January instead ranks `sides` above `local_extent`, `points` above the 0.75-density spill scalars, and the `volume` spill inside the 0.75 group.

This confirms the recorded frame-allocation-priority fixed point: the ranking input differs in something invisible in the emitted stream.

**Other probes.**
- Scoping `points` and `color`, and renaming `center`: byte-identical, so inert.
- `point_from_line3d` helper spelling: fixes the side-branch x87 operand order, but emits the forbidden `_point_from_line3d` COMDAT and leaves the layout unchanged. Rejected.

**Reopen criterion.** A donor explaining VC7's frame ranking input, optionally combined with the `_point_from_line3d` owner ruling. If this park is ever re-baselined, `scratch/workers/hs_runtime/tv1.txt` is the better-evidenced spelling, at zero credit.

## `_hs_inspect_enum`

**Residual.** Normalized bytes are identical. The only difference is relocation `@0013`:
- January: `__hs_type_scenery_default+12912`.
- Ours: `_hs_enum_table-256`.

These are the same image address. The csplit relocation-alias class is confirmed unrepairable from `config/`, so this function was not touched.

## Proposals

- **Orchestrator: unpark `_hs_evaluate_inequality`** (dedicated commit) after verifying `scratch/workers/hs_runtime/reopen_inequality.c`. This means removing its `config/parked.json` entry and applying the macro plus the three case-body replacements to `source/hs/hs_runtime.c`.
- **No header or config edits requested.** The macro is TU-local.
