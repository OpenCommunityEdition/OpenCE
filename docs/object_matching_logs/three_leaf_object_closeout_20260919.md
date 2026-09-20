# Three-leaf whole-object closeout audit (2026-09-19)

## Scope and result

This bounded read-only audit rechecked three proposed whole-object closures:

- `source/interface/progress_bar`;
- `source/game/aim_assist`;
- `source/effects/contrails`.

No production source or configuration was changed. None of the three units has
new evidence satisfying its recorded reopen criterion, so repeating the known
source-shape matrices would violate the campaign's do-not-repeat discipline.
All three production files retain the best truthful, house-rule-compliant
source already present.

Fresh PID-isolated gates, each with
`--forbid-emitted-symbol _point_from_line3d`, report:

| unit | exact | residual | unwritten | remaining row |
| --- | ---: | ---: | ---: | --- |
| `source/interface/progress_bar` | 53 | 1 | 0 | `_draw_gravy_layer`, target 352 / candidate 336 padded bytes |
| `source/game/aim_assist` | 14 | 1 | 1 | `_local_player_aim_assist`, 368 / 368 but hash-different; `_player_aim_projectile` remains unwritten |
| `source/effects/contrails` | 18 | 1 | 0 | `_contrail_add_points`, target 960 / candidate 944 padded bytes |

The emitted-symbol guard passes for every unit.

## Why no candidate was compiled

### `progress_bar:_draw_gravy_layer`

The current 16-byte gap is the already measured allocator-rematerialisation
boundary. January materialises four rectangle corners in registers and recycled
stack homes; VC7 copy-propagates the six previously tested ordinary C forms
back to direct structure-memory operands. Target and candidate have the same
frame and the same 22 ordered relocation identities. The current reopen
criterion requires a new VC7 lever that preserves a corner loaded from an
address-taken local structure without `volatile`, barriers or other steering,
or January-side evidence that the rectangle is modified between vertices.
Neither exists in the current evidence set.

### `aim_assist:_local_player_aim_assist`

This is the recorded callee-saved allocation/x87 tie. January assigns
`local_player_index` to EBX and the angular vectors to EDI/ESI, reusing dead
parameter homes; the current truthful source needs one additional local home.
The pointer-binding lead fixes most of the x87 block but leaves the pitch-dot
term order and register allocation unresolved. Early-return, result-local,
declaration-order and explicit-component forms are already measured negatives.
Moreover, this is not presently a one-function object closure:
`_player_aim_projectile` is still unwritten, so matching the 368-byte residual
alone could not complete `aim_assist.obj`.

### `contrails:_contrail_add_points`

The 16-byte gap is the recorded callee-saved allocation choice. January spills
the long-lived `contrail` pointer and uses EDI/EBX for point-position,
marker and scenario-location common subexpressions; the current compiler keeps
`contrail` in EBX. The target and candidate already have the same 28 ordered
relocation identities. Helper-routing, declaration-scope, marker-index and
named-location-pointer variants are measured negatives. Reopening requires
January-grounded source evidence that changes those allocation weights, which
is not currently available.

## History census

`tools/campaign/branch_sweep.py` found no better reachable source blob:

- ProgressBar: 20 historical blobs; current remains best at 53/1/0 and every
  complete-body candidate retains the 16-byte size delta.
- AimAssist: 7 historical blobs; current remains best at 14/1/1.
- Contrails: 13 historical blobs; current remains best at 18/1/0 and the older
  complete-body candidates retain a 32-byte total residual delta.

The next attempt should wait for evidence meeting the existing reopen criteria,
not spend another shape-search wave on these functions.
