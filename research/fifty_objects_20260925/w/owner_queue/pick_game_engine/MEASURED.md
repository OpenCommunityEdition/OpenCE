# MEASURED - owner_queue/pick_game_engine (base cdc8ebd3, Halo objects 379)

Everything below was measured in this directory. The tree was clean before and after (`git status` empty).
The step-by-step record is in LEDGER.md (S0-S8). The tools are in tools/; they only write under this slug.

## 1. Patch application at cdc8ebd3

| Packet | Patches | `git apply --check`, each alone | Full set, one call | Rebuilt from a HEAD export |
|---|---|---|---|---|
| optionA (alias) | 01 F01R, 02 F02, 03 F03aR, 04 F03b, 05 F04, 06 status flip | clean | clean | equals the measured shadow `optA`, every file |
| optionB (view copy) | 01 F01RB, 02-06 as A | clean | clean | equals `optB_seq` |
| no-single-consumer variant, alias | 01A, 02 F02-NP, 03-06 | clean | clean | equals `vTUnp` |
| no-single-consumer variant, view copy | 01B, 02 F02-NP, 03-06 | clean | clean | equals `vTUnpB` |

- The tree moved since review4's base (16542e46). 466698b8 edited hud_nav_points.c, but F04 still applies unchanged.
- F07 alone does not `--check` at cdc8ebd3, because it needs F01R. optionB therefore ships a standalone F01RB,
  which equals F01R plus F07 (byte-checked).
- F01R, F02, F03aR, F03b and F04 are the reviewed patches, byte-for-byte.
- These patches are new, generated with tools/pgen.py from the HEAD blobs: F01RB, F01R-NP, F01RB-NP, F02-NP and the
  status flip.

## 2. Functions (every January function of the affected object)

The compile harness uses the production cflags from build.ninja and runs in the shadow root. The comparison is
tools/gatecmp.py, which uses `coff_compare.section_infos_equal` against build/split and build/base.

| Build | vs build/split | vs build/base (302 section-defining symbols) |
|---|---|---|
| head (control) | 180/180 EXACT | 0 changed |
| optionA | 180/180 EXACT | 0 changed |
| optionB | 180/180 EXACT | 0 changed |
| no-single-consumer variant, A and B | 180/180 EXACT | 0 changed |
| lab: strip test (neither alias nor cast) | **179/180**: custom_motion_sensor_positions RESIDUAL | 1 changed |
| lab: vTU (nav-point trio in genuine hud.h) | **179/180**: _populate_statistic_buffer RESIDUAL | 1 changed |

The optionA and optionB objects are **identical**: same section order and no content difference (tools/gediff.py).
The pick changes no byte of the object.

gate.py `--source` cannot compile these packets, because its /I list points at the tree's unpatched headers.
gatecmp uses the same comparator.

## 3. Full sweep, all 447 build/base TUs (tools/buildall.py + tools/cmpdirs.py)

The comparator is canonical COFF: every section except .debug$S (raw bytes, plus relocations by target name) and
the full symbol table. $L and $label$N numbering is canonicalised.

- **Control.** The head shadow matches build/base for 447/447 TUs (SAME). build/base is current at cdc8ebd3.
- **optionA.** 446 SAME. The only difference is game_engine.
- **optionB.** 446 SAME. The only difference is game_engine.
- **Variants.** optionA vs no-single-consumer variant: 447/447 SAME. vTUnp vs vTUnpB differ only in game_engine.c,
  and their game_engine objects are identical.

What differs in game_engine (all strictly EXACT):

- `_game_engine_update_player_always_invis` is emitted later. It now calls game_engine_test_trait, which is defined
  later in the file, so C2 defers it.
- The raw relocation at custom_motion_sensor_positions+0x83 and render_nav_points+0xba is spelled
  `_global_goal` + 0x400 instead of `_global_variant` + 0. This is the csplit relocation alias: both spellings
  resolve to the same .bss byte.
- $L numbering changes.

**game_engine.h consumer sweep.** /showIncludes over the patched shadow finds 37 game_engine.h consumers. All 37
are inside the full sweep: 36 SAME, plus game_engine as noted above. The other edited headers and their consumers:

| Header | Consumer TUs |
|---|---|
| game_engine.h | 37 |
| player_control.h | 21 |
| hud_messaging.h | 8 |
| sound_classes.h | 6 |
| hud_globals_definitions.h | 1 |
| hud_nav_points.h | 2 |
| item_collection_definitions.h | 1 |
| scenario_netgame_definitions.h | 1 |
| players.h (not edited by the packet) | 71 |

## 4. Whole-object admission battery (optionA, optionB, vTUnp and vTUnpB objects give identical results)

| Check | Result |
|---|---|
| scratch/tools/object_audit.py | PASS. The text is identical to production's audit (obj path aside). |
| pdb_storage on the candidate's symbols | 269 split symbols, 0 disagreements with cachebeta publics. |
| surplus_identity on the candidate | 11 candidate-only code COMDATs, 0 not identical. Same set as production. |
| scratch/tools/provider_link.py | SELECTED-PROVIDER LINK PASS, both orders, 39 surplus externals. |
| objdiff-cli 3.3.1 mini project (odproj/) | 27922/32397 code, 173/180 functions, 3792/3792 data. Same as prod, 0 per-function deltas. |
| `python -B -m tools.audit_semantic_matches` on odproj | accepted_ledger 180/180 per candidate. The same 7 functions are semantic-coff-only as in production's build/semantic_report.json: nonplayer_post_rasterize, post_rasterize, remap_vehicle, remap_weapon, update, internal_rasterize_score and populate_statistic_buffer. |
| tools/audit_object_admission.py at HEAD (read-only) | game_engine is already a CANDIDATE (function gap 0, data gap 0 in the strict report). The status flip will not be revoked. |
| Parks / rejections | none for source/game/game_engine. |
| Data | .data 32/32 and .bss 1148/1148 strict exact; objdiff 3792/3792. No data section changes. |

**/W3 /Zs census, 447 TUs (head vs optionA).** Two TUs change.

- hud_nav_points: the pre-existing C4013 `error` warning only moves from line 351 to line 346.
- game_engine:
  - the C4013 `console_printf` warning is gone;
  - C4244 long->short drops from 6 to 4;
  - double->float becomes double->real;
  - there are **no new warnings**.

The no-single-consumer variant has the same 6 game_engine warnings, modulo line numbers.

## 5. Owner ruling 2026-09-21 #3 ("no one-consumer header") applied to F02: measurements

| Header (F02) | Consumers | Genuine home tried | Result at cdc8ebd3 |
|---|---|---|---|
| hud_globals_definitions.h | 1 | hud_definitions.h / hud.h | Does not compile without a HUD-wide rewrite. 7 TUs define their own `struct hud_globals_definition`, and main.c and interface.c declare `hud_globals` with other types. |
| item_collection_definitions.h | 1 | item_definitions.h (vGenI) | hud_weapon `_render_weapon_hud` moves. It is non-exact and not parked: 4 bytes change and alndiff gets slightly worse (794 -> 796 differing lines). Not zero-regression. |
| scenario_netgame_definitions.h | 1 (5 with F06) | scenario_definitions.h plus F06 siblings (vGenS) | **Strict regression:** ai_communication `_ai_communication_update_speech_timers` goes from EXACT to residual. The PARKED `_ai_test_ballistic_line_of_fire` moves (the park goes stale), and hud_weapon moves. |
| hud_nav_points.h | 2 (game_engine plus its owner hud_nav_points.c) | hud.h (vTU) | No other TU moves. But game_engine.c must then include hud.h (23 new names), and **game_engine's own `_populate_statistic_buffer` goes residual**. Count oracle, lab only: K = 8 to 12 dummy names flip it, while K = 1 to 7 and 13 to 40 do not. This is a declared-name-count tie (held Layer-2 class). |
| **Feasible form:** all three single-consumer layouts TU-local in game_engine.c (vTUnp) | - | - | 180/180. Byte-identical to optionA in all 447 TUs. Audit, pdb, surplus and link all PASS. |

## 6. Objects that become COMPLETE if the owner says yes

The owner must say yes to exactly one pick. Either pick, with or without the variant, does this:

- **source/game/game_engine** (180 functions; 32,397 code bytes; 3,792 data bytes).
- Halo objects go from 379 to 380. No other object changes state: the sweep shows 446 SAME.

Objects that still would not be COMPLETE:

- game_engine_king: held separately, in the authentic-bug class.
- F06 is optional and not part of either packet.

## 7. Risks the integrator must re-check at landing

- **Full ninja plus stable_verdicts.** This harness reproduces build/base exactly (control 447/447), but the official
  gate is still the full build.
- **Concurrent owner-queue lanes.** Any lane that edits game_engine.h, player_control.h, hud_messaging.h,
  sound_classes.h, hud.h or game_engine_king.c must be re-swept together with this packet. These headers are
  count-sensitive: vGen moved ai, ai_communication and hud_weapon.
- **The admission also carries a pre-existing construct that neither review examined.** The `debug_player_color`
  aggregate has `word pad` members. RULING.md, question 3, covers it.
