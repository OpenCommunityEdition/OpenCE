# `saved_game_files.obj` selective Opus reconciliation (2026-09-04)

## Result

This packet selectively reconstructs the evidence-backed portion of the frozen
Opus `saved_game_files.obj` work on canonical commit
`6d351b3578237a58b5036297417370e65bb066c6`.  It does not cherry-pick the donor
branch or modify the frozen tree.

The strict object result advances from **6/46** to **38/46** exact functions:

- **+32 strict-exact functions**;
- **+7,436 meaningful exact code bytes** (`124 -> 7,560`);
- **+7,744 padded exact code bytes** (`160 -> 7,904`);
- **+548 exact data bytes** (`0 -> 548`: 8 `.data` + 540 `.bss`);
- **zero rename-stable regressions** in the full 8,245-function sweep.

The final object has 5 credible fuzzy residuals and 3 genuinely unwritten
functions.  It remains `NonMatching` and receives no false completion label.

Campaign totals move from 855,687 to 863,123 meaningful exact code bytes,
5,955 to 5,987 accepted exact functions, and 2,007,670 to 2,008,218 exact data
bytes.  These deltas are exactly the object-local gains above.

## Provenance and audit boundary

- canonical base: `6d351b3578237a58b5036297417370e65bb066c6`;
- frozen donor: `C:\halo-worktrees\opus-small-families-30k-20260902`;
- donor branch/tip: `opus/small-families-30k-20260902` at
  `eeedd72e5ba0fc1761519f68cad241fe606320e7`;
- principal donor source commit: `608c66380`;
- donor naming commit inspected: `1689e891e`;
- donor notebook read in full:
  `scratch/reports/saved_game_files.md`;
- donor naming evidence read:
  `scratch/reports/naming_net-io-text.md`;
- prior canonical saved-game ledgers, campaign methodology, exact-match
  playbook, throughput lessons, verification policy, prototype-inference
  policy, and Claude repository guidance were read before admission.

Only the saved-game reconstruction, authenticated symbol renames, corrected
owner declarations, five measured park records, and this ledger are retained.
No dirty donor tree or unrelated donor commit was imported.

## Source reconstruction

The recovered translation unit uses ordinary typed C for the saved-game
lifecycle, mutex protocol, path persistence, XDK metadata operations, mapfile
enumeration, packed profile indices, checksum generation, default profiles,
entry lookup/add/remove, open/close, delete, and display-name helpers.

The 32 newly strict functions are:

1. `_saved_game_file_close`
2. `_saved_game_file_get_useable_untitled_profile_name`
3. `_saved_game_files_take_mutex`
4. `_saved_game_file_name_unique`
5. `_saved_game_file_remember_player1_last_used_profile_directory`
6. `_saved_game_file_retrieve_player1_last_used_profile_directory`
7. `_saved_game_file_remember_last_used_multiplayer_variant_directory`
8. `_saved_game_file_retrieve_last_used_multiplayer_variant_directory`
9. `_saved_game_file_remember_last_used_multiplayer_map`
10. `_saved_game_file_retrieve_last_used_multiplayer_map`
11. `_saved_game_file_generate_checksum`
12. `_find_or_create_directory`
13. `_enumerate_mapfile_begin`
14. `_enumerate_mapfile_end`
15. `_append_entry_to_mapfile`
16. `_open_mapfile_for_reading`
17. `_close_mapfile_after_reading`
18. `_read_next_entry_in_mapfile`
19. `_count_enumerated_profiles_in_mapfile`
20. `_saved_game_files_take_mapfile_mutex`
21. `_build_saved_game_file_index`
22. `_saved_game_files_initialize`
23. `_enumerate_default_playlist_profile_files`
24. `_enumerate_default_player_profile_files`
25. `_get_nth_entry_in_mapfile`
26. `_add_new_entry_to_mapfile`
27. `_remove_nth_entry_in_mapfile`
28. `_saved_game_file_get_display_name`
29. `_delete_enumerated_saved_game_file`
30. `_saved_game_file_open`
31. `_saved_game_file_get_path_to_enclosing_directory`
32. `_saved_game_files_enumerate_default_files`

Every listed function passes strict section bytes, padding, relocation count,
relocation type, relocation destination, and addend comparison.

## Donor corrections made during reconciliation

### The path tables are not an invented struct array

The donor notebook proposed one array of an 8-byte
`memory_unit_definition`.  January's indexed load disproves that layout:
`_enumerate_mapfile_begin` uses an effective address equivalent to
`[memory_unit * 4 + 4]`.  An array of 8-byte structs would require an 8-byte
stride.

The admitted source therefore preserves the two natural parallel arrays:

- `memory_unit_root_path` at target file offset `3238196`;
- `memory_unit_mapfile_path` at target file offset `3238200`.

Together they own the exact 8-byte `.data` section and its two string
relocations.  This is both source-meaningful and byte-exact; the unsupported
aggregate facade was rejected.

### The default player-profile filename came from target bytes

The donor used
`z:\\saved\\player_profiles\\default_profile\\%02d\\blam.sav`.  The raw
January string at target file offset `2787472` is instead
`z:\\saved\\player_profiles\\default_profile\\%02d.sav`.  Correcting that
evidence mismatch closes `_enumerate_default_player_profile_files` exactly.

### BSS ownership is real, contiguous, and exact

The canonical starter declared `saved_game_files_globals` as an undefined
external, which could never claim the target BSS.  The owner now defines it
with a zero initializer after the 256-byte static display-name buffer.  VC7
therefore emits January's exact 540-byte section:

- `_saved_game_file_display_name` at `+0x000`, 256 bytes;
- `_saved_game_files_globals` at `+0x100`, 284 bytes.

The aggregate BSS bytes, symbol offsets, linkage, and relocation ownership all
match.  No `bss_` address-derived production identifier remains.

### Names and declarations

Private helper names are backed by January strings, the January atlas,
September linker-map order, exact caller topology, or a combination of those
sources.  The mapfile mutex pair and `enumerate_memory_units` have independent
wrapper/map corroboration.  The remaining unwritten mapfile helper is named
`update_nth_entry_in_mapfile` from its target behavior and its own "failed to
update entry" diagnostic, rather than retaining an address-derived placeholder.
Target symbols are marked static only for true private helpers.

Public saved-game declarations now live in
`source/saved games/saved_game_files.h`; player and playlist lifecycle/count
declarations live in their owner headers; and `file_get_size` lives in
`source/tag_files/files.h`.  The previous donor-local cross-subsystem
declarations were removed.  The playlist count prototype was corrected to the
owner's actual `word` return type; the full rebuild confirms no codegen loss.

All declarations retain the campaign format: one parameter per line, `void`
on its own line for no-argument functions, cseries integer types, and explicit
`return;` in void function bodies.

## Honest residual frontier

Five complete ordinary-C functions are recorded in `config/parked.json` with
fresh hashes and objdiff measurements:

| Function | Target / candidate padded bytes | Relocations | Objdiff | Boundary |
| --- | ---: | ---: | ---: | --- |
| `_create_enumerated_saved_game_file` | 912 / 896 | 50 / 50 | 97.72535% | shallow fallback lifetime/register allocation and trailing padding |
| `_saved_game_file_find_profile_index_for_directory_path` | 352 / 432 | 25 / 33 | 72.14286% | VC7 inlines exact `close_mapfile_after_reading`; January calls it |
| `_saved_game_files_delete_all_custom_profiles` | 224 / 224 | 14 / 14 | 96.51316% | one January loop-alignment NOP |
| `_saved_game_files_enumerate_available_to_local_player_index` | 368 / 432 | 23 / 31 | 74.090164% | defined failure result plus private-helper inline-budget divergence |
| `_saved_game_perform_file_system_checks` | 208 / 208 | 10 / 10 | 85.783134% | equivalent saved-game count-loop rotation |

No noinline/force-inline directive, padding source, fake dependency, inert
branch, volatile/register forcing, barrier, pragma, raw byte emission, inline
assembly, false linkage, or nonsensical control flow is retained to force
these residuals.

The available-profile enumerator deliberately initializes its output count to
zero before mutex acquisition.  January's outer failure path appears to read a
dead parameter slot through an uninitialized local; retaining that artifact
would be undefined, nonsensical source, so this packet accepts the honest fuzzy
cost instead.

The remaining genuinely unwritten target functions are:

- `_update_nth_entry_in_mapfile` — 480 padded bytes;
- `_enumerate_memory_units` — 992 padded bytes;
- `_synchronize_metadata_display_name_with_profile_name` — 1,040 padded
  bytes.

They are intentionally not filled with speculative source.

## Verification

Validation was run from the isolated reconciliation worktree after all header
and symbol changes:

- full `ninja -j 6`: pass, including every transitive header includer;
- focused `gate.py source/saved games/saved_game_files --all`:
  `38 exact / 5 residual / 3 unwritten`;
- rename-stable before/after sweep:
  `+32 functions / +7,744 padded bytes / 0 regressions`;
- `python -B -m pytest -q`: `261 passed`;
- `python -B -m tools.parked_functions`:
  `205 active / 0 stale / 0 invalid`;
- `python -B tools/audit_object_admission.py`:
  `0 candidates / 0 contradicted`; only the two pre-existing explicit
  rejection records remain;
- `python -B -m tools.audit_semantic_matches`:
  `0 unit errors`;
- changed-file fake-match scan: 0 review leads;
- `git diff --check`: pass;
- `units.obj` sentinel: `189 exact / 0 residual / 0 unwritten`;
- built `saved_game_files.obj` symbol audit: no `point_from_line3d` symbol or
  COMDAT.

The January inline schedule is therefore unchanged outside this object, and
the packet introduces no forbidden `point_from_line3d` owner.

## Canonical replay

The isolated commit was replayed after the admitted network-client and
network-server-message packets.  The additive `config/parked.json` conflict was
resolved by retaining both networking park sets and all five saved-game
records; no source conflict or donor-wide overwrite was accepted.

The full current-tree rebuild and rename-stable sweep against
`scratch/after-network-server-handler-replay-final-20260904.json` reproduce the
isolated result exactly: **32 strict gains / 7,744 padded bytes / 7,436
meaningful bytes / zero regressions** across all 8,245 January function
sections.  `saved_game_files.obj` remains **38 exact / 5 parked residual / 3
unwritten**, and the owner BSS/data packet adds **548 exact data bytes**.

Aggregate canonical progress moves from 864,305 to **871,741 / 2,198,102
meaningful code bytes**, from 5,989 to **6,021 / 11,060 functions**, and from
2,015,358 to **2,015,906 matched data bytes**.  Linked objects remain **391 /
833**.  The current semantic audit evaluates 6,435 functions across 473 units,
accepts 6,094 exact proofs, and reports zero unit errors.

Current-tree validation also passes with **212 active / 0 stale / 0 invalid**
parks; zero admission candidates, contradictions, or revocations and only the
two inherited explicit rejections; zero focused fake-match findings; 261
tooling tests; a clean diff check; exact
`units::_unit_preprocess_node_orientations`; the unchanged 19/2/1 server
message-handler frontier; and no `point_from_line3d` symbol in the rebuilt
saved-game object.  The protected bitmap files retain their audited local
SHA-256 values
`AB6002BF2BAECDBC18C7B308BB7846C58FBCF1C7C72F586621D2B1F271D49140`
and `B435F38E406C7DCDB60B4DF12E78BCF98D1E4B0E57FE2AB2240BC78F799EBA93`.
