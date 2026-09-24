# MEASURED - owner_queue/objlights_render_debug (base cdc8ebd3, Halo objects 379)

All numbers are strict `coff_compare.section_infos_equal` rows unless stated. Scorer objdiff-cli 3.3.1.
Tools are slug-local copies (tools/), each read before use; all outputs are in this directory.

## Controls
| control | result |
|---|---|
| csplit with an unmodified config copy (cfg_ctrl) vs build/split | 833/833 objects byte-identical |
| shadow tree tree0 (copy of source/ at cdc8ebd3) | object_lights 43/43, render_debug 36/36; objcmp vs build/base IDENTICAL |
| git status | clean before and after (0 entries) |

## Emulated split
`cfg_OL` = config/ + 4 in-place `"static": true` rows (patch 02). csplit changes ONLY
`source/objects/object_lights.obj` (splitcmp 1/833). render_debug needs no symbols.json change (D1 lab only:
`lab_cfg_RDd`, changes only render_debug.obj).

## gate --all, every January function of every affected object (treeFINAL = cdc8ebd3 + 01,02,03_YES,04_YES,05,06,07)
Built from the patch files only; files byte-equal to the measured trees. `final/gate_all.txt`.

| object | rows | vs production | notes |
|---|---|---|---|
| objects/object_lights | 43/43 EXACT | 43/43 | vs cfg_OL_split |
| render/render_debug | 36/36 EXACT | 36/36 | |
| render/render (object_lights_rendering.h) | 13/13 | objcmp IDENTICAL | |
| render/render_objects (object_lights_rendering.h) | 22/22 | objcmp IDENTICAL | |
| ai/actor_moving (vehicle_datum.h) | 31/36 | objcmp IDENTICAL | residuals pre-existing |
| physics/physics (vehicle_datum.h) | 13/17 | objcmp IDENTICAL | residuals pre-existing |
| units/vehicles (vehicle_datum.h) | 37/39 | objcmp IDENTICAL | residuals pre-existing |
| ai/action_vehicle (selected bounding-sphere provider) | 17/17 | objcmp IDENTICAL | unchanged source |

Includer sets are complete: object_lights_rendering.h is included only by object_lights.c, render.c,
render_objects.c; vehicle_datum.h only by actor_moving.c, physics.c, vehicles.c; no header includes either.
0 exact rows lost, 0 moved rows.

## object_audit (emulated split)
| object | result |
|---|---|
| object_lights (final) vs cfg_OL_split | PASS: all January-owned sections ok, 71 January symbols, 0 differ |
| object_lights PRODUCTION vs cfg_OL_split (control) | FAIL(4): exactly the 4 storage rows (3/2) that 01+02 fix |
| render_debug (final) vs build/split | PASS: 63 January symbols, 0 differ |

## Storage (cachebeta publics)
object_lights 71 split symbols / 0 disagreements; render_debug 63 / 0.

## Surplus identity + selected-provider link (both orders)
| object | candidate-only code COMDATs | not identical | provider_link |
|---|---|---|---|
| object_lights | 12 | 1: `_object_get_bounding_sphere` vs action_vehicle (the Q1 item) | PASS (33 surplus names incl. the bounding sphere; selection ANY both sides) |
| render_debug | 12 (production 11; new `_point_from_line3d` == action_charge selected copy) | 0 | PASS |

`_object_get_bounding_sphere` detail (bsphere_selected_vs_OL.txt): size 144/144, normalized sha EQUAL, 9/9
relocations, 7/9 identical; ONLY the DIR32 rows at +37 and +76 differ:
`??_C@_0BF@EIEPJPCC@?4?4?2objects?2objects?4h?$AA@` ("..\objects\objects.h", selected) vs
`??_C@_0CB@NDAECIIM@c?3?2halo?2source?2objects?2objects?4h@` ("c:\halo\source\objects\objects.h", ours = January
object_lights' own 33-byte .rdata; object_audit requires it).

Rule-6 exception for render_debug's new `_point_from_line3d`: identical to the selected copy; callers
render_debug_vector/tick/line_offset strictly exact; ownership/build sweep above has 0 regressions; selected-provider
link PASS; genuine real_math.h `__inline`, no hand expansion (all-inlined class admitted by ruling 20260921 #5).

## Data
| object | production | with packet |
|---|---|---|
| object_lights | 1104/2645 (.data 1541 B at 99.74%: '$'-literal defect) | 2645/2645 with 03_YES (+complete) or 03_NO_ALT (allow_incomplete_unit); +1,541 B |
| render_debug | 30236/30236 | unchanged (also under D1) |

Semantic entry verified by running `tools.semantic_progress.apply_semantic_data_matches` on a mini objdiff 3.3.1
project (verify_semantic_entry.py): `_lights_section` 1541 B, 1 relocation, normalized sha 527b43a1...,
target == base resolved; YES entry + complete -> +1541; NO_ALT -> +1541; negative control (YES entry, unit not
complete) -> SemanticProgressError. Mini objdiff code: object_lights 12294/12294 (43/43); render_debug 8235/9448
(34/36 raw: `_render_debug_add_cache_entry` 93.6 and `_render_debug` 94.2, the known under-credit rows already in
semantic_report hidden_exact) - identical to production for every variant (RD, RDe, D1 lab).

## (e) tentative definitions (patches 06, 07)
| measure | render_debug (06) | object_lights (07) |
|---|---|---|
| gate | 36/36 | 43/43 |
| objcmp vs the non-(e) candidate | IDENTICAL | IDENTICAL |
| only delta | 6 external section-0 symbols value 0 -> 1 (COMMON) | 5 symbols value 0 -> 1,1,2,12,4 (COMMON) |
| sizes vs January linker_common records | 1 x6 = records 50..55 | = records 108,109,110,112,113 |
| link with January linker_common.obj, both orders | 0 LNK2005/LNK1169 | 0 |
| link with base consumers (hs_globals_external [+ render_objects]) both orders | 0 duplicates; all watched symbols resolved (production: 12 unresolved lines) | 0 duplicates; all resolved (production: 8 unresolved lines) |

## (d) lab (never lands; lab/)
| variant | gate vs split | object_audit |
|---|---|---|
| aggregate (production / 05) | 36/36 vs build/split | PASS |
| D1: 7 separate `= 0` statics, declaration order, lab symbols.json (rename + 6 static rows) | 36/36 vs lab split | PASS, 69 January symbols 0 differ |
| D1 against the unmodified split | 33/36 (reloc NAME only; sha and destinations equal) | - |
| uninitialised separate statics | 36/36 (name-matched relocs) | FAIL(7): .bss 29715 B, name-hash order |
Plain-CL lab: `= 0` statics in member order give January's exact offsets (0, 0x400, 0x7400, 0x7404, 0x7408, 0x740A,
0x740B; 29708 B); uninitialised ones do not.

## Hygiene scan (advisory, not gates)
- fake_match_scan: 0 leads on object_lights.c/.h, render_debug.c, vehicle_datum.h (final) and the D1 lab file.
- No consumer-local non-static prototypes in either .c; no `float`; no caller-local views after 05.
- /W3: render_debug has 6 C4013 implicit declarations (ai_debug_render, collision_debug_render,
  render_debug_object_damage, texture_cache_debug_render, render_debug_recording, render_debug_fog_planes); present
  in production; precedent render.c (Matching) has 2. object_lights: pre-existing C4133 (line 853 -> 855) and C4244s.
- render_debug.c keeps a stale "symbols in this file" comment with _code_/_bss_ placeholders (admitted render_objects
  and path carry the same kind of comment).

## Objects that become COMPLETE
| ruling | object | credit |
|---|---|---|
| Q1 YES (01,02,03_YES,04_YES) | source/objects/object_lights | +1 object, +1,541 data |
| Q1 NO (01,02,03_NO_ALT) | none | +1,541 data (allow_incomplete_unit) |
| Q2d D0 or D1 YES (05 [+06] [+D1 pair] + 08) | source/render/render_debug | +1 object (functions and data already full; objdiff's $L-label credit for its 2 under-credit rows is scorer-only) |
| Q2d HOLD | none | 05/06 still land at zero credit |
No function is newly exact in either item (all 43 + 36 are already exact).
