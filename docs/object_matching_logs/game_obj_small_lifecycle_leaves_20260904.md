# `game.obj` small lifecycle leaves, 2026-09-04

## Scope and authority

This packet was reconstructed in the isolated
`agent/game-small-leaves-current-20260904` worktree from canonical integration
commit `55351b58d0060b5569971cf50d0b2a5514cce632`. Canonical itself was not
edited and nothing was pushed. The bounded target set, in increasing January
padded size, was:

- `_game_set_game_variant_from_name` (96 bytes);
- `_game_unload` (128 bytes);
- `_remove_quitting_players_from_game` (176 bytes); and
- `_game_load` (208 bytes).

January PC COFF instruction bytes, relocations, symbol ownership, and strings
are authoritative. Before editing, the campaign methodology and throughput
guides, every existing `game.obj` ledger, the January target disassembly for
all four owners, the relevant Stian January-PC lift, the independent later HCEA
implementations, local exact-repository history, and Marathon's map-teardown
analogue were read. HCEA and Marathon were used only as semantic corroboration.
No applicable `CLAUDE.md` or `AGENTS.md` exists in the exact-match worktree;
the Stian research checkout's repository instructions were also read and only
their binary-first/no-speculation principles were carried across.

## Strict gains

Two complete, independent leaves are strict-exact:

| Function | Prior state | Padded gain | Meaningful gain | Relocations | Normalized SHA-256 |
| --- | --- | ---: | ---: | ---: | --- |
| `_game_unload` | unwritten | 128 B | 128 B | 15 | `0deee83d29499ac60f39c90a83211e620cca001c076409e3a02835b7b9b5c8c1` |
| `_remove_quitting_players_from_game` | unwritten | 176 B | 173 B | 10 | `08823fcfdd5f5dd8e6524201a9c42bcb3ff2b5318a6553bc7a7cca51ddd92d25` |
| **Total** |  | **304 B** | **301 B** |  |  |

`game_unload` is the January precache-pump and map teardown: it marks loading
in progress, displays the progress widget, pumps status/render/throttle/present
until the signed status becomes nonzero, closes the widgets, reports damaged
media for status 2, ends precaching, unloads a loaded scenario, disables the
random-seed log, and clears the typed map flag. A project `long` status is
required by January's `movsx esi, ax` followed by full-width comparisons; a
`short` spelling produced a measured one-byte meaningful residual and was
rejected.

`remove_quitting_players_from_game` is the older January path, not HCEA's
later network-client cleanup loop. It iterates the typed player data, reads the
semantically named `quit_out_of_game_time`, marks a player only at the exact
scheduled tick, verifies the unit through `unit_get`, kills it without
statistics, and emits January's silent diagnostic when the requested tick has
already passed. The later cleanup/restart branches are absent from January and
were excluded.

## Honest fuzzy parks

Two source-complete owners are retained and fail-closed in `parked.json` rather
than distorted into false matches:

| Function | Target / candidate | Relocations | Objdiff | Remaining issue |
| --- | ---: | ---: | ---: | --- |
| `_game_load` | 208 / 208 B | 19 / 19 | 96.5% | final independent pointer-load schedule |
| `_game_set_game_variant_from_name` | 96 / 48 B | 4 / 2 | 54.0% | VC7 eliminates the authenticated intermediate local and unreachable inlined null path |

The retained `game_load` body is the Stian/Janaury shape: three assertions at
source lines `0x192` through `0x194`, natural inlining of the already-exact
options verifier, random-seed logging, a typed `sizeof(*options)` copy of the
complete 0x10C options structure, `scenario_load`, and the typed loaded flag.
HCEA's later `game_precache_new_map` call is rejected because January contains
no such relocation. Target and candidate differ only after `scenario_load`:
January cleans the stack and tests AL before loading `game_globals` into EAX;
VC7 schedules that independent load earlier into ECX. Declaration order,
cached-pointer order, an inner declaration scope, and direct-global forms were
boundedly tested. The natural forms return to this fixed point; the direct
form worsens to 224 bytes and duplicates global loads. The classifier returns
UNKNOWN, so the park is honestly `unclassified`.

The variant-name setter retains the historically recovered two-local source:
copy the returned named variant into a semantic local, then pass its address to
the already-exact setter. January proves both 0x68 locals and the inlined
setter's unreachable null branch. Current VC7 instead proves the pointer
nonnull, removes that branch, and writes the returned variant directly to the
global. January source-order placement, historical placement beside the
setter, individual/all later-header removal, and restoration of the historical
local-prototype context did not alter the optimized 48-byte fixed point. The
missing dead branch was not hand-authored and no emission anchor was admitted.
The classifier again returns UNKNOWN.

## Types and declaration ownership

- `game_runtime_globals_prefix::options` now uses the complete TU-private
  `struct game_options`, making the 0x10C load copy typed and in-bounds without
  exposing a shared-header definition-position hazard.
- `player_datum::unknown_cc` is renamed to the HCEA/Stian/January-authenticated
  `quit_out_of_game_time`, including its offset assertion and both existing
  users. No layout changes.
- `random_seed_debug_log` is declared by `math/random_math.h`; its empty owner
  now has the house-rule terminal `return;`.
- `main_pregame_render` and `main_rasterizer_throttle` are declared by
  `main/main.h`, and the obsolete consumer-local declarations are removed from
  `main.c`. Existing `main_present_frame` ownership is reused.
- Cache, UI, error, scenario, game-engine, player, data, and unit calls use
  their owning headers. Unit lookup goes through the typed `unit_get` macro.
- All reconstructed no-argument definitions use an explicit `void` line; each
  parameter has its own line and every void function ends with `return;`.

Adding the two missing `main.h` owner prototypes advances the compiler-private
name of `shell_xbox.obj`'s already-adjudicated SEH scope-table owner from
`$T18267` to `$T18271`. The aligned rdata payload, padded size, flags, both
relocations, and `_main` source ownership remain identical to January. The
existing semantic-data record is updated to the actually emitted private owner
name; no data bytes or exception are added.

## Exclusions and policy

No later-build network-server refresh, map-precache call, player cleanup loop,
raw object cast, raw byte offset, address-derived name, false prototype,
representation pun, undefined behavior, manual bit trick, volatile/register
forcing, barrier, pragma, inline assembly, forced inline/noinline, dummy
dependency, fake branch, or nonsensical exact-only source is retained. The
January inline schedule is preserved. The rebuilt `game.obj`, `players.obj`,
`main.obj`, `random_math.obj`, and `network_client_manager.obj` have no
`point_from_line3d` symbol or COMDAT. The protected, still-NonMatching
`units.obj` retains its pre-existing candidate-owned helper COMDAT unchanged;
this packet adds no call, owner, or new emission site.

## Verification

- Full configure and `ninja all_source progress semantic_progress`: pass.
- Focused `game.obj` gate: 20 exact / 2 residual / 5 unwritten, versus
  18 / 0 / 9 before the packet; all 18 inherited exact owners remain exact.
- Campaign progress: 884,206 to 884,507 meaningful exact code bytes
  (`+301`), 6,071 to 6,073 exact functions, data unchanged at 2,018,310 bytes,
  and linked objects unchanged at 391/833.
- Rename-stable whole-tree sweep: 8,245 functions on both sides, exactly two
  gains / 304 padded bytes / zero regressions.
- Semantic report: 473 units, 6,501 functions evaluated, 6,129 semantic exact,
  6,146 accepted exact, and zero unit errors.
- Park registry: 226 active / zero stale / zero invalid.
- Protected Units sentinel: 189/189 exact; in particular,
  `_unit_preprocess_node_orientations` remains exact at 1,920 padded bytes,
  87 relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
- Point-symbol scan: none in any directly modified owner object; the inherited
  `units.obj` owner remains present in both the baseline and packet builds and
  is not introduced or selected by this work.
- Object-admission, semantic-admission, changed-source fake-match, forbidden
  helper-symbol, tooling-test, and final diff checks are recorded as passing
  in the isolated commit.
