# source/saved games/player_profile

## verdict
ADMISSION_FIX_PROPOSED

## newly_exact_functions
[]

## candidate_files
[
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/player_profile/production.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/player_profile/production_owner_gated.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/player_profile/owner_gated_over_production.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/player_profile/production_defined_full.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/player_profile/owner_gated_over_defined_full.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/player_profile/final_min.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/player_profile/final_og.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/player_profile/final_def.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/player_profile/config/symbols.json",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/player_profile/mk_cands.py",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/player_profile/od_new.txt",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/player_profile/od_default.txt",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/player_profile/od_write_thread.txt",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/player_profile/hcex_player_profile_compiland.txt",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/player_profile/audit_final_og_sim.txt",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/player_profile/LEDGER.md"
]

## production_changes
There are two alternative packets. Every patch passes `git apply --check` on 954eebd2 (LF patches, same convention as the saved_game_files patches). Stacking was verified in a temp repo. No shared header changes: player_profile.h is untouched, and nothing in source/ or build/split other than player_profile references the renamed table.

(A) ADMISSIBLE NOW, zero code credit: scratch/w/player_profile/production.patch (full file: final_min.c)
1. source/saved games/player_profile.c:
   - `long player_profile_primary_colors[NUMBER_OF_AVAILABLE_PRIMARY_COLORS] =` becomes `static long profile_color_table[NUMBER_OF_AVAILABLE_PRIMARY_COLORS] =`.
   - The use in player_profile_get_rgb_color becomes `color = profile_color_table[color_index];`.
   - The header comment `_data_00316880 (0000)` becomes `_profile_color_table (0000)`.
   - Delete the invented `byte pad[3];` member from struct player_profile_runtime_globals. HCEX _player_profile_globals has no pad member, and the size stays 0x6C by alignment; the verify typedefs still compile.
   - In player_profile_write_thread_proc, move the `if (failed) { delete_enumerated_saved_game_file(player_profile_index); }` block from the end of the open-success arm to just after the open if/else, before saved_game_files_release_mutex(). This is the /Od 0x8597f2 placement.
2. config/symbols.json line 22790: edit it IN PLACE to `{ "file_offset": 3238016, "flags": 0, "name": "_profile_color_table", "static": true },`. Do not re-serialise. An edited full copy is at scratch/w/player_profile/config/symbols.json.
3. Regenerate the split with csplit only, then run a full ninja build and the stable sweep.

Expected result:
- Against a csplit-simulated split, player_profile stays 19 exact / 3 residual.
- new and create_default bytes and sha are unchanged, so their parks stay valid.
- Re-baseline ONLY the _player_profile_write_thread_proc@4 park: base 384 / 27 relocs / normalized_sha256 bbb652e083aadc92e51e4445f93d49d41941e7c53f50cc1883658c355aa579cb / objdiff 88.451324 (was 88.40708).
- Data stays at 1644/1644.

The optional production_defined_full.patch (final_def.c) additionally:
- replaces the invented padded struct with HCEX's `struct player_profile_internal { struct player_profile profile; XCALCSIG_SIGNATURE checksum; }` + `union player_profile_block { byte data[SAVED_GAME_FILE_BLOCK_SIZE]; struct player_profile_internal profile_block; }`;
- adopts the /Od RTC local names `profile_file` (new, write thread) and `full_path` (create_default).

Its objdiff effect is mixed:
- new 76.57 -> 81.26
- write_thread -> 87.566
- create_default 83.80 -> 82.659 (size 336 -> 320)
- object fuzzy 95.34 -> 95.71

It re-baselines all 3 parks with these values:
- wt 384/27 sha 2d4283752fce12c07d725d55a9652db562bb2915a3616eee5c856f67c9bc5de4 87.56637
- new 464/18 sha 4f42ddba4987c94403cfbf71ace340ba622d71174ce7b6cc9cf6dde511267601 81.26
- create_default 320/14 sha c4f4ba33e3473e3e145d0cca590641ee5c124d00cf7e9c781f05f0f891612b3f 82.65909

(B) OWNER-GATED, report only, do NOT land without rulings R1-R3: production_owner_gated.patch (full file: final_og.c), or owner_gated_over_production.patch on top of (A). On top of (A) it:
- adopts the HCEX union type and /Od names;
- write thread and create_default: `union player_profile_block block;` with no initialiser (BUG comment);
- create_default: `if (file_create(&file) && file_open(&file, FLAG(_permission_write_bit)) && file_set_position(&file, 0)) { succeeded = file_write(...); if (!file_close(&file)) error(...); }` (BUG comment);
- player_profile_new: `union player_profile_block block = {0};`, drops `boolean succeeded`, then `if (!file_set_position(&profile_file, 0) || !file_write(&profile_file, sizeof(block), &block)) { error(...); delete_enumerated_saved_game_file(player_profile_index); player_profile_index = NONE; }` followed by the BUG-commented `saved_game_file_close(&profile_file, player_profile_index);`.

Result: all 22 functions EXACT. If the rulings are granted:
- retire the 3 parks in config/parked.json (lines ~120/139/158, via tools.campaign.unpark);
- after the full build and sweep, promote config/config.json line 400 (`source/saved games/player_profile.c`) from NonMatching to Matching.

## evidence
Baseline gate: 19 exact / 3 residual.
- create_default: [size 336!=288, sha]
- new: [relocs 18!=19, sha]
- write_thread: [size 384!=352, sha]
- Data in report.json is 100%: .bss 108, .data 72, .rdata 1464.

New first-party evidence since the 20260909 owner rejection:
1. The later /Od build reproduces all three January defects.
   - player_profile_new (/Od 0x85d0c0): the failure arm sets index=NONE (0x85d1fa) and then reaches the SHARED saved_game_file_close(&profile_file, index) (0x85d20c). RTC names: profile_file 280, block 8192. The block init is `mov byte [block],0 + memset(block+1,0,0x1fff)`, a 1-byte head, matching January's `mov byte [ebp-0x204],bl`.
   - create_default (/Od 0x85acf0): the block has NO initialiser, and the seek is folded into `file_create && file_open && file_set_position`, so a failed seek skips file_close. RTC names: file, full_path, block.
   - The write-thread body (/Od 0x8596d0): block uninitialised; `if (failed) delete` sits AFTER the open if/else (0x8597f2).
   - The later build_default memsets 0x1ffc, so the later format fully overwrites its (still uninitialised) block. January's 0x30 profile + 0x14 signature leaves 444 stack bytes in the file.
2. HCEX.pdb player_profile compiland:
   - `union player_profile_block { unsigned char data[0x200]; struct player_profile_internal profile_block; }` is the local `block` in new, create_default and read. Raw bytes FIRST explains January's byte-width head store.
   - `static long profile_color_table[0x12]` is a File Static = our 72-byte .data table at 0x316880. It is absent from cachebeta publics, and in build/split only player_profile.obj references it.
   - _player_profile_globals has no pad member.
3. Solved the +0x10b byte that ledgers had called unexplained since 20260909. Production's `if (failed)` inside the open arm binds `je` to the other arm's release_mutex copy (0x146); the /Od placement binds it to its own copy, as January does (cand_wt1). lab_wt2 (that placement + uninitialised block) makes write_thread EXACT. The uninitialised-padding ruling is therefore the ONLY write-thread blocker.

final_og (owner-gated):
- Against a csplit-regenerated split (edited symbols.json only; differs from the current split only in the renamed symbol, storage 2->3): 22/22 EXACT (simgate.py).
- object_audit with the simulated target: PASS, 51 January symbols, 0 differ, all sections ok.
- provider_link: SELECTED-PROVIDER LINK PASS. The 8 surplus items are 4 pooled literals, 3 D3D tables and __real@3b808081.
- surplus_identity: 0 code COMDATs.
- objdiff 3.3.1: code 4119/4119, 22/22 functions, data 1644/1644.
- /W3: no TU-local warnings. fake_match_scan: 0 leads.

final_min against the simulated split: 19/3, with production bytes except the write thread; objdiff 95.34539.

Inert labs: lab_nopad and lab_hcexnames. Recorded negatives were not repeated.

## blockers
Three owner rulings on authentic January defects. Each is attested by January's bytes AND the later first-party /Od build, and each has a BUG comment in final_og.c:
- R1: the 512-byte file block is left uninitialised in the write thread and in create_default, so 444 stack bytes are written to disk. This is not UB: the indeterminate bytes are read as byte.
- R2: create_default leaves the file open when the seek fails.
- R3: player_profile_new calls saved_game_file_close(&file, NONE) after a failed write, which fires the 603/606-609 asserts. It also deletes the file before closing it.

The 2026-09-09 saved_game_family_reconciliation rejected R1-R3 without the /Od attestation. No defined source reaches January's bytes:
- R1 needs zero fill instructions and no extra relocation;
- R2 needs the close skipped;
- R3 needs `push esi`, the returned NONE register, as the close argument.

Separately, the object needs the (A) storage/name fix (profile_color_table static) to pass pdb_storage once the functions are exact.

## reopen_criteria
The owner admits R1-R3 with the BUG-comment form. Then:
- apply owner_gated_over_production.patch (or production_owner_gated.patch);
- regenerate the split with csplit;
- run a full ninja build and the stable sweep;
- retire the 3 parks;
- mark player_profile Matching.

Without the rulings, reopen only on new first-party evidence of a defined January spelling. None is possible for R1/R3 given the byte proofs above.

## task notes
I found no landable path that makes the three functions exact; each exact form is an owner-gated original bug. The whole object reaches 22/22 exact, audit PASS and data 100% only under the owner-gated patch. Patch (A) is landable now: it fixes storage and naming and adds the /Od statement placement, with no code credit. Worker rules held: no tracked file was edited, and ninja/configure/git mutations were not run. csplit was run only into scratch/w/player_profile/splitsim with a config copy (splitcfg/). The /Od and HCEX binaries were read as data only. Precedent for the verdict: saved_game_files wave 1 (ADMISSION_FIX_PROPOSED with a separate owner-gated patch). Integrator caution: the symbols.json rename needs a csplit regen before the gate. Against the CURRENT split, _player_profile_get_rgb_color reports [reloc-identity] until the regen. That is expected, and it matches exactly against the simulated split.
ledger: C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/player_profile/LEDGER.md
