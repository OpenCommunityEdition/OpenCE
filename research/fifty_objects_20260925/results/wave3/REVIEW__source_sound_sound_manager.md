# REVIEW source/sound/sound_manager

approve: True

## per_object
[
 {
  "unit": "source/sound/sound_manager",
  "approve": true,
  "issues": "65/65 EXACT with the candidate header. object_audit PASS against the emulated split (0/113 symbols differ). 45 storage edits all match cachebeta publics. 21 surplus sections are identical and provider-link PASS. objdiff 3.3.1 100% on code, data and functions. A1 restores January behaviour (0x3d2 -> 0x406). A2's redundant term is /Od-verbatim and load-bearing. Advisory only: the later /Od build's function-scope `permutation` is byte-inert (not a blocker)."
 },
 {
  "unit": "source/sound/sound_definitions",
  "approve": true,
  "issues": "Candidate object is IDENTICAL to the production object, 7/7 EXACT. The single-exit body is /Od 0x877810-attested at every return point, and HCEX gives short old_range_index. The `else` and the result-before-closest order are byte-inert /Od readouts. It must land together with header.patch (shared header, 18 consumers; the 16 others were swept IDENTICAL)."
 }
]

## checks
All checks were run independently in scratch/w/review3_sound_manager/ on tree 954eebd2. Evidence is in LEDGER.md and REVIEW.md.

(1) Patch application.
- All five patches pass `git apply --check` on the current tree.
- Applied to a copy of the working-tree files, they reproduce final/sound_manager.c, final/sound_definitions.c, final/sound_definitions.h and the config/{symbols,parked,config}.json copies byte-for-byte, with CRLF kept.

(2) gate.py.
- sound_manager with the candidate header injected through --edits: 65/65 EXACT, _point_from_line3d guard passed.
- Control, the same source with the production long header: 63/2. The header edit is load-bearing and really applied.
- sound_definitions: 7/7 EXACT. Its object is IDENTICAL (non-debug sections and symbols) to build/base.

(3) Emulated split. My own csplit (config copy plus the candidate symbols.json) produced 833 objects, and only sound_manager.obj differs from build/split.

(4) object_audit against the emulated split: PASS. 113 January symbols, 0 differ, and every January-owned section is ok. Against the unedited split, the only differences are the 45 storage 2/3 rows.

(5) Storage.
- symbols.json diff: 45 lines, each a pure `" },` -> `", "static": true },` append.
- Of the 65 entries in [1815168,1831696), the 44 static ones are exactly those absent from cachebeta publics and the 21 external ones are exactly the public ones.
- `_sound_manager_globals` is not public, and no other January split object names it (xref.py).

(6) Surplus.
- surplus_identity on the candidate obj: 21/21 IDENTICAL (9 header COMDATs, 9 literals, 3 D3D tables).
- provider_link with the candidate obj: 21/21 PASS, SELECTED-PROVIDER LINK: PASS.
- .drectve is identical to production.

(7) objdiff 3.3.1 (sha1 3130e428).
- sound_manager candidate vs emulated split: 16085/16085 code, 3952/3952 data, 65/65 functions. Production control: 14521 code, 63/65 functions.
- sound_definitions: 922/922, 212/212, 7/7.

(8) Header blast radius: the 16 other sound_definitions.h consumers, compiled with and without the candidate header, give 16/16 IDENTICAL objects. No header includes sound_definitions.h.

(9) Scans: fake_match_scan finds 0 leads. CL /W3 /Zs shows the same warning set for candidate and production, with no C4013 and no C4028. The law-(g) scan finds no file-scope function prototype without a TU definition.

(10) Parks: the patch removes exactly the two sound_manager parks, and parked_functions on the copy reports stale [] and invalid [].

(11) First-party evidence.
- HCEX DIA2Dump: `short old_range_index` confirmed.
- /Od 0x877810 dumped: the single-exit body is attested at 0x87788d, 0x877895, 0x87791f and 0x877921 (the dead else jmp), and result is stored at 0x877962 before closest. The frame order equals the declaration order.
- January 0x3c2 `je 0x406` and 0x3d2 `jne 0x406` both land on the waiting test, which proves the combined A1 form is January's behaviour. /Od 0x89a5f3 agrees.
- /Od 0x89a575-0x89a5cb: the single channel_get_state result is re-read from [ebp-0x70], so it is a named short local, and the redundant `== queued` term is verbatim.

(12) Strip tests.
- sound_definitions: dropping the `else` or restoring the old store order is byte-inert (both are attested /Od readouts). An early return in either arm gives 272!=256, and the production early-return body with a short parameter reproduces astra's 1-byte regression, so the single-exit shape is load-bearing and attested.
- sound_manager: stripping the redundant term gives a residual [sha] (load-bearing, /Od-verbatim). Moving channel_state to the top of the else block is byte-inert, and the /Od slot order picks the outer-if block. Reverting A1 to the nested form gives 1376 [size] (load-bearing and January-proven).

(13) Fallback re-measured: production sound_manager.c with the new header gives 64/65.

(14) Advisory probes P1/P2: making `permutation` a function-scope local, as in the later /Od build, gives 65/65 (byte-inert).

## issues
No blocking issues. No amendment is needed: the worker's five patches stand and MUST be applied together. production.patch and header.patch alone would recreate the owner-rejected short/long mismatch (C4028).

Disclosures for the integrator:

(a) A1 changes behaviour relative to production, but it restores January's behaviour. January's 0x3d2 `jne 0x406` sends the case "next definition pending but the permutation chain continues" to the waiting test and the permutation advance; production skips that case. /Od 0x89a5f3 agrees. The lane-D revert measured against production, not January. The logic is sensible, not an authentic-bug class.

(b) The redundant `channel_state == _sound_channel_queued &&` term is load-bearing. It is admissible only because /Od attests it verbatim at 0x89a5a1.

(c) This supersedes astra research5 and the w1/n3a/n4 ledger notes that treated the parameter width as owner-blocked. The owner's rejection covered the contradicting header/definition form. This packet is coherent: the HCEX `short old_range_index`, plus the /Od single-exit body that is new first-party evidence. It is not in any section-7 owner-gated class: 0 consumer regressions, no extern-over-static.

(d) The shared-header full sweep must cover all 18 consumers under source/: cache/predicted_resources, cache/xbox_sound_cache, devices/devices, effects/effects, effects/particles, game/game_engine_multiplayer_sounds, hs/hs_library_external, interface/hud_sounds, interface/ui_widget, items/projectiles, items/weapons, objects/objects, scenario/scenario, sound/game_sound, sound/sound_definitions, sound/sound_dsound_xbox, sound/sound_manager, units/unit_dialogue.

(e) Regenerate the split after the symbols.json edit. Only sound_manager.obj changes.

(f) Minor worker inaccuracy: 23 (not 24) of the 25 other system_milliseconds callers include cseries_windows.h; xbox_sound_cache.c and network_game_globals.c do not. The verdict does not change.

(g) Advisory, pre-existing, not introduced by this packet: the later /Od build declares one function-scope `permutation` ([ebp-0x44]) and reads the final gain through it. Probes P1/P2 show both spellings are byte-inert, and the later build's function-level declaration order differs anyway, so this is only a candidate for a later house-clean pass.

(h) Pre-existing linker_common data externs (sound_data, looping_sound_data, sound_channels, loud_dialog_hack, debug_looping_sound) are unchanged. January's split has the same UNDEF symbols, and render_objects and collision_debug set the precedent.

Evidence: C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/review3_sound_manager/REVIEW.md and LEDGER.md.
