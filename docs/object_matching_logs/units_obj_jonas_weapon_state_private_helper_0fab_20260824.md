# `units.obj` weapon-state private-helper closure at 0fab (Jonas, 2026-08-24)

## Result and frozen boundary

This ledger records one bounded Jonas/Codex closure from latest cumulative
Units commit `0fab4bb1c991ddb114588e634150a65376bef9cc` on branch
`jonas/units-weapon-state-private-helper-0fab-20260824`. The frozen packet is
exactly the public dispatcher `unit_handle_weapon_state_change` and its
required same-TU private helper `code_00198400`.

A fresh 0fab isolation compile and the sole natural production compile both
emitted the complete packet strict exact: 649 meaningful bytes, 656 padded
bytes, and 41 relocations. The hardened function census advances from 156/189
to 158/189 exact. The January runtime-data census remains 123/129 exact. No
inherited exact function or data owner is lost, and `units.obj` remains
deliberately `NonMatching`.

The retained source contains no `register` keyword or forcing construct. No
header, protected path, configuration, semantic ledger, parked record,
storage declaration, or other translation unit changed.

## Fixed authority and artifact identities

- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `units.obj`: 138,090 bytes, SHA-256
  `e6a24ca597147dacff1f06806613d6bcfa4fa48f359dfbf542c34935f674b19f`.
- Cumulative source commit:
  `0fab4bb1c991ddb114588e634150a65376bef9cc`.
- Cumulative translation-unit blob:
  `45e28394c8073ab4eebea1892855d7665b29ca6a`, 239,929 raw Git-payload
  bytes.
- Hydrated clean 0fab baseline object: 133,253 bytes, whole-file SHA-256
  `9d6518991cbc0832704aa57dff0767b57471bb25f5a815459b28016e8e9debac`.
- Frozen retained pre-commit translation-unit identity:
  `6dd52f4b3e95d6af36e3386583d1850bc8aa42f9`; checkout payload 253,251
  bytes, SHA-256
  `59601fbcd0dee7f64e17c2e7ebb2a51ba5215b3a0b3a2fb8e14cf921ad4fcbbe`.
- Immutable sole production first-shot object: 135,029 bytes, whole-file
  SHA-256
  `27cd0b50aed14274efc52fa4946fdb63508af647ec7ccfb553262761bd51734d`.
- Compiler: XDK 3911 Microsoft 32-bit C/C++ Optimizing Compiler
  `13.00.9254.1`; `xbox/bin/vc7/CL.Exe` is 81,920 bytes with SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
  Natural flags remain `/O2 /Oy- /DDEBUG /Dxbox` with the repository include
  graph.

Historical source is reconstruction evidence only. January COFF, January
callers and relocations, current typed contracts, and the hardened comparator
are admission authority.

## Complete Claude/Jonas documentation and tooling audit

Before emission, every reachable Claude/Jonas Units-object ledger was
enumerated and reconciled. The consolidated Claude `units_obj.md`, its full
do-not-repeat inventory, and the complete historical
`units_obj_weapon_state.md` WH1-WH22 record were read in full. Direct supporting
records read in full include:

- `units_obj_private_helper_unlocks.md`;
- `units_obj_lifetime_controls.md`;
- `units_obj_animation_action.md`;
- the Jonas private-foundation, inventory, animation-caller, typed-projectile,
  vehicle-scripting, and vehicle-exit ledgers through 0fab.

The current `docs/matching_methodology.md`,
`docs/exact_match_acceleration_playbook.md`, and object-log `README.md` were
also read in full. Claude tooling at
`deb5554ab8d16da80da934e2aa7081bc7d1f594f` was inspected directly:
`tools/campaign/mk_repro.py`, `lab_compile.py`,
`tools/c2dbg32/BACKEND_MODULE_MAP.md`, and `gen_config_lab.py`. The corrected
module-map constraint was observed: string-xref addresses are cold assert or
error sites, not hot backend breakpoint targets. No debugger-derived forcing
or stale breakpoint assumption was used.

The two cumulative additions after the earlier proof were audited in full.
`unit_scripting_enter_vehicle`, `vehicle_scripting_unload`, and
`unit_adjust_projectile_ray` occur later in the translation unit. The complete
0fab prelude through this private closure is unchanged from the earlier exact
context, and all three cumulative exact functions remain present and strict.

## Carrier-history correction and policy-clean private convention

The later consolidated Claude carrier spells the helper's animation-slot local
with the policy-forbidden `register` keyword. That spelling is unnecessary.
Authenticated commit `9d534507b379ffbb18d993434d5719a89b9d8306`
records `code_00198400` as 448/448 exact using an ordinary block-scoped
`short anim_slot`, typed graph traversal, and no forcing construct. Commit
`62573db0a6de2baedc8ef20a83993de2e79550e4` records the final exact
two-result dispatcher topology. The completed earlier 68a proof commits
`71a57df8ca46e47296a524ada1a129c078726a8f` and
`0d60fd37` independently preserve the same policy-clean packet and committed
replay evidence.

The fresh 0fab scratch fixture combined the exact current prelude, the
ordinary-`short` helper body, the exact dispatcher, and only the source-local
typed declaration `unit_animation_start_action(long, short)`. VC7 emitted:

| Scratch body | Padded bytes | Relocs | Normalized SHA-256 | Result |
|---|---:|---:|---|---|
| `_code_00198400` | 448 | 24 | `6b902fccee7df603d88f668e360fa10a819bf8d6b54300f4210e8db825830e19` | strict exact |
| `_unit_handle_weapon_state_change` | 208 | 17 | `d9c77138327a0e90b58de773e0a7d1fcf6385e174b3ca8929f90bef54bfe17d3` | strict exact |

The earlier exact-context negative control is retained as exclusion evidence:
declaring the action formal as `long` produced a 224-byte dispatcher against
the 208-byte target; the target-proven `short` declaration produced the exact
body. Scratch work did not consume the single production emission.

The helper remains `static`. Natural same-TU caller pressure reproduces the
January private EAX convention for `unit_index` and all six private calls from
ordinary readable C. There is no `register`, assembly, intrinsic, pragma,
barrier, annotation, ABI invention, or byte-forcing expression.

## Typed behavior and real caller pressure

`code_00198400` enforces the overlay-priority gate, rejects the standard
fourteen incompatible animation states, and follows typed repository accessors
from unit definition to animation graph, unit seat, weapon class, and weapon
type. Overlay actions 1..6 map to the weapon-class slots for diving
front/back/left/right and turning left/right. The accepted path selects a
random permutation, initializes frame zero, and stores the overlay action. The
debug-only missing-animation path preserves the original warning literal and
typed labels/list lookup.

`unit_handle_weapon_state_change` uses two independent `short` result locals.
States 5 and 6 select action animations; states 1, 2, 3, 4, 7, and 8 select
the six overlays. Post-switch dispatch calls `unit_animation_start_action`,
otherwise the private helper. The real external producer path remains
`source/items/weapons.c::weapon_set_state`, whose local declaration and call
both establish `unit_handle_weapon_state_change(long, short)`. No header was
changed to manufacture pressure.

The January dispatcher has 17 relocations: two calls to
`_unit_animation_start_action`, six calls to `_code_00198400`, and nine
internal switch/table relocations. The helper has 24 relocations across typed
object/tag/block access, animation selection, its internal switch tables, and
the debug dependency/string identity.

## Strict production result

The hardened comparator requires equal padded bytes plus every normalized
relocation address, type, destination, and addend.

| Function | January RVA | Meaningful / padded | Relocs | Normalized SHA-256 | Result |
|---|---:|---:|---:|---|---|
| `_code_00198400` | `0x00198400` | 441 / 448 | 24 | `6b902fccee7df603d88f668e360fa10a819bf8d6b54300f4210e8db825830e19` | strict exact |
| `_unit_handle_weapon_state_change` | `0x001986f0` | 208 / 208 | 17 | `d9c77138327a0e90b58de773e0a7d1fcf6385e174b3ca8929f90bef54bfe17d3` | strict exact |

The gain is exactly 649 meaningful bytes, 656 padded bytes, and 41 relocation
identities.

## Frozen one-shot and policy record

The final tracked packet first passed the exact natural CL parse with `/Zs`.
The baseline object remained byte-identical at SHA-256
`9d6518991cbc0832704aa57dff0767b57471bb25f5a815459b28016e8e9debac`.
The next and only code-producing production `units.c` command was the normal
Ninja edge `build/base/source/units/units.obj`. Both bodies were strict exact
on that shot. There was no production retry, body tune, scheduling experiment,
or debugger intervention.

The retained source is typed readable C. Its added lines contain no assembly,
raw address or byte-offset access, pointer/integer reconstruction, pointer or
union pun, inactive-union access, undefined behavior, `volatile`, `register`,
optimizer pragma, intrinsic, force-inline/noinline/attribute annotation,
compiler barrier, synthetic anchor, or byte-forcing expression.

## Complete 189-function and 129-data census

The direct scan uses all 189 January external underscore-function owners and
accepts candidate static owners by name, as required for private functions.

| State | 0fab baseline | Candidate |
|---|---:|---:|
| strict exact | 156 | 158 |
| present nonexact | 11 | 11 |
| absent | 22 | 20 |
| exact padded code bytes | 29,728 | 30,384 |
| exact relocations | 1,279 | 1,320 |

The gain set is exactly `_code_00198400` and
`_unit_handle_weapon_state_change`. The eleven nonexact residuals are
unchanged: `_code_00198fd0`, `_unit_throw_grenade_release`,
`_unit_set_or_test_seat_and_weapon_label`, `_unit_animation_set_state`,
`_unit_preprocess_node_orientations`, `_unit_euler_aiming_update`,
`_unit_start_animation_impulse`, `_unit_ready_desired_weapon`,
`_unit_enter_seat`, `_unit_died`, and `_unit_update`.

The twenty absent residuals are `_unit_export_function_values`,
`_code_00197f90`, `_code_00198050`, `_code_00198070`,
`_unit_animation_start_action`, `_unit_test_spawning`,
`_unit_test_animation_impulse`, `_units_debug_get_next_unit`,
`_units_debug_get_closest_unit`, `_code_0019b160`, `_unit_find_nearby_seat`,
`_code_0019dff0`, `_unit_cause_player_melee_damage`,
`_unit_update_animation`, `_code_001a0cf0`, `_unit_place`, `_code_001a2030`,
`_unit_exit_seat_end`, `_unit_try_and_exit_seat`, and
`_unit_damage_aftermath`.

The complete 129-owner January runtime-data census remains 123 exact, zero
present nonexact, and six absent: 6,359 exact logical bytes and seven
relocations. No target data owner, writable aggregate, BSS, COMMON symbol, or
storage declaration changed.

## Initial full gates

- Fresh 0fab isolation: both bodies strict exact.
- Exact `/Zs` parse-only pass: success; baseline object unchanged.
- Sole natural production compile: success; both bodies strict exact.
- Direct two-name hardened comparison: `all_equal: true`, reproducing 656
  padded bytes, 41 relocations, and both hashes above.
- Complete function census: 158 exact / 11 present nonexact / 20 absent;
  exactly two gains and zero inherited losses.
- Complete target-data census: 123 exact / zero present nonexact / six absent;
  zero gain and zero inherited loss.
- Complete 568-edge Halo plus libcmt build: pass; the frozen Units object was
  already up to date and was not rebuilt.
- Installed objdiff report and semantic audit: pass; 470 units, 4,226
  functions evaluated, 4,085 semantic exact, 112 hidden exact / 64,495 hidden
  code bytes, 4,146 accepted exact, and zero unit errors. Both retained names
  are explicitly present in the hidden-exact evidence.
- Campaign progress remains at the current ledger-backed 375/833 objects,
  4,122/11,060 functions, and 499,793/2,198,102 code bytes. The two new strict
  hidden names receive no semantic-ledger credit in this source-only wave.
- Object admission: zero candidates and zero revocations; only the inherited
  `shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, zero invalid.
- Complete tooling suite: 179/179 tests pass.

The generic pre-wave snapshot was not manufactured from a dirty source: the
gate correctly refused that invalid state. Instead, the clean 0fab baseline
commit/blob/object is pinned above, its complete 156/11/22 and 123/0/6 census
was captured before emission, and the candidate preserves every inherited
acceptance fingerprint. The clean committed-state snapshot and forced replay
below are the regression-gate authority.

## Commit/replay boundary

This initial ledger records only frozen one-shot evidence and does not prewrite
a committed replay. The next steps are to remove scratch artifacts, commit
exactly `source/units/units.c` plus this new ledger as Jonas, prove a clean and
up-to-date object, capture a clean implementation-state snapshot, verify and
delete only the isolated generated `units.obj`, run one normal one-target
Ninja rebuild, and rerun the manifest, two-name comparator, full 189/129
census, semantic audit, and ownership checks. Actual replay evidence will be
appended here in a separate additive Jonas ledger-only commit. No amend, push,
history rewrite, or worktree removal is authorized.

## Actual committed-state forced replay

The Jonas implementation-and-initial-ledger commit is
`d30957950be81a14e77e6d48fe9027555dd8e3d5`. Both author and committer are
Jonas Volman `<jonas.volman@openai.com>`. Its committed translation unit is
blob `6dd52f4b3e95d6af36e3386583d1850bc8aa42f9`: 244,151 raw Git-payload
bytes, payload SHA-256
`418e49550322751e79a126c62cdd2b67620589424d8c31252bd881f6d15b6c5a`.
The initial ledger is blob `82e76cee2d292d299f028387dffe0edec2a760fc`:
13,040 bytes, payload SHA-256
`f35cc06fe7b4606340f2ef80d14d85775165248c11779b80f741453d58612748`.

Tracked state was clean at that commit, and an explicit one-target Ninja dry
run reported `ninja: no work to do`. The gate's built-in `--no-build` mode then
captured this clean implementation-state snapshot:

```text
build/regression_units_weapon_state_committed_0fab_20260824.json
commit: d30957950be81a14e77e6d48fe9027555dd8e3d5
size: 5,870,179 bytes
SHA-256: bd8ba3d8bc1b030539b7c50b9752a33574cecdf38b37a62ffdd84e219c17212c
```

The generated object resolved to
`C:\Users\isabe\Documents\Codex\2026-07-13\i-w\units-weapon-state-private-helper-0fab-20260824\build\base\source\units\units.obj`.
Its normalized absolute path was proven to begin with the isolated worktree
root, and its basename was proven exactly `units.obj`. Before deletion it was
135,029 bytes with whole-file SHA-256
`27cd0b50aed14274efc52fa4946fdb63508af647ec7ccfb553262761bd51734d`.
Only that verified generated file was removed, and absence was checked.

One ordinary one-target Ninja rebuild then ran exactly:

```text
[1/1] CL build\base\source\units\units.obj
```

The replay object is again 135,029 bytes with phase-specific whole-file
SHA-256
`7d1ce04910c59429534476118594ee9d96dd2099bcf8a13d7321979feeda605b`.
The whole-file difference is confined to normal COFF build metadata; every
runtime acceptance fingerprint is unchanged.

The committed regression check is authoritative for all captured function,
non-code, relocation, and symbol-owner evidence:

```text
ok: true
failures: 0
warnings: 0
still_exact: 158
newly_exact: 0
changed_nonexact: 0
```

Independent replay proofs also passed:

- direct two-name hardened comparison remains `all_equal: true`, reproducing
  both normalized hashes, 656 padded bytes, and 41 relocation identities;
- complete function census remains 158 exact / 11 present nonexact / 20
  absent, 30,384 exact padded bytes and 1,320 exact relocations;
- complete target-data census remains 123 exact / zero present nonexact / six
  absent, 6,359 exact logical bytes and seven relocations;
- the helper remains the intended private owner: January csplit external
  storage class 2 and natural candidate static storage class 3, both
  value-zero type-`0x20`; the public dispatcher remains value-zero type-`0x20`
  external storage class 2;
- `_unit_animation_start_action` remains a normal undefined external in the
  candidate, preserving the bounded support boundary;
- semantic audit again reports 4,085 semantic exact, 112 hidden exact / 64,495
  hidden code bytes, 4,146 accepted exact, and zero unit errors;
- the complete Halo and libcmt targets report no work, the committed source
  blob is unchanged, and the final one-target Ninja dry run reports no work.

This appended record is ledger-only. No source change, candidate retry, body
tuning, helper forcing, header/protected/config/storage edit, adjudication,
amend, push, history rewrite, or worktree removal occurred after the
implementation commit.
