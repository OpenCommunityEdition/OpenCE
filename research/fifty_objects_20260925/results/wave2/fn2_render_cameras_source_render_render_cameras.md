# source/render/render_cameras

## verdict
FUZZY_IMPROVED

## newly_exact_functions
[]

## candidate_files
[
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/render_cameras/production.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/render_cameras/render_cameras.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/render_cameras/production_owner_gated.patch",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/render_cameras/render_cameras_owner_gated.c",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/render_cameras/LEDGER.md",
 "C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/render_cameras/od_build_frustum.txt"
]

## production_changes
Nothing is strict exact, so no credit. There are two fuzzy-improvement packets. Both keep all 20 other functions EXACT.

(1) LANDABLE, house-rule clean. Apply C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/render_cameras/production.patch (git apply --check passes against 931ed8dc). The full candidate file is scratch/w/render_cameras/render_cameras.c.
- Replaces the body of render_camera_build_frustum.
- Deletes the unused `enum render_camera_warning_condition`. Its 22 constants are replaced by the /Od-attested `short condition_index` counter. Deleting it has 0 effect on the other functions.
- Keeps production's union+if field-of-view diagnostic.
- Result: 3392 B / 113 relocs, frame 0x6c, objdiff 3.3.1 88.98603 -> 94.318436. New park base: size 3392, relocation_count 113, normalized_sha256 59bd8a23e4655c735566084045c1996849ca90ebb17294450456c8fd9146e3a0.

(2) OWNER-GATED best body. Apply scratch/w/render_cameras/production_owner_gated.patch (git apply --check passes). The full file is scratch/w/render_cameras/render_cameras_owner_gated.c.
- Identical to (1) except the vassert reads the float's bits directly as `*(long *)&camera->vertical_field_of_view`. This is a new representation view cast and it is load-bearing, so it is owner-gated under brief section 7.
- Evidence for the cast: /Od 0x83d805 reads the bits straight from the camera field; January does `mov ecx,[edi+0x28]`; landed precedent at units.c:11494.
- Result: 3376 B / 113 relocs, frame 0x6c, objdiff 98.6946. New park base: size 3376, relocation_count 113, sha bf7d0cf4713777d20496f3be84e0755dfea411fd9bf22cf10715cf200eae91a5.

Either way the park stays active. If a body lands, refresh measurements.base and objdiff_percent in config/parked.json and update the evidence text.

No symbols.json, config or header change is needed.

Surplus COMDAT change for both packets: adds SELECT_ANY _set_real_point3d and _set_real_vector3d, and drops _plane3d_negate. provider_link.py passes for both.

## evidence
Full ledger: scratch/w/render_cameras/LEDGER.md (about 300 compiles).

New evidence: the /Od+RTC build, fn 0x83d600, RTC descriptor 0x83f098 (locals x_axis, y_axis, z_axis, viewspace_normal, viewspace_plane, projected_bounds, frustum_vertex, frustum_midpoint, clip_plane). No earlier lane had used it for this function. Each /Od shape was then measured against January's bytes:

(a) No left_plane_z / bottom_plane_z locals. /Od writes `1.0f + center_x` inline and `(center_x + 1.0f) * far_x`. VC7 CSEs these into January's [ebp-0x1c] home, which fixes the far x0 operand order.

(b) The far scales and midpoint are written as divisions, verbatim from /Od: `-camera->z_far / projection_x_scale` and `-half_z * bounds_center_x / projection_x_scale`. There are no inverse locals. VC7 itself produces January's `fld 1.0; fdiv px; fst [ebp-0x20]` reciprocal, and the whole midpoint cluster matches.

(c) The projection block is the /Od-literal form:
- a named `z_far` local assigned after clip_offset (January loads z_far once and dups it with `fld st(0)`);
- field stores in the else branch;
- `clip_offset = -d / n.k`;
- `n.i = projection_scale * n.i / n.k`;
- 4 explicit negations instead of plane3d_negate.
The whole projection cluster matches, including the n.i/n.k x87 phi and the 113th relocation (fld 0.0). A 32-variant matrix shows only the row form and else form are load-bearing.

(d) The explicit negation also fixes the first warning check's pointer and term order.

(e) The /Od short `condition_index++` counter is a pure +1 IL symbol. Its bytes are identical to a dead-local instrument at D=1. It fixes the M8 i-term orders in the warning dot products.

(f) Projection x-scale: the load-bearing grouping parentheses `((hw/vh*vw)*tan)` fail the strip test. They are replaced by the /Od-attested separate statement `half_bounds_width = half_bounds_width / viewport_height * viewport_width;` (/Od 0x83d766). Bytes are identical.

Measured progress: bf5a regrafted 48 REAL -> P2 41 -> P5 37 -> P7 23 -> gated candidate 15 REAL. Instruction counts are equal (1080/1080), size, relocs and frame match, and the slot census is identical to January.

Checks on the candidates:
- whole-TU gate: exact 20, residual 1;
- object_audit: 64 January symbols, 0 differ; the only section DIFF is build_frustum;
- fake_match_scan: 0 leads;
- provider link: PASS.

Strip tests: `(real)` casts on fabs are byte-inert; the counter's type (short/word) is inert; declaration order is inert; helper-first definition order is inert.

## blockers
Remaining machine difference in the gated body: 15 REAL regions.

(1) CP, 1 hunk at 0x216. In the first cross_product3d j-row, `a.k*b.i` loads [edi+0x14] first, where January loads [edi+0x18] first. This is an M8 IL-ordinal key. The two-knob instrument (u dead user locals, b dead inline temps) finds exactly one solution, (u,b) = (1,7): January has one more IL symbol before CP1 and one fewer, mod 8, between CP1 and the far section. No attested natural source fact produces that. The full /Od inventory (P10: no viewport int locals, side-plane set_real_vector3d) fixes CP but moves the check-1 i-term (16 REAL). Explicit z-axis negation and the other toggles make it worse.

(2) W-core, 14 hunks. These are dependency-free placement differences:
- the `mov eax,<id>` slot at warning checks 5, 10, 15 and 21;
- the plane-pointer reload `mov eax,[ebp-N]` before versus after `fld [esi+8]` at midpoint checks 18, 19 and 20.
Register assignment is identical. W-core never changed under any lever: u 0..31 (period exactly 8), b 0..7, the 64-combo /Od toggle matrix, counter type, casts, inventory, or helper definition order. The methodology stop rule has fired.

The object is therefore NOT completable: build_frustum is its only non-exact January-owned section.

## reopen_criteria
(a) A decoded VC7 rule, or authentic January source, for where the code generator places a constant `mov eax,imm` argument load and spill reloads around a private-ABI static call inside straight-line x87 code (W-core, 7 placement pairs). (b) Independent evidence of January's local and inline-helper inventory between the first cross product and the far section, e.g. a build closer to January than the /Od build, or HCEX line records. It must supply +1 IL symbol before CP1 and -1 (mod 8) after it. Start from scratch/w/render_cameras/render_cameras_owner_gated.c; the lab tools there are ub.py, clusters.py and toggles.py. (c) An owner ruling on the `*(long *)&camera->vertical_field_of_view` direct bit read. It is needed for the 3376-byte shape; the union form costs 16 bytes and 55 more REAL regions.

## task notes
Notes for the integrator:\n\n- The old park blocker, `_pi` float folding, is gone at 931ed8dc: real_math.h now defines `_pi` as a float literal. The earlier bf5a packets needed a diagnostic override; these do not.\n- The earlier mod8 'D=2' hint was measured on the 3408-byte production body and does not transfer.\n- Which packet to take:\n  - production.patch is house-rule clean and improves fuzzy match only (94.32%). Take it only if a zero-credit fuzzy improvement is wanted; it requires a park measurement refresh.\n  - production_owner_gated.patch is the closest body (98.69%, 3376/113, frame 0x6c). It needs an owner ruling on the float-bits view cast before it can land.\n- Neither packet changes headers, symbols.json or config.\n- Lab instruments (dead locals, dead magnitude_squared3d calls) were used only as oracles under scratch/w/render_cameras/lab and appear in neither candidate.
ledger: C:/halo-worktrees/claude-fifty-objects-20260925/scratch/w/render_cameras/LEDGER.md
