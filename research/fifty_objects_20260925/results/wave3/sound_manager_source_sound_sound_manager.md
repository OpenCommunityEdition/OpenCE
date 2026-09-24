# source/sound/sound_manager

## verdict
OBJECT_COMPLETE_CANDIDATE

## newly_exact_functions
[
 "_sound_set_definition_end",
 "_update_channel_for_looping_sound"
]

## candidate_files
[
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/sound_manager/final/sound_manager.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/sound_manager/final/sound_definitions.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/sound_manager/final/sound_definitions.h",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/sound_manager/config/symbols.json",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/sound_manager/config/parked.json",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/sound_manager/config/config.json",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/sound_manager/production.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/sound_manager/header.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/sound_manager/symbols.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/sound_manager/parked.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/sound_manager/config_status.patch"
]

## production_changes
Apply all five patches TOGETHER from the worktree root. Each passes `git apply --check`, and applying them to a copy of the tree reproduces the final/ files byte-for-byte (CRLF kept). Paths are under scratch/w/sound_manager/.

(1) production.patch changes source/sound/sound_manager.c and source/sound/sound_definitions.c.
  (a) sound_definitions.c: sound_definition_find_pitch_range_by_pitch takes the third parameter as `short old_range_index` (HCEX DIA2Dump: short old_range_index) and uses the /Od 0x877810 single-exit body. The in-range arm does `result = old_range_index;` (it used to return early). The loop is wrapped in `if (result == NONE) { ... }`. The in-bounds arm is `{ result = range_index; break; } else { pitch_ratio ... }`. The closest-ratio update stores `result` before `closest_pitch_ratio`, as /Od does. The function ends with `return result;`. The (short) casts are dropped.
  (b) sound_manager.c:
    - Add `#include "cseries/cseries_windows.h"` after cseries/profile.h, and delete the consumer-local `long game_time_get(void);` (a duplicate of game.h:176) and `unsigned long system_milliseconds(void);`.
    - `static struct sound_manager_globals sound_manager_globals = { 0 };`.
    - Pass plain `sound->pitch_range_index` (no `(word)` cast) at both find_pitch_range call sites.
    - In update_channel_for_looping_sound: add an outer `if (type != stop && (type != start || !fade_in))` block with a block local `short channel_state = channel_get_state(sound->playing_channel_index);` and the test `channel_state != _sound_channel_queued || waiting || (channel_state == _sound_channel_queued && pp->next == NONE && next_def != NONE)`. Inside it, use the COMBINED `if (next_def != NONE && (!pp || pp->next == NONE)) {...} else if (!waiting) {...}` (lane-D A1+A2).

(2) header.patch: SHARED HEADER source/sound/sound_definitions.h. The prototype's third parameter goes from `long pitch_range_index` to `short old_range_index`. It must land together with the sound_definitions.c definition: a split landing recreates the owner-rejected header/definition mismatch (C4028). There are 18 consumer TUs, found by grep: source/cache/predicted_resources.c, cache/xbox_sound_cache.c, devices/devices.c, effects/effects.c, effects/particles.c, game/game_engine_multiplayer_sounds.c, hs/hs_library_external.c, interface/hud_sounds.c, interface/ui_widget.c, items/projectiles.c, items/weapons.c, objects/objects.c, scenario/scenario.c, sound/game_sound.c, sound/sound_definitions.c, sound/sound_dsound_xbox.c, sound/sound_manager.c, units/unit_dialogue.c. The integrator's full sweep should cover all of them.

(3) symbols.patch / config/symbols.json copy: 45 in-place line edits appending `, "static": true`. They cover the 44 sound_manager function entries in file_offset [1815168,1831696) that pdb_storage flags, plus `_sound_manager_globals` (5057888). Nothing was re-serialised.

(4) parked.patch: retire the parks for _update_channel_for_looping_sound and _sound_set_definition_end (lines 1146-1167 removed). parked_functions validates the result: stale [], invalid [].

(5) config_status.patch: source/sound/sound_manager.c status NonMatching -> Matching.

## evidence
Baseline gate --all: 63 exact / 2 residual. Both residuals were blamed on the 16-bit-parameter fingerprint: January's `xor r,r; mov r16,[sound+0x8e]` for the third argument of sound_definition_find_pitch_range_by_pitch. The owner rejected the short-header/long-definition form. A coherent short header and definition regressed the owner by 1 byte (`mov si,word` instead of January's `mov esi,dword [ebp+0x10]`), but astra research5 measured that only with the early-return body.

NEW FINDING: the load width depends on the body.
- A VC7 lab (lab/width.c) compiled a short parameter with the /Od 0x877810 single-exit body. It emits `mov esi,dword [ebp+0x10]`, and alndiff against January's owner shows no normalized differences.
- The mechanism lab (width2.c) traced this to the in-range `result = old_range_index` copy, which coalesces the parameter web with the dword-homed `result`.
- /Od evidence for the owner body: 0x87788d `result = param`; 0x877895 `if (result == NONE)`; 0x87791f break; 0x877921 `else`; 0x877962 result stored before closest. The frame order result/range/closest/range_index/range/pitch_ratio matches the declaration order.
- HCEX supplies the name and type `short old_range_index`.
- The /Od callers push `movzx r, word [sound+0x8e]`, the same way they push the known-short tick_index.

Results:
- The coherent candidate gives sound_definitions 7/7 EXACT, and its object is IDENTICAL (non-debug sections and symbols) to the production build.
- sound_manager is 65/65 EXACT and the _point_from_line3d guard passes.
- Header blast radius: all 16 other consumers compile IDENTICAL objects with the new header.

Looping row: the production body differed from January in three places, not two. The third is January 0x3d2 `jne 0x406`, which lands on the waiting-flag test, i.e. the combined `else if (!waiting)` form; the production nested form skips it. /Od 0x89a5d1-0x89a5f3 agrees. The /Od block local `channel_state` sits at [ebp-0x70], between new_sound_index at -0x6c and permutation_index at -0x74, and the redundant `state == queued` re-test is at 0x89a5a1. Stripping that term loses January's `xor eax,eax` at 0x37e.

Storage: cachebeta has no public for the 44 statics or for _sound_manager_globals, and no other January split object references `_sound_manager_globals`. A scratch csplit with the edited symbols.json changes only sound_manager.obj (833 objects compared).

Audits:
- object_audit against the emulated split: PASS, 0 of 113 symbols differ.
- Storage against cachebeta publics: 0 disagreements in both candidate objects.
- Surplus: 21 candidate-only sections (9 header-inline COMDATs, 9 literals, 3 XDK D3D tables pulled in by the cseries_windows.h include) are all IDENTICAL to January's selected providers, and provider_link PASSes for all of them.
- objdiff 3.3.1 against the emulated split: 16085/16085 code, 3952/3952 data, 65/65 functions. The production control reproduces 14521 code and 63 functions. sound_definitions scores 922/922 and 212/212.
- fake_match_scan: 0 leads.

Fallback: production looping plus the header/definition packet alone already makes _sound_set_definition_end EXACT (64/65).

## blockers
No mechanical blockers remain. Reviewer flags, all disclosed:

1. A1 is January's behaviour, proven by byte 0x3d2 -> 0x406 and by /Od. The lane-D orchestrator had reverted it as a 'behaviour change', but it is the production nested form that deviates from January.
2. A2 keeps the redundant `channel_state == _sound_channel_queued &&` term. It appears verbatim in /Od (0x89a5a1) and is load-bearing: the strip test loses the 0x37e xor.
3. The previous owner rejection covered the INCOHERENT header/definition pair. This packet is coherent `short` in both places and is /Od- and HCEX-attested. The owner should confirm that it supersedes the 'Rejected: _sound_set_definition_end header prototype contradicting its definition' row and astra research5's 'do not tune the body'. The body is the /Od-attested shape, not a tuned one.
4. The cseries_windows.h include adds 3 identical, link-clean D3D surplus tables (collision_debug precedent).
5. The consumer-local DATA externs (sound_data, looping_sound_data, sound_channels, loud_dialog_hack, debug_looping_sound) are unchanged: nothing in our tree defines them (they are January linker_common records). This is the same state as the admitted collision_debug (21 externs) and render_objects (3). January references sound_data, looping_sound_data and sound_channels only from sound_manager.obj.
6. Optional cosmetic change: HCEX names the first parameter `sound`, but it was kept as `definition`.

## reopen_criteria
If the reviewer rejects A2's redundant term, the looping row goes back to residual (only the 0x37e xor is missing), and the object blocks on that single row. Land the header+definition packet anyway: it closes _sound_set_definition_end (64/65). If the include is rejected, keeping the two local prototypes changes no function bytes but leaves the law-(g) item open.

## task notes
What changed from the previous state: the only thing that blocked the earlier coherent-short attempt was the owner's body shape. January's owner is the /Od single-exit form, so the dword parameter load comes from `result = old_range_index`, not from a long parameter type.

The whole packet touches six tracked files: sound_manager.c, sound_definitions.c, sound_definitions.h, symbols.json, parked.json and config.json.

Verification I ran, all under scratch:
- gate on both units
- a scratch csplit
- object_audit against the emulated split
- surplus identity
- provider_link in both orders
- an objdiff 3.3.1 mini-project
- a 16-consumer header sweep

I did not run ninja, git writes or tracked edits; the integrator should do the full ninja/progress/stable sweep.

The lane-D worker's candidate (claude-lane-d-refresh-20260922 scratch/lane_d/w/sound__sound_manager/candidate.c) supplied A1/A2. It had been reverted only because January's 0x3d2 branch target was misread. scratch/w/sound_manager/hdr/ keeps the lane-D mismatched-header repro (the owner-rejected form) for reference only; it is NOT the proposed packet. The emulated split tree scratch/w/sound_manager/split/ (833 objects) can be deleted after integration.
ledger: C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/sound_manager/LEDGER.md
