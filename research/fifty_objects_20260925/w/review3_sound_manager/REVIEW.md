# Adversarial review: sound_manager OBJECT_COMPLETE_CANDIDATE (wave 3)

- **Reviewer slug:** scratch/w/review3_sound_manager/ (full probe log in LEDGER.md)
- **Tree:** 954eebd2
- **Worker:** scratch/w/sound_manager/

## Verdict

**APPROVE.** The worker's five patches (production, header, symbols, parked, config_status) stand as
submitted and need no amendment. **They must be applied together.**

## Independently re-run

| Check | Result |
|---|---|
| `git apply --check` on the current tree | all 5 clean; applied to a copy of the working-tree files they reproduce final/ and config/ byte-for-byte (CRLF kept) |
| gate `--all`, sound_manager, with the candidate header injected | **65/65 EXACT**; `_point_from_line3d` guard passed |
| control: same source with the production (long) header | 63/2, so the header edit is load-bearing and really applied |
| gate `--all`, sound_definitions | **7/7 EXACT**; object IDENTICAL to build/base (non-debug sections and symbols) |
| csplit with the candidate symbols.json (review slug) | 833 objects; only sound_manager.obj differs from build/split |
| object_audit against the emulated split | PASS: 113 January symbols, 0 differ, every January-owned section ok |
| storage | all 65 symbols.json entries in [1815168,1831696): the 44 static entries are exactly the ones absent from cachebeta publics, and the 21 external ones are exactly the public ones. `_sound_manager_globals` is not public, and no other January split object names it |
| symbols.json copy | 45 lines changed, each only `" },` -> `", "static": true },`; not re-serialised |
| surplus | 21 candidate-only sections: 9 header COMDATs, 9 literals, 3 D3D tables. All 21 IDENTICAL to January's providers; provider_link PASS for all 21 |
| objdiff 3.3.1 (sha1 3130e428) | sound_manager 16085/16085 code, 3952/3952 data, 65/65 functions (production control: 14521, 63). sound_definitions 922/922, 212/212, 7/7 |
| header blast radius | 16/16 other sound_definitions.h consumers compile IDENTICAL objects. No header includes sound_definitions.h |
| fake_match_scan | 0 leads |
| CL /W3 | same warning set as production; no C4013, no C4028 |
| parks | exactly 2 removed; `parked_functions` reports stale [] and invalid [] |

## Evidence verified

- **HCEX (DIA2Dump):** params are `struct sound_definition *sound`, `float pitch` and `short old_range_index`.
- **/Od 0x877810:** the single-exit body is attested at every return point:
  - `result = param` at 0x87788d;
  - `if (result == NONE)` at 0x877895;
  - `break` at 0x87791f;
  - a dead `jmp` at 0x877921, which is the `else`;
  - `result` is stored (0x877962) before `closest` (0x87796a).
  - The frame order result/range/closest/range_index/range/pitch_ratio equals the candidate's declaration order.
- **January looping bytes:**
  - 0x3c2 `je 0x406` handles next_def == NONE.
  - 0x3d2 `jne 0x406` handles pp->next != NONE.
  - Both land on the waiting test at 0x406.
  - So January's own behaviour is the combined `if (next_def != NONE && (!pp || pp->next == NONE)) ... else if (!waiting)`.
  - The production nested form jumps to 0x482 instead, so it is the deviation.
  - /Od 0x89a5f3 `jne 0x89a640` (the waiting test) agrees.
  - The lane-D "behaviour change" revert was measured against production, not January.
- **/Od 0x89a575..0x89a5cb:**
  - One `channel_get_state` call is stored to word [ebp-0x70] and re-read at 0x89a5a1, so it is a named short local, not a temp.
  - The redundant `== queued` term is verbatim.
  - The slots -0x6c new_sound_index, -0x70 channel_state, -0x74 permutation_index, together with C89's
    declarations-at-block-start rule, place channel_state in the outer-if block.

## Strip tests

- **sound_definitions:**
  - Dropping the `else`, or restoring the old store order, is byte-inert. Both constructs are /Od readout, not decoration.
  - An early return in either arm gives 272 != 256.
  - The production early-return body with a short parameter reproduces astra's `mov si,word` regression.
  - So the full /Od single-exit shape is load-bearing and attested.
- **sound_manager:**
  - Stripping the redundant `channel_state == _sound_channel_queued &&` gives a residual [sha]. The term is load-bearing,
    and /Od attests it verbatim.
  - Moving channel_state to the top of the else block is byte-inert. The candidate's placement follows /Od.
  - Reverting A1 to the nested form gives 1376 [size]. A1 is load-bearing and January-proven.

## Rulings context

- The owner rejected a header prototype that contradicts its definition. This packet's form is coherent (short in
  both), so that rejection does not apply.
- astra research5 measured the coherent form only with the early-return body. The /Od single-exit body is new
  first-party evidence and supersedes the ledger notes (w1, n3a, n4) that the width is owner-blocked.
- The packet is not in any section-7 owner-gated class:
  - no declaration-count canary regression (16/16 IDENTICAL);
  - no extern prototype over a static definition;
  - no authentic-bug reproduction;
  - no view cast.

## Notes for the integrator

1. **Land production.patch and header.patch together.** Either one alone recreates the rejected mismatch (C4028).
   The full sweep covers these 18 consumers of the shared header, all under `source/`:
   - cache/predicted_resources, cache/xbox_sound_cache
   - devices/devices
   - effects/effects, effects/particles
   - game/game_engine_multiplayer_sounds
   - hs/hs_library_external
   - interface/hud_sounds, interface/ui_widget
   - items/projectiles, items/weapons
   - objects/objects
   - scenario/scenario
   - sound/game_sound, sound/sound_definitions, sound/sound_dsound_xbox, sound/sound_manager
   - units/unit_dialogue
2. **Regenerate the split after the symbols.json edit** (csplit or ninja). Only sound_manager.obj changes.
3. **Disclose** A1 as a restoration of January behaviour, with the byte proof above.
4. **Minor inaccuracy in the worker's notes:** 23 of the 25 other `system_milliseconds` callers include
   cseries_windows.h, not 24. xbox_sound_cache.c and network_game_globals.c do not. The verdict does not change.
5. **Advisory, pre-existing:** the later /Od build uses one function-scope `permutation` ([ebp-0x44]) and reads the
   final gain through it. Probes P1 and P2 are both 65/65 (byte-inert), and the later build's function-level
   declaration order differs anyway. This is not a blocker. It is a candidate for a later house-clean pass.
6. **Pre-existing COMMON externs:** the linker_common data externs are unchanged. January has the same UNDEF symbols,
   and render_objects and collision_debug set the precedent.
