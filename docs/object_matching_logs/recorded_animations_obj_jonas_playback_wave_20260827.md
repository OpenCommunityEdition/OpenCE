# `recorded_animations.obj` playback worker and wrapper recovery (2026-08-27)

## Result

This wave advances `recorded_animations.obj` from 2/16 to 6/16 strict-exact
functions. It adds the private thread lookup and all three public playback
wrappers while preserving both accepted disposal functions.

| Strict function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_recorded_animations_dispose` | 48 | 4 | `b9b64bb030325c73231a51ab101f24b1d8ce3ec37789c76d2686751f0cc07ed0` |
| `_recorded_animations_dispose_from_old_map` | 16 | 2 | `9dedd614496101c84c04e338b0fb2167843b37380b3f0e2a50007ce9c2bc7edc` |
| `_code_000836c0` | 96 | 4 | `dfe89944f5a5133e3b05901bba5076f4bcda484539e5373342c7a97452e745b7` |
| `_recorded_animation_play` | 32 | 1 | `ef585acc1d2013cd265d49eaa84ecdaf199a4240338ab525e2e5415f55138fa2` |
| `_recorded_animation_play_and_delete` | 32 | 1 | `35ec6175e1878120e0ea7fdb6a81827fd940b345897631bc2401ac44c32740ea` |
| `_recorded_animation_play_and_hover` | 32 | 1 | `c11e67670227db5072be50fc141406af4628cdaabce20cdbd8b1377aed52d2a3` |

The four new owners contribute 151 meaningful bytes: 85 bytes for the lookup
and 22 bytes for each wrapper. Including the two disposal owners, the object
now has 200/2,684 meaningful code bytes exact.

The recovered `_code_000839a0` playback worker is a real typed implementation,
not a wrapper-enabling stub. It is 640/640 padded bytes, has the target's 38
relocations and the same relocation destinations in the same semantic order,
and emits the exact target assertion expression and error strings. Its
normalized instruction bytes remain residual because the compiler assigns the
long-lived unit and animation-index values to the opposite ESI/EDI roles and
schedules several typed field loads differently. That residual is retained as
the lawful foundation for the remaining object.

## Evidence and history audit

Before editing production, the complete reachable source history for
`source/cutscene/recorded_animations.c` was swept through the scratch compiler.
There were three unique blobs; the cumulative source was already best at 2/16.
The historical `jonas/recorded-animation-wrappers-wave-20260820` tree contained
only the symbol skeleton, not hidden implementations.

The prior accepted ledgers were read in full:

- `recorded_animations_obj_jonas_dispose_old_map_20260821.md`;
- `recorded_animations_obj_jonas_dispose_first_shot_20260826.md`.

Their private-EAX warning was decisive: compiling the public donor wrappers as
ordinary external forwards is not target-authentic. The worker and lookup had
to exist in the same translation unit so VC7 could derive the private register
ABI naturally.

Semantic source came from the pinned local donor mirrors:

- `build/audit/refs/halocea/src/blam/cutscene/recorded_animation_play_internal.c`;
- `build/audit/refs/halocea/src/blam/memory/get_controlling_thread.c`;
- the three HCEA playback wrapper sources under
  `build/audit/refs/halocea/src/blam/cutscene/`;
- `build/audit/refs/stian-halo/src/halo/cutscene/recorded_animations.c`.

January split COFF and relocation-annotated disassembly remained the authority
for control-flow order, field widths, private ABI, exact strings, calls,
relocations, and padding. In particular, the January helper's loop is a
`while (thread)` scan with an equality break. Expressing that ordinary source
topology made `_code_000836c0` strict exact and simultaneously gave all three
wrappers their exact EAX-plus-stack call shape.

Claude's active worktrees were read only. No file, index, object, or build
artifact in those worktrees was modified.

## Policy and validation boundary

The retained implementation uses typed structures for the animation thread,
debug entry, playback codec table, scenario block, and data-array iterator. It
contains no assembly, volatile/register steering, pragma, intrinsic,
attribute, barrier, raw address, raw byte-offset access, pointer/integer
reconstruction, punning, undefined behavior, synthetic anchor, payload, or
object-byte forcing. All declarations are C89-compatible and every function
has an explicit return.

The scratch gate reports 6 exact, one residual, and nine unwritten functions.
Direct production-object comparison proves all six accepted owners have equal
padded bytes plus ordered relocation address/type/destination/addend identity.
The final frozen source was rebuilt through one normal selected-object Ninja
edge for the helper/CFG wave.

The campaign gate and branch sweep were also made concurrency-safe during this
audit: alternate branch blobs now compile through `gate.py --source` from a
PID-scoped scratch source. `branch_sweep.py` no longer overwrites or restores
the production source, so it cannot collide with Claude or another active
worker.
