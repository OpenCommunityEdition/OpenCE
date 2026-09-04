# `network_server_manager.obj` pregame-pair Opus reconciliation (2026-09-04)

## Scope

This packet selectively reconciles two small pregame-readiness functions from
the frozen Opus donor `opus/small-families-30k-20260902`, commit `8aed00b49`:

- `_server_has_a_player_on_each_machine`: 144 padded bytes
- `_server_ok_to_countdown`: 80 padded bytes

Together they contribute 224 strict padded bytes and 217 meaningful code
bytes.  The donor commit was not cherry-picked.

## Source-quality review

`server_has_a_player_on_each_machine` walks the typed server machine and player
arrays, uses the existing `network_player_is_valid` predicate, and rejects any
valid client machine without an associated player.  `server_ok_to_countdown`
combines the existing pregame predicates with the typed minimum-player count.
Both functions are used only within their owning translation unit, so no
cross-TU declaration or unrelated header edit was introduced.

The retained source contains no address-derived identifier, raw offset,
inline assembly, forced inline, volatile/register steering, representation
pun, fake dependency, or nonsensical matching-only logic.

## Verification

- Focused gate: both functions exact at 144/144 and 80/80 padded bytes.
- Object gate: 26 exact, 0 residual, 44 unwritten (previously 24/0/46).
- Rename-stable whole-tree sweep: +2 exact functions / +224 padded bytes,
  zero regressions across 8,245 target functions.
- Full Ninja build and semantic report: success; 5,902 semantic exact, 5,919
  accepted exact, zero unit errors.
- Progress report: 837,167 / 2,198,102 matched code bytes and 5,851 / 11,060
  matched functions; data remains 2,001,713 matched bytes and 391/833 objects
  are linked.
- Fake-match scan: zero review leads in the changed source.
- Park audit: 185 active, 0 stale, 0 invalid.
- Object-admission audit: 0 candidates, 0 contradictions; inherited explicit
  rejections unchanged.
- Tool tests: 261 passed.
- The January inline schedule remains intact and the packet emits no
  `point_from_line3d` COMDAT.

Stable snapshots:

- before: `scratch/after-game-options-loading-leaves-20260904.json` (5,891 exact)
- after: `scratch/after-network-server-pregame-pair-20260904.json` (5,893 exact)
