# `units.obj` Jonas seat-query and render-debug wave (2026-08-24)

## Result and fixed boundary

This Jonas-owned wave starts from exact clean cumulative commit
`1d52d3e2969096617d749549912bb078293d4ef7` on isolated branch
`jonas/units-seat-query-wave-20260824`. Before any code-producing emission,
the independently retainable boundary was frozen at exactly these five public
typed bodies:

| Fixed body | January RVA | Meaningful / padded bytes | Relocations | First-shot disposition | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- | --- |
| `_unit_get_seat_entrance_point` | `0x00197AE0` | 466 / 480 | 18 | strict; retained | `bda25ddd55a2ea62f49f33a72e6b2b1a40ee29a0ad357f5d661f29adbae1d36d` |
| `_unit_get_melee_range_and_ticks` | `0x00197CC0` | 224 / 224 | 8 | strict; retained | `d47a6180324dbf4a44b19c2e5f0bedf64f55c4a9140a63ec05695054caab2597` |
| `_unit_can_enter_seat` | `0x001985C0` | 190 / 192 | 5 | strict; retained | `82fa85daaaf8a71837e1e3d43fff1c07b1ca8c9e5d96a5da06037b7d1a737349` |
| `_unit_render_debug` | `0x0019C950` | 512 / 512 | 34 | strict; retained | `ac078c80a049a70ba539201e84249db20689f7b14cd5bb3f2393e1810dba326e` |
| `_vehicle_scripting_find_available_seats` | `0x0019D8B0` | 416 / 416 | 21 | strict; retained | `f65315c450c0cd1abf88080d7a309140ca6fe5fcae7a0cbff10e0347854c9d5b` |

The one natural five-body candidate compile admitted all five without a
source retry, tuning pass, or removal. The wave therefore adds 1,808
meaningful code bytes, 1,824 padded bytes, and 86 exact relocations. The only
dependency inside the fixed set is the real January call from
`unit_render_debug` to `unit_get_seat_entrance_point`; the set is closed.

`units.obj` remains `NonMatching`. Every other missing or residual body is
outside this wave. No header, protected source, configuration, semantic
exception, parked record, completion label, storage owner, or pre-existing
Claude/Jonas ledger is edited.

## Immutable target, canonical source, and compiler

- January `cachebeta.exe`: 6,584,672 bytes, SHA-256
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January csplit `source/units/units.obj`: 138,090 bytes, SHA-256
  `e6a24ca597147dacff1f06806613d6bcfa4fa48f359dfbf542c34935f674b19f`.
- Exact cumulative starting `source/units/units.c` blob
  `e4bd8cd0690ad222763900ed156571123318f9bb`: 128,549 raw bytes, raw
  SHA-256
  `fe7651e454b5ef9821006fb5dd6f28d3dff20fe0077d604bd2f553a7dc595abe`.
- Exact inherited canonical `units.obj`: 77,309 bytes, SHA-256
  `b543ef7908a60d64914c8e6466572ea094032d1db65b5d616f42bd80c209db73`.
  It was copied before emission to
  `build/audit/units_seat_query_wave_baseline.obj` and remained unchanged
  through the parse-only check.
- Retained staged `source/units/units.c` Git payload before the ledger commit:
  blob `713bfda1024761cac01ad889e60d675f3b2ad826`, 139,363 raw Git
  payload bytes, raw SHA-256
  `d81a5282e50cf52261ed5d81a1b5b19221fb0da3204c1169a8c8a40629f33002`.
- Immutable first-shot object
  `build/audit/units_seat_query_wave_firstshot.obj`: 82,056 bytes,
  SHA-256
  `431452a35ec3ebd05d31be53ab37c43ad28ff5d4aff1be9d32b7f38055e8e50b`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86. `xbox/bin/vc7/CL.Exe` is 81,920 bytes with SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
  The repository flags remain `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` with
  the XDK 3911 include graph.

The inherited object contained 673 symbols, 265 sections, 138 code sections,
127 total non-code sections, and 109 runtime non-code sections after excluding
debug and directive records. None of the five fixed symbols was defined.

## Claude corpus and tooling evidence

The complete historical Units-object documentation inventory was reviewed as
reconstruction and exclusion evidence. The large evolving `units_obj.md` and
the animation, combat, leaf, state, compiler, provenance, caller, residual,
and do-not-repeat ledgers constrain what this bounded wave may import. The
documents dedicated to the five fixed functions are the direct source-shape
authority:

| Claude evidence | Source/ledger blob | Bytes | Raw SHA-256 |
| --- | --- | ---: | --- |
| Complete readable carrier at `deb5554ab8d16da80da934e2aa7081bc7d1f594f` | `source/units/units.c` `124f29e32497c091c4163b5faafc58dbf8890161` | 305,068 | `173fd6fdcc09d979ba6507aefa2cd77790f8f17f93413b9acbb679c32bb8f7c4` |
| Same carrier's consolidated Units ledger | `fd66a5252ce1d085d712909c8e65c4f7994cd629` | 37,465 | `8f24f77d8c59bc69977374aa22ef98034bb6427a11e93cc46024659e6d718490` |
| Seat geometry/melee source at `2ca548d0952f` | `092d1bd13a35c4472433ab6148f45e03e36c9d1e` | 275,352 | `a6541a6adfff4efe02a031b3fb0366ce02dcfaea93ffbd640881c608355ec47d` |
| Seat geometry/melee ledger | `3f2ecf103713ca9fdf67e3b77e5b754ae68f2683` | 6,217 | `08235245dcc8fcf232138a9041dec1da75cd0f8a55a71b8d5490efe84937f342` |
| Seat scripting source at `1aed36ceb7` | `a47ecc0884627e27d6182c92eb18b8a4dffe008b` | 146,191 | `7e4c3121fbd197c726aae1045273795691d8ed43a7f9c37f72332d7961e1b5b5` |
| Seat scripting ledger | `38121f8b5e00b1f1e6c55663d38b2b6ea18bac45` | 6,712 | `12cfb9895a4619990d7f38c2f767e4c89620f2b3e6b44cf4d585f2a674afe512` |
| Render-debug source at `e2f5b1c5ce` | `254d9d6fe341e489261da098b954076af8ac585d` | 277,764 | `8b4e66efe38f215cd1aee6cf6be1f473e9774617492ae3c302069386c83b9020` |
| Render-debug ledger | `dfae8bb37a11eafd14ddf036cde5576b29ce2d06` | 9,065 | `4c6351316316259c8af551afb728aa3180314c7798335632f1355b4b0de6532c` |
| Available-seat source at `18df118571` | `bb0f514211ac01c06ac7d608bd45562bece81e05` | 227,694 | `ed455b4d6b189edc906065bb09bab70d68039921091f622fe3ea68fce2070bc8` |
| Available-seat ledger | `1c08d29413de2829761c0318f6fefb0d00305c27` | 4,184 | `c81dfa546f43839e3b5b6ba5a499082427670257f48a7cf182d03949fa8157b0` |

The five complete function texts manually transplanted into the current
typed translation unit compare byte-for-byte with the authenticated
`124f29e3` carrier. Their UTF-8 body-text SHA-256 values are respectively
`040aba6864bf91dfe2b27e286e3a751e448801600ccc15d89d787db5ba01cff2`,
`a147de03d6e33ebf889fdd235f9ff7b04dab8668c2657643280ba5a5dd6fa925`,
`FD5D309F94C3E5A838D75937D73F7539B5AFCD5DFA2FB9CBBECE42D3FCB2796E`,
`ff1d722a2415c5f26e02c07dc4569b2f453bc1a992f3aefde60e6ee7f3e8c61f`,
and
`257f2e0932e9f7b61b88e30d9cd73b0e577c573ec31ec5bfb1ca93aa15874bf2`.

The governing current tooling documentation was also read rather than
reconstructed from memory: `docs/matching_methodology.md` blob
`a827ff53e7c8055a33f0b2fd74913d8a1cd8b75b`, the exact-match acceleration
playbook blob `5d60e78fe7628824476cd2186f4243ff998e1743`, and the object-ledger README
blob `6f29514ea3856a4cb4e3aa11abc623d45ca80d73`. The hardened comparator,
regression gate, semantic audit/progress, admission audit, and parked validator
were the exact HEAD blobs `c6c2239e`, `dcf188a9`, `3ec25657`, `d070f843`,
`dc738499`, and `b07ec7a2`. No tool, configuration, exception, or acceptance
rule was modified.

The historical commits contain unrelated bodies, headers, storage, and old
policy debt. No historical commit was cherry-picked and no file was
transplanted wholesale; only the five readable typed C bodies and the minimal
source-local declaration support were applied.

## Clean HCEA and current typed owners

The clean Xbox semantic reference is exact HCEA commit
`c168af2e747d3095d9a29418ae401f3a39544863`. January PC COFF remains the
only byte, ABI, and relocation authority. HCEA supplies independent typed
behavior evidence for four bodies:

| HCEA donor | Blob | Bytes | Raw SHA-256 |
| --- | --- | ---: | --- |
| `src/unit_get_seat_entrance_point.c` | `d0cdd077bc8a6ea6c33f45da2445d53b519a5094` | 4,983 | `4e704e3bf9a7beca9a1eb64fe7b19efacc4f407a8f71a3e4ed1c8254d555e3c4` |
| `src/unit_get_melee_range_and_ticks.c` | `de35e501c0d3772f079cefac88ae93d9997b4b98` | 2,827 | `2161aeb214d3dfc9660aa33cf62124f48d1686db6b4ab5366783af0401a3367f` |
| `src/unit_can_enter_seat.c` | `f064164a956f715a0fce3cb47f19ada28a9db404` | 2,226 | `27c89d73009170700adadcbbe8c63001aedad8d5ca49cd9f986b4239c7ac0d1c` |
| `src/vehicle_scripting_find_available_seats.c` | `7cccc55ec938d66b9cb48a273ce58aa8a60bf8ba` | 4,148 | `66028af8fa1ec704688cc294249a38fcf357115a169f1e0c8af92fd5aacf1aee` |

HCEA has no `unit_render_debug` donor. For that function the January call
graph and Claude's dedicated strict experiment ledger are authoritative; the
October build is only a semantic map and was not imported.

The current canonical types and declarations were authenticated before
emission. Direct owners include `units.h` blob `54492a4e`,
`unit_definitions.h` `40f984e5`, `model_animation_definitions.h` `bddc1e3c`,
`model_definitions.h` `40ec3ad0`, `objects.h` `738c831f`, `players.h`
`3773541e`, `render.h` `acc33838`, `render_debug.h` `12768a0d`, and
`cseries.h` `3c91ac46`. Current `object_types.c` blob `3473be06` owns the
render-debug table declaration. Current `game_allegiance.h` blob `98862ca8`
does not declare `game_team_is_enemy`; current `game_engine.c` blob
`47515f3e` independently proves the exact typed declaration
`boolean game_team_is_enemy(short, short)`. The same declaration is therefore
source-local in `units.c`, with no shared-header edit.

## Typed behavior and do-not-repeat constraints

The seat-entrance body obtains the unit model and animation graph, searches
animation seats by case-insensitive label, deliberately performs the second
animation-seat lookup, selects the seat-enter animation, multiplies the seat
marker by frame-zero root motion, and resolves `<marker> enter-hint`. Its
output order is exit, entry, then hint. The outer positive-count guard and
`do/while` topology are preserved; an early return, `for` rewrite, shared
`goto`, removed lookup, or reordered output changes the January schedule.

The melee helper selects the active animation seat and weapon class, chooses
melee attack or airborne melee, initializes its Boolean result only after the
animation lookup, returns `private_key_frame_index`, and obtains timing from
`animation_get_x_offsets`. It does not use the second private key frame and
does not hoist result initialization.

The seat-entry predicate performs the explicit self test before loading the
target's first child. It walks generic child objects, uses a typed unit view
only after the unit mask check, records a requested-seat occupant, and tests
enemy allegiance through the already-live generic child object's owner team.
Hoisting the first-child load or using the typed getter's pointer for that
allegiance operand changes allocation and is forbidden by the Claude ledger.

The render-debug routine preserves three independent debug gates. It reuses
the camera and origin point locals in the seat branch, intentionally ignores
the `head` marker count (the documented original bug), captures
`mouth_aperture` before copying the marker position, copies that position into
the reusable origin local, and formats `%.2f`. A safety fix for the missing
marker, direct use of the marker subobject, separate seat point locals, or
reordered mouth operations is intentionally not applied.

The available-seat filter validates the substring and five-value desire mode
at the original assert lines, assigns its result count only after substring
setup, lowercases each label into a 256-byte buffer, applies driver/gunner
flag predicates, rejects occupied seats, and stops at output capacity. Its
source-local enum values are exactly not-driver `0`, gunner `1`, passenger
`2`, driver `3`, any `4`, count `5`. No shared enum storage or header change
is needed.

## ABI, callers, and dependency closure

All five definitions are ordinary public C `__cdecl` functions. The candidate
COFF records each at value zero, type `0x20`, external storage class `2`.
`_game_team_is_enemy` remains an undefined external function of the same type
and storage class. There is no private register convention, residual helper,
or hidden storage dependency in the fixed set.

The complete January caller census across every split object proves:

- `_unit_get_seat_entrance_point`: REL32 from
  `action_vehicle.obj:_code_000094c0+0x8A`, same-TU
  `_unit_render_debug+0x130`, and same-TU `_unit_find_nearby_seat+0xAF`.
- `_unit_get_melee_range_and_ticks`: one REL32 from
  `action_charge.obj:_action_charge_setup+0x1FA`.
- `_unit_can_enter_seat`: REL32 from
  `players.obj:_code_000ab440+0xBD`, same-TU
  `_unit_find_nearby_seat+0x191`, and same-TU `_unit_enter_seat+0x1D`.
- `_unit_render_debug`: one DIR32 table reference from
  `object_types.obj:.data+0xF8`.
- `_vehicle_scripting_find_available_seats`: REL32 from protected
  `actions.obj:_actor_action_try_to_enter_vehicle+0x3F`, protected
  `ai_script.obj:_code_00047160+0x43`, and same-TU
  `_vehicle_scripting_load_magic+0x43`. Its own switch table has the exact
  internal DIR32 relocations at `+0x124`, `+0x18C`, `+0x190`, `+0x194`, and
  `+0x198`.

Only the real render-debug-to-seat-entrance edge lies inside this wave. The
other same-TU callers may remain absent or residual independently because the
five selected definitions require no body from them. Protected callers and
local declarations were inspected but not edited.

## Bounded implementation and one-shot discipline

The tracked implementation diff is confined to `source/units/units.c`. It
adds the existing typed model-definition, render, and render-debug includes;
the source-local seat-desire enum; the source-local typed allegiance
declaration; and the five authenticated bodies. It adds no writable object,
header declaration, raw layout, or caller change.

Before emission, `git diff --check` passed and each body was text-equal to the
authenticated Claude carrier. Exactly one `/Zs` parse-only CL invocation
passed and left the inherited object SHA-256 unchanged at `b543ef79...db73`.
The candidate source was then frozen.

Exactly one natural code-producing candidate command was run:

```text
ninja -f build.ninja build\base\source\units\units.obj
```

It produced exactly one `[1/1] CL ... units.obj` edge. The first-shot object
was copied immutably before comparison. The hardened five-name comparator
reported `all_equal: true`; therefore all bodies were retained as-is. There
was no alternate spelling, declaration experiment, statement reorder,
compiler-flag change, retry, or post-emission source tuning. The later full
graph build found `units.obj` already up to date and did not compile it again.

The retained C contains no assembly, intrinsic or calling-convention
attribute, optimizer pragma, `volatile` scheduling device, raw address or
byte-offset access, pointer/union pun, inactive-union read, undefined
overflow, synthetic anchor, serialized overlay, object-byte forcing, or
writable storage definition.

## Complete 189-function census

The hardened target function scan filters csplit compiler-local labels
`$L7450`, `$L7451`, and `$L8061`; they are not independent source functions.
Across all 189 underscore-named January function owners:

- inherited canonical state: 107 strict exact, 8 present nonexact, 74 absent;
- retained candidate: 112 strict exact, 8 present nonexact, 69 absent;
- inherited exact functions preserved: 107/107;
- fixed-set gain: exactly the five rows in the result table.

The 112 strict functions total 13,884 meaningful bytes, 14,800 padded bytes,
and 612 exact relocations. The eight unchanged present nonexact functions are:

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

No residual receives credit from this wave. The 69 absent functions are the
pre-wave absent set minus only the five admitted names.

## Complete 129-section data and ownership census

Every one of the 127 January `.rdata` sections plus the complete `.data` and
`.bss` sections was paired through its natural owner and compared by
normalized bytes and relocations:

- inherited canonical target data: 63 exact, zero present nonexact, 66 absent;
  3,997 logical bytes and seven relocations;
- retained candidate target data: 66 exact, zero present nonexact, 63 absent;
  4,145 logical bytes and seven relocations;
- all 63 inherited exact target sections remain exact;
- the three gained January-owned strings total 148 bytes and zero
  relocations.

| Newly exact January owner | Bytes | Relocs | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `??_C@_0M@GBEBNJOA@?5enter?9hint?$AA@` | 12 | 0 | `d76884ab59bcb7ba380433b6b6cb0592a3793c6e59fdb2159dc5cb4e1dbf6d24` |
| `??_C@_0BE@BPHFKLIJ@seat_substring_name?$AA@` | 20 | 0 | `ac99dc53ac8960135711e6310e092b71282e6b07fd3b36c2eed16fd8f110ff6` |
| `??_C@_0HE@KJEMFFL@...seat_desire_type...@` | 116 | 0 | `f763cb9ebf26f6ec2821e1e49aa649efcbb7539f5ef056561d671fb0c38472db` |

The runtime non-code owner A/B comparison is separately fail-closed. The
baseline has 109 runtime owners; the candidate has 113. All 109 common owners
have identical acceptance fingerprints, none was removed, and exactly four
read-only COMDATs were added: the three target-owned strings above plus the
five-byte `%.2f` owner
`??_C@_04GEFJINEE@?$CF?42f?$AA@` with SHA-256
`bba564d8153a92a0333d7a3f994f428d7ef7c7f3983630d888c8a9f94247e9c3`.
January references that formatting string as an undefined external, so it is
accepted for function relocation equivalence but receives no target-data
credit. The already-present `head` and float COMDAT owners do not change.

The complete target `.data` remains strict at 1,564 bytes, seven relocations,
and normalized SHA-256
`6ed61c39a648905cf23b12b916e2adadfe97b6e5aa492c29a14c7265809ef1a7`.
Target `_unit_globals` remains strict at four BSS bytes, zero relocations, and
normalized SHA-256
`df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119`.
There is no new writable `.data`, `.bss`, COMMON, aggregate, or storage owner.

## Pre-wave manifest and structural renumbering

A clean one-unit pre-wave regression snapshot was captured from exact commit
`1d52d3e2` with `--no-build` before editing:

```text
build/regression_units_seat_query_pre_20260824.json
size: 4,576,915 bytes
SHA-256: 9bfa9634109e396eb52d430351f9f4e49cffd38a00a8e07bb83cd69c177b032d
```

The manifest correctly reports the five fixed functions as `NEWLY_EXACT` and
fails closed on the expected new function/debug sections, four runtime
read-only owners, symbol-set growth, and downstream object-local section and
symbol renumbering. Its schema freezes those numeric identifiers, so natural
source-position insertion also reports spurious changes for many inherited
accepted functions and non-code sections. Source was not reordered to placate
numeric indices, no adjudication was supplied, and no regression rule was
weakened.

The independent 189-function comparator removes only object-local numeric
section identifiers while preserving padded bytes and every relocation
address, type, destination, and addend. It proves all 107 inherited accepted
functions. The separate 129-target-section and 109-owner runtime A/B proofs
prevent that normalization from hiding any data or ownership regression. A
fresh committed-state snapshot below will have stable numbering across an
identical-source delete/rebuild replay.

## Initial full gates

- Full `halobetacache_build libcmt_build`: pass, all 568 catch-up edges
  completed. The frozen Units object was already up to date and was not
  rebuilt. Warnings were inherited in scenery, `network_game_manager`, TIFF,
  and protected `ai_debug.c`; `units.c` emitted no warning.
- Local csplit regenerated the complete authenticated January split tree.
  Direct `objdiff-cli report generate` then passed. `build/report.json` is
  1,571,734 bytes with SHA-256
  `8654a1bb9161e660b248059b60daa385e5f3bf180a917df1821d8359de08a1b9`.
- Semantic audit: 470 units, 4,177 functions evaluated, 3,552 missing-base
  symbols, 4,039 semantic exact, 104 hidden exact / 62,432 hidden code bytes,
  36 ordinary-only, 35 structurally accepted, one rejected, 12 local symbols
  skipped, 4,100 accepted exact, and zero unit errors.
- Campaign progress: 375/833 objects, 4,084/11,060 functions,
  486,589/2,198,102 code bytes, and 1,835,088/4,176,062 data bytes.
- Halo progress: 273/468 objects, 3,917/7,574 functions,
  473,675/1,770,166 code bytes, and 1,829,896/3,923,451 data bytes. Libcmt
  remains 102/212 objects, 167/476 functions, 12,914/55,015 code bytes, and
  5,192/8,637 data bytes.
- Ordinary `units.obj` report: 108/192 exact functions and
  13,240/53,478 meaningful code bytes; 4/6,752 ordinary data bytes and
  42.545597% fuzzy score. Ordinary objdiff shows
  `_vehicle_scripting_find_available_seats` at 94.405594%, but the hardened
  comparator proves its padded bytes and all 21 relocations exact; no
  exception or fuzzy credit is used.
- Object admission: zero candidates and zero revocations; only the inherited
  unrelated `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active entries, zero stale, zero invalid.
- Complete tooling suite: 179/179 tests pass.
- `git diff --check`, changed-path, deleted-path, protected-source,
  source-policy, storage, linkage, caller, and Ninja dry-run checks pass.

Only `source/units/units.c` and this newly created Jonas ledger are tracked
changes. No committed-state replay is claimed in this initial record. After
Jonas commits this source and ledger, a clean implementation-state snapshot,
verified same-root object deletion, normal same-path rebuild, immediate
regression check, direct five-name comparison, complete 189/129 census,
runtime-owner audit, and no-work dry run will be appended in one additive
ledger-only commit. No amend, push, history rewrite, or worktree removal is
authorized.
