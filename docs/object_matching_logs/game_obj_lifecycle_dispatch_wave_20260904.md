# `game.obj` lifecycle/dispatch wave, 2026-09-04

> Integration decision: canonical retains only `game_tick` and
> `game_precache_new_map` (+768 padded / +760 meaningful bytes). The complete
> four-function packet is preserved locally at commit
> `d98ec50b9847ace3f1dfc26df42d3400b0cacacf`, branch
> `codex/game-lifecycle-header-blocked-20260904`, worktree
> `C:\Users\isabe\Documents\Codex\2026-07-13\i-w\worktrees\game-lifecycle-header-blocked-20260904`.
> Correct initializer declarations in `decals.h` and `game.h` independently
> regress an inherited Decals exact function. The two initializers and those
> causal declarations are withheld, not discarded. The final integration
> batch ledger supersedes the isolated full-wave counts for canonical credit.

## Scope and authority

This bounded wave starts from canonical commit
`96086c9c4c187dd539af13bdaa9252d6e46ad8f6`. It reconstructs only the four
previously unwritten January owners `_game_initialize`, `_game_tick`,
`_game_initialize_for_new_map`, and `_game_precache_new_map`. The already
parked `_game_load` and `_game_set_game_variant_from_name` were not reopened,
and `_game_safe_to_save` remains deliberately unwritten after its earlier
rejected reconstruction.

January PC COFF bytes, relocations, strings, symbols, and ABI behavior are the
byte authority. The Pastudan January lift and the local HCEA source were used
only as semantic corroboration. In particular, HCEA's later
`game_tick(int game_ticks)` signature was not imported: January's owner is a
no-argument function. The campaign methodology, throughput guidance, and all
prior `game.obj` ledgers were read before implementation.

## Strict result

All four fresh functions are strict exact under the hardened per-function
gate:

| Function | Padded gain | Meaningful gain | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_game_initialize` | 272 B | 257 B | 47 | `45cee772ee3a94ec05225fa83e11287efed3815e65a8233e5cbefcec0d56c711` |
| `_game_tick` | 272 B | 268 B | 37 | `7045cefc636f85c42ccc5f0f314e55768a9114db23a440e52dc72abf06ec47f9` |
| `_game_initialize_for_new_map` | 352 B | 345 B | 57 | `08aa3554c632b86bedfa03bb1c36579153699d4a3aef6fc3a9a08a759520b578` |
| `_game_precache_new_map` | 496 B | 492 B | 51 | `61591a0e9cdb15e7d848cff1e63ab642cd428f7adc2d809df3469d695cc19ba9` |
| **Total** | **1,392 B** | **1,362 B** |  |  |

The final isolated candidate is
`scratch/game-lifecycle-dispatch-bss-final-20260904.obj`. Its full target
function census is 24 exact, two unchanged residuals, and one unwritten owner.
All 20 inherited exact functions remain strict exact. There is no
candidate-only code definition; `_game_safe_to_save` is the sole target-only
code owner.

## Source reconstruction

`game_initialize` is the January subsystem initialization dispatcher in its
observed order, including the runtime globals allocation and zeroing, variant
zeroing, math reset, all subsystem initializers, transport/network startup,
saved-game/event/input/UI/Bink startup, and the final progress-bar
initialization. `transport_initialize` is retained as a status-returning call:
January proves EAX returns `0` or `-5`; the family-consistent `short` spelling
is an ABI inference from the owning transport status API, not a claim that
debug type records proved the original typedef.

`game_tick` brackets the exact update sequence with profile and collision-log
periods, asserts the active game, updates allegiance/units/AI/players, selects
the typed `real` seconds-per-tick value, preserves the random-seed lock around
rumble and first-person weapons, and finishes the engine/editor/script/object,
HUD, and player-effect updates in January order.

`game_initialize_for_new_map` restores the saved random seed, recreates the
game-engine state, invokes the complete January new-map dispatcher, starts
statistics and the update server, enables the typed runtime active flag, then
places objects and AI before allowing UI widget loads. The two January
assertions use the recovered source-line identities `0x1D1` and `0x1D2`.

`game_precache_new_map` retains the ordinary queue/cancel/begin decision tree,
blocking progress pump, error handling, damaged-media path, map-save path, and
single-player profile memory. The status comparison uses the real owner enum
constant `_cached_map_file_failed`, moved without value changes from
`cache_files_windows.c` to `cache_files.h`. A block-local typed
`game_runtime_globals_prefix *globals` is used only for the two adjacent pairs
of runtime-state stores; this natural lifetime produces January's register
reuse without a fake dependency, cast, volatile qualifier, or source-order
trick. A project `long` receives the signed precache status, matching the
observed full-width control flow.

## Data and symbol ownership

The source now restores all ordinary storage owned by January `game.obj`:

| Owner | Size | Relocations | Normalized SHA-256 | Result |
| --- | ---: | ---: | --- | --- |
| `.data` beginning at `_global_game_difficulty_level_names` | 1,544 B | 5 | `b9fc935546bd5102ec9a33ff40c8f818b2864dca2dff3398dcf8a49e664d0c30` | strict exact |
| public `.bss` `_game_globals` | 4 B | 0 | `df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119` | strict exact |

The data begins with the four public difficulty-name pointers and includes the
private `game_update_section` at offset `0x10`. January independently records
that private owner at image address `0x2DDDC0`; it is named semantically rather
than by address. `_game_globals` is an external/public symbol in both target
and candidate, not an invented private. No COMMON owner or additional
writable storage is emitted.

## Declaration ownership and house rules

The four bodies consume declarations from their owning subsystem headers.
Missing pure API declarations were added to the existing owners for AI,
allegiance, cache files, cheats, decals, interface/HUD/first-person weapons,
players/player queues, profile, recorded animations, sound classes, telnet,
transport, and game statistics. `update_server_new` belongs to
`player_queues_new.h`; `main.c` now includes that owner and drops its
consumer-local, incorrectly `void` declaration. Existing owner declarations
for `game_engine_dispose`, `collision_log_begin_period`, and
`collision_log_end_period` replace their duplicate declarations in `game.c`.
No layout was moved into a shared header for this wave.

Every new no-argument definition/declaration uses `void` on its own line,
parameters are one per line, and every void function ends in an explicit
`return;`. Project `real`, named enum values, typed runtime fields, and existing
subsystem APIs are used instead of primitive stand-ins, raw offsets, raw
object/tag casts, or manual bit logic. No function is forced inline or
noinline, and no inline assembly is present.

## Exclusions and honest boundary

No later-build lifecycle call, raw layout view, address-derived private name,
false prototype, representation pun, undefined behavior, dummy expression,
optimizer barrier, register/volatile forcing, pragma, fake branch, or
nonsensical exact-only logic is retained. The January inline schedule is
preserved. The emitted-symbol guard confirms that this object does not define
`_point_from_line3d`.

The two pre-existing residuals remain parked under their existing evidence:
`_game_load` is a 208-byte natural near-match with a final independent pointer
load scheduled differently, and `_game_set_game_variant_from_name` remains the
48-byte candidate for January's 96-byte owner because VC7 eliminates the
authenticated intermediate/dead inline path. This wave makes no new claim for
either function and does not erase their valid source progress.

## Verification boundary

- `python tools/campaign/gate.py source/game/game --all
  --forbid-emitted-symbol _point_from_line3d --out
  scratch/game-lifecycle-dispatch-bss-final-20260904.obj`: pass, 24 exact / 2
  residual / 1 unwritten.
- Hardened COFF comparison: the complete 1,544-byte `.data` and 4-byte `.bss`
  owners are strict equal, including all five data relocations and public/static
  symbol classes.
- Complete emitted-code inventory: zero candidate-only code owners; only
  `_game_safe_to_save` is absent from the candidate.
- No repository-wide build, stable sweep, semantic report, park validation,
  admission decision, commit, or push is claimed here. Those remain the root
  integration gate, including investigation of unrelated full-build header
  sensitivity found outside this isolated object gate.
