# `actor_combat.obj` interrupted-Opus cleanup (2026-09-04)

## Result

The interrupted Opus continuation contains no additional strict matches for
`actor_combat.obj`: both its preserved `w6_actor_combat.obj` and current
canonical classify the unit as **18 exact / 6 residual / 10 unwritten**.
One source-quality improvement is nevertheless retained.  The
`actor_aim_grenade` prop-target path now constructs a typed
`real_point3d desired_grenade_target` instead of filling a
`real_vector3d` and casting its address to an unrelated structure pointer.

This packet contributes **zero strict functions, zero padded bytes, zero
meaningful bytes, and zero data bytes**.  It improves the honest residual from
79.125% to 82.03977% while leaving its padded extent and relocation count
unchanged:

| Measure | January target | Retained candidate |
| --- | ---: | ---: |
| Padded extent | 544 | 576 |
| Ordered relocations | 22 | 22 |
| Normalized SHA-256 | `e0f88f129bff43bffd1418d177c79d4849c02f0e71e1acf1cf188c16030193fb` | `e9bba5b05f536e0aafd56e6fa491a8560347116525e58787e26f295b0049a264` |
| objdiff similarity |  | 82.03977% |

## Evidence and retained cleanup

January instructions, relocations, ABI, and typed canonical layouts remain
authoritative.  The local HCEA `actor_aim_grenade` lift independently
corroborates that the temporary passed to
`actor_combat_retarget_grenade` is a point copied from
`prop->body_position`, with `0.2f` added to its `z` coordinate.  The retained
source expresses exactly that type and behavior.  It uses the existing typed
`prop_get` accessor and private helper signature, creates no new ABI or owner,
and avoids a pointer/representation pun.

All 18 previously exact functions remain exact.  The other five parked
residuals retain their measured hashes and percentages, and the protected
January inline schedule remains unchanged.

## Rejected donor changes

The remaining dirty donor hunks add a new static
`actor_combat_get_grenade_projectile_definition` abstraction and route both
`actor_combat_reaim_grenade` and the already exact
`actor_combat_build_grenade_trajectory` through it.  The helper has no January
function section, public symbol, PDB name, or independent source evidence; it
survives only because VC7 inlines it into both callers.  Retaining it would be
an avoid-inlining violation and an optimizer-facing refactor with no semantic
gain.  Those hunks are therefore excluded.

The donor does not implement any of the ten unwritten January sections and
does not improve the strict status of any residual.  The previously rejected
`random_boolean`, collateral-damage, and `point_from_line3d`-emitting nearby-
target candidates are not reopened because this continuation supplies no new
authoritative evidence for them.

## House-rule audit

The accepted change uses the project point and real types, a natural same-line
initialization, named fields, the existing owner declaration, and ordinary C.
It introduces no address-derived name, raw byte offset, caller-local
prototype, shadow structure, manual bit arithmetic, inline helper, assembly,
volatile/register forcing, barrier, pragma, fake dependency, representation
pun, undefined behavior, or nonsensical path.  It emits no
`point_from_line3d` symbol or COMDAT.

## Validation

- focused gate: 18 exact / 6 residual / 10 unwritten;
- all 18 inherited exact functions preserved;
- target-address-keyed whole-tree comparison: zero strict gains and zero
  regressions;
- full XDK 3911 source build and semantic/progress reports: pass after the
  parked measurement was refreshed;
- `actor_aim_grenade`: 82.03977%, 576 padded bytes, 22 relocations;
- parked-function, admission, and focused fake-match audits: pass;
- protected Units sentinel remains strict exact;
- no `point_from_line3d` symbol in the rebuilt object;
- tooling tests and `git diff --check`: pass.
