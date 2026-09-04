# `game.obj` safety-pair Opus reconciliation (2026-09-04)

## Scope

This packet completes the two game-safety leaves deferred from the earlier
options/loading packet. They were selectively reconstructed from the frozen
Opus donor `opus/small-families-30k-20260902`, commit `8aed00b49`:

- `_game_all_quiet`: 64 padded bytes
- `_game_safe_to_speak`: 32 padded bytes

Together they contribute 96 strict padded bytes and 75 meaningful code bytes.
The donor commit was not cherry-picked.

## Source and declaration review

Both routines are natural short-circuit predicates over existing subsystem
queries. `game_all_quiet` checks projectiles, items, effects, dangerous units,
and AI visibility; `game_safe_to_speak` checks projectiles and dead players.
The code uses the named boolean APIs directly and contains no recreated masks,
raw state offsets, or matching-only control flow.

The donor relied on six declarations placed locally in `game.c`. This packet
instead puts the three previously missing declarations in their owning
`ai/ai.h`, `effects/effects.h`, and `game/players.h` headers and consumes all
six owner headers from `game.c`. The duplicate `game_all_quiet`,
`game_safe_to_speak`, `game_safe_to_save`, and `game_is_cooperative`
declarations were removed from `hs.c`, which already includes `game/game.h`.
The two changed no-argument declarations in `game.h` use the canonical
multiline `void` form.

Because several owner headers are broad, the verification deliberately rebuilt
98 transitive consumers. The stable sweep found no C2 definition-position or
allocation regression. The retained source contains no address-derived name,
inline assembly, forced inline, volatile/register steering, representation
pun, fake dependency, or nonsensical coincidental match.

## Verification

- `game.obj` gate: 18 exact, 0 residual, 9 unwritten (previously 16/0/11).
- `hs.obj` gate: 445 exact, 3 residual, 0 unwritten, unchanged after duplicate
  declaration removal.
- Rename-stable whole-tree sweep: +2 exact functions / +96 padded bytes, zero
  regressions across 8,245 target functions.
- Full Ninja build and semantic report: success; 5,934 semantic exact, 5,951
  accepted exact, zero unit errors.
- Progress report: 843,658 / 2,198,102 matched code bytes and 5,882 / 11,060
  matched functions; data remains 2,001,721 matched bytes and 391/833 objects
  are linked.
- Fake-match scan: zero review leads over the changed source/header surface.
- Park audit: 188 active, 0 stale, 0 invalid.
- Object-admission audit: 0 candidates, 0 contradictions, 0 revocations; the
  two inherited explicit rejections are unchanged.
- Tool tests: 261 passed.
- Rebuilt and target game/HS objects emit no `point_from_line3d` symbol.
- Protected bitmap source hashes remain unchanged.

Stable snapshots:

- before: `scratch/after-devices-accessor-trio-20260904.json` (5,923 exact)
- after: `scratch/after-game-safety-pair-20260904.json` (5,925 exact)
