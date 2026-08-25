# `units.obj` bounded control/leaf four-body wave

Date: 2026-08-24

Owner: Jonas

Verdict: **retain four strict functions; no whole-object completion claim**

## Scope and frozen inputs

This isolated lane begins at clean first-wave commit
`1ab99d0f4052448e5a093f71c21d21c848df9145` on branch
`jonas/units-control-leaf-wave-20260824`. It deliberately does not use the
older canonical `8549f46c` baseline. The only production source path changed
is `source/units/units.c`; the only other committed path in this lane is this
new Jonas ledger. No shared header, configuration, protected neighboring
source, pre-existing ledger, or runtime-storage definition changed.

Before emission, the boundary was frozen to exactly four public bodies:

1. `unit_control`
2. `unit_debug_ninja_rope`
3. `unit_estimate_position`
4. `unit_handle_region_destroyed`

The initially considered dependency pair `unit_set_actively_controlled` and
private same-TU `_code_0019a170` was removed from this boundary before the
code-producing compile because an independent private-foundation lane had
already retained it. This lane neither duplicates nor tunes that pair. There
is no private-function closure inside the final four-body boundary.

Frozen payloads:

| Artifact | Git object / size | Raw payload SHA-256 |
|---|---:|---|
| Baseline `source/units/units.c` at `1ab99d0f` | `686c23e90655f970b9bcedb9aa3447123d006c72`, 124,248 bytes | `ca8d3e1843354436bcede267243dcc84b9d752c9aa39d09b1c122ea3bf2311bb` |
| Retained `source/units/units.c` | `aa54b954662bc1d474dcf1fbcd3b2ef528dfdac6`, 134,732 bytes | `cc53d548f978e158cf425fe778e6e18478e89f8171d4c895688543f32786fa61` |
| January csplit `source/units/units.obj` | 138,090 bytes | `e6a24ca597147dacff1f06806613d6bcfa4fa48f359dfbf542c34935f674b19f` |
| Clean first-wave candidate `units.obj` | 75,827 bytes | `6b395323ef1904c22472b3cd16058134b6229b3b7152bd06ee9eb93223e20ccf` |
| One-shot retained candidate `units.obj` | 82,512 bytes | `395d1dd3ca0b71ecfa93a58bde8dbd87a7f649dbb728177cdd7ef6aadc573bbb` |
| First-wave regression manifest | 4,546,705 bytes | `1d7aff878b457c80c6e42c5637acfe2b076a1080d63a748a3b075f32a0b2d729` |
| VC7 `CL.Exe` 13.00.9254.1 | 81,920 bytes | `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11` |

Git source sizes and hashes are raw blob bytes, not checkout line-ending
transformations. The January compiler flags are the generated repository
flags `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` with the normal include graph.

## Claude/HCEA evidence and authority boundary

The complete Claude `units.obj` ledger and its final readable carrier were
reviewed in full, including its comparator contract, object-specific
do-not-repeat findings, residual classifications, campaign harness warnings,
and tooling notes. The readable carrier is Claude `units.c` blob
`124f29e32497c091c4163b5faafc58dbf8890161`, and the historical ledger is
`docs/object_matching_logs/units_obj.md` in the final Claude worktree. They
are reconstruction evidence only: no historical wholesale commit was
cherry-picked and none of their completion claims grants credit here.

Independent HCEA semantic donors were also read from the clean research
cache. Their raw payload evidence is:

| HCEA file | Bytes | SHA-256 |
|---|---:|---|
| `src/blam/units/unit_control.c` | 3,306 | `8f9efe7cfe93a929828dcb26b8cb9af6b789138829420b09a41625604dad939d` |
| `src/blam/units/unit_debug_ninja_rope.c` | 1,914 | `d6b5dd231449475ef771a66ca8a8751c1fac1b4a7ac4105782376cbe52922d19` |
| `src/blam/units/unit_estimate_position.c` | 2,909 | `e63c6259fe821454c78105692782f9b7a4b29966966c6db6c107ee640b9af6fe` |
| `src/blam/units/unit_handle_region_destroyed.c` | 1,176 | `a557d43e70837d17abc64200c157910ae4772703b3710c0019596c59bcd503de` |

HCEA is later Xbox/server semantic provenance. In particular, its
`unit_control` has a later three-argument form and therefore cannot establish
the January PC ABI. The January PC COFF, exact January callers, current typed
layouts, and direct hardened comparison control every accepted signature,
layout, byte, relocation, and owner claim below.

## ABI, layouts, callers, and closure

The authenticated C signatures are:

```c
void unit_control(long unit_index, struct unit_control_data const *control_data);
void unit_debug_ninja_rope(long unit_index);
void unit_estimate_position(long unit_index, short estimate_mode,
	real_point3d const *body_position, real_vector3d *desired_facing,
	real_vector3d *desired_gun_offset, real_point3d *estimated_position);
void unit_handle_region_destroyed(long object_index, short region_index,
	unsigned long damage_flags);
```

All four are ordinary C `__cdecl` functions and naturally emit the January
leading-underscore names. The `unit_control_data` declaration is kept
translation-unit local because this wave had no authorization to edit the
shared header. It uses current typed `word`, `short`, `real`, and
`real_vector3d` members and has a compile-time `sizeof == 0x40` check. The
region-destroyed head flag is a named TU-local enum value. `vehicles.h` is an
owning declaration include required by the typed vehicle query in
`unit_estimate_position`; none of these support declarations owns runtime
storage.

Every January split-object relocation to the four names was scanned:

| Callee | January xrefs |
|---|---|
| `_unit_control` | `actors:_code_00029df0+0x6B`, `_code_00029f50+0xAC`, `_code_0002cdb0+0x2B0`; `actor_type_infection:_code_00027470+0xDAD`; `recorded_animations:_recorded_animations_update+0x102`; `players:_players_update_before_game+0x634` |
| `_unit_debug_ninja_rope` | `player_control:_code_000a79c0+0x19F` |
| `_unit_estimate_position` | `action_charge:_code_000023d0+0x10B`; `action_flee:_code_00003490+0xC3`; `action_search:_action_search_perform+0x125`; `actor_combat:_actor_combat_update+0xF43`; `actor_firing_position:_code_00013db0+0x71/+0x167`, `_actor_select_firing_position+0x243`; `ai_debug:_ai_debug_render_actor+0x2D91` |
| `_unit_handle_region_destroyed` | address-taken DIR32 entry at `object_types.obj` section offset `0xDC`; no direct REL32 caller |

The cross-object signatures, January argument consumption, current
`unit_datum`, object, collision, point/vector, vehicle, and control layouts,
and the emitted exact relocations agree. No same-TU private callee must be
recovered to admit these four, and no out-of-scope body or caller was edited.

## Bounded one-shot method and policy

The complete candidate passed a parse-only `/Zs` check before emission. The
initial parse exposed the missing private declaration of
`struct unit_control_data`; the typed TU-local layout and size assertion were
added, and `/Zs` then passed. Parse-only work produced no object. At that
point the exact four-body boundary plus typed TU-local support was frozen.

Exactly one natural code-producing candidate command was then run:

```text
ninja -f build.ninja build\base\source\units\units.obj
```

It invoked the unmodified VC7 compiler once through the normal full-Units
edge. There was no source retry, spelling sweep, function reordering,
compiler-flag change, per-function compile, or post-emission tuning. Each
strict section was adjudicated independently on that first result; all four
passed and were retained. The later full Halo/libcmt catch-up gate confirmed
the object was up to date and did not rebuild it.

The production diff adds four readable typed C bodies, one owning include,
one named local enum, and one checked private type declaration. It adds no
assembly, `volatile` code-generation device, `_ReadWriteBarrier`, pragma,
intrinsic, attribute, raw byte offset, serialized overlay, alias/lifetime
pun, undefined behavior, code-generation anchor, byte patch, writable
object, or storage ownership. The point/vector casts in
`unit_estimate_position` are between the engine's compatible point/vector
unions and reproduce the authenticated natural source form; they are not raw
storage overlays. Pre-existing policy debt in inherited bodies is unchanged
and receives no credit.

## Strict four-body result

The hardened comparator requires padded code bytes plus normalized relocation
address, type, destination, and addend equality. Meaningful sizes come from
the January report; padded sizes and hashes come directly from COFF.

| Function | January RVA | Meaningful / padded bytes | Relocs | Normalized SHA-256 | Result |
|---|---:|---:|---:|---|---|
| `_unit_control` | `0x0019F280` | 921 / 928 | 63 | `bb1247b53fc57bfe3a12b999966884276466a4db5eb681dcacdf8c82aeb058cd` | strict exact |
| `_unit_debug_ninja_rope` | `0x00199B20` | 273 / 288 | 23 | `e8ec36baac1a179b412621033bf2779a6abe9bb98335bc7b7829242c259d8f13` | strict exact |
| `_unit_estimate_position` | `0x00198CC0` | 308 / 320 | 14 | `b38dbe557603e938d9b723fdd10f6c73eacf9ea7b4a6f77a79dfe74f6a1c69d3` | strict exact |
| `_unit_handle_region_destroyed` | `0x0019B5C0` | 59 / 64 | 2 | `f8d973a381756aed997f7e18e08684c3a8a402e76eeb6198785b544067851db5` | strict exact |

The direct comparator reports `all_equal: true` for every name. The wave adds
1,561 meaningful bytes, 1,600 padded exact bytes, and 102 exact relocation
records.

## Full Units code/data census

A complete normalized scan of all 189 January underscore-function owners
compared the clean `1ab99d0f` object and the one-shot candidate:

| State | Baseline | Candidate |
|---|---:|---:|
| strict exact | 104 | 108 |
| present nonexact | 8 | 8 |
| absent | 77 | 73 |
| exact padded code bytes | 12,432 | 14,032 |
| exact relocations | 505 | 607 |

The gain set is exactly the four frozen names and the regression set is
empty. The eight nonexact residuals are unchanged:
`unit_animation_set_state`, `unit_cause_continuous_melee_damage`,
`unit_drop_item`, `unit_ready_desired_weapon`, `unit_running_blind`,
`unit_set_or_test_seat_and_weapon_label`, `unit_throw_grenade_release`, and
`unit_update`. This lane does not import Claude's residual adjudications or
completion count as credit; it records only the independently measured
current object.

The complete 129-owner January runtime-data census moves from 63 exact / 0
nonexact / 66 absent (3,997 bytes, 7 relocations) to 77 exact / 0 nonexact /
52 absent (4,863 bytes, 7 relocations). The 14 gains are precisely the target
assert/diagnostic strings emitted by the retained bodies, totaling 866 bytes.
There is no inherited data regression.

The candidate also gains four read-only select-any floating COMDATs. They are
not natural January `units.obj` data owners, so they receive no unit data
credit. Cross-object owner attribution independently proves each candidate
section is four bytes, zero relocations, selection 2, and byte-exact to the
January natural owner:

| COMDAT | January natural owner | SHA-256 |
|---|---|---|
| `__real@3e800000` | `source/ai/action_alert.obj` | `75e253f50979177eba47b2d0805ad36038789108924514d2a761a70de057d16f` |
| `__real@3f733333` | `source/ai/actor_moving.obj` | `1db1f0f344b58605d168fb4a2fc7df167d331adc0dc13979949425d10bb577bf` |
| `__real@40400000` | `source/ai/actions.obj` | `ea2845900b5856c9bf354b1aa9761b5aa6888e5ed61738fe9579ca42bc0f6054` |
| `__real@41c80000` | `source/ai/action_vehicle.obj` | `7b5162138ba6d8ae67efbf14d2266a9b11a531bf8e8b090e4bb2586f8f87a52e` |

The remaining 14 candidate-only runtime owners are the exact target strings
above. No baseline runtime owner was removed; there is no new writable
`.data`, `.bss`, COMMON, or partial-storage owner.

## Initial repository gates

- `/Zs` parse-only check: pass before the single production emission.
- Sole natural candidate production compile: pass; no warning from
  `units.c`.
- Direct four-name hardened comparator: `all_equal: true`.
- Complete 189-function census: 108 exact / 8 nonexact / 73 absent; exactly
  four gains and zero inherited regression.
- Complete 129-data-owner census: 77 exact / 0 nonexact / 52 absent; 14 exact
  string gains and zero inherited regression.
- Runtime owner A/B and select-any attribution: no writable/COMMON drift and
  all four float owners authenticated independently.
- Full `halobetacache_build libcmt_build`: pass. The 568-edge catch-up did not
  rebuild the frozen `units.obj`; warnings were inherited in unrelated TUs.
- Exact installed csplit: pass, 833 objects; January `units.obj` target hash
  remained exact.
- Exact installed objdiff report generation, semantic audit, and progress:
  pass; 470 units, 4,173 functions evaluated, 4,096 accepted exact, zero unit
  errors. Overall progress is 22.13% matched / 14.05% linked.
- Object admission: zero candidates and zero revocations; one inherited
  `shell_xbox` contradiction remains reported.
- Parked-function validation: three active, zero stale, zero invalid.
- Complete tool regression suite: 179/179 passed.
- Source-scope, policy, storage, caller, JSON, and diff checks: pass under the
  explicit `units.c` owner authorization.

The copied Ninja log attempted to refresh already-installed downloader edges
when the report phony target was tried; sandboxed network was unavailable.
No source or object changed. The authenticated installed `csplit` and
`objdiff-cli` binaries were then invoked directly, followed by the repository
semantic/progress tools, and all passed. This operational detail grants no
matching exception.

## Commit/replay boundary

This initial ledger records only the frozen one-shot candidate evidence and
does not claim a committed replay. The next steps are: commit exactly
`source/units/units.c` plus this ledger as Jonas; prove the generated object is
up to date; capture a clean implementation-state regression snapshot with the
gate's built-in `--no-build` mode; verify and delete only the isolated
generated `units.obj`; perform one normal Ninja rebuild; then rerun the
regression check, direct four-name comparison, full 189/129 census, and owner
audit. The actual replay evidence will be appended to this same new ledger in
a separate additive ledger-only Jonas commit. No amend, push, history rewrite,
or worktree removal is authorized.
