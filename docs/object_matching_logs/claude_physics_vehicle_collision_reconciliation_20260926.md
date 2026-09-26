# Physics vehicle collision: independent W6 reconciliation

Canonical baseline: `07fa92af698467a25a2409a56c0e95a51ba65f71`.
Donor: `09f5208f8a2b8be564519b800f7bdafb20fa19fc`, branch
`claude/compiler-application-20260925`. The owner requested reconciliation
and publication if the landing passed. Only the 23-line delta in
`source/physics/physics.c` is imported, plus this reconciliation record.
Q13 and every other held source/scorer/ownership packet remain excluded.

## Result

`_physics_compute_vehicle_collision` is independently strict exact:
**1,160 meaningful code bytes / 1,168 padded bytes / 14 relocations**.
Normalized SHA256:
`5aae4029c87d46546c56a16dd66295a34483988ce63e9c2e5ee4db7fe12b6333`.

Halo code advances **1,590,066 -> 1,591,226 / 1,770,166 (89.89%)**;
credited Halo functions **7,458 -> 7,459 / 7,574**. Complete objects stay
**388/468**, credited data stays **2,587,011**. Physics has 14 exact and
3 residual target functions; no whole-object completion is claimed.
The previously reconciled main update-time function earns no duplicate credit.

## Source and first-party evidence

Independent review inspected the supplied later unoptimized executable as
data, resolving the helper thunks and inspecting their actual bodies.
At function `0x7BB7D0`:

- Four calls at `+0x96`, `+0xBC`, `+0xE2`, `+0x108` initialize the live force
  and torque accumulators with `set_real_vector3d` before their first use.
- `+0x27C..+0x2A1` corroborates the expression
  `2.0f * mass_scale * global_gravity / global_physics_collision_depth * penetration`.
- `+0x320` calls the genuine four-argument `point_from_line3d`, replacing
  canonical's three hand-expanded component assignments.
- `+0x3F8` stores the collision flag after both cross products and all four
  force/torque additions, at the end of the block.

These are meaningful source corrections. No casts, macros, dead locals,
qualifiers, layouts, artificial dependencies, compiler directives or new UB
are introduced. The separate initialization is evidenced, not a dummy store.
The arithmetic change is NOT justified as universally equivalent FP algebra:
its provenance is independently attested and the final compiler output
matches January exactly. Later-build evidence does not establish unique
January source text. The compiler scheduling/VN analysis explains a mechanism,
not independent source provenance. Coupled changes are justified separately.

Donor evidence remains in
`research/compiler_application_20260925/workers/W6/`, including V1/V2 cards
and the failed controls. Supplied `physics.patch` SHA256:
`86e88ac96dd02b7a611222c8aea2a6fa8c536044331429646adb90182ba42a34`.
Integrated source equals the donor commit exactly; source SHA256:
`49cc368300ee9f798af0fef6a268915e41b36501074ae5f774a351bf7d6838e7`.

## Folded inline helper and ownership

The only new defined symbol is `_point_from_line3d`: 48 bytes, zero
relocations, public storage class 2, function type 0x20, entry offset zero.
Its raw and normalized bytes equal both January's selected action_charge
copy and the current compiled action_charge provider. SHA256:
`9b763841f8519177ac2d3fd471226972b9fa92cf1e9061e9f92359077dc82741`.
Section flags agree (`0x60501020`). January's physics split neither defines
nor references it: this uses the existing all-inlined shared-header class,
not a new ownership exception. The exact caller and full gates satisfy its
admission conditions; the surplus helper earns no additional code credit.

The compiled caller/provider copies use SELECT_ANY (2), and bounded
selected-provider duplicate-definition checks pass in both input orders.
No `/FORCE` is used. Unresolved dependencies are expected in the two-object
probe; this is not a successful whole-program link or boot claim.
For completeness, an additional diagnostic linking the raw synthetic January
split reports six duplicate helpers in either order: its selected sections
use NODUPLICATES (1). That diagnostic is explicitly NOT recorded as passing
and does not substitute for the current-source provider checks.

## Fresh gates

- Full ninja before and after: pass; actual objdiff remains 3.3.1.
- Whole-board stable snapshot: 8,252 owners, **7,630 -> 7,631 exact**;
  one gain, zero regressions.
- Whole-TU non-debug inventory: 82 -> 83 sections. Only the target changes,
  only the disclosed helper is added; all other 81 sections remain equal.
  All 42 non-code sections, empty COMMON set, and 86 pre-existing named
  definitions retain their storage/type/value and section flags/selection.
- Parks: **76 active / 0 stale / 0 invalid**, unchanged. The target had no
  park to retire; `config/parked.json` remains untouched.
- Admission unchanged: 10 candidates / 0 contradicted / 1 rejected / 0 revoked.
- `/W3` diagnostic census unchanged: C4146 x1, C4244 x11, all inherited.
- Fake scan is identical before/after: 26 inherited board-wide leads.
- Tools tests before and after: **1,161 passed, 5 skipped, 26 subtests passed**.
- `git diff --check` clean; source CRLF preserved. No header, config, tooling,
  compiler/scorer or data changes.

Private receipts are in `scratch/reconcile_physics_collision_20260926/`:
before source/object, build logs, gates, stable snapshots, semantic reports,
warning census, parks/admission/fake/test logs, and independent
`verification.json` with hashes, full section census and both provider orders.
Reference executables and compiler assets are not included in publication.
