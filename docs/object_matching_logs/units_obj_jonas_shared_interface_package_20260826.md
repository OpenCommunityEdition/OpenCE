# Units shared-interface package (Jonas, 2026-08-26)

## Result and authorized boundary

The operator authorized the pending Units shared-header package with the
required phrase, conditioned on byte-matching standards; the fail-closed
one-shot protocol below is that condition. This wave starts from clean
cumulative commit `278d55f9e57f8ccac6d5ced80c22fa29db688f79` on isolated
branch `jonas/units-shared-interface-20260826` and applies the three
interface corrections frozen in the read-only packet
(`i-w\outputs\UNITS_SHARED_INTERFACE_PACKAGE_FROZEN_20260826.md`):

1. remove the stale public `unit_ready_desired_weapon` declaration and keep
   the helper translation-unit-private;
2. change the animation-impulse formal from `short` to `long` consistently
   and use the natural four-byte typed tester scratch; and
3. change the game-engine damaged-player damage-type formal from `long` to
   `boolean` across the callback member, wrapper declaration, and wrapper
   definition.

Exactly one production emission of the 33-TU affected closure adjudicated the
package. **All six promised owners are strict exact on that sole shot:**

| January owner | Padded | Relocs | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_unit_ready_desired_weapon` | 432 | 28 | `ef9948642c40b1c7ed12419d3594176c8a95ce0a965641607b4656020d5d13c4` |
| `_code_001a2030` | 64 | 3 | `6090b208fa2eeac097ab40901051c49f89951bee54a622363d14c51bfd8765af` |
| `_unit_exit_seat_end` | 656 | 23 | `229c70eae582daa2590313251b16ed2e020a38c832643a153aee22dd5de25a24` |
| `_unit_start_animation_impulse` | 288 | 11 | `db9ae277a81fe8df84632bfeb74678459480eff07df3a181eb0c65a80b5f1416` |
| `_unit_test_animation_impulse` | 192 | 7 | `186cc732892efe8197f5539ad09d64ad7fdb3b264ab6ef8a40c41bcf4e41d1dd` |
| `_unit_damage_aftermath` | 1600 | 60 | `8bb721e70465d8c4c3d09069f879176ddc08a639d1e67c75fe525f9d367870dd` |

The direct hardened Units census moves from 168/12/9 to **174 exact / 11
emitted nonexact / 4 absent** code owners and from 126/0/3 to **128 exact /
0 emitted nonexact / 1 absent** runtime-data owners. The two data gains are
the aftermath console-format string
`??_C@_0CE@MIPDNKIJ@p?$CFd?3?5body?5?$CF?42f?5shld?5?$CF?42f?5from?5?$CFs@`
and `__real@4001475cd2000000`; the sole remaining absent data owner is
`__real@3d0f5c29`, owned by still-absent `_unit_cause_player_melee_damage`.
`units.obj` remains `NonMatching`; no completion label is claimed.

`_code_001a0cf0` is emitted as real nonexact support required by
`_unit_damage_aftermath` and receives no credit: 1,568 padded bytes, 43
relocations (all identities and addends equal to January, including
`_base_seat_labels+8` into the strict `.data` aggregate), candidate
normalized SHA-256
`ad4d59f2ea92489e...` versus January
`365511af9b5f31d519ee339975e9f667004f04caa4d8c868266e41c17278fc63`.

## Immutable authority, worktree, and compiler

- January `cachebeta.exe`: 6,584,672 bytes, SHA-256
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `source/units/units.obj`: 138,090 bytes, SHA-256
  `e6a24ca597147dacff1f06806613d6bcfa4fa48f359dfbf542c34935f674b19f`
  (regenerated in this worktree by csplit and re-hashed).
- Baseline commit `278d55f9`; baseline blobs
  `units.c c2ef1a3b08bec91cc2c534443f09732b6f3003cd`,
  `units.h 54492a4ee4cb8b37739f9ebbfa89c72fbd0e08fc`,
  `game_engine.c 47515f3ecf28ab5a8aeabdd196b1846b75b66501`,
  `game_engine.h e8d54eec1c666e866884542a07350b28b42b04b4`.
- Retained staged blobs:
  `units.c a62d26fd2aba7e8018f5786c1f449bc4937b612e`,
  `units.h 0184ad19e7f7b9966ecb1eb2756b889c4ad0b0bd`,
  `game_engine.c f63f8a84182b64045fc3926afeed8f37d700b249`,
  `game_engine.h 8d0526700a54f70e5f9331db86c6260651eb9da8`.
- Compiler: XDK 3911 VC7 `CL.Exe` 13.00.9254.1, 81,920 bytes, SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`;
  flags `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` and the unchanged include
  graph. The generated `build.ninja` CL command line was respelled to this
  worktree's absolute compiler path because the relative forward-slash form
  fails CreateProcess in this invocation context; same binary, same flags,
  no configuration change.
- Pre-edit worktree-local `units.obj` (full clean baseline build):
  SHA-256 `6ded5e7da2c0cb07926340e4045dcea8af017805f33849e38736f44d38956a22`;
  post-emission 150,373 bytes, raw SHA-256
  `65258dabe3b2043f66b7018aa24bb9f051ac08e7347a04cb8cf16d395380d432`.
  Pre-edit `game_engine.obj` SHA-256
  `4d0b09446b43c182852b03837ccd3a520fb834e9aa62c8d271dcfb4b31b59004`;
  post-emission 97,824 bytes, raw SHA-256
  `4806536057caa6bc959080bfd3735bbc6a799e98b7bb01c843d4512eb5909557`.
  Raw whole-object hashes are timestamp/debug-phase specific; admission is
  padded-section plus complete relocation identity, never a whole-object
  hash.

The full baseline was built natively in this worktree before editing (569
base objects + 833 split objects) and reproduced the frozen packet baseline
exactly: Units 168/12/9 and 126/0/3, the 33-object closure table
identical row-for-row (867 inherited exact code owners), and the split hash
above.

## Source provenance and applied form

The readable carrier is Claude blob
`124f29e32497c091c4163b5faafc58dbf8890161` at `deb5554a`. The four new
bodies (`unit_test_animation_impulse`, `code_001a0cf0`,
`unit_exit_seat_end`, `unit_damage_aftermath`) were transplanted verbatim
from carrier lines 3872-3920, 1563-1998, 5469-5588, and 7902-8242, with
exactly two documented policy transforms:

- the tester's four-byte codegen union (prohibited) became the natural
  `short interpolation_frame_count;` under the coherent `long` formal —
  January's own frame (one dword local at `[ebp-4]`, full-dword impulse
  load `mov edi,[ebp+0xc]`) is the level-1 evidence; the union defended
  against short-formal argument-padding recycling that cannot occur with the
  long formal. This previously-unmeasured spelling produced the exact
  January tester on the sole emission; and
- `code_001a0cf0`'s call through the carrier's raw `+0xb6` debug macro
  became the accepted named-field spelling
  `TEST_FLAG(unit->object.functions_active_flags, _unit_debug_function_active_bit)`
  already emitted by the update-animation closure.

`code_001a2030` was already present in the baseline source as the dead
invented-name static `unit_adjust_for_seat_change` (community commit
`bc6663de`), unemitted because its only January caller was absent. Per house
rules §4 the wave removed that name (prototype and definition) and applied
the carrier body under the split's placeholder name at the carrier-relative
position before `unit_enter_seat`.

TU-local support added with the bodies, all carrier-authenticated and
storage-free: declarations for `ai_handle_death`, `unit_make_damage_sound`
(defined in January `unit_dialogue.obj`; kept TU-local to hold the public
interface delta at the three scoped corrections), and `biped_exit_seat_end`;
the damage-animation/-direction/-part enums; the aftermath-flag enum
(`lethal=0`, `player_damage_type=4`, `died_instantly=6`); the two
direction-cone `#define`s; the aftermath animation-flags mask; and TU-local
stand-ins for `_damage_data_suppress_unit_reaction_bit = 4` and
`_damage_definition_pings_resistant_units_bit = 2` /
`_does_not_ping_units_bit = 4`, whose owning shared headers
(`objects/damage.h`, `objects/damage_effect_definitions.h`) are outside this
authorization. `units.h` lost the stale ready declaration and its impulse
declarations became `long` in one-parameter-per-line form; `game_engine.h`
and `game_engine.c` changed only the three damage-type formals.

The retained diff (4 files: +1,041/-18 lines) contains no assembly,
`volatile`, barrier, pragma, intrinsic, attribute, `register`, raw offset,
pointer or union pun, inactive-union access, undefined behavior, synthetic
anchor, byte patch, or compiler-option change. `git diff --check` passes.

## One-shot discipline

The complete candidate was applied by a count-asserted script (16 edits; any
count mismatch aborts). `/Zs` parse-only passed for both edited TUs on the
first attempt with no declaration residue. The affected-TU closure was
recomputed from the generated graph at the patched state: exactly the
frozen 33 TUs (28 reach `units/units.h`, 10 reach `game/game_engine.h`).
Because the Ninja graph tracks no header dependencies, the 33 affected
objects were deleted with per-path containment proof (each resolved absolute
path begins with this worktree root), absence was verified, and a dry run
then listed **exactly 33 CL edges and nothing else**. One ordinary `ninja`
invocation compiled all 33. There was no retry, tuning, or post-emission
source change. The only compile diagnostic was the known inherited
`ai_debug.c(293)` C4047 warning in an untouched protected file; `units.c`
and `game_engine.c` emitted no warning.

## Whole-closure preservation

- Direct hardened comparison of all 33 affected objects against their
  January split targets reproduces the frozen baseline table exactly, with
  `source/units/units` the only changed row (174/11/4 code, 128/0/1 data).
  Total inherited exact code owners across the closure: 867, all preserved;
  campaign-wide exact totals rose only by the six Units gains.
- Named sentinels: `_game_engine_player_damaged_player` remains strict exact
  at 80 bytes / 5 relocations / `75c31dac76987b65...` after the boolean
  change; `_unit_add_weapon_to_inventory` remains strict exact
  (304/11/`de883e04...`); fully exact `objects.obj` remains 121/121 +
  99/99; protected `ai_debug.obj` remains 11/3/46 + 268/1/69; the strict
  `.data` aggregate (1,564 B / 7 relocs / `6ed61c39...`) and `_unit_globals`
  BSS (4 B / `df3f6198...`) are unchanged.
- The 33-unit pre-edit regression manifest
  (`build/audit/units_shared_interface_pre_20260826.json`, 38,522,813
  bytes, SHA-256
  `cbfd5d0fd6845ca802e34b3bdb65362844a2a29d791b6a11b1979bfb74339e86`,
  pinned to `278d55f9`) fails closed as designed on the intended package:
  its six warnings are exactly the six `NEWLY_EXACT` owners, and its 238
  strict findings were adjudicated one class at a time at the runtime
  evidence level (padded size, normalized hash, relocation
  address/type/addend/destination):
  - **every runtime-evidence delta in the entire closure lives in
    `source/units/units`**, and the changed set is exactly the intended one:
    five appeared owners (`_unit_test_animation_impulse`,
    `_unit_exit_seat_end`, `_unit_damage_aftermath`, `_code_001a2030`,
    `_code_001a0cf0`), two owners that became exact
    (`_unit_ready_desired_weapon`, `_unit_start_animation_impulse`), and two
    support owners whose call sites re-formed around the now-private helper
    (`_unit_enter_seat`: 592/34, now `a0bfb14f...`; `_unit_update`:
    5,984/262, now `6117f737...`; both were and remain nonexact).
    `_code_00198fd0` kept its previous bytes (`197a8fec...`) despite the
    new tester caller; `_unit_died`, `_unit_update_animation`,
    `_unit_animation_set_state`, `_unit_euler_aiming_update`,
    `_unit_preprocess_node_orientations`, and
    `_unit_throw_grenade_release` are byte-identical to the baseline;
  - all remaining manifest findings are `$L` compiler-local label
    renumbering (the header edits shift VC7's internal label counters in
    every including TU; every added/removed symbol in every affected object
    is a `$L` label, zero real symbols changed), defined-versus-undefined
    callee metadata for the newly defined owners, and physical
    section/symbol renumbering from natural source-position insertion in
    `units.c` — the same fail-closed structural classes documented by every
    prior wave. The manifest's four `objects.obj` "accepted" identities
    that vanished are `$L` labels, not functions; `objects.obj`'s real
    census is unchanged at 121/121;
  - the candidate's new select-any literal COMDATs authenticate
    byte-exact against their January natural owners
    (`__real@3fe921fb60000000` → `ai_debug.obj`, 8 B;
    `__real@40000000` → `action_vehicle.obj`, 4 B) and receive no data
    credit. No writable `.data`, `.bss`, or COMMON owner changed anywhere.

## Full gates

- `halobetacache_build` and `libcmt_build`: no work remaining after the
  33-edge emission; report, semantic-report, and progress regeneration pass.
- Semantic audit: 470 units, 4,271 functions evaluated, 4,130 semantic
  exact, 114 hidden exact / 64,705 hidden code bytes, 4,191 accepted exact,
  zero unit errors. The authoritative baseline at the same commit measures
  4,125 semantic / 114 hidden / 4,186 accepted, so the delta is exactly the
  five external gains; `missing_base_symbol` falls by exactly the four newly
  emitted externals (3,462 → 3,458). Static `_code_001a2030` is strict
  exact by direct comparison but, like prior private helpers, receives no
  ordinary-report or semantic-ledger credit in this source-only wave.
- Campaign progress: 375/833 objects, 4,165/11,060 functions,
  505,876/2,198,102 code bytes, 1,835,212/4,176,062 data bytes;
  halobetacache 273/468 objects, 3,998/7,574 functions, 492,962 code bytes;
  libcmt unchanged at 102/212, 167/476, 12,914 bytes.
- Object admission audit: zero candidates, zero revocations; only the
  inherited unrelated `source/shell/shell_xbox` completion-label
  contradiction remains.
- Parked-function audit: three active, zero stale, zero invalid.
- Complete tooling suite: **179/179 tests pass** (`pytest tools/`).
- Source scope: exactly `source/units/units.c`, `source/units/units.h`,
  `source/game/game_engine.c`, `source/game/game_engine.h`, plus this new
  ledger. The other protected sentinels (`vehicles.c`, `ai_debug.c`,
  `ai_script.c`, `actions.c`) are untouched.

## Commit/replay boundary

This initial ledger records the frozen one-shot evidence. After the
implementation commit is clean, a fresh 33-unit accepted-state snapshot will
pin that commit; the 33 generated objects will be containment-proven,
deleted, rebuilt by one ordinary Ninja invocation, and re-checked
immediately (expecting zero failures against the committed snapshot); the
six-owner comparison, complete 189/129 censuses, and closure table will be
repeated; and a final dry run must report no work. The same sequence then
runs at corrected HEAD after the ledger-only replay commit. No push, amend,
rebase, history rewrite, or worktree removal is performed.

## Actual committed-state forced replay

(appended after the implementation commit)
