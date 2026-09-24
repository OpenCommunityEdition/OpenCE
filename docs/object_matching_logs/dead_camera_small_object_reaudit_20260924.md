# `dead_camera.obj` small-object re-audit (2026-09-24)

Base: canonical `4d1ebf179023f46efb77929fb599e514601152fb`. This re-audit
changed no production source, configuration, or matching credit.

Fresh compilation with the repository's VC7 command and January split target
(`python tools/campaign/gate.py source/camera/dead_camera --all --disas
_dead_camera_update`) gives three exact functions and one residual:

| Function | Padded bytes | Result |
| --- | ---: | --- |
| `_dead_camera_new` | 288 | exact |
| `_player_get_next_player_with_a_unit` | 176 | exact |
| `_player_has_allies` | 112 | exact |
| `_dead_camera_update` | 1,248 | residual; normalized SHA only |

The latter has the same size and 46 relocations as January. The one real
instruction mismatch is the spectator fallback at function offset `0x176`:
January reads `mov eax, dword ptr [ebp + 0x10]`; current source emits
`mov eax, esi`. The 1-byte difference moves downstream addresses by one but
does not denote further independent source defects.

The 2026-09-15 [wave-n1 ledger](dead_camera_obj_opus5_next150_n1_20260915.md)
proved that a defined initializer for `next_unit_index` forces an extra store,
while the exact donor leaves the variable uninitialized when no next player
exists. The later first-party `/Od` build initializes it to `NONE`, consistent
with a bug fix after January. The
[Astra rejection](astra_90pct_rejected_hypotheses_20260920.md) records the
owner's express exclusion of this donor and requires a new defined,
independently authenticated reconstruction before reopening it. Fresh bytes
do not overturn that exclusion. In particular, an address/alias/volatile
spelling to force a memory load would be source-shape steering, not a credible
reconstruction of the original logic.

`dead_camera.obj` therefore stays `NonMatching` at 3/4. Reopen only if an
independent first-party source record or a credible, *defined* source form
explains January's parameter-home load without losing any of the three exact
siblings. Do not use the archived uninitialized-local donor or mark this
object matching merely because its only residual is one byte.

## Follow-up: exact-caller reachability audit

The remaining source-shape search found no plausible *defined* expression
that reads the `result` parameter home as a unit index without either an
extra initialising store or representation steering. A fresh diagnostic
removal of the stand-in `else` gates 4/4 exact; the edit was immediately
reverted, and the fresh production build again gates 3/4.

An independent call-graph check does narrow the bug's runtime reachability:

1. `_director_update` (strict exact, 496 B) calls a camera procedure only
   inside its `local_player_get_player_index(local_player_index) != NONE`
   branch. `_local_player_get_player_index` itself is strict exact (80 B).
2. `_director_update_controls` (strict exact, 960 B) passes that same local
   player index in `controls`. `_director_choose_camera_game` (strict exact,
   448 B) also obtains that player's datum before calling `dead_camera_new`.
   The other `dead_camera_new` call is inside `_scripted_camera_update`
   (strict exact, 1,664 B), which receives those controls through the same
   director procedure. The two translation units spell the first control
   field differently, but the scripted path casts its low half, which is the
   director's `short local_player_index`.
3. `_dead_camera_new` (strict exact, 288 B) initializes
   `current_player_index = player_index` from that valid local player.
   `_player_get_next_player_with_a_unit` (strict exact, 176 B) returns either
   an eligible datum index or the old index; it never changes a valid old
   index to `NONE`. Inductively, every valid-camera update has
   `next_player_index != NONE`.

Thus the unassigned branch is unreachable under the reconstructed valid
camera lifecycle; this is stronger evidence than the prior single-function
donor. It is **not** original source, and it does not prove safety for a
corrupt/restored camera or an out-of-contract direct call. The owner's
explicit prohibition on the bare-local donor therefore still governs.
No source/config/status change or byte credit is requested here. A future
owner may use this cross-object proof when reconsidering the original-bug
policy, but the next landing still needs an explicit ruling or first-party
source evidence.
