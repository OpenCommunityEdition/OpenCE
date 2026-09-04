# `game.obj` options/loading Opus reconciliation (2026-09-04)

## Scope

This packet selectively reconciles two independent leaves from the frozen
Opus donor `opus/small-families-30k-20260902`, commit `8aed00b49`:

- `_game_options_verify`: 32 padded bytes
- `_game_map_loading_in_progress`: 32 padded bytes

The donor commit was not cherry-picked.  Its dependency-heavy
`game_all_quiet`, `game_safe_to_speak`, and nonmatching variant-name setter are
not part of this packet.

## Source-quality review

`game_options_verify` validates the typed difficulty enum with `VALID_INDEX`.
`game_map_loading_in_progress` returns the typed runtime flag and optionally
copies the `real` loading progress.  Both declarations already live in the
owning `game/game.h`; no caller-local prototypes or unrelated header edits were
introduced.

The retained code contains no address-derived identifier, raw offset, inline
assembly, forced inline, volatile/register steering, representation pun, fake
dependency, or nonsensical matching-only logic.

## Verification

- Focused gate: both functions exact at 32/32 padded bytes.
- Object gate: 16 exact, 0 residual, 11 unwritten (previously 14/0/13).
- Rename-stable whole-tree sweep: +2 exact functions / +64 padded bytes, zero
  regressions across 8,245 target functions.
- Full Ninja build and semantic report: success; 5,900 semantic exact, 5,917
  accepted exact, zero unit errors.
- Progress report: 836,950 / 2,198,102 matched code bytes and 5,849 / 11,060
  matched functions.
- Fake-match scan: zero review leads in the changed source.
- Park audit: 185 active, 0 stale, 0 invalid.
- Object-admission audit: 0 candidates, 0 contradictions; inherited explicit
  rejections unchanged.
- Tool tests: 261 passed.
- The January inline schedule remains intact and the packet emits no
  `point_from_line3d` COMDAT.

Stable snapshots:

- before: `scratch/after-path-input-node-leaves-20260903.json` (5,889 exact)
- after: `scratch/after-game-options-loading-leaves-20260904.json` (5,891 exact)
