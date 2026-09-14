# `ai_communication.obj` — Opus5 100K consolidated lane, wave D park re-investigation (2026-09-14)

## Verdict

- Real file `source/ai/ai_communication.c` **unchanged** (all targets were
  LOCKED parks; the non-parked residuals were either owner-blocked or had no
  exact-capable change). Final real-file gate: `== exact 40 residual 8
  unwritten 0`, point guard passed, all six parks equal `measurements.base`.
- **Park-reopen proposal (strict EXACT in scratch):**
  `_ai_conversation_find_participant` 2048/85, normalized SHA-256
  `344df5e9415b7b470e528153f3097ef3ff894d72ef2253625e61af4c1ecafdeb` == target;
  whole-TU scratch gate 41 exact / 7 residual; no sibling or park loss.
- **Park-remeasure proposals (zero credit, much closer fuzzy bodies):**
  `_ai_conversation_begin` 1648/53 -> 1744/54 (size and relocations now equal
  target; 96.3% aligned instructions; only a frame-slot rank tie remains) and
  `_ai_communication_finished` 896/29 -> 1568/86 (target 1584/87; 24.4% ->
  85.3% aligned; January debug-string graph reconstructed).

## 1. Scope and provenance

- Unit `source/ai/ai_communication.c`, target `build/split/source/ai/ai_communication.obj`.
- VC7 13.00.9254 (XDK 3911), `/O2 /Oy- /DDEBUG /Dxbox` (build.ninja).
- Evidence: January COFF disassembly of each target function
  (`scratch/workersD/ai_communication/{begin_jan,fp_jan,fin_jan,tw_jan}.txt`),
  January frame layouts and relocation multisets, January assertion anchors,
  HCEA `ai_conversation_begin.c`, `ai_conversation_find_participant.c`,
  `ai_communication_finished.c` (semantics/topology only), prior ledgers
  (`lane_reconciliation_batch5_fuzzy_ai_bitmap_20260909`,
  `ai_communication_obj_private_speech_graph_20260909`,
  `ai_communication_obj_jonas_speech_research_wave8_20260828`,
  `claude_fable_opus_reconciliation_checkpoint_20260908`,
  `ai_communication_obj_opus5_100k_20260914` + wave A worker report).

## 2. Validated baseline (lane HEAD 548451f6d)

`== exact 40  residual 8  unwritten 0`.

| Function | Target size/relocs | Base size/relocs | Status |
| --- | --- | --- | --- |
| `_ai_communication_event` | 8064/336 | 7776/319 | fuzzy residual (wave A) |
| `_ai_communication_find_actor_to_reply_to_player` | 1104/65 | 1088/63 | needs units.h/units.c (P1) |
| `_ai_communication_finished` | 1584/87 | 896/29 | PARKED |
| `_ai_conversation_begin` | 1744/54 | 1648/53 | PARKED |
| `_ai_conversation_find_participant` | 2048/85 | 2032/85 | PARKED |
| `_ai_communication_actor_talk_weight` | 912/24 | 912/24 | PARKED (sha) |
| `_ai_communication_get_player_rating` | 832/49 | 832/49 | PARKED (sha) |
| `_ai_communication_update_speech_timers` | 672/43 | 672/43 | PARKED (sha) |

## 3. Accepted controls (scratch candidates, not in the real file)

### `ai_conversation_find_participant` — strict EXACT (`scratch/workersD/ai_communication/park_reopen_find_participant.c`)

Every change is required (single-edit ablation regresses to `[sha]` or size)
except where noted:

- `selection_valid` flag: selection-type cases set `selection_valid = FALSE`
  and one post-switch `if (!selection_valid) { rejection_counts[4]++; continue; }`
  (January has exactly one reject[4] increment site reached from friendly,
  in-vehicle and not-in-vehicle; HCEA's lift also carries `valid`). This alone
  fixed the case-block layout/cross-jumps **and** the frame slots
  (rejection_counts [ebp-0x34], nearest [ebp-0x24]).
- `player_rating` distinct from `candidate_rating`; the rating is copied into
  the candidate only when a player target exists; the not-near-player test uses
  `player_rating < 2.0f` (January [ebp-0x18] vs [ebp-0xc]; HCEA agrees).
- `first_participant = nearby_unit_count == 0` computed once after the
  placed-participant scan (January `sete [ebp-0x5c]` outside the loop).
- `char reason_string[512]` (January frame 0x41c: reason_string [ebp-0x41c]
  abuts ai_name [ebp-0x21c]).
- `short possible_actor_count` (January `movsx eax,si`, `test si,si`).
- Variant selection `found / count > 0 { ==1 ? [0] : random; 0x13C7 assert } /
  reject[6]` (January's found path skips the 0x13C7 assert).
- `distance_squared3d(&candidate body, &nearby_unit_positions[i])` and
  `nearest_distance_squared > distance_squared`; `candidate_actor_index ==
  conversation->actor_indices[i]`.
- January init-store orders: outer `selected_variant_index`,
  `selected_actor_index`; inner flags, `candidate_object_index`, `best_rating`,
  `possible_actor_count`; `nearby_unit_count = 0` at the scan loop; loop-top
  `candidate_rating`, `candidate_player_distance`, `player_target_unit`; no
  loop-top NONE/NULL for candidate actor/index/variant (the specific-object
  branch sets them); best-candidate stores actor, rating, distance, variant.
- Not needed (dropped): disembodied `participant_found`/`selected_variant_index`
  store order.

Checks: other 40 EXACT rows identical; guard passed; other parks equal base;
owner census vs `build/base` no new/lost owners; `coff_compare` all_equal;
fake scan 0 leads; `CL /Zs /W3` 14 -> 14 warnings.

### `ai_conversation_begin` — 1744/54, 96.3% (`park_remeasure_begin.c`)

- Looking-at loop `while ((player = next) != NULL && !player_can_see)` (4th
  `data_iterator_next` relocation).
- Two flags `participant_not_ready` / `participant_missing` set in the break
  block, then `missing -> can_begin = FALSE; else not_ready -> keep_trying,
  can_begin = FALSE` (HCEA blocking flags).
- `if (TEST_FLAG(mask, index))` choosing between two console_printf calls.
- `if (!found_specific_unit) can_begin = FALSE; else { visible loop }`.
- triggering player stored before nearest distance; final loop reads
  `conversation->actor_indices[index]` at each use; actor pointer local before
  `unit_can_see_point`; January init-store declaration order.

### `ai_communication_finished` — 1568/86, 85.3% (`park_remeasure_finished.c`, body `fn_finished_v5.c`)

Full January debug graph: `consider_string[512]` passed to
`ai_communication_consider_speech`; `debug_string[1024]` with "REPLY %s: ",
per-reply "%s:", focus vector, wrong-dmg / override-scripted / nobody /
playercant / 0-chance / rand %.2f>%.2f / rand-failed / filter / u-%s-%s
prints, assert 0x9E5 `!reply_to_player`, DATUM_INDEX_TO_ABSOLUTE_INDEX encounter,
individual speech-item stores + `csmemset(&speech.ai.information_data)`,
`update_speech_timers(..., reply_vocalization_type, NONE, reply_table_index)`,
">>%s<<" strupr, final focused `error(2, ...)`; success-first arms and a `for`
header (vocalization reload at the loop head).

## 4. Experiment matrix

| Candidate | Function | Result | Kept |
| --- | --- | --- | --- |
| b1..b19 (begin_edits.py) | begin | 1648/53 -> 1744/54, 61.9% -> 96.3% | proposal |
| see_noloc, dist_else, flags_first, decl_p1..p3, single better_player_rating, `!(has_alt && found_alt)` | begin | byte-identical / no slot change | no |
| f1..g9 (fp_edits.py) | find_participant | 64.1% -> 94.3% | superseded |
| sw_rev/sw_b/sw_c/sw_d case orders, empty default position, nearest scope, rejection decl order | find_participant | no gain | no |
| h8/fp1 selection_valid | find_participant | **EXACT** | proposal |
| p1 nested magnitude, p2 explicit components | get_player_rating | no / worse | no |
| nocast, orrev, kcast | actor_talk_weight | byte-identical | no |
| n1 nested/do-while, n3 flat for+continue, n4 success-first, n5 for header | finished | 80.0 / 84.7 / 80.8 / 85.3% | n5 proposal |
| n5p TU-local short animation_type (P1 probe) | finished | pushes match; unit_index/speech_priority register swap remains | evidence |
| q1..q3 decl/ternary/init, r1 chain, r2 combined chance test | finished | byte-identical | no |
| e1/e2 if-forms for actor_communication_team | event | not inlined (unchanged) | no |

## 5. Do-not-repeat

- find_participant: do not permute case labels; the reject[4] flag is the lever.
- begin: declaration order, loop-local vs function-scope rating flag, and
  `!(a && b)` spellings do not move found_alternate's frame slot.
- get_player_rating: nested/explicit magnitude spellings.
- actor_talk_weight: cast removal, `|` operand swap, knowledge cast (all
  byte-identical).
- event: rewriting the team `?:` as if/else does not make VC7 inline
  `actor_communication_team` (it inlines fine at top level; the call-site
  context blocks it).

## 6. Residual classification

- begin (fact): frame layout only — found_alternate at [ebp-0x16] vs January
  [ebp-0x1d], mask [ebp-0x20] vs [ebp-0x24], frame 0xb4 vs 0xb8; emitted slot
  reference census identical. Diagnostic: removing found_alternate's init moves
  it below mask, so its rank sits between best_distance and mask in January —
  a frame-allocation-priority tie with no visible source trace (inference).
- finished (fact): register swap unit_index ebx(J)/edi vs speech_priority
  edi(J)/ebx, which lets our two identical find_global push sequences
  cross-jump (86 vs 87 relocations), rand-failed/0-chance block order, argument
  load orders; plus ABI (P1 short animation_type) for exactness.
- get_player_rating: x87 operand order (k,i,j vs k,j,i). talk_weight: push
  schedule + byte-OR load order. update_speech_timers: esi/edi permutation
  (declaration-context sensitive). find_actor_to_reply_to_player: P1
  (units.h/units.c). event: header P2 + call-site inlining + scheduling.

## 7. Reopen criteria

- find_participant: orchestrator unparks and lands fp1 (strict EXACT).
- begin: a natural source difference that changes found_alternate's frame rank.
- finished: P1 applied, then a natural reason for January's unit_index-in-ebx
  choice.
- get_player_rating: a natural magnitude spelling yielding k,i,j.

## 8. Disposition

Object partial 40/48 in the real file; 41/48 once the find_participant
park-reopen lands. No exact credit claimed from fuzzy percentages.
