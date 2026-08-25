# `units.obj` bounded inventory/public seven-body wave

Date: 2026-08-24

Owner: Jonas

Verdict: **retain seven strict functions; no whole-object completion claim**

## Scope and frozen inputs

This lane began from clean campaign commit
`8549f46cd0370eefba28b309e2f5db66bf204f60` in the isolated branch
`jonas/units-inventory-public-six-20260824`. The only source path changed is
the explicitly authorized owner translation unit `source/units/units.c`; the
only other path added is this new Jonas ledger. No shared header,
configuration file, protected neighboring source, pre-existing ledger, or
runtime-storage definition changed.

The fixed dependency-closed source wave is:

1. `unit_add_equipment_to_inventory`
2. `unit_add_grenade_to_inventory`
3. `unit_approve_weapon_swap`
4. `unit_drop_current_weapon`
5. `unit_inventory_next_weapon`
6. `unit_shield_sapping_update`
7. private same-TU helper `unit_weapon_next_index`

The helper was added to the originally proposed six-body boundary before any
code-producing compile. Historical exact evidence showed that the wrapper and
drop body were not independently admissible against the defective canonical
helper. No other body, including any of the 14 residual Claude functions, was
in scope.

Frozen payloads:

| Artifact | Git object / size | Raw payload SHA-256 |
|---|---:|---|
| Baseline `source/units/units.c` at `8549f46c` | `19a15c4a3a47849ffd8ba0c13989c0c389452519`, 118,776 bytes | `44930da19d5f2331995a282524f8b01cce5e6f283564ac76c695fb9c1f81f76d` |
| Retained staged `source/units/units.c` | `686c23e90655f970b9bcedb9aa3447123d006c72`, 124,248 bytes | `ca8d3e1843354436bcede267243dcc84b9d752c9aa39d09b1c122ea3bf2311bb` |
| January csplit `source/units/units.obj` | 138,090 bytes | `e6a24ca597147dacff1f06806613d6bcfa4fa48f359dfbf542c34935f674b19f` |
| Clean pre-edit candidate `units.obj` | 72,398 bytes | `b21a7d4dd3ba5bec2d7e1e0d9ec9db5503ee62bd474c3fa9e296cc3474cfc4f6` |
| One-shot retained candidate `units.obj` | 75,827 bytes | `881b98385530cc6f62fa6234346483d6c615bffeee303658ae0b721ae6ec70bf` |
| Clean pre-edit regression manifest | 4,479,107 bytes | `7c5486e788e2e61b0a45b29265c76835cd826002598e41109e164d23f1848958` |
| VC7 `CL.Exe` 13.00.9254.1 | 81,920 bytes | `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11` |

All Git payload sizes and hashes above are raw `git cat-file blob` bytes, not
CRLF checkout bytes.

## Independent source provenance

The clean HCEA semantic reference worktree was reachable and clean at
`c168af2e747d3095d9a29418ae401f3a39544863`. Its seven raw donor payloads are:

| HCEA path | Git blob | Bytes | SHA-256 |
|---|---|---:|---|
| `src/unit_add_equipment_to_inventory.c` | `7568f9545b28e72c53633c0e9b438d0fb8fcad1c` | 2,092 | `124a2ef7ffdceaa132a81b093cdf2f67f6af0f192493e77d08f3a5e58f505312` |
| `src/unit_add_grenade_to_inventory.c` | `f74bc4572f7c184231b3b333ecc32239115c3c36` | 2,263 | `850b599d5e36b8ae5bcea961fd14fe2c3cf0061e2d36d2c42a83892449d21ada` |
| `src/unit_approve_weapon_swap.c` | `d78ffc20555d4d3dfa38371cba4be1eeb1416c22` | 2,433 | `8760a02fda2a778b77abf666f312f0a7db6b929bdf89ea4f5c075bea18736a10` |
| `src/unit_drop_current_weapon.c` | `ad0eb791cf212a5f4c10844699dd1afc5f72af4b` | 3,757 | `d006b78d529ef417f2ff84b9e9a48819f244bb1cb0e965036bcc89400f8de937` |
| `src/unit_inventory_next_weapon.c` | `643044cb404dd5845a2dc3c90e1cee951d0fc4b0` | 447 | `5c17bb9dd5e9b552f26e95357f3c74d8bc3005c655fdc6f8bcec40f48136483c` |
| `src/unit_shield_sapping_update.c` | `38f669592a6f0747d5b051d8478db5ef52305139` | 4,007 | `ed04cfe5e8800b17cbc9875238884b4c24e2f53d1f3ceff6ddfffbd89899f820` |
| `src/unit_weapon_next_index.c` | `f42a26a7ab12ee7f9e2bd2b225524c88fc6a3f8f` | 2,630 | `12b071eba532043b9cead41cfdd9138d6c6c04a1d5405dbfeb44112470e8f97b` |

HCEA is semantic Xbox provenance, not PC byte authority. January PC COFF and
the January PDB/caller graph control ABI, layout, instruction bytes, and
relocation identities.

The readable PC body carrier used for bounded manual transplantation is the
clean Claude source blob
`124f29e32497c091c4163b5faafc58dbf8890161` at `deb5554a`: 305,068 raw bytes,
SHA-256
`173fd6fdcc09d979ba6507aefa2cd77790f8f17f93413b9acbb679c32bb8f7c4`.
Commit `311b00d51f6bf0dce48fc78a91bc1a773386fa75` is the final carrier for the
six public-body forms; `bab8ccf0d52e0fc622552caacc77f521fa030eaf`
records the wrapper origin. The dependency repair is independently preserved
in `a12aea8208436c5ae22990a3a7d3f50b749a807a`; its `units.c` blob is
`2ca3eb1b5713323827118f6aa9aec92f6fa0c8e7`, 118,688 raw bytes, SHA-256
`1cf95cb963e46f5ed419c7f52f4fc21527e21e7b958f8b919dfc367bda03204d`.
No historical wholesale commit was cherry-picked because those carriers also
contain out-of-scope headers, bodies, and current-policy debt.

## ABI, callers, and dependency closure

The authenticated C signatures are:

```c
boolean unit_add_equipment_to_inventory(long unit_index, long equipment_index, short replace);
boolean unit_add_grenade_to_inventory(long unit_index, long equipment_index);
boolean unit_approve_weapon_swap(long unit_index, long weapon_index);
boolean unit_drop_current_weapon(long unit_index, boolean immediate);
short unit_inventory_next_weapon(long unit_index, short current_index, short delta);
void unit_shield_sapping_update(long unit_index);
static short unit_weapon_next_index(long unit_index, short current_index, short delta);
```

The six public identifiers are ordinary C `__cdecl` functions and naturally
emit leading-underscore COFF names. `unit_weapon_next_index` is source-static.
The csplit target exposes that split function section with storage class 2,
but a scan of every January split object finds no cross-object undefined
consumer. Its complete January xref set is same-TU only:
`unit_drop_current_weapon` twice, `unit_handle_deleted_object`,
`unit_add_weapon_to_inventory`, `_code_001a2030`, `unit_enter_seat`, and the
public wrapper. The retained candidate emits only the dependency-closed exact
drop/wrapper callers; all currently emitted helper call sites compare exact.
No register-carried private ABI was declared or exported.

January cross-object REL32 xrefs authenticate the public boundary:

| Callee | January callers and relocation offsets |
|---|---|
| `unit_add_equipment_to_inventory` | `actors.obj:_actor_customize_unit+0x1FC`; `players.obj:_code_000ab440+0x6C` |
| `unit_add_grenade_to_inventory` | `players.obj:_code_000acb50+0xFC` |
| `unit_approve_weapon_swap` | `players.obj:_code_000acb50+0x2C0` |
| `unit_drop_current_weapon` | `game_engine_ctf.obj:_code_0009f8c0+0x3C`; `players.obj:_code_000aa240+0x6C`; `players.obj:_players_update_before_game+0x44A`; same-TU `unit_start_flaming_to_death+0x16`, `unit_died+0x23C`, and `unit_update+0x5EE/+0x6CA` |
| `unit_inventory_next_weapon` | `player_control.obj:_code_000a79c0+0x23F`; `hud.obj:_code_000bf870+0x58D` |
| `unit_shield_sapping_update` | no January split-object xref; the public body and signature remain independently authenticated |

January and the exact donor use `short replace` for equipment. Protected
`game/players.c` still has an old guessed local `boolean replace` declaration.
That declaration was deliberately not edited. On this x86 ABI both arguments
occupy a promoted 32-bit stack slot and the January callee consumes the low
16 bits; therefore the machine contract is exact, while the formal C
declaration drift is recorded rather than hidden. The owner-header blast
radius and protected caller make a header/caller cleanup a separate future
wave.

The pickups use typed `item_datum`, `equipment_definition`, unit, grenade,
player, and tag-block layouts supplied by the two added owning includes. The
two powerup values are TU-local named enum constants, so no shared-header or
runtime-storage change is needed. Shield sapping uses typed animation,
iterator, player, unit, and damage structures. It deliberately retains the
January-proven original comparison of `animation.state.index` with the loop
frame rather than silently substituting `frame_index`.

## Bounded one-shot method

Before emission, configuration and the exact candidate were checked with
parse-only `/Zs`. Parse-only work produced no object. The source boundary was
then frozen at the seven bodies above.

Exactly one natural code-producing candidate command was run:

```text
ninja -f build.ninja build\base\source\units\units.obj
```

It invoked VC7 once with the repository flags `/nologo /c /O2 /Oy- /DDEBUG
/Dxbox` and the normal include graph. There was no spelling retry, scheduling
experiment, body reorder, compiler-flag change, or post-emission source edit.
All seven independently met the strict gate, so nothing was pruned. The later
full Halo/libcmt gate confirmed `units.obj` was already up to date and did not
compile it again.

The retained diff is readable typed C only. It adds two owning includes, two
TU-local enum values, six public definitions, the historically authenticated
private helper repair, and the natural `short next_index` correction in
`unit_drop_current_weapon`. It adds no assembly, volatile scheduling device,
barrier, `noinline`, pragma, raw offset, serialized overlay, union lifetime
forcing, alias pun, undefined behavior, synthetic anchor, byte forcing, or
writable object definition. `git diff --cached --check` passes.

## Strict seven-body results

The hardened comparator requires padded bytes plus normalized relocation
address, type, destination, and addend equality. Meaningful size comes from
the January report; padded size and hashes come directly from COFF.

| Function | January RVA | Meaningful / padded bytes | Relocs | Normalized SHA-256 | Result |
|---|---:|---:|---:|---|---|
| `_unit_add_equipment_to_inventory` | `0x0019A400` | 276 / 288 | 20 | `c73e63ef81c7a432a6a2b74fe6ce92cda9fe78062659f92200261d14cdc5afaf` | strict exact |
| `_unit_add_grenade_to_inventory` | `0x0019A270` | 244 / 256 | 15 | `cb1ebdded4808ea3ce2251a305a1ea6ca92f1de7f6475db3ff58abe9bd0c3a30` | strict exact |
| `_unit_approve_weapon_swap` | `0x0019DCB0` | 207 / 208 | 7 | `5463df3ab10b158fe6e787644204ba7082839b52c9e7d43782390f4d7d2a5ffc` | strict exact |
| `_unit_drop_current_weapon` | `0x0019DEF0` | 250 / 256 | 12 | `b4e3f97ff390fbb079711667523fabd1dff595264127aeb42e5e6173bc843ed3` | strict exact |
| `_unit_inventory_next_weapon` | `0x001A1430` | 26 / 32 | 1 | `63dc655e15a81b4448586c34e03ca199484db4395623ed2159f68f1e41169579` | strict exact |
| `_unit_shield_sapping_update` | `0x00199800` | 337 / 352 | 12 | `bb340e4fba80d328b0c9eae89850bbc186bbb21fffa4446cd6d2f73b4dd81b97` | strict exact |
| `_unit_weapon_next_index` | `0x0019DD80` | 360 / 368 | 12 | `dbd1f447689b07eb386e4e616952f1de91fbf22abcd05cab0b361a588211b492` | strict exact |

The direct seven-name comparator reports `all_equal: true`. In particular,
the helper calls retain symbolic `_unit_weapon_next_index` relocations, and
shield sapping retains the exact `__real@41800000` relocation at `+0xD0`.

## Complete 189-function regression census

The per-symbol hardened scan filters the three csplit compiler-local labels
`$L7450`, `$L7451`, and `$L8061`; they are not independent source functions.
Across all 189 underscore-named January functions the retained object is:

- 104 strict exact;
- 8 present nonexact;
- 77 absent.

The 104 exact functions are exactly the 97 clean-baseline accepted functions
plus the seven rows above. Every one of the inherited 97 was independently
recompared by normalized bytes and relocations; there are zero inherited
regressions.

The eight present nonexact functions remain:

```text
_unit_animation_set_state
_unit_cause_continuous_melee_damage
_unit_drop_item
_unit_ready_desired_weapon
_unit_running_blind
_unit_set_or_test_seat_and_weapon_label
_unit_throw_grenade_release
_unit_update
```

The 77 absent target functions remain:

```text
_code_00197e30
_code_00197f90
_code_00198050
_code_00198070
_code_001980d0
_code_00198130
_code_00198170
_code_00198190
_code_001981f0
_code_00198230
_code_00198400
_code_00198e40
_code_00198fd0
_code_0019a170
_code_0019a640
_code_0019b0b0
_code_0019b160
_code_0019b410
_code_0019b4c0
_code_0019b600
_code_0019bf70
_code_0019c460
_code_0019dff0
_code_0019ea70
_code_001a0cf0
_code_001a1f50
_code_001a2030
_unit_add_weapon_to_inventory
_unit_adjust_projectile_ray
_unit_animation_start_action
_unit_can_enter_seat
_unit_cause_melee_damage
_unit_cause_player_melee_damage
_unit_control
_unit_damage_aftermath
_unit_debug_ninja_rope
_unit_detach_from_parent
_unit_died
_unit_enter_seat
_unit_estimate_position
_unit_euler_aiming_update
_unit_exit_seat_end
_unit_export_function_values
_unit_find_nearby_seat
_unit_flame_to_death
_unit_get_melee_range_and_ticks
_unit_get_seat_entrance_point
_unit_handle_deleted_object
_unit_handle_region_destroyed
_unit_handle_weapon_state_change
_unit_impact_melee_damage
_unit_leap_begin
_unit_melee_attack_begin
_unit_new
_unit_place
_unit_postprocess_node_matrices
_unit_preprocess_node_orientations
_unit_record_damage
_unit_render_debug
_unit_scripting_enter_vehicle
_unit_scripting_vehicle_test_seat_list
_unit_set_actively_controlled
_unit_start_animation_impulse
_unit_start_flaming_to_death
_unit_start_running_blindly
_unit_start_user_animation
_unit_test_animation_impulse
_unit_test_spawning
_unit_throw_grenade_begin
_unit_try_and_exit_seat
_unit_unsuspecting
_unit_update_animation
_units_debug_get_closest_unit
_units_debug_get_next_unit
_vehicle_scripting_find_available_seats
_vehicle_scripting_load_magic
_vehicle_scripting_unload
```

No residual receives matching credit from this wave.

## Runtime data, literals, and ownership

Every one of the 129 January target non-code sections was compared by its
natural owner symbol with hardened bytes and relocations:

- clean baseline: 60 accepted target sections and 69 absent/nonexact gaps;
- retained candidate: 63 strict-exact target sections, zero present-nonexact,
  and 66 absent;
- all 60 inherited accepted sections remain strict exact: 3,781 logical bytes
  and seven relocations;
- the three newly exact assertion strings add 216 target-owned read-only bytes
  and zero relocations;
- current exact target-data total: 3,997 logical bytes, seven relocations.

The three newly exact target owners are:

| Owner | Bytes | Relocs | SHA-256 |
|---|---:|---:|---|
| `??_C@_0EG@DMMDGKIO@equipment_definition?9?$DOequipment?4@` | 70 | 0 | `b7865a154141ae1574a32e982847bee85eeb9a42250244ac37b687744dc2bf14` |
| `??_C@_0EJ@KGELGNBO@equipment_definition?9?$DOequipment?4@` | 73 | 0 | `d9ec564e5fe8cc282b5398b2a4f180645633758bac0302421c18411c3d0fd123` |
| `??_C@_0EJ@NJDBEHGI@equipment_definition?9?$DOequipment?4@` | 73 | 0 | `d6d2dd5384a10c5524afd6e75ff6613f30dc924e4b9d013f5fd267c7e09c837d` |

Thus the baseline 69-target-section gap changed only by those three exact
owners. The final 66 absent owners are the other target-only identities
recorded in the pre-edit manifest; none is emitted nonexactly.

The candidate runtime non-code owner set changed from the clean baseline by
exactly four select-any `.rdata` definitions: the three target-owned strings
above and `__real@41800000`. The latter is a reviewed csplit attribution, not
new linked-image data and receives no `units.obj` target-data credit. January
`units.obj` imports it as section 0/value 0/storage 2, while January
`actor_firing_position.obj` owns the canonical external COMDAT. Cross-object
hardened comparison proves the candidate copy is exact at four bytes, zero
relocations, SHA-256
`140efb356462f70dd1c7f1dfb10bcc07d0f14d439043fb9e9d50f4d7be71ea96`;
both owners use COMDAT selection 2 (select-any). The linker therefore
coalesces an already-authenticated value.

No baseline runtime owner was removed. The only common fingerprint change is
`.debug$S`, caused by the isolated absolute source path and line records; it is
not runtime data. Existing `_unit_globals` `.bss` and
`_magic_base_animation_seat_index` `.data` remain strict exact. There is no
new writable `.data`, `.bss`, or COMMON owner, and no partial storage credit.

## Pre-edit manifest limitation and hardened disposition

The clean pre-edit regression manifest is preserved at the hash above. Its
identity-level check correctly discovers the seven newly exact functions, but
inserting formerly absent COMDATs at their natural January source positions
renumbers later COFF section and symbol indices. The current manifest schema
freezes those object-local numbers, so it reports empty `still_exact` and
spurious changes for the 97 inherited accepted functions even though their
bytes and normalized relocations are unchanged. It similarly presents the
eight residuals as `changed_nonexact`. Reordering source to placate numeric
indices was explicitly forbidden and no retry was made.

This is a tooling limitation, not an allowlist: the complete 189-function
per-symbol hardened comparison above strips only object-local numeric section
identifiers while preserving bytes, relocation addresses/types/destinations/
addends, linkage names, and ownership. The separate 129-section runtime census
and before/after owner-set proof prevent that normalization from hiding a data
or storage change. A committed-state snapshot will be used for the forced
rebuild replay, where section numbering is stable across identical source.

## Initial gates

- `/Zs` parse-only check: pass, before the one production emission.
- Sole candidate production compile: pass, no warning from `units.c`.
- Direct seven-function comparator: `all_equal: true`.
- Full normalized 189-function scan: 104 exact / 8 nonexact / 77 absent;
  seven gains and zero regression among the inherited 97.
- Full target-data scan: 63 exact / 0 nonexact / 66 absent; all 60 inherited
  accepted sections preserved.
- Candidate runtime owner A/B: only the three target-exact strings plus the
  reviewed exact select-any float attribution; no writable/COMMON delta.
- Full `halobetacache_build libcmt_build`: pass. The 568-edge catch-up build
  did not rebuild frozen `units.obj`; warnings shown were inherited in
  unrelated translation units.
- Exact local objdiff report generation, semantic audit, and progress: pass;
  470 units, 4,169 functions evaluated, 4,092 accepted exact, zero unit
  errors. Campaign progress is 22.06% matched / 14.05% linked overall.
- Object admission audit: zero candidates, zero revocations; one inherited
  `shell_xbox` contradiction remains reported.
- Parked-function audit: three active, zero stale, zero invalid.
- Tool regression suite: 179 tests passed.
- Source scope, protected-path, banned-construct, storage, caller, and Git
  payload audits: pass under the explicit `units.c` owner authorization.

The Ninja report phony target attempted to refresh two already-installed tool
binaries because the copied `.ninja_log` carried a different downloader
command hash; sandboxed network was unavailable. No source or object was
changed. The exact installed `objdiff-cli` report generator and repository
semantic/progress commands were then run directly and passed as recorded
above. This operational detail grants no exception or matching credit.

## Commit/replay boundary

This section records only pre-commit and initial candidate evidence. It does
not claim that a committed replay has already happened. After the source and
this initial ledger are committed, a clean implementation-state snapshot,
verified generated-object deletion, normal Ninja rebuild, regression check,
direct seven-name comparison, full 189/129 census, and final owner audit will
be appended in a separate ledger-only commit. No amend, push, or history
rewrite is authorized.

## Actual committed-state forced replay

The implementation commit is
`07b7a5ca17c974a497993dabd4ca9caaa13d08ab`. Its committed source payload is
unchanged from the retained evidence above:
`686c23e90655f970b9bcedb9aa3447123d006c72`, 124,248 raw bytes, SHA-256
`ca8d3e1843354436bcede267243dcc84b9d752c9aa39d09b1c122ea3bf2311bb`.
The initial committed ledger payload is
`bc6601e571b25d520ce1eb4bb7c47e79cb570eda`, 19,619 raw bytes, SHA-256
`46ebc2479afbd04ec08a20c49c67c675ea6263278f22f6e967ab1ef89161976c`.

The worktree was clean at that implementation commit. Ninja first confirmed
that `build\base\source\units\units.obj` had no work pending. Because the
copied Ninja log still wanted to refresh the unrelated downloader edges, the
repository gate's built-in `--no-build` capture was used only after that
explicit object up-to-date proof and after the exact local report had passed.
The resulting clean implementation-state snapshot is:

```text
build/regression_units_inventory_public_seven_replay_20260824.json
commit: 07b7a5ca17c974a497993dabd4ca9caaa13d08ab
size: 4,546,705 bytes
SHA-256: 1d7aff878b457c80c6e42c5637acfe2b076a1080d63a748a3b075f32a0b2d729
```

The generated object resolved to
`C:\Users\isabe\Documents\Codex\2026-07-13\i-w\units-inventory-public-six-20260824\build\base\source\units\units.obj`,
which was verified to remain beneath the isolated lane root. Before deletion
it was 75,827 bytes with whole-file SHA-256
`881b98385530cc6f62fa6234346483d6c615bffeee303658ae0b721ae6ec70bf`.
That exact file was deleted and absence was verified. A normal one-target
Ninja rebuild then ran exactly one `CL ... units.c` edge and succeeded.

The rebuilt object is again 75,827 bytes. Its whole-file SHA-256 is
`6b395323ef1904c22472b3cd16058134b6229b3b7152bd06ee9eb93223e20ccf`;
the whole-file difference is confined to non-runtime COFF build metadata.
The committed-state regression check is authoritative for its captured
fingerprints and reports:

```text
ok: true
failures: 0
warnings: 0
still_exact: 104
newly_exact: 0
changed_nonexact: 0
```

That check covers every captured function, runtime non-code section,
relocation, and symbol-owner record. A post-check Ninja dry run reports
`ninja: no work to do`.

Independent direct replay proofs also passed:

- seven-name hardened comparison: `all_equal: true` for all seven retained
  code COMDATs, including every relocation identity;
- complete underscore-function census: 104 exact / 8 present nonexact /
  77 absent, exactly matching the initial retained classification;
- complete January target-data census: 63 exact / 0 present nonexact /
  66 absent, preserving all 60 inherited accepted sections and the three
  newly exact strings;
- cross-object `__real@41800000` comparison: `all_equal: true`, four bytes,
  zero relocations, unchanged select-any ownership evidence;
- no writable `.data`, `.bss`, COMMON, function-owner, runtime-literal, or
  undefined-external ownership drift from the implementation-state snapshot;
- committed source blob unchanged; only this ledger is modified for the
  additive replay record.

This is the actual replay, not a prewritten claim. No source change, amend,
push, history rewrite, body tuning, or protected neighboring-path edit was
made after the implementation commit.
