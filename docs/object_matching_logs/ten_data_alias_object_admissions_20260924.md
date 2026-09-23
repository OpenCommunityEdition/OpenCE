# Ten whole-object data-alias admissions (2026-09-24)

Baseline: canonical `ae12a3c1`, 439/833 verified complete objects, 23
data-incomplete admission contradictions, 8,245 functions / 7,577 strict
exact. Claude's active `actor_combat` / `path_smoothing` scope was untouched.

The following ten objects already had all target functions exact. Their sole
unmatched target-owned data sections were independently found to have equal
extent, section flags, owner storage, normalized payload, and resolved
relocation destinations. The pinned, fail-closed evidence is in
`config/semantic_data_matches.json`.

| Object | Data symbol | Bytes |
| --- | --- | ---: |
| `source/bitmaps/libtiff/tif_print` | `_tif_print_rcsid` | 164 |
| `source/game/game_engine_oddball` | `_oddball_engine` | 136 |
| `source/game/game_globals` | `_global_material_type_strings` | 132 |
| `source/items/projectile_definitions` | `_default_projectile_material_response` | 160 |
| `source/memory/byte_swapping` | `_byte_bs_codes` | 144 |
| `source/memory/zlib/deflate` | `?my_version@?1??deflateInit2_@@9@9` | 4 |
| `source/memory/zlib/zutil` | `_z_errmsg` | 44 |
| `source/networking/network_server_manager` | `_network_game_server_memory_do_not_use_directly_in_use` | 8 |
| `source/physics/collision_bsp_definitions` | `_global_collision_bsp_definition_data` | 900 |
| `source/physics/collision_usage` | `_global_collision_function_names` | 138 |

Total: **1,830 previously undercredited data bytes; zero new code bytes**.
The corrected board is 449/833 verified complete objects, Halo-category
339/468. The admission audit's contradictions/revocations fall 23 to 13;
all ten cleared entries are exactly the ten rows above. No unrelated
contradiction is waived. Base-only literals in these objects were checked
for SELECT_ANY provider equivalence; the candidate has no new unmatched
code owner. Ordinary selected-provider pair links were checked in both
orders where a provider was present.

Two further status-only proposals were tested and **not admitted**:
`render_particles` has 3/3 exact functions but its 1,528-byte target data
section remains uncredited; `rasterizer_xbox_profile` has 16/16 exact
functions, but its earlier candidate-only SDK COMDAT owner rejection and
shared-declaration debt remain binding. Their `NonMatching` labels were
retained. Pair links with selected providers are useful evidence, not a
substitute for the full owner/admission checks.

Verification: full `ninja`; stable function diff against
`scratch/claude_object_closure_merged_20260924.json` gained 0 and regressed
0; parks 103 active / 0 stale / 0 invalid; pytest 1,154 passed / 5 skipped /
26 subtests; `git diff --check` clean. This batch is an accounting and
status repair, not a source-code reconstruction wave.
