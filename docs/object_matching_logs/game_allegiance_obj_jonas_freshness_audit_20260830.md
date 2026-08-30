# `game_allegiance.obj` freshness audit (2026-08-30)

## Outcome

`source/game/game_allegiance.obj` remains `13/15` strict exact.  This
read-only closeout census found no newer source or object artifact that closes
either residual, so no production source, header, configuration, ownership,
park, or comparison policy changed.

The two residual packets remain:

| Function | Target / candidate padded bytes | Relocations | Target / candidate normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_code_00096300` | `416 / 416` | `2 / 2` | `3eba7c13... / abbb6b20...` |
| `_game_allegiance_create` | `224 / 224` | `5 / 5` | `47961c6c... / be3a5bd...` |

All 13 inherited exact functions remain the unique best production frontier.
The 40-byte team-name table and four-byte BSS pointer owner also remain exact.

## Machine-wide census

The audit covered all 1,574 registered worktrees, every reachable
`source/game/game_allegiance.c` blob, normal `build/base` objects, ignored
`build/audit`, `scratch`, and `outputs` object artifacts, and the authenticated
Punpckhdq, Stian, Pastudan, HCEA, Claude, and Fable material available on the
host.  The source census collapsed 1,574 copies to nine unique blobs.  The
object census inspected 17,698 candidate paths, 17,347 distinct raw COFF
objects, and 127 objects carrying the game-allegiance symbol set.

No object artifact contains an exact copy of either residual.  No source blob
beats the checked-in 13/15 frontier.  The Fable-only unique source is older and
less complete; reachable Git history contains no post-private-cluster source
advance.

## Fixed boundaries

`_code_00096300` already reproduces the complete 142-instruction bit-vector
graph and both relocations.  Its remaining 13-event register/lifetime
bijection is the documented January ESI/EDI and team-word allocation cycle:
January keeps the permanent boolean in `DL` and team words in `CX`/`SI`, while
VC7 assigns `CL` and `SI`/`DI` and extends the ESI restore through the tail.

`_game_allegiance_create` already has the target size and all five relocation
identities.  Its remaining 26-event allocation cycle is January keeping the
global table role in EDI and the hot record field in DX, while the current
compiler keeps globals in EDX and shrink-wraps EDI around the loop.

The complete rejected-family boundary remains in:

- `docs/object_matching_logs/game_allegiance_obj_jonas_lifecycle_wave_20260821.md`
- `docs/object_matching_logs/game_allegiance_obj_jonas_public_leaves_20260828.md`
- `docs/object_matching_logs/game_allegiance_obj_jonas_public_continuation_20260828.md`
- `docs/object_matching_logs/game_allegiance_obj_jonas_private_cluster_20260829.md`

Reopen only with January local/source records, an exact same-compiler donor, or
a new typed-C lifetime fact that directly explains one of those allocation
cycles.  Do not repeat index-width, truncation, predicate-order, declaration,
scope, direct-global, pointer-induction, alias, store-order, or helper-spelling
families already recorded there.

