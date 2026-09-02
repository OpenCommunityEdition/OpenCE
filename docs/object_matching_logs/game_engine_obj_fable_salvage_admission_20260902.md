# `game_engine.obj` Fable salvage and admission (2026-09-02)

## Verdict

The preserved Fable wave is credible after reconciliation.  It advances
`source/game/game_engine.obj` from 166/180 to 169/180 strict-exact functions,
closes all six formerly unwritten slots, and retains every canonical exact
function.  The admitted strict gain is three functions and 1,488 target padded
bytes.  The remaining three newly written bodies are semantic, target-shaped
fuzzy reconstructions and are explicitly parked; they receive no exact credit.

This admission was performed in the isolated worktree
`work/fable-game-engine-salvage-20260902` from canonical commit
`778961b899d926354d38a33ef2db4463e1137c24`.  The read-only preserved input was
commit `5d2bc08d8` on `root/fable-small-families-salvage-20260902`; it was imported
as local snapshot commit `f8aff9f3f`.  Nothing was pushed and the canonical
checkout was not modified.

## Authorities and source provenance

- January's split object, `build/split/source/game/game_engine.obj`, is the byte
  authority.  Candidate measurements use XDK 3911 and
  `tools/campaign/gate.py`, whose strict predicate compares padded section
  bytes, normalized bytes, and relocation identity/count.
- January disassembly and relocations establish control flow, callback offsets,
  constants, and the otherwise surprising ignored unit-validation call in
  `game_engine_player_killed`.
- The independent HaloCEA reconstructions under
  `C:/Users/isabe/Documents/Codex/reference/halocea/src` corroborate the six
  functions' algorithms and semantic names.  In particular,
  `game_engine_player_killed.c`, `game_engine_update_item_spawn.c`,
  `game_engine_get_state_message.c`, `find_closest_player_index.c`,
  `game_engine_get_place.c`, and `game_engine_should_spawn_player.c` were read
  as secondary semantic evidence, not copied as a byte oracle.
- HaloCEA database enum headers authenticate the game-engine message values,
  multiplayer-sound values, postgame states, score mode, and place-flag bit
  meanings.  Existing local names, structures, cseries types/macros, object
  accessors, and exact siblings were preferred wherever they differed from the
  later port.
- The preserved experiment ledger
  `docs/object_matching_logs/game_engine_obj_fable_small_families_20260901.md`
  records every rejected source-shape family.  Its historical `unknown_d1`,
  `unknown60`, and `/W3 /WX` statements are superseded by this admission ledger.

## Strict baseline and admitted gain

The canonical baseline at `778961b89` gated as:

```text
== exact 166  residual 8  unwritten 6  (of 180 listed)
```

The reconciled candidate gates as:

```text
== exact 169  residual 11  unwritten 0  (of 180 listed)
```

All 166 baseline exact functions remain exact.  The only newly exact functions
are:

| Function | Padded bytes | Relocations | Normalized SHA-256 |
|---|---:|---:|---|
| `_game_engine_get_state_message` | 320 | 10 | `20ff545777ef3592b92e0608f9abc73f010429f15a655e4a3645f1ef1842b831` |
| `_game_engine_update_item_spawn` | 336 | 11 | `7e6f5319e160462229c51753194e9aa87f9ce25176cb9f2239cb508aaa885ee8` |
| `_game_engine_player_killed` | 832 | 39 | `4d4a8b42b57d29707d911f742b640cbb3e9783404a2bc369cabb7e73760e15a9` |

The padded-byte gain is `320 + 336 + 832 = 1,488`.

## House-rule reconciliation

The preserved bodies were retained only after the following cleanup continued
to pass the strict gate:

- `autoaim_compute_target` now has its typed prototype in its owner header,
  `source/game/aim_assist.h`.  `find_closest_player_index` uses `real`,
  `real_point3d`, and `real_vector3d` outputs instead of an unowned local
  `void *` declaration.
- The callback at `struct game_engine + 0x60` is named and typed as
  `player_killed_player` in `source/game/game_engine.h`; the gameplay function
  no longer casts an `unknown60` function pointer.  The public prototype now
  calls its second parameter `killing_object_index`, matching its actual use.
- `player_datum.quit_out_of_game`, named message constants, named postgame and
  multiplayer-sound constants, the DB-authenticated
  `_equipment_created_at_rest_bit`, `NUMBEROF`, and `TICKS_PER_SECOND` replace
  inherited unknown fields and magic numbers.
- The place function's second parameter is the existing
  `enum get_score_type`; it uses `_get_score_team` and named flag bits owned by
  `source/game/game_engine_place.h`.
- Typed `player_get`, `unit_get`, `item_get`, scenario/tag-block, and
  item-collection access are retained.  The one generic `object_get` is
  intentional because the code must inspect and distinguish a biped, vehicle,
  or other object.  The adjacent ignored `unit_try_and_get` is not fake code:
  January calls `_object_try_and_get_and_verify_type` with unit mask `3` and
  discards its result, and HaloCEA independently records it as validation.
- The closest-player body deliberately retains consecutive 400.0 and 900.0
  comparisons against the same squared-distance local.  Although the second
  bound is mathematically redundant, this is not a coincidental source trick:
  January's instruction stream loads that same stack local for both compares
  against the exact `400.0f` and `900.0f` constants, and the independent
  reconstruction records the same pair.  Removing it would contradict both
  available evidence.
- Functions keep the established multiline parameter style and explicit
  terminal `return;`.  No inline assembly, force-inline/noinline attribute,
  volatile/register forcing, compiler barrier, pragma optimization control,
  fake dependency, invented address identifier, ABI coercion, or nonsensical
  source was added.

The callback/header edits preserve structure size and offsets.  Before final
admission, the direct consumer inventory found 19 translation units including
`game_engine.h`; `game_engine.c` is the only direct `aim_assist.h` consumer,
and `game_engine.c` plus `game_engine_slayer.c` directly include
`game_engine_place.h`.  A complete `all_source` rebuild and the independent
572-object strict exact-set sweep cover the compiler's header-position
sensitivity rather than assuming ABI-compatible declarations are harmless.

## Honest fuzzy parks

These complete bodies are useful semantic progress but are not presented as
exact.  Their detailed evidence, hashes, and reopen criteria are recorded in
`config/parked.json`:

| Function | Target/base padded bytes | Target/base relocs | Objdiff | Class |
|---|---:|---:|---:|---|
| `_find_closest_player_index` | 448/448 | 18/18 | 94.2638% | instruction scheduling |
| `_game_engine_should_spawn_player` | 256/256 | 9/9 | 99.71265% | register allocation |
| `_game_engine_get_place` | 368/368 | 12/12 | 94.49254% | register allocation |

The first residual is confined to terminal conversion/failure block ordering.
The second is January's retained integer-one register versus VC7's natural
immediate and countdown-register choice.  The third is the allocation of two
boolean byte locals plus `AND` versus `TEST`.  The retained `all_tied & tied`
is meaningful because both operands are normalized booleans; spelling it as a
logical AND or explicitly materializing the operation grows the function by
16 bytes and was rejected.  All three parks require new first-party/local
evidence or a natural compatible-compiler donor before reopening.

## Remaining unit residuals

```text
  176  _default_starting_location_rate_function       sha
  112  _drawline                                      sha
  448  _find_closest_player_index                     sha (parked here)
 1152  _game_engine_generate_title_string             base 1088, relocs 70/73
  368  _game_engine_get_place                         sha (parked here)
   96  _game_engine_get_starting_location_rating      sha
 2528  _game_engine_post_rasterize_post_game          sha
  256  _game_engine_should_spawn_player               sha (parked here)
  640  _game_engine_verify_current_map                base 1104, relocs 60/52
  144  _multiplayer_message                           sha
  496  _select_players_to_display                     base 480
```

`game_engine.obj` therefore remains active and `NonMatching`; it is not an
object-complete claim.

## Final verification

- `python tools/campaign/gate.py source/game/game_engine --all`: 169 exact,
  11 residual, zero unwritten.
- Complete `ninja all_source` and split/report builds: pass.
- Independent per-function comparison of all 572 built units against a freshly
  configured and fully rebuilt detached worktree at exact canonical commit
  `778961b89`: zero lost exact functions; exactly the three closures above
  gained.  A previously reused sibling build was discarded as a baseline when
  concurrent lanes advanced it during this audit.
- Whole-object code-owner census: the candidate-only code-owner set is
  identical to canonical's inherited 11 math helpers.  The six new definitions
  all correspond to January target owners; no new candidate-only helper is
  emitted, and `point_from_line3d` is absent.
- `python -m tools.parked_functions`: 109 active, zero stale, zero invalid.
- Semantic/progress reports: 473 units scanned, 5,303 functions evaluated,
  5,140 semantic exact, 5,153 accepted exact, 137 hidden exact / 81,391 hidden
  bytes, and zero unit errors.  Progress reports 674,495 / 2,198,102 code
  bytes and 5,107 / 11,060 functions, with no admission credit assigned to the
  three fuzzy parks.
- `python -m pytest tools -q`: 258 passed.
- Fake-match scan of all four changed C/header files: zero review leads.
- Object-admission audit: zero candidates or contradictions; the sole reported
  repository rejection is the pre-existing `player_rumble` COMDAT-ownership
  record, unrelated to this wave.
- `git diff --check`: pass.

An additional `/W3 /WX` diagnostic compile is **not** clean and is not an
admission claim.  It stops first on inherited warnings in `cseries.h` and
`real_math.h`, then lists older narrowing/undeclared-call warnings elsewhere
in this 8,000-line translation unit.  The new/reconciled bodies add none of the
reported warnings.  The preserved ledger's assertion that this whole TU was
`/W3 /WX` clean is therefore corrected here rather than silently repeated.

## Do not repeat

- Do not hand-expand the first state-message helper call; the natural three
  calls let XDK 3911 inline only the first and are byte-exact.
- Do not restore the item-spawn `long`/`do-while` loop or raw `+0xC` tag access;
  the exact sibling-shaped `short` loop and typed collection field are exact.
- Do not add an early source `return;` to the quitting-player arm or move the
  final message tests inside the non-quitting arm; those restore the rejected
  block layout.  The function already has its required explicit terminal
  return.
- Do not remove the target-proven ignored unit validation solely because its
  result is unused.
- For the fuzzy functions, do not retry volatile/register qualifiers, pragmas,
  barriers, assembly, forced inlining, storage overlays, dummy dependencies, or
  the source-shape variants exhaustively catalogued in the preserved ledger.
