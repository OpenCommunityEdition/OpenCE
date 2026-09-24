# main (source/main/main) - wave-3 worker ledger (fifty-objects lane, base 954eebd2)

## Baseline (gate --all on verbatim copy base.c)
`== exact 92  residual 2  unwritten 1` ; non-exact:
- UNWRITTEN 16 _main_crash
- residual 544 _main_frame_rate_debug [relocs 51!=52, sha]
- residual 1440 _main_update_time [relocs 118!=117, sha]
Full rows: base_all.txt

## Prior work read (do not repeat)
- crash_policy_closeout_20260826: _main_crash = `mov dword ptr [0], offset "chucky was here!..."; ret`; forbidden spellings: null cast, raw address,
  pointer/integer reconstruction, volatile, asm, pragma, intrinsic, obj patch. Owner-gated (brief s7: authentic NULL deref class).
- w3d/n3a/n4/n5 (opus5 lanes): frame_rate_debug family closed (arm-order, chained assignment, write-back placement, d1/d2, f1/f2/f4);
  residual = threaded duplicate `mov dl,[+902]` on reset&&debug edge + dl/bl CLEAR store.
- structural lane 20260923 (/c/halo-worktrees/claude-structural-20260923/scratch/lane/w/main__main/REPORT.md): update_time best v25 =
  3 blocks / 5 rows (selected_interval phi SI vs AX); v1..v28 recorded; frame_rate_debug f1..f6 recorded. Their candidate.c NOT landed.

## Probe 0 - start point = structural-lane candidate (struct_cand.c, never landed)
gate --all: `exact 92 residual 2 unwritten 1`; update_time `[sha]` (relocs fixed, 117==117), frame_rate_debug `[relocs 51!=52, sha]`.
No EXACT row moved vs base. update_time residual (sbs 0x3f8..0x416): January phi of selected_interval lives in SI
(debug path best_interval already SI; non-debug path `je 0x3ff` -> `mov si,ax`; store `mov [+950],si` after `mov eax,[ebp-0x28]`);
ours phi in AX (`mov eax,esi` on debug path, store `mov [+950],ax` before `movsx`).

## Probe 1 - _main_crash (c1.c)
Hypothesis: January 11 B = `mov dword ptr [0], offset ??_C@_0CK@...chucky...; ret` (DIR32 at +6); /Od 0x6aff50 is the same single
store with NO frame slot (so no named pointer local), /Od caller 0x5e5380 pushes one dword arg (hs `main_crash(char const *reason)`, the
existing hs.c prototype). Source: `*(char const **)NULL = "chucky was here!  NULL belongs to me!!!!!"; return;` placed right after
main_start_time (January definition order stop_time, start_time, crash, print_version).
Result: `_main_crash` EXACT first compile; gate `exact 93 residual 2 unwritten 0`; zero EXACT rows moved (name-count safe at this position).
STATUS: OWNER-GATED (brief s7 authentic NULL-dereference class; crash_policy_closeout_20260826). Reported, not landable.

## Finding 2 - storage audit (pdb_storage.py): 26 main.obj functions are NOT cachebeta publics (=> file-static in January)
create_local_players, main_change_map_name, main_exit, main_frame_rate_debug, main_framerate_throttle_enabled, main_game_render,
main_initialize_time, main_load_core_private, main_lost_map_private, main_new_map, main_queue_map_private, main_reset_map_private,
main_reset_time, main_respawn_private, main_revert_map_private, main_run_demos_private, main_save_core_private, main_save_map_private,
main_saving_map_private, main_setup_connection, main_skip_cinematic_private, main_skip_private, main_switch_to_structure_bsp_private,
main_update_time, main_won_map_private, sort_controllers_ascending. HCEX.pdb independently: `static void main_update_time()`.
Only main_game_render is also prototyped outside main.c (main.h:27). All others referenced only in main.c.

## Probe 2 - c2.c = c1 + `static` on 23 (all but main_game_render; frame_rate_debug/reset_map_private already static)
Result: `exact 81 residual 2 unwritten 12` - 12 functions VANISH (static + unreferenced => not emitted): main_exit,
main_framerate_throttle_enabled, main_load_core_private, main_lost_map_private, main_queue_map_private, main_respawn_private,
main_revert_map_private, main_run_demos_private, main_save_core_private, main_saving_map_private, main_skip_cinematic_private,
main_switch_to_structure_bsp_private. January emits them yet has NO in-object relocation to them and they are not public =>
January's source CALLED them and VC7 inlined every call (same as main_reset_map_private today: static, inlined twice in main_loop,
still emitted). main_loop in our tree hand-expands their bodies (rule 13 violation).
/Od 0x6b1150 main_loop (later build) attests the shape: the !game_in_editor() branch is a chain `if (flag) xxx_private();` - 16 calls
(0x6b4ed0 switch_bsp, 0x6b3390 lost_map, 0x6b56e0 won_map, 0x6b4330 respawn, 0x6b4880 saving_map, 0x6af420 change_map_name,
0x6b4410 revert, 0x6b4c90 skip_cinematic, 0x6b4230 reset_map, 0x6b4580 save_core, 0x6b0f50 load_core, 0x6b34a0 menu_load,
0x6b0f80 load_last_solo, 0x6b4490 run_demos, 0x6b4cc0 skip, 0x6b4110 queue_map).

## Probe 3 - c3.c = c2 + main_loop calls the private functions (hand expansions removed); main_loop tail `main_exit();`;
## main_update_time `framerate_throttle = main_framerate_throttle_enabled();` (January emission order: reset_time, throttle_enabled, update_time)
Result: `exact 93 residual 2 unwritten 0` - all 12 re-emitted EXACT, main_loop EXACT (1648/204 sha 47124c50 unchanged),
update_time/frame_rate_debug bytes unchanged (inert). Storage now matches cachebeta for 25/26 (main_game_render needs main.h).

## Probe 4 - c5.c: merge best_interval into selected_interval on the CURRENT (v25-based) body (structural lane's v18/v19 were on v16)
Result: worse - selected_interval memory-homed ([ebp-0x1c]/[ebp-0x20] slot swap), EBX/EDI swap for minimum_target_index, join reloads
from memory. Merge negative confirmed on the v25 base.

## Probe 5 - name-count ORACLE (lab only, never landable): 1..30 `struct oracle_tag_k;` before main_update_time (on c3)
Result: 30/30 byte-identical to c3 (sha ad56536c5eb1). The SI/AX phi is NOT a TU name-count tie -> source-structure fact.

## Probes 6-9 (update_time join; all on c3)
- c6a/c6b/c6c: `selected_interval = best_interval;` moved right after the loop (a: all post-loop uses via selected; b: uses unchanged;
  c: copy before " des" print) -> all `[size 1456!=1440]`: the whole web goes callee-saved ESI from the idiv (`mov esi,eax` @0x96). Negative.
- c7a: separate `requested_interval` + `else { selected_interval = requested_interval; }` edge copy (January's `mov si,ax` block sits
  after the debug block, i.e. where an else-arm would be laid out) -> byte-identical to c3 (copy coalesced). Negative (short form of
  structural v27).
- c8a/c9b: add before store -> phi goes ECX (live across cdq), store after adc, size 1456. c9a `target_index += (cur = sel);` -> identical to c3.
- Reading: January IL order at the join is conv(sel) -> lo -> load min.hi -> STORE cur (last use of phi SI) -> mov esi,edx; add/adc.
  Ours (store-first source) emits store before conv; add-first source emits store after adc. No tested C statement order yields
  "store between conversion and add". Residual = phi register (SI vs AX) + that store position. STOP per methodology rule
  (structural lane v1..v28 + c5..c9 = ~36 shapes; only register/IL-order left).

## Probe 10 - name-count ORACLE for _main_frame_rate_debug (lab only): 1..40 opaque tags before its definition (c3)
Result: 40/40 byte-identical (3c132f904203, 51 relocs). Not a name-count tie.
January CFG reading: January == our CFG + ONE jump-thread of the edge (reset && debug) -> S -> test2(debug) -> body: the thread
creates S' (copy of the sample-index load) placed before the body, and test2's fall-through needs `jmp 0x70` over S'. So the source
arm structure is already right; the open question is why VC7 threads the global re-test in January and not in ours.

## Probe 11 - frame_rate_debug threading micro-labs (lab/t1*.c, standalone TU with main.c's cflags; lab/lab.py)
t1 (our structure) reproduces ours: no thread. Variants a (debug_frame_rate extern-only), b (static), f (n3a f1 arm shape),
g (long sample_index), h (tiny body) -> none threads the global re-test. Diagnostic goto form (t1goto, NOT landable) lays S2 as an
inline then-arm (`je CLEAR; mov al,[idx]; jmp body`), NOT January's layout (S2 before body behind test2's `jmp`), so January's shape is
a post-layout thread, not a source goto. Combined with n5's CLEAR-store fact (January stores framerate_sample_index from DL, i.e. did NOT
constant-propagate sample_index=0; ours uses pinned BL), January's optimizer made two value-tracking decisions ours does not. No source
lever found. STOP (reopen criterion of n5 unchanged).

## Probe 12 - main_game_render storage needs a main.h edit (shared header): consumer blast-radius sweep
Harness: sweep/sweep.py copies each of the 20 main.h consumers (grep of `#include "main.h"`/`"main/main.h"`) into a PRIVATE dir with a
shadow main.h (verified picked up with a #error shadow), compiles with the unit's build.ninja cflags; gatecmp.py = gate-equivalent
section_infos_equal verdicts vs build/split; cmp.py = byte-level section diff between tags.
- base (original main.h): 833/849 consumer functions exact.
- new (main_game_render prototype REMOVED): REGRESSES ui_widget_game_data_input_functions `_player_profile_3wide_list_update` (1232 B)
  -> declaration-count tie (-2 names).
- swap (remove main_game_render prototype + add the genuine public `void main_crash(char const *reason);` beside main_print_version,
  January order stop/start/crash/print_version): 833/849, byte-identical to base in all 20 consumers (0 differing sections).
  main.c with swap header (sweep/mainc.py): prod_hdr 92/95 (only crash/frd/ut non-exact), prod_gated_hdr 93/95, 0 warnings.
Disclosure: the swap is count-neutral by construction; each half has independent evidence (main_game_render not a cachebeta public and
only called by main_loop; main_crash IS a cachebeta public, called from hs (hs.c holds a consumer-local prototype)).

## Production candidates
- prod.c = base + 25 statics + main_loop private calls + main_exit call + main_framerate_throttle_enabled call + structural-lane fuzzy
  bodies (update_time v25, frame_rate_debug f2). gate --all: exact 92 residual 2 unwritten 1; EXACT rows identical to base.
- prod_hdr.c = prod + static main_game_render (needs main.h swap). prod_gated.c / prod_gated_hdr.c = + main_crash (owner-gated).

## Probe 13 - consumer-local prototype cleanup in main.c (rule 9), measured on all four variants
- remove main.c's duplicate `extern struct bitmap_data *bitmap_2d_new(...)` (bitmaps/bitmaps_internal.h, already included, declares it)
- replace main.c's local `extern void game_end_credits_start(void);` with `#include "interface/attract_mode.h"` (genuine owner header,
  "header included in hcex build")
Result: prod2/prod_hdr2/prod_gated2/prod_gated_hdr2 byte-identical to prod/prod_hdr/prod_gated/prod_gated_hdr in EVERY non-debug
section (0 differing); gate rows unchanged; with the swap main.h (sweep/mainc.py) prod_hdr2 92/95, prod_gated_hdr2 93/95, 0 warnings.

## Final audit of the full owner-gated candidate (prod_gated_hdr2.c + swap main.h)
- gate --all: exact 93 residual 2 (update_time [sha], frame_rate_debug [relocs 51!=52, sha]) unwritten 0
- object_audit vs CURRENT split: only the 2 residual .text DIFFs + 26 `storage 2/3` rows (exactly the 26 cachebeta non-publics that
  config/symbols.json copy marks "static": true; split regen required). All January-owned .rdata incl. the chucky literal ok.
- storage vs cachebeta publics: 0 disagreements (prod2 without header swap: 1, _main_game_render).
- surplus: 42 candidate-only sections (unchanged vs base); surplus_identity 2 COMDATs (_fast_ftol, _arctangent) IDENTICAL;
  provider_link SELECTED-PROVIDER LINK: PASS.
- fake_match_scan: 0 leads.

## Data gap census (1,848 B = report.json total_data 4507 - matched 2659)
- .rdata 1796 B @ 97.510376% (all-or-nothing -> 0 credit): the ONLY January-owned .rdata section missing from base is the 42-B
  `??_C@_0CK@IJIKLLCA@chucky?5was?5here?$CB?5?5NULL?5belongs?5t@` literal owned by _main_crash. Closes only with the owner-gated crash.
- .data 52 B @ 22.771671%: `_global_difficulty_level` section (difficulty, spawn count, throttle, screenshot size, static
  scenario_paths[10]); coff_compare resolved-equal (size 52, 10 DIR32 relocs to the `levels\...` literals, sha 7f308100e812...);
  objdiff 3.3.1 '$'-literal relocation-name defect -> needs a pinned config/semantic_data_matches.json entry at admission.
- .bss 2659 B: 100%.

## FINAL (wave 3) - verdict FUZZY_IMPROVED (+ owner-gated exact _main_crash); object NOT completable yet
Patches (LF, git apply --check clean vs 954eebd2; sequences verified in a CRLF/autocrlf temp repo):
1. production.patch (main.c only, landable, zero strict credit): 25 cachebeta non-public functions -> static; main_loop calls the
   private helpers again (hand expansions removed; VC7 re-inlines them byte-identically) + main_exit(); main_update_time reads
   main_framerate_throttle_enabled(); structural-lane fuzzy bodies (update_time v25 1440/117 sha ad56536c..., frame_rate_debug f2
   544/51 sha 3c132f90...); duplicate bitmap_2d_new prototype dropped; game_end_credits_start from interface/attract_mode.h.
   gate --all 92/2/1, EXACT rows identical to base. Parks to RE-BASELINE (not retire): _main_update_time base -> 1440/117/
   ad56536c5eb119b981a9d0b89feb8bc57be3d72a21b4fcdfac4b4939db97ccdc; _main_frame_rate_debug base -> 544/51/
   3c132f904203944a7f5f7c3b746a7458964f073c1fc4374aba0988bb5f3db4ff (objdiff % to re-measure after ninja).
2. header_swap.patch (SHARED HEADER main.h + main.c one-liner; apply after 1): main_game_render prototype out of main.h (static),
   genuine public main_crash prototype in. 20-consumer private sweep: byte-identical (removal alone regresses
   _player_profile_3wide_list_update 1232 B). Consumers: see sweep/consumers_all.txt.
3. owner_gated_crash.patch (after 1+2) / owner_gated_crash_nohdr.patch (after 1 only): _main_crash EXACT. OWNER-GATED.
4. config/symbols.json copy: 26 lines "static": true (line 4188 _main_game_render only together with patch 2).
Remaining machine differences: update_time join phi SI vs AX + store position (IL order); frame_rate_debug un-threaded global
re-test (1 reloc, 8 B) + CLEAR store operand DL vs BL.
