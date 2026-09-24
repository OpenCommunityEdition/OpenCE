# review3_main - adversarial review of worker scratch/w/main (verdict FUZZY_IMPROVED), base 954eebd2

## R0 setup
- source/main/main.c == scratch/w/main/base.c (tree clean at 954eebd2).
- Temp CRLF/autocrlf repo (tmprepo/): production.patch -> header_swap.patch -> owner_gated_crash.patch all apply;
  results equal candidate_production_main.c (=prod2.c), candidate_main.h, candidate_full_gated_main.c (=prod_gated_hdr2.c); p1+p2 == prod_hdr2.c.
- gate --all base.c: 92/2/1 (update_time [relocs 118!=117, sha], frame_rate_debug [relocs 51!=52, sha], crash UNWRITTEN).
- gate --all p1.c (= production patch): 92/2/1; per-function verdicts identical to base; update_time now [sha] (117==117).

## R1 production.patch (p1.c) verification
- seccmp base.obj vs p1.obj (all non-debug sections, bytes+relocs by name): only _main_frame_rate_debug and _main_update_time .text differ;
  23 function symbols flip storage 2->3. worker prod.c (pre-cleanup) vs p1.obj: 0/196 sections differ => bitmap_2d_new dup removal and
  attract_mode.h include are byte-inert (verified independently).
- alndiff vs January: update_time base 609 lines -> p1 114 lines (only the SI/AX phi + store position block + offset shifts);
  frame_rate_debug base 192 -> p1 127 (only thread `jmp; mov dl,[902]` + CLEAR store dl/bl). Semantic changes in update_time
  (`>` not `>=` on throttle_start_index; " des %d" prints best_interval; ok/wt arms swapped with inverted test) are January-attested
  (no alndiff rows in those regions).
- January split relocations (.text) to the 14 zero-ref functions: none (exit, throttle_enabled, load_core, lost_map, queue_map,
  reset_map, reset_time, respawn, revert, run_demos, save_core, saving_map, skip_cinematic, switch_bsp). cachebeta publics: all 26 absent;
  main_crash/main_loop/main_print_version present. HCEX.pdb: `static void main_update_time()`, `static void main_game_render(float,float)`,
  main_crash non-static.
- /Od main_loop 0x6b1150: the 16-call chain uses DIRECT (non-ILT) calls = file statics in that build; loop exit calls 0x6aff90 directly,
  and 0x6aff90 is main_exit (switch on connection -> dispose client / client+server, then dispose sequence). => main_exit() call attested.
- /Od has NO main_framerate_throttle_enabled (later update_time 0x6b5280 calls rasterizer 0x86c200 instead): the call site of the
  accessor is NOT /Od-attested; only its existence as an inlined static is proven (January emits it, no reloc, non-public).
- csplit emulation (splitcfg/ + worker symbols.json -> splitout/): 833 objs, only main.obj differs from build/split.
  object_audit vs emulated split with p1.obj: only crash (missing), 2 residual .text, and _main_game_render storage 3/2 (needs patch 2).
  gate-equivalent vs emulated split: 92/2/1 unchanged.

## R2 accessor call placement (main_framerate_throttle_enabled)
- alt_rt.c (call in main_rasterizer_throttle's `if` instead of update_time) and alt_both.c: 92/2/1, 0/196 sections differ vs p1.
  => the call SITE is byte-undetermined; the worker's update_time placement is the natural one (local named framerate_throttle,
  accessor defined immediately before update_time) but it is a choice, not an attestation. Acceptable (a call must exist somewhere).

## R3 header swap (patch 2) - independent sweep (sweep/sweep.py, private dirs, source dir first on /I)
- Pipeline validation: 20 consumers compiled with the ORIGINAL main.h as shadow == build/base objects, 0 differing sections in all 20.
- swap header: 0 differing sections in all 20 consumers (even internal $T/$SG label numbering identical).
- remove-only header: _player_profile_3wide_list_update BYTES differ (canary regression confirmed) + many internal-label renumberings.
- add-only header (main_crash prototype only): no byte differences, only internal-label renumbering.
  => the swap is exact declared-name-count compensation (disclosed by worker); same class as the Layer 2 lock/unlock prototype move.
- main.c p1+p2 (p12.c) with swap header: 0/196 sections differ from p1.obj; only _main_game_render 2->3.
  vs emulated split: gate 92/2/1; object_audit: only crash (missing) + 2 residual .text; 0 storage diffs.
- p1+crash_nohdr (== prod_gated2): gate 93/2/0. p1+p2+crash (p123) with swap header vs emulated split: gate 93/2/0,
  object_audit symbols 169/169 ok, sections: only the 2 residual .text.

## R4 other checks
- /W3 (/Zs): base 25 warnings == p1 25 warnings (same multiset; 4 pre-existing C4013: xbox_demos_launch,
  player_ui_get_single_player_local_player_controller, input_abstraction_update, cache_files_give_time_to_precache). Worker's
  "0 warnings" was at default level; no NEW warnings.
- fake_match_scan p1/p12/p123: 0 leads. surplus_identity: 2 COMDATs identical; provider_link p1.obj and p12 obj: PASS.
- Park re-baseline values reproduced exactly (update_time 1440/117/ad56536c..., frame_rate_debug 544/51/3c132f90...).
- .data _global_difficulty_level: split == p1 == build/base (52/10/7f308100...), section_infos_equal True.
- January " des %d" pushes movsx(si)=best_interval (0x3b6..0x3c5) => candidate's best_interval argument is a genuine fix of
  base's requested_rate.
- Remaining pre-existing object-admission blockers (NOT introduced by the worker): consumer-local `extern scripted_camera_set`,
  `extern struct main_hud_globals_definition *hud_globals` (caller-local struct view), `float` fields in main_globals, 4 C4013.

## R5 amendment (optional, byte-identical): main_exit /Od shape
- Hypothesis: /Od 0x6aff90 main_exit = switch(connection){1: dispose client; 2: dispose client+server} then ONE shared dispose
  sequence; the tree's main_exit duplicates the 4-call dispose tail in the server arm and a default arm (spelling from 36d4c588,
  undocumented). Now that main_loop inlines main_exit(), the /Od shape should be preferred if byte-identical.
- exit_od.c / p1x.c: gate 92/2/1; 0/196 sections differ vs p1.obj; fake scan 0; /W3 25 (same); provider_link PASS.
- Wrote main_exit_od_shape.patch (applies on tree or after production.patch) and production_amended.patch (= production.patch +
  main_exit shape; git apply --check clean on the worktree; header_swap + both crash patches still apply after it).
  Full file: candidate_production_main_amended.c (CRLF, == p1x.c).

## R6 structural-lane review items (manifest "not landed: >= -> > change and dropped requested_rate assignment need review")
- `>`: January 0x1d7..0x1eb `cmp hi; jl else; jg then; cmp lo; jbe else` = strict start > initial+slot. ATTESTED.
- dropped `requested_rate = field (or 30)`: that store was dead in base (overwritten by 999 / 60/best before any read; non-debug
  path never reads it). January 0x75 16-bit field load reproduced only by the direct ?: (structural lab l8). No semantic change.
- " des %d": January pushes movsx(si)=best_interval (0x3b6/0x3c5) -> base printed the wrong value; candidate fixes it.
- /Od 0x6b4110 main_queue_map_private RTC names `progress` [ebp-8] -> the removed main_loop precache_progress local belongs there.

## R7 symbols.json
- Worker copy: exactly 26 in-place line edits (4107,4108,4128,4143-4162,4182,4183,4188), CRLF preserved, no reserialisation.
- review3_main/config/symbols.json = worker copy with line 4188 reverted (25 lines) for landing WITHOUT patch 2.
  csplit emulation (splitcfg25 -> splitout25): only main.obj changes; 95/95 target function sections section_infos_equal to the
  current split (park target measurements unchanged); object_audit p1.obj & p1x.obj: 169 symbols, only crash + literal missing.

## VERDICT
- LAND: production.patch (or production_amended.patch) + 25-line symbols.json + park re-baselines. Zero strict credit.
- HOLD for owner ruling: header_swap.patch + symbols line 4188 (exact declared-name-count compensation; Layer 2 class).
- OWNER-GATED: owner_gated_crash*.patch (NULL write; 20260826 closeout forbade the null-cast spelling).

- Cleanup: removed tmprepo/ (nested .git), splitcfg*/ and splitout*/ (regenerable: copy config/{config,contribs,relocs,splits}.json +
  the chosen symbols.json into a dir and run build/tools/csplit.exe -i cachebeta.exe -p <dir> -o <out>). Results recorded above.
