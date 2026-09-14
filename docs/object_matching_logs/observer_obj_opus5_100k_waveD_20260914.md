# `observer.obj` opus5 100k wave D park re-investigation (2026-09-14)

## Scope and provenance

- Translation unit `source/camera/observer.c`, target `build/split/source/camera/observer.obj` (January 2002 Xbox
  debug build, VC7 13.00.9254, repository `build.ninja` flags `/O2 /Oy-`).
- Evidence: January target COFF (instructions, relocations, `.rdata` assertion literals and data bytes), January PDB
  public symbols, HCEA `blam/camera/observer_*.c` (names, signatures, the differential body), the Stian build-2276
  lift of `observer.c` (`FUN_0008ab90` flags/result topology). No atlas or Ghidra output exists for this object.
- Prior ledgers read in full: `observer_obj_replacement_exact_packet_20260912.md`,
  `observer_obj_real_update_caller_cluster_20260905.md`,
  `observer_obj_integrator_penetration_independent_review_20260905.md` (they state that no source-shape variants
  were attempted on the eight residuals).
- All eight residuals are PARKED. Every experiment was made in scratch copies; `source/camera/observer.c` is
  unchanged (`git diff --stat` empty). Worker notes: `scratch/workersD/observer.md`; scratch dir
  `scratch/workersD/observer/`.

## Validated baseline (lane HEAD)

`gate.py source/camera/observer --all --forbid-emitted-symbol _point_from_line3d`: exact 17, residual 8, unwritten 1
(`_observer_collision_test_differential`, 64). Residual measurements equal `config/parked.json` `measurements.base`.
The final real-file gate is identical (object `scratch/workersD/observer/final.obj`), no park drift, owner census
unchanged, fake scan 0.

## Results (scratch, strict `section_infos_equal`)

### A. Standalone park-reopen - `scratch/workersD/observer/park_reopen_with_t.c`

Gate 18 exact / 7 residual / 1 unwritten; guard pass; the other seven parks equal their base measurements.

| Function | Padded | Key evidence |
| --- | ---: | --- |
| `_observer_collision_test_with_t` | 224 | `xor bl,bl` result flag first; flags is a homed local `0x40e1` conditionally rewritten to `0x40a1` (`if (ignore_media) SET_FLAG(flags, media, FALSE)`), not a branch-free `SET_FLAG(..., !ignore_media)`; `*t = collision.t; result = TRUE` inside the hit branch (single exit). Stian's lift has the same topology. |

### B. Cluster packet - `scratch/workersD/observer/park_reopen_cluster.c`

Gate 20 exact / 6 residual / 0 unwritten; guard pass; `update_command` and `update_positions` keep their parked
base bodies (no drift); fake scan 0.

| Function | Padded | Status in packet | Key evidence |
| --- | ---: | --- | --- |
| `_observer_collision_test_differential` | 64 | strict EXACT | Private register ABI (eax basis, ecx destination, esi origin) proves an in-TU caller; January `check_penetration` contains its body inline at both probe sites (`fld differential; fmul basis; fadd destination` into one 12-byte local, `differential` homed in the safe_distance slot). VC7 keeps the out-of-line copy of a static inlined at every call site (hs_compile precedent). The 20260912 "zero incoming calls => fake" reading is refuted. Written with explicit components (a `point_from_line3d` body emits the forbidden COMDAT). |
| `_observer_postcheck` | 2112 | strict EXACT | `.rdata` literals `valid_world_real_point3d(&focus_position)`, `valid_world_real_point3d((real_point3d *) &observer->focus_offset)`, `valid_focus_distance(focus_distance)`, `valid_world_real_point3d(&observer->result.position)`, `valid_field_of_view(observer->field_of_view)` prove predicate-macro asserts expanding to per-component `valid_real`+range tests (the old body emitted `valid_real_point3d`/`valid_real_vector3d` calls, +3 relocs); heading via `normalize2d` on a `real_vector2d`; location declared in its own block (it shares `[ebp-8]` with the `valid_real` temp); requires the January private ABI of `check_penetration` (eax=&forward, stack &up). |
| `_observer_check_penetration` | 752 | reloc-identity (data name) | Tail is cseries `ABS(last_miss_t < last_hit_t ? lower : upper)`: the unparenthesised first use parses as `c ? lower : (upper>=0)` (fild of the boolean); probes call the differential; `sine_region_angle` is an `.rdata` object loaded before `*distance`; sign is an integer select `(probe_index & 2) ? 1 : -1`; separate `first_t`; `collision_t` per loop block; offset components `-forward->i*(*distance + safe_distance)` with no named reach local; `best_sign` uninitialised (no store). Only non-equal reloc: `_rdata_00256f3c:24`. |
| `_observer_update_accelerations` | 464 | reloc-identity (data name) | Polynomial branch first; r^2/r^3 hoisted; counts and maximum re-read via strength-reduced indexed tables; stored then compared. Only non-equal reloc: `_data_002dcc6c:12`. |
| `_observer_update_velocities` | 512 | reloc-identity (data name) | Polynomial branch first; r^2..r^4 hoisted; `else if (valid && (time_force || force_time)) memset; else if (valid) decay` (valid bit retested); indexed counts. Only non-equal reloc: `_data_002dcc6c:12`. |
| `_observer_update_polynomial` | 1440 | reloc-identity (data name) | Three count relocations => indexed table at every use; the 0x1F6 assert is the shared camera-command validity expression applied to `&observer->last_command` (null test on the address); velocity term written first in each sum (VC7 evaluates the right operand first). Only non-equal relocs: 3x `_data_002dcc6c:12`. |

`--alias observer_maximum_accelerations=rdata_00256f3c --alias observer_parameter_real_counts=data_002dcc6c` clears
the +0 data references; the remaining rows are exactly the +12 (`observer_parameter_derivative_real_counts`) and
+24 (`sine_region_angle`) addends, which need `config/symbols.json` names.

Shared-assert note: the identical validity expression in `observer_set_camera` (0xE9), `observer_update_command`
(0x172) and `observer_update_polynomial` (0x1F6) is written as one TU-local macro
`match_assert_valid_observer_command(file, line, command)`; `observer_set_camera` stays EXACT. The macro name is not
attested. `valid_world_real` (scalar helper under `valid_world_real_point3d`) is byte-required: flattening it drops
the parenthesised parameter and breaks postcheck (paren law).

Owner census (packet): new header-inline SELECT_ANY COMDATs `normalize2d`, `magnitude2d`, `magnitude_squared2d`,
`scale_vector2d` (already emitted by many base objects); new `_observer_collision_test_differential` (January
section); January's own assertion literals; lost `valid_real_point3d` COMDAT, the old assertion literals and
`__real@3e322d0e` (none in the target). New static `.rdata` `sine_region_angle`.

## Not landed (best scratch bodies)

| Function | Best | Residual classification |
| --- | --- | --- |
| `_observer_update_command` | `ecmd1.json`: 1040/33 (size/relocs equal), ~50 insns | Structure fixed (post-assert `observer->pending_command` reloads, pre-assert timer/flags/last pointers, cross-jumped `MIN(*last_timer, 2.f)`, `pending->timer < *last_timer && !force_time`). VC7 merges the command-timer induction variable into last_timer+delta; January keeps separate IVs. Declaration order, increment order, const, indexing variants measured. IV/register tie. |
| `_observer_update_positions` | `epos6.json`: 1568/40 (size/relocs equal, slot map identical), 46 insns | No displacement memset; `struct observer_derivative displacement` (56-byte frame object); assert loop uses parameter_index (January literal `observer->velocities.n[parameter_index]`); snap / poly / decay / `parameter_index < orientation` topology; rotation via `observer_apply_rotational_displacement(delta, position, position + 3)`; inline-able axes predicate. Remaining: three-term sum order in inlined header math (January C,A,B vs ours C,B,A), commutative fmul operand order in the repair cross products, one lea placement. Value-age tie. |

## Do-not-repeat

- `point_from_line3d` in the differential or penetration (emits the forbidden COMDAT).
- `scale_vector3d(forward, -reach)`, negate+scale either order, explicit/i-first cross components, cross on
  `&perturbations[0]`, cross before the up copy (changes the private ABI), parameter-order swap, named `reach`.
- positions: header `valid_real_vector3d_axes2` call (stays out-of-line in this caller while the repair block's
  inlines exist), rotational vector local copies, declaration-order and loop-counter scope permutations.
- command: const pointers, declaration/increment order, indexed timer/last/flags variants.
- postcheck: flattened `valid_world_real_point3d` (paren law), function-scope `location`.

## Reopen criteria

- Cluster packet: `config/symbols.json` static data names `_observer_maximum_accelerations` 0x256F3C,
  `_sine_region_angle` 0x256F54, `_observer_parameter_real_counts` 0x2DCC6C,
  `_observer_parameter_derivative_real_counts` 0x2DCC78, then re-gate the packet (expected strict EXACT for all
  seven) and unpark with_t, differential, penetration, postcheck, accelerations, velocities, polynomial together.
- update_command: a VC7 induction-variable-merge rule or new source evidence for the loop variables.
- update_positions: a rule for three-term sum ordering of inlined header math in this context.

## Disposition

Real file unchanged; no new exact credit this wave. Park-reopen proposals: A (with_t, strict EXACT, standalone) and
B (cluster, pending data names). update_command and update_positions remain parked with improved scratch bodies.
