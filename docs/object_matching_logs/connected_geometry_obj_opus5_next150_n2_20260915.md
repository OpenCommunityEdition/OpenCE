# `connected_geometry.obj` — opus5 next-150K lane, wave n2 (2026-09-15)

Baseline and final real-file gate (`--forbid-emitted-symbol _point_from_line3d`, guard passes):
**6 exact / 4 residual / 0 unwritten**, park drift 0 (both parks), fake scan 0 leads.
`git diff --stat -- source/tool/connected_geometry.c` is empty. No credit is claimed.

## `_triangle_coplanar` (parked `instruction-scheduling`): 4 differing instructions, park remeasure proposed

Target 384 / 11. Census: frame identical (`sub esp,0x10` = the 16-byte `real_plane3d triangle_plane`,
no other slot), relocations 11/11 identical, 151 vs 151 instructions. Nothing structural.

`scratch/workers/n2_connected_geometry_actor_stimulus_etc/cg/v1.c` is the CURRENT production file with only
this body replaced by the genuine-helper form recommended by laws_w3 C1:

```
fabs(dot_product3d((real_vector3d const *)pointN, &plane->n) - plane->d) < 0.01f   (x3)
plane3d_from_points(&triangle_plane, point0, point2, point1) != NULL
dot_product3d(&triangle_plane.n, &plane->n) > 0.0f
```
with `plane = predicate_data` declared after the three point fetches. It measures **384 / 11 with exactly 4
differing instructions**, against 10 for the body currently in production, and it deletes that body's
hand-expanded plane distance (`point0->y*plane->n.j + point0->z*plane->n.k + point0->x*plane->n.i - plane->d`)
and its three order-forcing locals `point_distance`, `point_product`, `facing` — the laws_w3 R15 class.

**Residual, fully localised:** the term order of the inlined facing dot. January `i,k,j`
(`fld [ebp-0x10]/fmul [esi]`, `[ebp-8]/[esi+8]`, `[ebp-0xc]/[esi+4]`); ours `j,k,i`. The three point dots are
`j,k,i` in both builds and byte-identical, and the leaf order inside each product already matches.
January's `i,k,j` is the reverse-recency order of the `plane->n` reads in the third point dot
(`+4` at 0x112, `+8` at 0x118, `+0` at 0x11f), which suggests the facing dot's IL is built after those ages
are updated.

Shapes spent (2, the budget for an x87-order-only residual):

| id | shape | result |
| --- | --- | --- |
| v2 | `real_plane3d triangle_plane;` declared before `plane = predicate_data` | byte-identical to v1 (declaration order is inert for an address-taken aggregate passed to an out-of-line call) |
| v3 | facing moved out of the `&&` chain into a nested `if` (same CFG) | byte-identical to v1 |

v3 **refutes** the 2026-09-01 closeout's claim that a sequential or nested facing flips the point sums: that
coupling belonged to the hand-expanded body, not to the helper form.

Not spent, with reasons: a two-term split (laws_w3 A35(2), the only recorded construct that yields `i,k,j`)
requires hand-expanding the helper (R15); swapping `dot_product3d`'s arguments is a commutative permutation
(R3) and would move the `fld` leaf away from January's; binding `plane3d_from_points`'s return value into a
pointer local adds a frame slot (January's frame is exactly 0x10).

**ORCHESTRATOR PROPOSAL (park remeasure, zero strict bytes).** Replace the parked body with `cg/v1.c`'s and
rebaseline its `config/parked.json` hash: strictly closer (10 -> 4 differing instructions at the same 384/11)
and it removes a hand-expanded helper body from production source. Not a reopen — it is not strict exact.

**Reopen criterion:** a construct that builds the facing dot's IL after the point dots' operand ages, without
hand-expanding `dot_product3d` and without adding a frame slot.

## `_connected_geometry_find_or_add_edge` (240 target / 256 real): owner-blocked, now byte-proven

Census: frame identical (`push ecx`, the single `[ebp-1]` `forward` byte); relocations 7/7 identical
(4x `dynamic_array_get_element`, 2x `dynamic_array_add_element`, 1x `dynamic_array_new`). The 16-byte gap is
ONE instruction, `mov byte ptr [ebp-1],1` at +0x10 — our `boolean forward = TRUE;` initializer.
January writes `[ebp-1]` at exactly three places and never on entry: +0x47 (forward match, 1), +0x4D
(reverse match, 0), +0x67 (new edge, 1). So January declares `forward` uninitialised and the byte is read at
+0xBE on a path reachable only when `geometry->edges.count < 0`.

The `>=` spelling that would remove the undefined read is refuted: +0x51 is `cmp esi,[ebx+0x10]; jne`, an
equality test, not `jl`.

A SECOND, independent residual survives any owner ruling: return-block layout. January
`[+0xC8 set-bit arm + ret] [+0xD7 return NONE] [+0xE1 clear-bit arm + ret]`; ours `[set] [clear] [NONE]`.
w2 spent seven layout shapes on it and laws_w3 A43 records that no source lever has ever moved an
epilogue/cross-jump survivor.

**ORCHESTRATOR PROPOSAL (owner ruling).** Decide whether `boolean forward;` may be left uninitialised here,
reproducing January's latent indeterminate read. Evidence above. NOTE a YES ruling does not by itself close
the row — the layout residual remains — so it is worth 240 B only together with a layout lever.

## `_connected_geometry_find_or_add_vertex` (parked `unclassified`): reopen criterion sharpened

Census: January's `push ecx` gives one 4-byte slot `[ebp-4]`, written twice (`fstp` +0x35, +0x5C) and read
twice (`fsubr` +0x41, +0x68); ours uses the same slot with `mov` / `fld`. Relocations 8/8, 80 vs 80
instructions. Residual: January `fld [ebx+4]; fstp [ebp-4]; call; fld [eax+4]; fsubr [ebp-4]` vs ours
`mov eax,[ebx+4]; mov [ebp-4],eax; call; fld [ebp-4]; fsub [eax+4]`.

**New proof that `[ebp-4]` is an anonymous x87 temporary, not our named `real pending_coordinate`:** January
copies floats with INTEGER moves everywhere else in this same function (+0xA3 `mov ecx,[ebx]; mov [eax],ecx`,
and likewise for +4 and +8 in the add path). A plain `real a = point->y;` therefore cannot emit `fld/fstp`.
That also explains the `fsubr`: the unnamed temp is the old leaf and the fresh call result is `fld`ed
(laws_w3 A35(1)).

Shape spent (1): w1 — all three conjuncts written with this TU's own proven inline-cast accessor idiom
(`fabs(point->y - ((real_point3d *)dynamic_array_get_element(&geometry->points, point_index,
sizeof(real_point3d)))->y) < EPSILON`), no `pending_coordinate` local. Result 176/8: VC7 evaluates the CALL
FIRST and then `fld [ebx+4]; fsub [eax+4]` with no spill at all. This re-derives w2's "inline getters 176"
row and supplies the reason: for `load - call`, VC7's operand ordering puts the call first, so an inline
getter can never produce the spill.

**Reopen criterion (replaces "an authentic construct that evaluates the component before the call"):** a C
construct that evaluates `point->y` before the `dynamic_array_get_element` call WITHOUT binding it to a named
local — a named local is copied with integer moves (proved above) and an inline getter reverses the
evaluation order (proved above).

## `_connected_geometry_add_triangle`: skipped as exhausted

On the laws_w2 section D do-not-repeat table (shrink-wrap: January's `push ebx` at +0x23 after the
`triangle_index == NONE` exit, ours in the prologue). No law in A1-A45 addresses prologue shrink-wrapping and
this wave found no new January-side fact. 0 shapes spent.

## Checks

Every scratch candidate (v1, v2, v3, w1) kept the six baseline-exact siblings strict EXACT, passed the
`_point_from_line3d` guard, and left the other parked body untouched. parkcheck: 2 parks, drift 0.
`tools/fake_match_scan.py source/tool/connected_geometry.c`: 0 review leads. The production file is
byte-identical to HEAD. No header, config, ninja, configure, pytest or git action was taken.
