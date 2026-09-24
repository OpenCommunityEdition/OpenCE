# ai/actions worker ledger (wave 2, fifty-objects lane, base 931ed8dc)

Target: close `_actor_action_handle_vehicle_entry` (960, park register-allocation) + audit 2404 unmatched data bytes.

## Prior art read
- docs/object_matching_logs/actions_obj*.md (all), astra review, opus5 waveD/w2/w3, lane B handoff row, structural owner packet row.
- Structural lane (C:\halo-worktrees\claude-structural-20260923\scratch\lane\w\ai__actions): EXACT 63/63 with
  Lane B od11 body + `goto result_exit` guard (g4) + TU-private macro actor_action_distance_squared ((d)*(d)) at the prop square.
  Strip test: plain `prop->distance * prop->distance` residual; `(d) * (d)` (single paren) ALSO exact. Owner-gated (packet 20260923 section 7).
- Recorded negatives: V1-V5, VE1-VE9, od1-od11, guard forms g1/g3/g5/g6/g7, double temp, named real temp, decl order, counter widths.

## Probe 0 (baseline, 931ed8dc production)
- gate --all: `== exact 62 residual 1` ; vehicle_entry residual [sha] 960/22.
- alndiff: (a) transposition @0x17b `mov [ebp-4],ecx` before `fmul st(1)` (January after); (b) false-epilogue topology
  (ours `mov dl,1 .. mov al,dl`, January `mov al,1` + separate `mov al,dl` guard epilogue at 0x3b1).
- object_audit: only DIFF row is vehicle_entry; 112 January symbols, 0 differ; data sections
  _global_action_functions (888/96) and _global_actor_mode_names (16/3) coff-identical.

## Probe 1: structural-lane candidate (od11 body + goto guard + TU macro) on this tree
- Production actions.c == structural lane baseline.c byte-for-byte, so candidate applies unchanged.
- gate: `== exact 63 residual 0` (struct_cand.c / struct_cand_gate.txt). Reproduced.
## Probe 2: strip test (same body, plain `prop->distance * prop->distance`)
- gate: residual [sha]; alndiff = ONLY the 0x17b transposition (epilogue fixed by od11+g4 body). strip_plain.c.
  => the od11+g4 body closes the epilogue family with ordinary C; the square spelling alone decides the store slot.

## Probe 3: mechanism battery (diagnostic only, NOT landable) - what IL shape does January's square have?
Hypothesis: the operand parentheses are an IL-visible node in VC7 c1; if so an UNMATCHED paren must defeat the load CSE.
Base = strip_plain.c (od11+g4 body); only the prop-arm square statement varies (mkvar.py / run.sh):
| var | spelling | result |
| d1 | `(d) * d` | residual, NO CSE: `fld d; fmul dword [esi+0x11c]` (+jmp, 6 hunks) |
| d2 | `d * (d)` | same as d1 |
| d3 | `(d * d)` (outer only) | = bare (3 hunks: the 0x17b transposition) |
| d5 | `(real)d * (real)d` (no-op cast) | = bare (cast dropped by c1) |
| d9 | `best = d; best *= d;` | NO CSE (as d1) |
| d10 | `best = d; best *= best;` | = bare |
| struct A2/A3 | `(d) * (d)` / `((d) * (d))` | EXACT |
Finding (new): a paren around ONE operand kills the CSE of the two loads, so each paren is a distinct IL node;
with BOTH operands wrapped the two wrapper nodes are CSE'd as a unit and the dup (`fld st(0)`) is of the wrapper,
which the list scheduler orders fmul-before-store exactly as January. January's IL therefore had an IDENTICAL
wrapper on both operands - the signature of one macro argument substituted twice (`((x) * (x))`). No non-macro
spelling (no-op cast, RMW forms, outer paren, named temp VE-od6) reproduces it. Same family as the owner-admitted
actor_perception_distance_squared (ruling 2026-09-20) and actor_moving sideslip (structural A6).
/Od (0x42ade6) shows `mov eax,[ebp-0x38]; mov ecx,[ebp-0x38]; movss [eax+0xf8]; mulss [ecx+0xf8]` - consistent with
macro or bare (macros are invisible in /Od); no helper call, so not an __inline function.

## Probe 4: landable fuzzy candidate cand_fuzzy.c = production + od11/g4 body, plain square, NO macro
- gate: `== exact 62 residual 1`; alndiff: only the 0x17b transposition (5 bytes, 300/300 insns). Production had
  2 families (transposition + false-epilogue topology). secdiff vs production: only vehicle_entry section changes.
- fake_match_scan: 0 leads (cand_fuzzy.c, struct_cand.c, production).
## Section-4 audit of the owner-gated exact candidate (struct_cand.obj)
- gate 63/63 EXACT; object_audit PASS (all January sections/symbols, 112 symbols 0 differ);
- pdb_storage (production symbols == candidate symbols): 0 disagreements; surplus_identity: 11 COMDATs, 0 not identical;
- provider_link: SELECTED-PROVIDER LINK: PASS (26 surplus incl. strings/reals/_distance3d via action_vehicle);
- no object_admission_rejections entry; parks: only _actor_action_handle_vehicle_entry (retire on landing).

## Data audit (2404 unmatched bytes)
- Both January data owners are coff-identical (section_info_resolved equal): _global_actor_mode_names .data 16 B / 3 relocs
  sha 374708ff...; the 888-byte .rdata table (_global_action_functions@0, _global_dive_animation_table@784,
  _global_state_move_position_orders@840, _global_default_states@864) 96 relocs sha b4bd37bc...; all 37 string COMDATs and
  7 float literals identical (object_audit). One-unit objdiff 3.3.1 report (od331/mk.py) reproduces build/report.json:
  .data 57.14286, .rdata 98.38118 -> cause = the '$'-literal relocation-name defect (3 + 14 relocs to ??_C@..?$AA@ strings).
- Credit path under 3.3.1: a single-section entry cannot work (verifier requires credited == whole-unit unmatched 2404 and
  the report has two data sections). A grouped entry (all 46 January data sections, datasim/manifest.json) FAILS the current
  verifier: objdiff's combined .rdata extent aligns the RUNNING offset to max(align,4) after each section
  (read.rs do_combine_sections) = 2388, while the verifier sums per-member padded sizes = 2396 (raw 2322).
  Simulated with the real apply_semantic_data_matches: "semantic data group does not cover the reported unmatched sections".
- Proposed verifier fix scratch/w/actions/verifier_combined_extent.diff (tools/semantic_progress.py): for padded grouped entries
  compute the per-section extent exactly as objdiff does (members must be every target section of that name, table order,
  running offset aligned to max(align,4)); plus skip the coverage comparison when the unit has no unmatched data (3.6.0-safe,
  owner note 2 of claude_data_ownership_closeout_20260924). Tests: tools/test_semantic_progress.py 28 passed on the patched copy;
  all 36 existing credited entries byte-identical results (shell_xbox grouped still +32); actions grouped entry
  (semantic_data_entry.json) credits exactly +2404 under 3.3.1 and credits nothing / raises nothing on a 3.6.0-like report.
- Admission simulation (patched verifier + entry + owner-gated exact candidate, unit marked complete):
  source/ai/actions -> function_gap 0, data_gap 0, decision audit-coff-ownership-before-admission (16 candidates, 0 contradicted).

## Deliverables
- production_fuzzy.patch (landable now): actions.c od11/g4 body with plain square + parked.json measurement/evidence update
  (base sha 7ef7a0e0..., objdiff 99.319725). parked_functions on a patched report/config: 93 active, 0 stale, 0 invalid.
- owner_gated_exact.patch (HELD): actions.c + TU-private actor_action_distance_squared macro (updated comment with the
  paren/CSE mechanism) + removal of the vehicle_entry park. gate 63/63; parks 92/0/0.
- Both patches: git apply --check clean; round-trip byte-identical to cand_fuzzy.c / cand_exact.c and config copies.

## Stop rule
Function: every structural family is closed by ordinary /Od-attested C except the square spelling; six diagnostic
spellings map the mechanism (symmetric operand wrapper) and none non-macro reproduces it; prior lanes spent V1-V5,
VE1-VE9, od1-od11, g1-g7. Stop. Reopen: an owner ruling on the TU-private squaring macro (or a first-party squaring
macro in a shared header), or new first-party text for this statement.
