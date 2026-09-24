# game_engine_king: measured at cdc8ebd3

The tree was HEAD eb33baec, which is cdc8ebd3 plus one ledger-only commit (no source or config changes). `git status` was clean before and after. No tracked file was edited. Everything below was re-run in this wave; the full probe log is in `../LEDGER.md` (K1-K12, B1).

## What the packet is

This is the review2 owner packet (`scratch/w/review2_game_engine_king/patches/OWNER_GATED_king_source_bugcomment.patch`) rebased onto cdc8ebd3, plus the worker's config patches.

- **Source:** the rebase needed no source change. The old patch still applied, and the result is review2's `strip/owner_bug_comment.c` plus exactly the two f7cd2e72 member renames: line 515 `universal_variant.score_to_win` and line 816 `game_engine_variant.king.moving_hill`.
- **Data entry:** `semantic_data_matches_king.patch` no longer applied (b9a8d587 appended entries), so it was regenerated. The entry is the worker's, with one added clause in the reason text that discloses the owner-admitted BUG. The measurements are unchanged.
- **Other config patches:** symbols, config status and symbol ownership applied cleanly; they were regenerated as canonical patches.

## Gates (every January function of the affected object)

The only affected object is `game_engine_king.obj`. The header is included only by `game_engine_king.c` (`git grep`). The build has 0 UNDEF references to any of the 23 re-storaged names: 621 objects in build/base and 833 in build/split.

`sgate.py` compiles in place with the patched header first on the include path. A `#error` probe (`king/probe/`) proved that the patched header is the one used.

| Candidate | vs build/split | vs emulated split (edited symbols.json) | Warnings |
|---|---|---|---|
| 01 packet (`king/new`) | 29/29 EXACT; the 22 callbacks show storage 2/3 (split not yet static) | **29/29 EXACT, storage equal** | C4700 `next_hill` only (the preserved read) |
| alt/01A /Od single-exit (`king/probe_se`) | - | **29/29 EXACT** | none (next_hill is assigned on the found path; C4701 is /W4) |
| if_declined storage-only (`king/storage_only`) | - | 29/29 EXACT | C4700 `next_hill_id` (as production) |
| production (tree), control | 29/29 (`gate.py --all`) | - | C4700 |

- The `_point_from_line3d` emitted-symbol guard passes for every candidate.
- `/W3 /Zs` shows the same two TU-local C4244 warnings (lines 922 and 972, `convex_hull2d`/`hill_points_to_2d` short args) in production and in the packet. Neither is introduced by the packet.
- `objcmp` against `new.obj`: production, 01A and storage-only all have 59 owner-keyed sections with 0 byte/relocation/flag differences.
- Symbol rows: only the 23 storage rows change 2 -> 3. Between 01, 01A and storage-only there are 0 symbol-row differences.

## Emulated split

`emu_split.py` runs `build/tools/csplit.exe -i cachebeta.exe`, using the slug's config copy.

- `emu/tip`, from the unedited config, is byte-identical to build/split (833/833), so build/split is current.
- `emu/king`, from the 02 symbols.json + 03 config.json, differs from build/split ONLY in `source/game/game_engine_king.obj`.

## object_audit (against the emulated split)

`object_audit_s.py` is `scratch/tools/object_audit.py` plus a `SPLIT_ROOT` override.

- **`new.obj`: PASS.**
  - 39 January symbols, 0 differ.
  - Every January section is ok. This includes `.data _king_engine` 136/136 and `.bss _king_globals` 560/560.
  - The 01A candidate also PASSES.
- **Controls:**
  - `new.obj` vs build/split: FAIL(23), storage only.
  - Production vs emulated split: FAIL(23).
  - Production vs build/split: PASS.
- **pdb_storage (SPLIT_ROOT variant):** `new.obj` vs the emulated split has 0 disagreements. Production has 23: the 22 callbacks and `_king_globals` are absent from cachebeta publics. The only publics are `_king_engine`, `_render_dynamic_quad` and `_render_dynamic_quad_initialize`.

## Surplus / provider_link

- **surplus_id_s (code + rdata):** 22 candidate-only external sections, 0 not identical/unprovided.
  - The 10 real_math.h code COMDATs (`_cross_product3d`, `_distance3d`, `_distance_squared3d`, `_magnitude3d`, `_magnitude_squared3d`, `_normalize3d`, `_random_range`, `_scale_vector3d`, `_square_root`, `_vector_from_points3d`) are identical to January's action_* copies.
  - The 12 literals are identical to their January definers. The `'king'` literal matches game_engine.
- **provider_link:** `new.obj` has 22 rows; **SELECTED-PROVIDER LINK: PASS** in both orders.
- **nodup_census** for the 10 helpers: 0 NODUP definers in build/base.
- The surplus set is unchanged from production (only storage differs).

## Data

`_king_engine` .data is 136 B with 23 relocations, normalized sha `4009f5b6117e9f00cb001a23b9a56b9dc51a694206c442ce715e2bed2d5465a7`, flags `c0400040` on both sides.

- `section_info_resolved` gives target == base in all 8 combinations of {tip symbols.json, edited} x {build/split, emulated split} x {production obj, new.obj}.
- `build/report.json` shows king .data at 98.24561%. Unmatched data is 136 B, which is exactly this section. The cause is the objdiff 3.3.1 `'$'` literal-name defect: the `'king'` literal is UNDEF in January (folded to game_engine) and a defined COMDAT in ours.
- The 04 entry is a single-section entry with no `allow_incomplete_unit`.

## Emulated admission chain

`emu_chain.py` does the following:

1. Builds a mini objdiff-cli 3.3.1 report for the unit. Control: the mini row for build/split + build/base equals the production `build/report.json` row. **True.**
2. Regenerates the semantic audit. Control: a fresh BASE run equals the `build/semantic_report.json` summary. **True.**
3. Runs the production chain in order:
   - semantic rejections, semantic matches, the accepted ledger;
   - data matches, ownership snapshots;
   - `revoke_incomplete_units`;
   - parks;
   - `audit_object_admission`.

| | Halo complete units | complete_code | complete_data | matched_data | matched_code | revoked | parks | admission |
|---|---|---|---|---|---|---|---|---|
| BASE (tree) | 379 | 890,841 | 714,870 | 2,576,122 | 1,573,109 | [] | 86/0/0 | 11/0/2/0 |
| EMU king (01-05) | **380** | 895,706 (+4,865) | 715,746 (+876) | 2,576,258 (+136) | unchanged | [] | 86/0/0 | 11/0/2/0, 0 contradicted |
| EMU king + glow | **381** | 902,693 (+11,852) | 716,138 (+1,268) | 2,576,258 | 1,574,838 (+1,729) | [] | 85/0/0 | 11/0/2/0 |

- The data credit `game_engine_king:_king_engine (+136 data bytes)` validates without `allow_incomplete_unit`.
- The ownership snapshot `game_engine_king:.bss (3 symbols)` validates.
- The king functions are already credited in production: 29/29, 4,865/4,865 code, `_find_next_hill` at 100%. The admission therefore adds object completion and data only, with no new strict function.

## Patches (`patches/`, see `APPLY_ORDER.txt`)

- `git apply --check` passes on each patch. It also passes on 01-05 combined, and on king 01-05 + glow 01-03 combined.
- `--whitespace=error-all` is clean.
- Trial-applying 01-05 onto tip copies reproduces `king/new` byte-for-byte (CRLF), and every JSON file parses.
- The edited symbols.json full copy is `king/new/config/symbols.json`. It is also saved as `king/symbols.json.cdc8ebd3_king_edited`.
- fake_match_scan on the packet .c/.h and the 01A .c: 0 leads.

## Objects that become COMPLETE if the owner says yes

- `source/game/game_engine_king`: 29/29 functions, 876/876 data, 39/39 symbols. Halo objects 379 -> 380.

No other object's completion depends on this ruling. The header edit reaches no other TU.
