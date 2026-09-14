# `players.obj` Opus5 150K house-clean lane, wave w3 (2026-09-14)

## 1. Scope and provenance

- Translation unit `source/game/players.c`; target `build/split/source/game/players.obj`.
- Lane `opus/150k-house-clean-20260914`, HEAD `403108e28`. Compiler XDK 3911 `CL.Exe` (VC7 13.00.9254), flags unchanged.
- **The real `players.c` is unchanged.** No header, config, park or tool was edited.
- Prior ledgers read: `players_obj.md`, `_fable_exact_packet_reconciliation_20260913`, `_january_safe_harvest_20260902`,
  `_jonas_residual_freshness_audit_20260830`, `_jonas_source_credibility_cleanup_20260830`, `_opus5_house_clean_20260913`,
  `_opus5_100k_20260914`, `_opus5_150k_w1_20260914`, the w1 worker notes, the 150K handoff, `scratch/w2/laws_w2.md`,
  and the w2/w3 collisions, bipeds and ai ledgers for the radius-argument family.
- Worker notes: `scratch/workers/w3_players_aim_assist.md`. Candidates: `scratch/workers/w3_players_aim_assist/`.

## 2. Gates

| Gate (real file, `--forbid-emitted-symbol _point_from_line3d`) | Result |
|---|---|
| Baseline | `== exact 61  residual 8  unwritten 1  (of 70 listed)`, guard passed |
| Final (real file unchanged) | identical listing; tinfo size/reloc/sha rows identical to the HEAD object |
| Parks | none for this unit |
| Owner census vs `build/base` | identical code and data owners |
| `tools/fake_match_scan.py players.c` | 0 leads |

Newly strict exact: **none**.

## 3. Per-function outcomes

| Function | January | Outcome | New evidence / shapes |
|---|---|---|---|
| `_player_examine_nearby_unit` | 16/0 | owner-blocked | target-only `ret` stub, unchanged policy; 0 shapes |
| `_player_examine_nearby_device` | 176/9 (ours 160) | owner-blocked | January `fld [edi+0x5c]; add esp,0x1c; fstp [esp]`: the x87-staged radius is folded into the pending stack adjust. This is the laws_w2 C3 parenthesised-argument family (`_object_test_vector`, `_ai_test_line_of_fire`, `_biped_update_moving`). No genuine macro exists; 0 shapes |
| `_player_teleport_internal` | 1296/61 (ours 1312/62) | not landed | tp1 (the w1 t1 body on HEAD) is 1280/61 with 428==428 instructions and all 61 relocation identities equal (relocdiff). tp2 (forward/up selection as if/else instead of `?:`) is 419 instructions, worse |
| `_player_handle_powerup` | 240/14 (ours 256) | not landed | analysis only (see §4) |
| `_player_handle_powerup_minor` | 160/9 | not landed | analysis only |
| `_player_update_powerups` | 112/5 | not landed | pu1 (natural long for loop) is 96 [size]; pu2 (natural short for loop) is 96 and also drifts `_players_update_after_game` |
| `_player_set_action_result` | 240/5 | skipped (exhausted) | header-owned distance3d x87 order; 0 shapes |
| `_player_examine_nearby_vehicle` / `_objects` | 448/25, 240/18 | skipped (exhausted) | see §4; 0 shapes |

## 4. Evidence recorded this wave

- **Powerup family.** January never CSEs `&player->powerup_durations[powerup_type]`. Both `handle_powerup` and `minor` use the index
  form at every access (`[ebx+esi*2+0x68]`). That single fact explains the rest:
  - the in-place `movsx esi,si`;
  - the 32-bit `test esi,esi` inside the inlined helpers, which reuse the callee-saved sign-extended value;
  - the non-constant `mov word [eax+0x3d2],si` store;
  - the 16-byte `handle_powerup` gap. Ours duplicates the cheap `add word [edi],ax` tail, while January shares one tail with a merged
    `mov [eax+0x1b4],ecx` store.

  In `update_powerups`, January uses a pointer IV, an index IV (`xor edi,edi` after the call) and a homed down-counter at `[ebp-4]`.
  Natural loops recover the preheader order (A12) but not the down-counter.
- **teleport_internal allocation map.**
  - January: esi holds the root index; edi holds `player_unit_index`, split around the inner loop; the root pointer is homed at `[ebp-0xc]`,
    a slot later reused by the short adjustment index; the scale slot `[ebp-0x18]` is later reused by the trigger index.
  - Ours: esi holds the root pointer and then the loop indices; `player_unit_index` lives in memory; the trigger index takes the
    `source_unit_index` parameter home. Frame 0x78 against January's 0x7c.
  - The movsx and movzx differences at +0x41c and +0x352 fall under the A18 caveat: the destination register equals the base register.
- **vehicle.** The baseline already inlines `player_set_action_result` at the constant flip site. The house-clean admission note that the
  re-fetch "changed January's inline schedule" is therefore imprecise: the flip inline is baseline behaviour. January differs by having
  three vehicle fetches and an out-of-line flip call with the eax private ABI.
- **A7 detector (C4013 `error`, line 942).** Adding `#include "cseries/errors.h"` (the genuine owner of
  `void error(short, const char *, ...)`) is byte-identical across the whole TU. It gains nothing, so it was not landed.
- **Storage classes.** All 70 January code symbols match ours; only the target-only stub is absent.

## 5. Rejected shapes (do not repeat)

- `update_powerups`: natural indexed for loops, long (pu1) and short (pu2), re-measured on HEAD.
- `teleport_internal`: if/else struct-copy selection for forward/up (tp2).

## 6. Proposals

None (no header or config text required). A byte-neutral C4013 cleanup (`#include "cseries/errors.h"`) is available to the
orchestrator at zero credit.

## 7. Reopen criteria

| Function | Reopen when |
|---|---|
| powerup family | a first-party spelling that suppresses the durations address CSE without changing the standalone helpers; measure the three callers, the three helper standalones and `update_after_game` together |
| `teleport_internal` | evidence lowering the root pointer's allocation priority below `player_unit_index`; start from `scratch/workers/players/tele_t1_fn.txt` |
| device | owner ruling on the parenthesised radius argument (shared with collisions/ai/bipeds) |
| vehicle / objects | a legal-C reason for the out-of-line flip call plus acceptance of January's third vehicle fetch |
| `set_action_result` | owner decision on the real_math.h distance3d/magnitude inline spelling |
| `examine_nearby_unit` | first-party stub source plus owner policy |
