# Decals + physics three-function lane (2026-09-24)

Worktree `C:\halo-worktrees\claude-decals-physics-20260924`, branch
`claude/decals-physics-20260924`, base `3c74fa36` (jonas/exact-pilots). Canonical
not edited; nothing pushed; no agents. Scope: `_decal_new_from_collision`,
`_decal_clip_to_surface` (source/effects/decals.c) and `_physics_update_old`
(source/physics/physics.c).

## Result

**Zero strict-exact functions, zero credited bytes. No production source change.**
All three functions moved materially closer (fuzzy, zero credit); none can land:
every best candidate uses the /Od-attested real `point_from_line3d`, which emits a
`_point_from_line3d` COMDAT, and the folded-inline exception (house rules, rule 6)
admits that only for a strictly exact caller.

Frozen baseline (3.3.1 scorer, full ninja): Halo code 1,561,123 / 1,770,166;
7,420 / 7,574 functions; parks 100; stable snapshot 8,245 functions / 7,591 exact;
admission 14/0/7/0; pytest 1,154 passed / 5 skipped / 26 subtests.

| function | canonical | best candidate | alndiff blocks | remaining |
|---|---|---|---|---|
| `_decal_clip_to_surface` 1,776 | `[sha]`, frame 0x70 | `[sha]`, frame 0x6c = Jan, 571/571 | 34 -> **4** | ONE adjacent two-load swap at Jan 0x4e1 |
| `_decal_new_from_collision` 6,176 | `[sha]`, frame 0x7304 | `[sha]`, frame 0x7308 = Jan, real insns 1782 = Jan | 362 -> 258 | frame slot order (quicksort regime), 4 x87 leaf sites |
| `_physics_update_old` 5,168 | `[size 5456, relocs 118, sha]` | `[sha]`, padded 5168 = Jan | 311 -> 187 | 5 x87 leaf sites, physics/total_torque slot rotation, x87/int interleave |

Per-function gate rows of all three candidate TUs equal the baseline rows except the
target rows (and, for physics, Lane B's `_physics_compute_new` and
`_physics_compute_vehicle_collision`, which improve to `[sha]` - out of scope, zero credit).
Original lane candidate TUs: `scratch/ledger/cand_decals_clip_S4.c`,
`cand_decals_nfc_N14.c`, `cand_physics_LD2.c`; per-function probe logs
`scratch/ledger/*_probe_log.md`.

Preserved after handoff: all three candidate TUs, both probe logs, and the
baseline/candidate gate rows are tracked at commit `8afe1cdc` on the
`research/decals-physics-fuzzy-20260924` branch. This is research-only source,
not canonical production code or exact-match credit.

## `_decal_clip_to_surface` - four source facts, one tie left

Evidence: /Od body 0x5669a0 (RTC descriptor 0x567690: surface_plane, surface_point2d,
surface_point2d_previous, clip_flags, surface_edge_plane, clipped, surface_edge_vector x2,
vector), HCEX.pdb (clip_flags `unsigned long`, `convex_polygon2d_clip_to_plane(...,
unsigned long *clip_flags, ...)`, decal_points2d_temp a function-static), January bytes.
Lane C's C3 frame solver (`claude-lane-c .../C3-frame-solver/FINDINGS.md` E1) reached the
same scope fix independently and left two hunks; this lane closes one of them.

1. **Real helper at the zoffset nudge** (/Od 0x567272). Canonical hand-expands
   `point_from_line3d` (x through the global, y/z through a `zoffset` local) - the form rule 6
   forbids. A lab `__forceinline` copy reproduces January 0x66a-0x6a0 byte-for-byte, so
   January's bytes ARE the inline lowering of the real call (double attestation).
2. **Scope**: `offset` (8 B, only `.y` homed) must merge into `previous_point`'s slot entry
   (F2 scope law). With the two points and the do-while in an inner block, frame
   0x70 -> 0x6c and every lea base equals January.
3. **Declaration order** per /Od frame order (edge_index, edge_iteration first): January's
   `xor ecx,ecx` zero pin and init-store order reproduce (Lane C's hunk 1 closed).
4. **Inline budget**: the real helper stays out of line under the five-condition emission
   `if`; lab bisection shows the zoffset site inlines with <= 3 condition levels. Spelling the
   three surface-flag tests as one mask, `!(surface->flags & (FLAG(two_sided) |
   FLAG(invisible) | FLAG(breakable)))` (Bungie idiom, collisions.c:365; January tests them
   with one `test byte [..+8],0xb`), inlines it. **Owner caveat:** the /Od revision spells
   three separate TEST_FLAGs; the mask is inferred from the inline law, not attested.

Remaining: January `mov ecx,[ebp-0x14]` (edge_iteration reload) before
`mov esi,[esi+edx*4+8]` (edge_index); ours the reverse. Measured inert: all 24 loop-tail
statement orders, 7 declaration orders, `++`/`+=`/`= +1`, named sphere booleans, wrap
counter, `unsigned long` clip_flags, texture_y scope, arm order, k=1..8 used/unused locals,
and the Lane C name-count oracle (0..45 file-scope names immediately before the
definition).

## `_decal_new_from_collision` - three source facts

Evidence: /Od body 0x568660 (aligned frame; `scratch/lane/od_linear.py`), RTC descriptor
0x56aa98, HCEX.pdb locals.

1. **Failure exits through the chain loop** (astra wave4 research, /Od 0x56a9bb..0x56aa6d):
   frame 0x7304 -> 0x7308, `xor esi,esi` zero pin; reproduced on current canonical.
2. **Real reflection helper** (/Od 0x568abc): canonical hand-expands
   `point_from_line3d(velocity, &n, -dot_product3d(velocity, &n), &axis_vector)`; the real
   call inlines, fixes the reflection segments and (via IL ordinals) the first dot's leaf
   order. Needs view casts (rule 24: per-site /Od attestation, byte-inert, ledger disclosure).
3. **Dequeued index local** (/Od 0x5693fb, user slot -0x21e8): `surface_index =
   surface_queue[surface_queue_read_index++];` before the clip call puts January's
   `inc edi` before the call.

Remaining: 117 SLOT blocks - the frame is in the quicksort (refs/size) regime; January homes
`deviant_surface_count` in the dead `editor_geometry` parameter slot `[ebp+0x20]` (19 refs)
where ours puts `definition` there. The /Od declares nearly everything at function level
(flat); earlier lanes already explored declaration orders/scopes here - not re-swept.

## `_physics_update_old` - /Od-attested statement and local shape

Base: Lane B `try_w3_real_all.c` (real point_from_line3d). Accepted (each measured):
B1 at-rest `dot_product3d(&v,&v)`; P3 no `probe_length` local, radius+antigrav_height
passed and recomputed (padded size -> 5168); F1 non-ice friction arm first (`!=`);
I1 inertia loop has its own counter (January homes it in the dead magic_force slot);
L-all /Od named scalars (dead `mass*gravity`, clamped alignment, powered dot, water/air
lift speed+lift, thrust, antigrav dot+force, const ice scale); D2 `dot_product3d(&up,
&forward)`. Rejected/inert: A1 (January copies magic_torque), A2/C helpers (LEAF worse),
G1 (VC7 canonicalises the product), I2/I3/GS operand orders, M1 main-counter block scope.
Scalar names are unattested (no RTC for scalars; HCEX enregisters them).

## Reopen criteria

- **clip**: any source or evidence that flips the 0x4e1 edge_iteration/edge_index load
  order while keeping S4 - then land clip alone (it becomes the sole `_point_from_line3d`
  emitter in decals.c): verify the COMDAT is byte-identical to January's selected copy,
  `provider_link.py` both orders, full ninja + 8,245-function stable diff, parks,
  admission audit, fake scan, pytest; disclose the flag-mask and inner-block inferences.
- **new_from_collision**: a frame-allocator simulation (Lane C P2 `entries.py` tables) that
  predicts January's slot order from reference counts, or HCEX/Od evidence for which local
  January's parameter-slot reuse selects.
- **physics_update_old**: an M8 ordinal model that predicts the five leaf sites (the
  cross products at Jan 0x3bc/0x3d8 and 0xc81..0xcb2) from the local census, or evidence for
  the physics/total_torque slot order.
