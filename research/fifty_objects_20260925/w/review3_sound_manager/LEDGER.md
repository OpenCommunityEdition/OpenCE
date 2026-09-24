# review3_sound_manager ledger (adversarial review of scratch/w/sound_manager OBJECT_COMPLETE_CANDIDATE)
Tree 954eebd2. Reviewer works only under scratch/w/review3_sound_manager/.

## 0. Setup
- Read WORKER_BRIEF (0-8), house rules, lane log, worker LEDGER.md.

## 1. Evidence checks
- /Od 0x877810 dumped (od_877810.txt): result [ebp-4]=NONE; first-block range [ebp-8]; in-range `mov ax,[ebp+0x10]; mov [ebp-4],ax`
  (0x87788d); `if (result == NONE)` 0x877895 jne exit; closest [ebp-0xc], range_index [ebp-0x10], loop range [ebp-0x14],
  pitch_ratio [ebp-0x18]; in-bounds arm result=range_index + `jmp exit` (break) + dead `jmp 0x877974` (=else); closest update
  stores result (0x877962) before closest (0x87796a). CONFIRMS the worker's single-exit body and declaration order.
  Param reads are `movsx ecx, word [ebp+0x10]` / `mov ax, word [ebp+0x10]` (consistent with short; not discriminating vs long+casts).
- HCEX DIA2Dump: params `struct sound_definition * sound`, `float pitch`, `short old_range_index` (all enregistered). CONFIRMED.
- Patches: all 5 `git apply --check` clean on 954eebd2; applied to a copy of the working-tree files they reproduce
  final/ + config/ copies byte-for-byte (cmp).
- gate (my run, cand/ copies + my edits.json header injection): sound_manager 65/65 EXACT, guard passed; control with
  production header (long): 63/2 (set_definition_end, looping residual) -> the header edit is load-bearing and really used.
  sound_definitions 7/7 EXACT; objeq vs build/base/sound_definitions.obj IDENTICAL.
- object_audit vs unedited January split: every January-owned section ok; 45 symbols differ ONLY storage 2/3 (44 fns +
  _sound_manager_globals). pdb_storage: all 45 absent from cachebeta publics. xref: _sound_manager_globals defined only in
  sound_manager.obj, no other January split object names it.
- January bytes 0x3bc..0x406: next_def==NONE -> 0x406 (waiting test); pp->next != NONE -> 0x3d2 jne 0x406 (waiting test).
  CONFIRMS the combined `if (next_def != NONE && (!pp || pp->next == NONE)) ... else if (!waiting)` semantics in January;
  production nested form jumps to 0x482 (skip) = production deviates from January behaviour. /Od 0x89a5d1..0x89a5f3 jne 0x89a640
  (waiting test) agrees.
- /Od 0x89a575..0x89a5cb: channel_get_state -> word [ebp-0x70]; `!=2 -> body; waiting -> body; !=2 -> end; pp->next != -1 -> end;
  next_def == -1 -> end` = the candidate's A2 condition incl. the redundant `== queued` term. Frame -0x6c new_sound_index,
  -0x70 channel_state, -0x74 permutation_index: C89 block-start rule places channel_state in a block opened after the crossfade
  block = the outer if block. HCEX lists only `properties` for this function (enregistered locals absent) -> `channel_state` is
  a descriptive name.
- symbols.json copy: 45 changed lines, each exactly `" },` -> `", "static": true },` (checked byte-wise, CRLF kept); all 65
  entries in [1815168,1831696): 44 static = exactly the non-public ones, 21 external = exactly the public ones.
- My csplit (config copy + candidate symbols.json -> review3 split/): 833 objects, only sound_manager.obj differs from build/split.
  object_audit (AUDIT_TARGET = emulated split) on my candidate obj: 113 January symbols, 0 differ, all sections ok -> PASS.
  Surplus: production's 18 (9 COMDAT helpers + 9 literals) + 3 D3D tables from cseries_windows.h.
- surplus identity (candidate obj, .text/.rdata/.data): 21/21 IDENTICAL; provider_link (candidate obj): 21/21 PASS,
  SELECTED-PROVIDER LINK: PASS.

## 2. Strip tests (reviewer-run; hypothesis: every non-plain construct is either byte-inert or /Od-attested)
- sound_definitions (short header): drop `else` -> EXACT; production store order (closest then result) -> EXACT; both -> EXACT
  => both /Od-attested details are byte-inert (kept as /Od readout, not bought).
- h1 loop `return range_index` (single-exit range arm) -> 272!=256; h2 range-arm `return old_range_index` (single-exit loop)
  -> 272!=256; production early-return body with short param -> [sha] (astra's 1-byte regression reproduced).
  => the full /Od single-exit shape is load-bearing AND /Od-attested at every return point (0x87788d, 0x877895, 0x87791f).
- sound_manager: strip redundant `channel_state == _sound_channel_queued &&` -> looping residual [sha] (load-bearing; /Od
  verbatim at 0x89a5a1 and the single channel_get_state call + reuse of [ebp-0x70] proves a NAMED local, not a temp).
- channel_state declared at top of the else block instead of the outer-if block -> 65/65 (byte-inert); the /Od slot order
  (-0x6c new_sound_index, -0x70 channel_state, -0x74 permutation_index) picks the candidate's outer-if block.
- A2 kept + A1 reverted to the production nested form -> looping [size 1376!=1360] => A1 load-bearing; January bytes prove
  A1 is January's behaviour (see section 1).

## 3. Blast radius, scorer, hygiene
- Header sweep (sweep/run.sh: production source with vs without the candidate sound_definitions.h via --edits, objeq):
  16/16 other consumers IDENTICAL (predicted_resources, xbox_sound_cache, devices, effects, particles,
  game_engine_multiplayer_sounds, hs_library_external, hud_sounds, ui_widget, projectiles, weapons, objects, scenario,
  game_sound, sound_dsound_xbox, unit_dialogue); row counts unchanged. No .h includes sound_definitions.h (grep).
- objdiff 3.3.1 (sha1 3130e428; od_proj/): cand vs emulated split 16085/16085, 3952/3952, 65/65; cand vs January split
  same; production control 14521/16085, 63/65; sound_definitions cand 922/922, 212/212, 7/7.
- fake_match_scan: 0 leads on the 3 candidate files. CL /W3 /Zs (warn.py): candidate and production warning sets identical
  (C4146 x1, C4244 x11 / x12); no C4013, no C4028.
- parked.json copy: exactly the 2 sound_manager parks removed (89 -> 87); parked_functions --manifest copy: stale [] invalid [].
- .drectve identical between production and candidate.
- Fallback re-measured: production sound_manager.c + candidate header -> 64/65 (set_definition_end EXACT, looping
  1376 [size]) = worker P13.
- Law-(g) scan: no file-scope function prototype in the candidate lacks a TU definition (44 static protos, all defined).
  Data externs (sound_data, looping_sound_data, sound_channels, loud_dialog_hack, debug_looping_sound) are pre-existing
  linker_common COMMON references; January split has the same UNDEF symbols (value 0); render_objects/collision_debug precedent.
- Minor worker inaccuracy: 23 (not 24) of the 25 other system_milliseconds callers include cseries_windows.h
  (xbox_sound_cache.c and network_game_globals.c do not). Does not change the conclusion.

## 4. Advisory probe (pre-existing, not introduced by the packet)
- The later /Od build keeps ONE function-scope `permutation` ([ebp-0x44], used at 0x89a1a8, 0x89a77d, 0x89a882) and reads the
  final gain through it; production/candidate use two block-scoped `permutation`s + an inline accessor for the final gain.
  P1 (function-scope permutation) and P2 (+ final gain via the local) are both 65/65 = byte-inert. The later build's
  function-level declaration order also differs (properties/definition/pitch_range/permutation before looping_sound), so it
  is not a clean readout for January's function-level declarations. Not a blocker; recorded for a future house-clean pass.

## 5. Verdict: APPROVE (no amendment needed; the worker's five patches stand as submitted, applied together).
