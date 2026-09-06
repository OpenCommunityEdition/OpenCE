# ActionObey command-owner trial: rejected and restored

Date: 2026-09-05. Baseline: `c32247e7651d680dcd133d72b2fbf4e8009cc124`.

## Disposition

No source or byte credit from this shared-header trial is retained. Its full
12-file source delta was reversed with a scoped patch; no checkout/reset/clean
or donor edit was used. The completed earlier shared swarm/biped/API packet
and structure-cache leaf remain intact.

The trial moved actor fire-target/combat-status enums to `actors.h`, moved
the unit animation impulse domain to `units.h`, and put command modifier
domains plus complete animation/script/recording reference records in
`ai_scenario_definitions.h`. Corresponding local shadows in nine C files
were removed. Datum field types and layouts were not changed; vehicle seat
desire ownership and Units implementation/private-name cleanup were not added.

The source proposal's first patch accidentally contained a redundant
`struct ai_command_definition;` forward declaration. This was removed as
an editing correction before the measured after snapshot. It was not
retained or searched as a compiler-order device.

## Measured failure

Actual Ninja dependencies identified **86** consuming base objects. The
complete strict sweep fell **6,233 -> 6,232**: the previously exact
`source/units/units::_unit_preprocess_node_orientations`, 1,920 padded bytes,
became residual. The full owner comparison found:

- 6,270 unchanged runtime sections;
- 84 changes restricted to compiler-local label spelling;
- one changed runtime owner, the Units function above;
- no added forbidden `_point_from_line3d` definition/reference.

The trial was rejected rather than exchanging inherited exact progress for
new dependency coverage. No header ordering, source spelling, flags, or
local facade variants were searched to rescue it.

## Restoration proof

The restored full sweep is **6,233 exact**, zero gains and zero losses.
All **6,355** runtime sections across the 86 affected objects compare
unchanged after restoration. Units is back to 189/189 exact.

The first restored capture correctly refused when its Ninja-dependency
database hash changed during enumeration. No compiler processes remained;
the next unchanged-state capture succeeded. The failed attempt wrote no
manifest or object snapshot and was not accepted as evidence.

Frozen evidence paths, relative to the canonical repository:

- `scratch/action_obey_command_owners_capture_20260905.py`
- `scratch/action-obey-command-owners-before-20260905.json`,
  SHA-256 `34c077c0b3cbe43476e5e9dadc1a24fc22d29c462db23976b8ae1b8a45d8702d`
- `scratch/action-obey-command-owners-after-20260905.json`,
  SHA-256 `090874e7e335ae137a823e4de032de6122ca89bee8860bfd397fec166a08a404`
- `scratch/action-obey-command-owners-restored-20260905.json`,
  SHA-256 `23e6ff746d10fa75f5c23449f16903138281cac98ca62b54da20053adcd4523a`
- corresponding `*-stable-before/after/restored-20260905.json` snapshots;
- `scratch/action-obey-command-owners-runtime-20260905/summary.json`;
- `scratch/action-obey-command-owners-restored-runtime-20260905/summary.json`;
- measured corrected-build and restored-build logs under the same prefix.

The capture folders preserve every proposed/restored source file and all 86
actual objects. This is a rejected trial, not proof that every source-authentic
owner reconstruction must fail.

## Corrections discovered during independent prerequisite review

The complete private `action_obey_command_begin` and
`action_obey_command_perform` proposals remain scratch-only and uncompiled.
Later direct PDB checks refined the initial proposed owner declarations:

- The true wait names are `_ai_atom_wait_modifier_alerted`,
  `_ai_atom_wait_modifier_visible_enemy`, and
  `_ai_atom_wait_modifier_told_to_advance`, values 0..2. The adjacent
  "always" literal belongs to loop modifier zero, not a fourth wait value.
  January's out-of-domain wait behavior still returns true.
- PDB records authenticate all fourteen unit animation impulse names and
  the full three scenario reference layouts. Their `unused` fields are
  genuine records, not fabricated opaque prefixes.
- PDB constants belong to unnamed enums. Descriptive named enum tags in the
  initial proposal were reconstructions, not recovered source names.
- The fire-target terminal is DB-spelled `number_of_actor_fire_targets`;
  the previously suggested uppercase spelling was a style reconstruction.
- Directly named crouch/targeting/initiative/die modifier constants can
  replace the private begin proposal's remaining raw boolean-valued literals
  when the proper shared owner is integrated.

These corrections are preserved in the prerequisite inventory and do not
authorize a definition-order or equivalent-spelling search. A future coherent
owner packet must use the refined evidence, preserve actual dependent
objects, and keep private functions private. No consumer-local enum/type/
prototype shadow or fake external private ABI is an acceptable workaround.

The independently acceptable smaller callback graph does not need this
failed owner packet; it is tracked separately.
