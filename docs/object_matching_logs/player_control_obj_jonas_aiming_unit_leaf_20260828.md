# `player_control.obj` aiming-unit leaf recovery (2026-08-28)

## Result

`source/game/player_control.obj` advances from 40/49 to 41/49 strict-exact
functions. The newly admitted `_player_control_get_aiming_unit_index` is 96
padded bytes with five relocations and normalized SHA-256
`85f3ed804f320738d098cbfef1b83c09ff52df74eb62555801272f1eac7c8925`.
All 40 inherited exact functions remain strict exact.

The retained source is ordinary typed C. It obtains a named
`struct player_control *` through the established bounds-checking accessor,
then passes that control's unit index to `unit_get_aiming_unit_index`. There is
no assembly, intrinsic, barrier, storage-class forcing, raw offset access,
representation trick, object patch, or comparator exception.

## Evidence and topology

The January target proves the complete wrapper shape:

- the shared local-player bounds assertion at source line `0xB1`;
- one relocation to the player-control global owner;
- one call to `unit_get_aiming_unit_index`;
- a 96-byte padded envelope and five total relocations.

The user-supplied HCEA recovery at
`research-cache/halocea-cseries-20260820/src/blam/game/player_control_get_aiming_unit_index.c`
independently confirms the typed unit-index delegation. A direct
array-expression spelling compiled to only 80
padded bytes: VC7 removed the otherwise dead address calculation. Naming the
typed `control` local retained January's `lea eax, [eax+ecx+0x10]` while leaving
the rest of the wrapper unchanged, producing strict equality on the next
compile.

## Bounded adjacent probe

The HCEA `player_control_initialize_for_new_map.c` donor was also checked as a
production-neutral follow-up. A typed reconstruction using the measured
`game_globals.player_control` tag block, four-entry look-rate arrays, and
`player_control_new_unit` reached January's 144-byte envelope, all 10
relocations, and exact code from `+0x0F` through the epilogue. The only
remaining difference was the initial `EDI` save schedule: January emits two
independent flag stores before `push edi`; VC7 hoists `push edi` ahead of both
stores for the ordinary-C declaration, ordering, chaining, and nested-scope
spellings measured here. That body and its new globals were rejected rather
than admitted as a nonexact production change.

`_player_control_permanent_impulse` was not attempted atomically because its
January call targets the still-unwritten private `_code_000a7300`; the target
passes the local-player state through that same-TU optimized relationship.

## Verification

- `tools/campaign/gate.py source/game/player_control --all`: 41 exact, zero
  residual, eight unwritten.
- Full `halobetacache_build`, `libcmt_build`, semantic report, and progress
  graph: successful.
- Semantic audit: zero unit errors.
- Object-admission audit: zero candidates, contradictions, or revocations.
- Parked-function audit: 13 active, zero stale, zero invalid.
- The strict campaign board credits this change as one function and 96 padded
  bytes. A forced full rebuild also exposed six older, previously integrated
  exact functions from stale objects; those are not attributed to this leaf.

The unit remains `NonMatching`; eight target functions are still unwritten.
