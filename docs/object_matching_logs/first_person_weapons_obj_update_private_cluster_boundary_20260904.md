# `first_person_weapons.obj` complete update-cluster ownership rejection (2026-09-04)

## Disposition

This was a scratch-only reconstruction against canonical commit
`ebba3978e494f0f91af355d35746e3dad1436ea6`. It completes the authentic
four-function lifecycle cluster behind `first_person_weapons_update` without
an external placeholder:

| January owner | Recovered semantic name | Padded | Meaningful | Relocs | Normalized SHA-256 | Scratch result |
|---|---|---:|---:|---:|---|---|
| `_code_000cd610` | `first_person_weapon_new_unit` | 96 | 84 | 6 | `fab9a0295ecae4406b810cef884976d57647dc742ae31abe22fbd889369c875a` | exact |
| `_code_000cd920` | `first_person_weapon_next_state` | 368 | 368 | 24 | `e067dbaed12a7bd5e2c79cbbc85033a6ca2763a937c14e111adb6b7f17a67637` | exact |
| `_code_000cda90` | `first_person_weapon_update` | 1,536 | 1,533 | 59 | `912d95683f14ff39699c4a1056af92441a20ceeec0584007512ce26b5d70478d` | credible residual |
| `_first_person_weapons_update` | `first_person_weapons_update` | 240 | 236 | 16 | `4414a44fbbd90865ec157d2f1de5a7f68309e1b172c65758b1c4d922807aff57` | exact |

The three strict closures represent **688 meaningful / 704 padded scratch
bytes**. They are not admitted. The complete candidate emits six select-any
code COMDATs that collide with January's selected no-duplicates owners under
the actual XDK linker. The integration disposition is therefore **zero
canonical bytes and zero canonical functions**. This does not impugn the
reconstructed target-function logic; it is a physical whole-TU ownership
failure under the current inline schedule.

No production C or header, target metadata, park, admission record, or build
output was changed by this packet. In particular, this review does not mark
`first_person_weapons.obj` matching.

## Frozen scratch proof

The baseline object was built with:

```text
python tools/campaign/gate.py source/interface/first_person_weapons --all --forbid-emitted-symbol _point_from_line3d --out scratch/first-person-weapons-update-final-before-20260904.obj
```

It passed the hard forbidden-symbol guard at **26 exact / 4 residual / 4
unwritten**. Its SHA-256 is
`95d3ca87ad20721a11d3a65f094467473ad45d9a5e83fd2fdd703eed36f92bd6`.

The final source is
`scratch/first-person-weapons-full-cluster-natural-20260904.c`, SHA-256
`363fac55178e34bc29afd51b5ea58e0e80ef957f25939b657918bafcdcbca8b1`.
The compile command was:

```text
python tools/campaign/gate.py source/interface/first_person_weapons --source scratch/first-person-weapons-full-cluster-natural-20260904.c --alias first_person_weapon_new_unit=code_000cd610 --alias first_person_weapon_next_state=code_000cd920 --alias first_person_weapon_update=code_000cda90 --all --forbid-emitted-symbol _point_from_line3d --out scratch/first-person-weapons-full-cluster-natural-final-20260904.obj
```

It passed at **29 exact / 5 residual / 0 unwritten**. The artifact SHA-256 is
`1d45c47aa5b8edd51a2b4e9ffeb6cda27db4db2a8e68e97cf10f46ee41c3f67b`.
All 26 inherited exact owners remain strict. All four inherited residuals
remain residual; the only new residual is the 1,536-byte per-player update.
The emitted-symbol guard found no `_point_from_line3d` definition.

The candidate contains 40 code definitions: all 34 January target functions
plus the six helper definitions audited below. The target contains 34 and the
unchanged canonical base object contains 30. No target function is missing.

## Per-player residual boundary

The natural `first_person_weapon_update` candidate has 1,552 padded / 1,545
meaningful bytes and 60 relocations, versus January's 1,536 / 1,533 and 59.
Its normalized SHA-256 is
`2c5b4f64b02e0565a0b72e2b975254ee7a097ba2221bf3d134f48d7cb52e96c5`;
the January hash is
`912d95683f14ff39699c4a1056af92441a20ceeec0584007512ce26b5d70478d`.
Objdiff measures 98.01%. The aligned streams begin diverging at ordinary
frame/local scheduling (`sub esp,0x14` in January versus `sub esp,0x10` in the
candidate) and substantially diverge only at the current header expansion of
`real_local_random`: January calls `_real_local_random`, while the candidate
calls `_get_global_local_random_seed_address` and `_real_seed_random`.

That is a natural fuzzy boundary. No source spelling, declaration-position,
register, volatile, forced-inline, pragma, inline-assembly, or dead-code search
was used to chase it.

## Source and semantic authentication

January COFF is authoritative for ABI, branch behavior, call order, bytes, and
relocations. HCEA supplies readable semantic names and bodies, with Pastudan
and the Sapien function map as secondary corroboration. The reviewed HCEA
source family was commit `570c83fd9c365dad6f2a3e7041705d5b84c7847c`.
HCEA's two-local-player manager was not treated as an Xbox byte oracle;
January and Pastudan establish the four-player loop.

The scratch source deliberately preserves several initially suspicious but
target-authenticated details:

- January `code_000cda90` at approximately `+0x234..+0x25e` and
  `+0x283..+0x2ae` tests the first-person-animation block count, produces a
  null pointer when the count is zero, and then dereferences it. Pinned HCEA
  source has the same expression. The reconstruction does not invent a null
  guard; safety relies on the established weapon-switch invariant that a
  selected first-person weapon has a nonempty animation block.
- January calls `tag_get` for the first-person model between the weapon and
  animation-graph lookups and discards its return value. The typed
  `model_definition_get(...)` expression is therefore a genuine validation
  lookup, not an unused-local or match anchor.
- `animation_update_kind_render_only` is value 0 in canonical
  `model_animations.c` and HCEA's `animation_update_kind.h`;
  `_animation_key_frame` is 1 and `_animation_will_restart_on_next_frame` is
  2 in HCEA and January's comparisons. January accepts either result before
  transitioning state.
- January always evaluates `unit_flying_through_air` after the velocity
  magnitude and uses the six-argument `object_impulse_sound_new` call. Those
  target behaviors supersede differing later-donor source.
- Access goes through typed object/tag helpers, real/enumerated types and
  existing APIs. Every new private declaration and definition remains in the
  C file, the already-correct public prototype remains in
  `first_person_weapons.h`, no raw offsets or representation casts are used,
  and all void functions terminate with `return;`.

The manager's ordinary call graph also explains codegen that might otherwise
look artificial: the 96-byte `first_person_weapon_new_unit` has a retained
standalone owner, while VC7 naturally inlines the same small helper into the
manager. The full private bodies are present; there is no external alias,
stub, reference anchor, or hand-expanded duplicate.

The local animation enum views are evidence-backed but remain a source-layout
debt: canonical model headers do not yet expose those owning enums. A shared
header migration, if desired, must be isolated and blast-tested rather than
mixed into this scratch packet.

## Semantic-name and linkage proposal (not applied)

The following target metadata rename would be appropriate only after the
ownership issue is resolved and the complete TU is admitted atomically:

| January file offset | RVA | Proposed target name | Storage |
|---:|---:|---|---|
| 841,232 | `0xCD610` | `_first_person_weapon_new_unit` | static |
| 842,016 | `0xCD920` | `_first_person_weapon_next_state` | static |
| 842,384 | `0xCDA90` | `_first_person_weapon_update` | static |

The semantic names are corroborated by HCEA and the function map. Static
storage is a strong inference from the same-TU-only January call graph and the
absence of these names from the January public PDB list; it is not claimed as
direct PDB proof. The public owner at file offset 843,920 / RVA `0xCE090`
remains `_first_person_weapons_update`.

## Complete helper-ownership audit

The final candidate emits six code COMDATs absent from January's physical
`first_person_weapons.obj`:

| Candidate-only symbol | Size/relocs | Candidate normalized SHA-256 | January selected owner | Selected-owner hash relation |
|---|---:|---|---|---|
| `_magnitude_squared3d` | 48/0 | `0d04579096eeab3a1c45f87774e1eb20ce4856dbac219f624a855e8c9ae99725` | `source/ai/action_alert.obj` | identical |
| `_square_root` | 16/0 | `0717923dacb4356a94553a32f0ba8a314904fae6d7d0bfc42ecc4af6f10415b7` | `source/ai/action_charge.obj` | identical |
| `_magnitude3d` | 48/0 | `114f10e7ee4543446daaaafc859a4ec729ba6c8c74c38825d05bd0f00a42352c` | `source/ai/action_charge.obj` | identical |
| `_real_local_random_range` | 32/2 | `7ed2e50e8e1a701a98fcceeb6a6b67e05d997f21bdb9ff5ee2e848fd1a8f409a` | `source/camera/bored_camera.obj` | identical |
| `_real_local_random` | 16/2 | `50eeeb3dc58069325659d8bc0b6f2c17bb9f4fdce278b2f1a37edb64f4fcc60c` | `source/effects/effects.obj` | different; target is `a8ad3cb30f329f39f3193d4112f79d5983d27e7d41b1c99b5e0b4cd98e555bd0` |
| `_signed_angular_difference` | 64/4 | `c45ba91e0bdd268247bfef87604a2b00a3561c66a78a2e77623331db34bfd8f8` | `source/game/player_control.obj` | identical |

Each candidate helper is `IMAGE_COMDAT_SELECT_ANY` (selection 2). Each
January selected owner is `IMAGE_COMDAT_SELECT_NODUPLICATES` (selection 1).
Equal normalized bytes in five rows do not make the ownership compatible.

The decisive XDK 3911 reproducer was:

```text
xbox\bin\vc7\Link.Exe /NOLOGO /DLL /NOENTRY /NODEFAULTLIB /FORCE:UNRESOLVED /OUT:scratch\fpw-helper-link-test-final-dll-20260904.dll scratch\first-person-weapons-full-cluster-natural-final-20260904.obj build\split\source\ai\action_alert.obj build\split\source\ai\action_charge.obj build\split\source\camera\bored_camera.obj build\split\source\effects\effects.obj build\split\source\game\player_control.obj
```

Before the expected unresolved project dependencies, Link.exe reports exactly
these ownership failures:

```text
action_alert.obj : error LNK2005: _magnitude_squared3d already defined in first-person-weapons-full-cluster-natural-final-20260904.obj
action_charge.obj : error LNK2005: _square_root already defined in first-person-weapons-full-cluster-natural-final-20260904.obj
action_charge.obj : error LNK2005: _magnitude3d already defined in first-person-weapons-full-cluster-natural-final-20260904.obj
bored_camera.obj : error LNK2005: _real_local_random_range already defined in first-person-weapons-full-cluster-natural-final-20260904.obj
effects.obj : error LNK2005: _real_local_random already defined in first-person-weapons-full-cluster-natural-final-20260904.obj
player_control.obj : error LNK2005: _signed_angular_difference already defined in first-person-weapons-full-cluster-natural-final-20260904.obj
fatal error LNK1169: one or more multiply defined symbols found
```

This is the same incompatibility class independently reproduced in
`docs/object_matching_logs/action_charge_obj_jonas_math_helper_recovery_rejection_20260826.md`,
where both input orders fail for a selection-2 candidate against a selection-1
`_magnitude_squared3d` owner. It is not a theoretical extra-COMDAT veto: the
ordinary linker demonstrably cannot discard these copies.

The candidate-only non-code inventory is benign but receives no credit. It
contains twelve selection-2 `.rdata` COMDATs: one assertion string and eleven
floating constants. Each is byte-identical to an existing selected owner and
is fold-compatible. There is no candidate-only writable data, BSS, or COMMON
owner. The candidate also exposes `__fltused` and the seed APIs used by its
wrong local-random expansion; these corroborate the inline-schedule mismatch.

## Reopen boundary

Do not retry source spelling, declaration order, forced-inline/noinline,
pragma, volatile/register placement, inline assembly, helper anchors, raw
arithmetic, or a caller-local extern. Do not weaken the selected-owner rule
or alter the shared January math schedule merely to admit this one object.

Reopen only after an authentic shared math declaration/definition and inline
schedule is established across all existing consumers such that the rebuilt
FPW TU emits none of the six colliding definitions. Then re-run the complete
34-target-function gate, the hard `_point_from_line3d` guard, the full
code/data/BSS/COMMON census, and an ordinary XDK link against all six selected
owners. The natural 1,536-byte parent may remain an honest park; exactness is
not required to retain credible source once whole-object ownership is valid.

Until then, the frozen source and artifact are durable reconstruction evidence
only, with **zero admitted delta**.
