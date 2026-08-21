# Typed tiny leaf wave 3 — Jonas checkpoint

This checkpoint records eleven independently strict-exact leaf functions
recovered on 2026-08-20 from authoritative campaign commit
`2f7fbec5720fe4fa04c9bee6cb6e4a04477db53a`.  The seven owning objects remain
`NonMatching`; this is partial-function evidence only.

## Signature and layout proof

All twelve audited candidates had complete return and parameter types before
the first source compile.  The HCEA Release PDB DIA type records report:

- `short heap_left_index(short)`;
- `short heap_right_index(short)`;
- `float hud_globals_get_scale(unsigned char)`;
- `void input_abstraction_reset_controller_detection_timer()`;
- `unsigned char cache_files_precache_in_progress()`;
- `unsigned short playlist_profile_number_of_default_profiles_on_disk()`;
- `unsigned short saved_game_file_get_type(long)`;
- `unsigned char transport_network_available()`;
- `void game_set_players_are_double_speed(unsigned char)`;
- `unsigned char game_players_are_double_speed()`;
- `void game_difficulty_level_set(short)`;
- `short game_difficulty_level_get()`.

The HCEA Debug PDB independently corroborates the HUD, cache, saved-game, and
four game signatures.  Repository declarations and target-local callers
provide independent type evidence for the game, saved-game, HUD, playlist,
input, and transport candidates.  January disassembly and relocations prove
the retained field offsets and leaf operations.  HCEA type records additionally
prove the game runtime prefix: `players_are_double_speed` is at `0x2`, the
embedded options begin at `0x8`, and difficulty is at runtime offset `0xE`.

## Strict retained results

`tools/coff_compare.py` reports exact padded size, normalized bytes,
relocation count, relocation type, destination identity, and addend for every
retained function.

| Object | Function | Meaningful / padded bytes | Relocs | Normalized SHA-256 |
| --- | --- | ---: | ---: | --- |
| `path_obstacle_avoidance.obj` | `_heap_left_index` | 12 / 16 | 0 | `917a9fedd32bb5126c2d04bdaf9fbf6fdcbc3aa75902d1106f38219e073fca73` |
| `path_obstacle_avoidance.obj` | `_heap_right_index` | 12 / 16 | 0 | `6a4798b5ee49d1972065decde68d52bfbfe40fa010e23ee6aab43167988be886` |
| `saved_game_files.obj` | `_saved_game_file_get_type` | 11 / 16 | 0 | `b1fb786039c8e4351b8442c7bcded97fbcce7ca3cedc6ec07bc0897a06630689` |
| `playlist_profile.obj` | `_playlist_profile_number_of_default_profiles_on_disk` | 7 / 16 | 1 | `1f82cf78b0df84e5b0bf619acb9ebfc8c165ff2558679479ef137123faa560a2` |
| `hud_draw.obj` | `_hud_globals_get_scale` | 7 / 16 | 1 | `410b62fd58558df56fc858a393c0a69c2aac6142be8edf5b4a366b99ce01a691` |
| `input_abstraction.obj` | `_input_abstraction_reset_controller_detection_timer` | 11 / 16 | 2 | `6401d253ae381cdce5605596c6384a543baf63876176a2fb07920f18734d9de8` |
| `game.obj` | `_game_set_players_are_double_speed` | 17 / 32 | 1 | `49422947cae7e2aeda7d16a5ae102b9a041fd9b31c52a6eded48ca6f3912e850` |
| `game.obj` | `_game_players_are_double_speed` | 9 / 16 | 1 | `17af9f50995ca94c2ba3d79997b48bd5fc6f56bc6053b77e817c7dcc6654b8a6` |
| `game.obj` | `_game_difficulty_level_set` | 19 / 32 | 1 | `fa722c4cadca20745287ef0441121e91fbd376bccc674af844dd67bd4c16da45` |
| `game.obj` | `_game_difficulty_level_get` | 10 / 16 | 1 | `d6bf822a580de3673043267fa11c2fa4d0ea26bdd7991192f3225bb77930ddc3` |
| `cache_files_windows.obj` | `_cache_files_precache_in_progress` | 6 / 16 | 1 | `bc02314523356da167b512d65b2d9c87044e51ae794cfb44b87d8a5a18b05e61` |
| **Total** | **11 functions** | **121 / 208** | **9** | |

The retained code increases the semantic exact ledger by eleven functions.
No owning object is marked complete.

## Runtime-data ownership

The four newly defined runtime objects use explicit zero initialization so VC7
emits external, section-relative `.bss` owners rather than COMMON symbols.
Their complete target-owned sections are strict exact:

| Object | Owner | Bytes | Relocs | Normalized SHA-256 |
| --- | --- | ---: | ---: | --- |
| `game.obj` | `_bss_0043e48c` | 4 | 0 | `df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119` |
| `input_abstraction.obj` | `_input_abstraction_globals` | 224 | 0 | `6eb69e26de2a26eda48af77d4cec893aa0cf4748a64cbefcfe11a22c1e680ad9` |
| `playlist_profile.obj` | `_bss_004d2858` | 116 | 0 | `5b517952cbe9c4c147bc3f3434f9d82409e76d09ea58905aefe7fb5415912d9a` |
| `cache_files_windows.obj` | `_bss_004cdff8` | 12,412 | 0 | `1a572ba8cb5486b4b5f81bfbd1aa501104c0e08130e63c09cda9dc5092c405bf` |
| **Total** | | **12,756** | **0** | |

Typed compile-time checks freeze the input timer at `0xD0`, playlist count at
`0x70`, cache copy flag at `0x3048`, and the corresponding complete aggregate
sizes.  The source accesses named typed fields; it contains no raw address or
offset dereference.

`hud_globals_get_scale` has one reviewed representation-only ownership detail.
January's csplit object represents `__real@3f800000` as an undefined external
symbol.  The candidate emits the same four bytes under the canonical external
`.rdata` COMDAT with selection `2` (select-any), and its function relocation
names exactly that symbol at the same address and type.  The COMDAT is
candidate-only and receives no data credit.  All January-owned non-code spans
remain unchanged.

## Rejected candidate

`transport_network_available` had a fully proven signature and compiled in the
bounded wave, but it was not retained.  Target and candidate both measured 16
padded bytes with one REL32 relocation to
`_XNetGetEthernetLinkStatus@0`; their normalized hashes were respectively
`73954fa0da7600bd1a60ddb3387eafabb788779227be715ec886a2fb1bdfe783`
and `eef4bf49c6b243bcaae881c7f8494b0d0f6d9e65505d6dbe7d8138d1f2b12b2b`.
The emitted mismatch was removed without source-shape tuning or retry.

## Source and policy audit

- Every retained definition is ordinary readable typed C with an explicit
  final return.
- No inferred ABI, assembly, volatile scheduling device, optimizer pragma or
  barrier, force-inline annotation, synthetic anchor, type-pun, raw
  dereference, undefined behavior, or byte-forcing construct is present.
- No configuration, semantic exception, parked-function record, frozen large
  object, or pre-existing Markdown file is changed.
- `vehicles.obj`, `ai_debug.obj`, `ai_script.obj`, `actions.obj`, and
  `units.obj` were kept source- and configuration-identical.  Their clean
  authoritative/current regression comparison reports no function, runtime
  data, relocation, or meaningful ownership change; only absolute-worktree
  `.debug$S` path records differ.

## Validation

- Full `halobetacache_build`, `libcmt_build`, semantic-report, and progress
  graphs pass with zero semantic unit errors.
- Semantic audit: 470 units, 3,937 functions evaluated, 3,798 semantic exact,
  3,859 accepted exact, and zero unit errors.
- Progress: 375/833 complete objects, 3,846/11,060 exact functions,
  464,554/2,198,102 exact code bytes, and 1,846,712/4,176,062 matched data
  bytes.
- The pre-edit whole-TU regression gate reports the eleven intended newly
  exact functions, the four reviewed target-exact BSS additions, compiler
  debug records, and symbol additions.  It reports no changed nonexact
  function.  The two pre-existing exact saved-game functions and HUD stack
  check independently remain strict exact.
- Admission audit: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` contradiction remains.
- Parked audit: three active, zero stale, and zero invalid entries.
- Tool suite: 179/179 tests pass.

A clean post-commit regression snapshot/check is required and recorded in the
final handoff.  No GitHub push is performed.
