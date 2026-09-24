# Adversarial review: source/interface/player_ui (claimed OBJECT_COMPLETE_CANDIDATE)

Reviewer slug: `scratch/w/review3_player_ui/`. Worker: `scratch/w/player_ui/`. Base tree: 954eebd2 (no .git in the
worktree; `git apply --check` was run against the working files).

## Verdict

**APPROVE, but only with the reviewer amendment 07.** Apply it together with 01-06, and prefer 04-amended over 04.
The packet as submitted has one real defect. Everything else re-verified independently.

The defect: patch 03 retypes `player_ui_get_edit_playlist_profile()` to `struct game_variant *`. That adds two new
**C4133** diagnostics in `source/interface/ui_widget_game_data_input_functions.c` (lines 2119 and 2757): "'=' :
incompatible types - from 'game_variant *' to 'playlist_profile *'". The worker never saw them because that unit is
missing from their consumer census. It directly `#include`s both `interface/player_ui.h` and `game/game_engine.h`, but
their `includes.json` lacks 16 of the 608 build units. The object bytes are identical, but the packet introduced a
type-incompatible implicit pointer assignment into production source.

## Patches (apply in this order)

| # | File | Source | Status |
|---|---|---|---|
| 01 | source/game/game_engine.h (SHARED) | worker `patches/01_game_engine_h_game_variant_layout.patch` | verified |
| 02 | 10 consumer .c renames | worker `patches/02_game_variant_member_renames.patch` | verified |
| 03 | source/interface/player_ui.h (SHARED) | worker `patches/03_player_ui_h_game_variant.patch` | verified; needs 07 |
| 04 | source/interface/player_ui.c | **reviewer `patches/04_player_ui_c_amended.patch`** (worker 04 plus a refresh of 5 stale `_code_000d05xx` names in the file's symbol comment) or worker 04 | object identical |
| 05 | config/symbols.json (+1 line) | worker `patches/05_symbols_json_player1_profile_path.patch` | verified by a real csplit run |
| 06 | admission config | worker `patches/06_admission_config.patch` | verified JSON and policy load |
| 07 | source/interface/ui_widget_game_data_input_functions.c | **reviewer `patches/07_ui_widget_game_data_input_functions_game_variant.patch`** | IDENTICAL object, removes the 2 C4133 |

All 7 pass `git apply --check` (and `--whitespace=error-all`) against the current tree. They apply in sequence to a
fresh copy (`applied2/`) with CRLF preserved. Full candidate files are in `cand/`.

optB (the 3-constant flag enum) is optional and verified: 45/47 consumers identical. Only the parked
`main _main_update_time` moves, so it needs a park re-baseline. units stays 189/189. Not needed for admission.

## Independent measurements

- **Full-board sweep, not just the listed consumers** (`tcomp.py`, `fullsweep.py`). A full copy of source/ has the patches
  applied, and each unit compiles in place with the build.ninja flags, /I rewritten to the copy. Each unit is compared
  with build/base section by section (name, flags, size, bytes, relocations by symbol) plus the full symbol table.
  - Compiler-local `$L`/`$name$N` label ordinals are normalised; 27 units shift them only.
  - Result: 453/454 source units IDENTICAL. player_ui differs only by the new `_player1_profile_path` static and the
    2 functions whose relocations now target it.
  - January exact counts are unchanged in all 454 units.
  - Control: production source with the same script is 454/454 IDENTICAL to build/base.
  - Files: `sweep_final_amended.txt`, `sweep_all_prod_control.txt`.
- **Consumer census** (/showIncludes over all 454 units):
  - game_engine.h: 38 units.
  - player_ui.h: 14 units.
  - Union: 46 units (`consumers_header_union.txt`). The worker listed 45 and missed
    `source/interface/ui_widget_game_data_input_functions`.
  - playlist_profile.h: 5 units. `saved games/saved_game_files` is also missing from the worker list, but that header
    is unchanged.
- **/W3 warning diff** on the 47 units (patched vs production, `warn_diff.py`):
  - Without 07: 2 net new C4133 in uwgdif.
  - With 07: **net 0**. The remaining +/- pairs are line shifts in race and slayer, plus the pre-existing uwehf C4133
    whose type text changes.
- **gate / exactness**: player_ui 42/42 EXACT (candidate obj `rev_player_ui.obj`, `rev_player_ui_amended.obj`). The
  shadow-free compile makes gate.py unnecessary: gate cannot see patched headers.
- **csplit regeneration (real tool, not simulation)**: `build/tools/csplit.exe -i cachebeta.exe` with a copy of config/.
  - Control: 833/833 objects byte-identical to build/split.
  - With patch 05: only player_ui.obj changes.
  - object_audit against the regenerated split: candidate **PASS** (68 January symbols, 0 differ).
  - Production against the new split: FAIL, `MISSING _player1_profile_path`. 04 and 05 must land together.
- **objdiff 3.3.1** (mini project, `od_new` vs `od_ctl`): the candidate against the new split scores the same as
  production against the old split.
  - Functions: 41/42 raw.
  - The known semantic entry `_set_local_player_controls_from_player_profile` scores 95.89041.
  - Data: 2004/2004.
- **Surplus**:
  - surplus_identity: 0 code COMDATs.
  - provider_link: PASS in both orders.
  - `string_audit.py`: the 3 strings (`""`, the 61-byte controller_index assert, `L""`) are each UNDEFINED in
    January's player_ui split. Each is section_infos_equal to the selected provider copy (action_obey,
    input_abstraction, game_engine), with COMDAT selection 2 on both sides.
- **pdb_storage**: 0 disagreements. `_player_ui_globals` is public in cachebeta. `player1_profile_path` is absent, so it
  is static.
- **fake_match_scan**: 0 leads (13 patched files).

## Evidence re-checks

- **HCEX `-type`** confirms:
  - `game_variant` = {wchar_t[0x18] human_readable_game_description, long game_engine_index, universal_variant,
    game_engine_variant, unsigned short flags}.
  - `universal_variant` has explicit `pad0..pad2` and `pad4..pad6`.
  - The ctf, slayer, king, oddball and race member names and order.
  - HCEX's January-size prefix ends at `vehicle_set` (+0x2C).
- **January field offsets** (UI assert labels in uwehf): slayer 0x4C 'death bonus', 0x4D 'kill penalty' and 0x4E
  'kill in order' match HCEX's no_death_bonus, no_kill_penalty and kill_in_order.
- **Other first-party strings**:
  - "failed to retrieve editable game variant" is present in the January uwehf split.
  - The race assert literal is present in the January race split.
  - The /Od build has the strings universal_variant, human_readable_game_description, game_engine_variant,
    score_to_win and odd_man_out.
- **Path static relocations** (relocs.json): +0x230 is referenced 6 times, only by
  `_player_ui_remember_player1_profile` (3) and `_player_ui_get_player1_last_used_profile_index` (3). The highest other
  referenced offset is +0x22C (`initialized`), so a natural 0x230 struct fits.
- **HCEX path**: `File Static char[0x100] player1_profile_path`, placed right after player_ui_globals. HCEX
  `_player_ui_globals` and `_player_data` have no pad members, so removing the unknown35/155/22D pads is consistent.

## Strip tests

- `static char player1_profile_path[0x100];` (uninitialised) puts the path at .bss 0 and the globals at 0x100, and 2
  functions lose exactness (40/42). So `= { 0 }` is load-bearing.
- With both statics uninitialised, player_ui_globals becomes COMMON and there is no .bss, which fails. The chosen
  spelling is therefore the only plain form that works. The same file already writes `player_ui_globals = { 0 }`,
  86 file-scope `= {0}` precedents exist (including admitted editor_flying_camera), and it follows the decoded VC7 .bss
  law.
- Race `match_vassert` with an explicit string becomes a plain `match_assert`. This is the plainer spelling, and the
  object is identical.
- The HCEX pads and `ball_spawn_delay` are load-bearing only through the declaration count (the worker's Oracle 1, and
  Probe 1). Both are first-party attested: pads by HCEX, `ball_spawn_delay` by January builder stores at 0x4D and by
  HCEX. So they are admissible, not fillers.
- No new block scopes, so no /Od declaration-order question arises.

## Oracles (not landing choices)

- Adding HCEX's `terminator_variant { long ignored; }` to the union costs units `_unit_preprocess_node_orientations`
  (188/189). The worker's omission is count-consistent with January.

## Advisory (non-blocking)

- oddball and race members are `long` where HCEX uses enum types; the enums are still TU-local.
- `universal_variant.flags` keeps the production `unsigned long` (HCEX has `long`). The worker's `shr` argument is
  weak, because masking with 1 allows `shr` for signed values too, but keeping the production type changes nothing.
- The flag constant is duplicated TU-locally: `_variant_is_system_default_bit` in player_ui.c and
  `_game_variant_is_system_default_bit` in playlist_profile.c. optB unifies them, at the cost of a park re-baseline.
- The reason text of the semantic_matches entry for `_set_local_player_controls_from_player_profile` still cites the
  playlist boundary, which is now stale.
- Follow-ups outside this object:
  - uwehf's `(byte *)` / `playlist_profile_item_options_prefix` views, and its pre-existing `wchar_t *` C4133 at
    lines 3324 and 3540.
  - uwgdif's remaining local `struct playlist_profile` view and `cached_variant_profile_entry`, which HCEX types as
    game_variant.
