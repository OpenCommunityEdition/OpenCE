# `player_rumble.obj` `_fast_ftol` owner re-audit — 2026-09-19

## Decision

Admit `source/game/player_rumble.obj` as a complete exact object. The source
still produces all 12 target functions and all 68 target-owned data bytes
strict exact. The sole 2026-08-31 admission veto, a candidate-only select-any
`_fast_ftol` COMDAT, now meets that veto's explicit reopen criterion: rebuilt
`source/ai/actor_combat.obj` supplies a compatible authenticated canonical
owner and ordinary XDK links discard the duplicate in either input order.

No source was changed for this admission. This is a stale policy-record
retirement after the canonical owner graph advanced, not new function credit.

## Exact object inventory

`python tools/campaign/gate.py source/game/player_rumble --all` reports 12
exact functions, zero residuals, and zero unwritten functions. The established
object audit records 1,385 meaningful / 1,488 padded code bytes and 68 / 68
raw data bytes. There is no target-only function or data owner.

The rebuilt player-rumble object and rebuilt actor-combat object each emit an
authenticated `_fast_ftol` helper with:

- 32 padded bytes;
- zero relocations;
- `IMAGE_COMDAT_SELECT_ANY` selection;
- normalized SHA-256
  `8d76b4470cb80ce310b0c4d5255470b182cfa0a78d93b9df3cdcb6f56678f248`.

The actor-combat copy is naturally exercised by the recovered combat graph;
it is not a synthetic owner added for this test.

## Link proof

Ordinary XDK linker probes were run with the rebuilt actor-combat and
player-rumble objects in both input orders. Both links exit successfully with
no `LNK2005` or `LNK1169`, proving that the compatible select-any copies
coalesce. Repeating the probes with `_fast_ftol` forced into and exported from
the result produces exactly one exported definition in both orders.

This is the condition requested by
`player_rumble_obj_jonas_object_accounting_audit_20260831.md`: a compatible,
authenticated, rebuilt canonical owner whose ordinary link disposes of the
player-rumble copy. The old admission-rejection entry is therefore removed;
the original audit remains as historical evidence.

## House-rule audit

- no C or header source changed;
- no function or global was renamed;
- no compiler flag, pragma, fake dependency, volatile/register steering,
  undefined behavior, byte embedding, or object post-processing was used;
- no `_point_from_line3d` owner is introduced;
- `_fast_ftol` remains the authenticated shared helper already owned by the
  recovered actor-combat graph.

The admission changes only the object status and removes the superseded
fail-closed policy record. Full build, stable regression, park, admission, and
treemap checks remain required for the integration batch.
