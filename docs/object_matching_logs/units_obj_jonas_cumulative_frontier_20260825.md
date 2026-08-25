# `units.obj` Jonas cumulative frontier (2026-08-25)

## Authoritative closeout state

This index records the policy-clean cumulative Units frontier at clean code
HEAD `1082b414a8e9de412e81b6c0acf64dbce9cc5a12`. It adds no code credit and
does not supersede the per-wave evidence linked below. At that commit:

- retained `source/units/units.c` is Git blob
  `c2ef1a3b08bec91cc2c534443f09732b6f3003cd`;
- the January split `units.obj` is 138,090 bytes with SHA-256
  `e6a24ca597147dacff1f06806613d6bcfa4fa48f359dfbf542c34935f674b19f`;
- the branch-local rebuilt object is 143,136 bytes with phase-specific raw
  SHA-256
  `984f05dfd8ce7e949f5ccca13676cc02b70e8ba6474b7d0bbedc3190b615862f`;
- zeroing the four-byte COFF timestamp gives branch-local whole-object
  SHA-256
  `92f0f15db649b8e9888e67377e309230906e591b4ff6eb15e570f070a7d0ac3e`;
- the direct code census is **168 strict exact / 12 emitted nonexact / 9
  absent** across all 189 January owners; and
- the target runtime-data census is **126 strict exact / zero emitted
  nonexact / 3 absent** across all 129 January owners.

Raw object hashes change with the COFF timestamp. Even the timestamp-zeroed
whole-object hash is worktree/debug-path local and is not portable between
worktrees. Admission authority is the padded section content together with
the complete relocation address/type/destination/addend identity, not a
cross-worktree whole-object hash.

The refreshed repository reports record 4,156 accepted semantic functions.
Campaign progress is 4,130 functions and 501,438 accepted code bytes. The
object remains `NonMatching`; no completion label or exception is claimed.

The 12 currently emitted nonexact January owners are:

```text
_code_00198fd0
_unit_animation_set_state
_unit_died
_unit_enter_seat
_unit_euler_aiming_update
_unit_preprocess_node_orientations
_unit_ready_desired_weapon
_unit_set_or_test_seat_and_weapon_label
_unit_start_animation_impulse
_unit_throw_grenade_release
_unit_update
_unit_update_animation
```

The nine absent January owners are:

```text
_code_001a0cf0
_code_001a2030
_unit_animation_start_action
_unit_cause_player_melee_damage
_unit_damage_aftermath
_unit_exit_seat_end
_unit_export_function_values
_unit_place
_unit_test_animation_impulse
```

## Ordered Jonas wave index and accounting

The 19 retained wave ledgers, in cumulative integration order, are:

1. [Inventory/public seven](units_obj_jonas_inventory_public_seven_20260824.md)
   — 7 exact code gains.
2. [Postprocess/IK cluster](units_obj_jonas_postprocess_cluster_20260824.md)
   — 3 exact gains.
3. [Private foundation](units_obj_jonas_private_foundation_wave_20260824.md)
   — 15 exact gains.
4. [Control/leaf four](units_obj_jonas_control_leaf_four_20260824.md) — 4
   exact gains.
5. [Seat-query/render-debug wave](units_obj_jonas_seat_query_wave_20260824.md)
   — 5 exact gains.
6. [Typed damage record](units_obj_jonas_typed_damage_record_wave_20260824.md)
   — 1 exact gain.
7. [Inventory/weapon closure](units_obj_jonas_inventory_weapon_closure_20260824.md)
   — 6 exact gains.
8. [Damage/lifecycle closure](units_obj_jonas_damage_lifecycle_closure_20260824.md)
   — 8 exact gains.
9. [Animation callers](units_obj_jonas_animation_callers_four_20260824.md) —
   4 exact gains.
10. [Vehicle scripting](units_obj_jonas_vehicle_scripting_wave_20260824.md) —
    2 exact gains.
11. [Typed projectile ray](units_obj_jonas_typed_projectile_ray_wave_20260824.md)
    — 1 exact gain.
12. [Vehicle-exit wrappers](units_obj_jonas_vehicle_exit_wrappers_20260824.md)
    — 2 exact gains.
13. [Typed declaration pair](units_obj_jonas_typed_declaration_pair_20260824.md)
    — 2 exact gains.
14. [Weapon-state private helper](units_obj_jonas_weapon_state_private_helper_0fab_20260824.md)
    — 2 exact gains.
15. [Update-animation private closure](units_obj_jonas_update_animation_private_closure_20260824.md)
    — 4 exact gains.
16. [Typed debug queries](units_obj_jonas_typed_debug_queries_20260824.md) —
    2 exact gains.
17. [Animation-update declaration](units_obj_jonas_animation_update_declaration_20260824.md)
    — 1 exact gain.
18. [Nearby-seat named-field recovery](units_obj_jonas_nearby_seat_named_field_20260825.md)
    — 1 exact gain.
19. [Format/terminal-return compliance](units_obj_jonas_format_return_compliance_20260825.md)
    — no code credit; reformats 15 campaign declarations and gives
    `_code_00198400` its required explicit terminal `return;`, with every
    runtime owner unchanged.

The first 18 bounded matching waves contribute 70 exact owners over the
authenticated 97-owner starting census. The nineteenth wave is a verified
no-codegen source-compliance correction. The remaining one-owner increase is an
**integration-only dependency synergy**: combining the public inventory work
with the private `code_0019a640` closure makes
`_unit_add_weapon_to_inventory` strict exact. The private-foundation lane
correctly classified that public caller as nonexact in isolation, so no
single wave ledger owns this extra credit. It is a required cumulative
regression sentinel: 304 padded bytes, 11 relocations, normalized target
SHA-256
`de883e04c6d062bfa397e4730c0b0b94ba4176d2f6d7f1a2a20d187bf06692c6`.

## Residual and reopen classes

The 21 current residual owners divide cleanly into three classes.

### Pending shared-interface package: six prospective exact gains

This package has **not** been authorized, applied, emitted, or admitted at
this HEAD. Existing isolated evidence identifies the following coherent
interface corrections and expected exact owners:

| Interface correction | Prospective strict owners |
| --- | --- |
| Remove the stale public `unit_ready_desired_weapon` declaration and keep the helper translation-unit-private | `_unit_ready_desired_weapon`, `_code_001a2030`, `_unit_exit_seat_end` |
| Change the animation-impulse formal from `short` to `long` consistently and use the natural four-byte typed tester scratch | `_unit_start_animation_impulse`, `_unit_test_animation_impulse` |
| Change the game-engine damaged-player damage-type formal from `long` to `boolean` across the callback, wrapper declaration, and wrapper definition | `_unit_damage_aftermath` |

The fixed package is a 35-translation-unit interface A/B, not a `units.c`-only
experiment. It must preserve the already exact game-engine wrapper sentinel
and every affected caller. It also naturally emits `_code_001a0cf0` as
nonexact support. If all six prospective owners pass the required A/B, the
projected census is 174 exact / 11 emitted nonexact / 4 absent. That projection
is not current credit. It would leave 15 residual owners: the policy-blocked
exporter plus the 14 measured residuals below.

### Policy-blocked owner

`_unit_export_function_values` remains absent. The complete Claude experiment
matrix found that its exact pointer setup requires `_ReadWriteBarrier()`.
That compiler fence is a prohibited scheduling control, so the exact carrier
form is inadmissible and the readable natural typed form remains nonexact.

### Fourteen measured code-generation residuals

After the shared-interface package, the remaining non-exporter frontier is:

| Residual family | Owners |
| --- | --- |
| Private/support | `_code_00198fd0`, `_code_001a0cf0` |
| Animation | `_unit_animation_set_state`, `_unit_animation_start_action`, `_unit_update_animation` |
| Lifecycle/seat | `_unit_died`, `_unit_enter_seat`, `_unit_place`, `_unit_set_or_test_seat_and_weapon_label` |
| Aiming/postprocess/root | `_unit_euler_aiming_update`, `_unit_preprocess_node_orientations`, `_unit_update` |
| Combat/grenade | `_unit_cause_player_melee_damage`, `_unit_throw_grenade_release` |

These are measured compiler/code-generation residuals, not proof of
impossibility. Representative closed packets include the three-byte
push/x87 scheduling tie in `_unit_cause_player_melee_damage`, the eight-byte
four-event scheduling residual in `_unit_throw_grenade_release`, and the
one-meaningful-byte allocator-coloring residual in `_unit_place`. Reopen any
of the 14 only for new source provenance, a newly typed dependency/interface,
or a demonstrated ordinary-C lever. None is classified as
compiler-internal or unreachable.

## Claude source and tooling reconciliation

The closeout inventory covers the consolidated and specialized Claude Units
object ledgers, per-function and final-topology records, compiler/QFE and
crossbuild provenance, donor/source provenance, and the associated campaign
and debugger documentation. Claude's authenticated evidence carrier is commit
`deb5554ab8d16da80da934e2aa7081bc7d1f594f`; its `units.c` blob is
`124f29e32497c091c4163b5faafc58dbf8890161`. Its nominal census is 175/189
strict code owners, but it is inadmissible wholesale: it contains the
exporter barrier, raw datum offsets/addresses including the `+0xB6` access,
pointer or inactive-union puns, `register` forcing, and
`__declspec(noinline)` forcing. It remains evidence, not patch authority.

The associated dirty tooling worktree has seven modified tracked files. Six
are uncommitted CL-path portability edits:

```text
tools/c2dbg32/gen_config_collapse.py
tools/c2dbg32/lab_collapse.py
tools/c2dbg32/lab_collapse2.py
tools/campaign/gate.py
tools/campaign/lab_compile.py
tools/campaign/units_hunt_all.py
```

The seventh, `tools/c2dbg32/dbg32.c`, contains a larger unvalidated debugger
change. None of these seven files, nor the worktree's untracked debugger,
vendored, object, or scratch outputs, was imported into the cumulative branch.

Two documentation cautions are binding:

- `tools/c2dbg32/README.md` still says breakpoints cap at six hits, while the
  tracked implementation uses `maxhits = 250`; the README is stale and is not
  runtime authority.
- Early portions of `tools/c2dbg32/IR_LAYOUT.md` interpret node `+0x28` as a
  physical-register assignment. The later empirical correction supersedes
  that text: the field is an operand width/size class, and the physical
  register-assignment and spill-decision sites remain unlocated.

UUA-41's accepted update-animation producer shape was measured with XDK 3911
VC7 `CL.Exe` 13.00.9254.1 only. It does not establish byte identity for 9210,
other QFEs, or a compiler suite. Together with the debugger cautions, this is
why no Units residual is promoted to a compiler-internal or unreachable claim.

## Binding source, format, and access rules

Any Units reopen must remain readable, natural, typed C. Inline assembly,
`volatile` scheduling, `_ReadWriteBarrier`, pragma/intrinsic controls,
attributes or `__declspec`/noinline controls, `register`, raw datum offsets or
addresses, pointer/union type puns, undefined behavior, synthetic anchors,
byte patches, and comparator exceptions are outside the accepted boundary.

The standing formatting/access rules also apply:

- a no-argument function spells `void` on its own line inside the parameter
  list;
- every parameter occupies its own line;
- every function, including a `void` function, ends with an explicit
  `return;` or value return as appropriate;
- typed tag access goes through the subsystem macro that wraps `tag_get`; and
- typed object access goes through the object-access macro instead of repeated
  casts after raw `object_get` calls.

Two retained target-exact functions are measured exceptions to that general
typed-object preference. In `_vehicle_scripting_load_magic`, replacing the
post-mask cast with `unit_get(unit_index)` changed the sole owner from
384/10/`f6df83b287af8e5318bed6dda05ef6d9b77261bacab5d2362cae4ca3101c389e`
to 400/11/`108925ea53ac5bd0076e2ba6229cdd7b409c27e3bac6b6b807969aed4fbdc475`.
In `_code_0019a170`, the analogous `unit_get(child_object_index)` change
moved the sole owner from
224/4/`48ec4825c898e5629cbb329e713372e94cedce56c050d4a030a791c61a797620`
to 240/5/`c69e2ae4adfa6e7ffab9fc355b6df1b19c04b1a6ff74715ba324c9ad8a7e0c2b`.
Each fixed one-shot added a second `_object_get_and_verify_type` call and
relocation after the existing generic retrieval and type-mask proof; each
regressed the census from 168 to 167 exact and was removed wholesale. The
current generic retrieval plus checked downcast is therefore retained as a
documented target-proven exception, not as a pattern for new code. Reopen it
only if an authenticated natural accessor can reuse the already verified
pointer without a second object lookup.

Use named structure fields, enum bits, and real subsystem interfaces. Private
same-translation-unit closures must have natural callers. The protected
neighbor paths `source/units/vehicles.c`, `source/ai/ai_debug.c`,
`source/ai/ai_script.c`, and `source/ai/actions.c` remain source-edit sentinels.

Before a bounded production wave, freeze its candidate and dependency closure,
run parse-only `/Zs`, and preserve an authenticated baseline. Use exactly one
natural production emission for the fixed wave; retain only strict padded-byte
and relocation-exact owners, remove misses wholesale, and do not tune or retry
without a genuinely new evidence-backed wave.

## Final acceptance and replay criteria

This additive index does not itself authorize or execute the pending header
package. If that package is authorized, final acceptance requires:

1. a clean 35-TU baseline manifest, successful parse-only checks, and exactly
   one normal production rebuild of each affected translation unit;
2. strict A/B preservation of every inherited exact function and data owner,
   every affected consumer object, the game-engine wrapper, the integration-
   only `_unit_add_weapon_to_inventory` sentinel, and the four protected-source
   sentinels;
3. direct Units code/data censuses, with each prospective gain admitted only
   on padded-byte and complete-relocation equality;
4. full `halobetacache_build` and `libcmt_build`, report regeneration,
   semantic/progress audit, object admission, parked-function validation, and
   all 179 tooling tests;
5. an implementation commit followed from a clean tree by a no-build
   regression snapshot, absolute-path containment proof for every generated
   object removed, literal-path deletion, and normal Ninja rebuild;
6. an immediate regression check with zero failures, warnings, inherited
   losses, or changed nonexact owners, followed by an additive ledger-only
   replay commit; and
7. the same snapshot/delete/rebuild/check sequence at corrected HEAD, plus a
   final Ninja dry run reporting no work.

Without an authorized shared-interface package, a final corrected-HEAD Units
replay must reproduce the current 168/12/9 code census, 126/0/3 data census,
4,156 accepted semantic functions, 4,130 campaign functions, and 501,438
accepted code bytes. No raw or whole-object hash alone can satisfy that replay.
