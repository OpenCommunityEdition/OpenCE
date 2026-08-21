# `devices.obj` Jonas group-get/new pair (2026-08-21)

## Result and fixed boundary

This Jonas-owned wave starts from exact clean canonical commit
`c228a83e924b8699dd043ac642c90c5cc27cf56d` on isolated branch
`jonas/devices-group-touch-new-20260821`. The dispatched boundary contained
exactly three ordinary-C bodies. Two were independently strict in the first
and only natural three-body code-producing `devices.obj` candidate compile
and are retained:

| Retained body | January RVA | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_device_group_get_value` | `0x00084D20` | `28 / 32` | 2 | `14d2c7728226358605c598e4f2a5a78223c4ed901c5c313792e88985254fd65c` |
| `_device_new` | `0x00084730` | `69 / 80` | 2 | `a82077d5a860161cb67250de07a5a1a827b0c5a07883a4464672a38b8216a901` |

`devices.obj` advances from 7/30 to 9/30 exact functions and from
314/4,463 to 411/4,463 meaningful code bytes. It remains `NonMatching`.
The strict gain is 97 meaningful bytes, 112 padded bytes, and four exact
relocations. No data credit is claimed.

The third fixed body, `_device_touched` at January RVA `0x00084D40`, was a
bounded first-shot miss and is absent from retained source. Its target is
65 meaningful / 80 padded bytes with three relocations and normalized SHA-256
`723eb9fb6ccbf02ce4dbf9d77f10331cb9c43b005e9f2832e2a75598228cda21`.
The immutable natural candidate also emitted 80 padded bytes and three
relocations, but normalized to
`ea9171793c57f04e76b5bb14461778ff0b7e19d55c6ea8b4938cfb89d06f2a10`.
January places the `_control_touched` call relocation at `+0x27`; the natural
typed candidate placed it at `+0x21`. The complete body and its sole
`device_controls.h` support include were removed wholesale. There was no
tuning, declaration reshaping, alternate expression, or second candidate
compile.

## Immutable target, source, and compiler provenance

- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January csplit `source/devices/devices.obj`: 13,148 bytes, SHA-256
  `c7cb2a7ec7efa3a2999a68c25d0a52e83ae1d3aa409ece76bf4611469c7c0b26`.
- Exact-base `source/devices/devices.c` Git blob
  `1a8cd2fe9c7f021e359220fea3fdfd4d39267500`, 4,442 raw payload bytes,
  payload SHA-256
  `cf09bc580e6152e802e8c3f9dea737fdc327762bbb5d6fead502a0c0e5c17a5f`.
- Retained `source/devices/devices.c` staged Git blob
  `b20ab75aa491468b606583165c727cabd8f4c2a6`, 5,278 raw payload bytes,
  payload SHA-256
  `e1fe6a3c8484d8ae441a26327d9dfdcdb65adcf87c17aad21263a6e84321943a`.
- Exact-base `source/hs/hs.c` Git blob
  `cd69d9c9911b6ae144042388627df5cfaa40e45e`, 200,483 raw payload bytes,
  payload SHA-256
  `2a7241541eb4a2b7f6195122be41865ab3f327965be28c4001a2178890468403`.
- Retained `source/hs/hs.c` staged Git blob
  `ed38056c21a03ca9c8402f330ba24d551132bd9c`, 200,474 raw payload bytes,
  payload SHA-256
  `6ee1be950d6d7b0b16d3846cf286cef052b88f0abd7d132ce9027aba3bfa5a66`.
  Its only source change is the authenticated declaration parameter correction
  from `unsigned short` to `short`.
- The shared `source/devices/devices.h` remains unchanged at blob
  `638221fd4dfd7ff7196b528798845ce0d194d997`, 2,135 raw payload bytes,
  payload SHA-256
  `6389caad1eefa0bb1ee1d3972b2b32bb6bde1a14150525db823be84a9382be5f`.
- The final pre-commit `devices.obj` is 2,709 bytes with phase-specific
  SHA-256
  `b7c27295feda16d2b50a609c6b24aedc56717ef8a92b51d49c00c3bc0346fa0b`.
  The immutable first-shot three-body object was 2,998 bytes with SHA-256
  `25c63bb49c7f08d139304c4f1c1aff47b5268d3bc0f405619f4a89cf7b07a74d`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86. `xbox/bin/vc7/CL.Exe` is 81,920 bytes with SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
  The repository's natural `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` flags and
  XDK 3911 include graph are unchanged.

The clean HCEA reference is exact commit
`c168af2e747d3095d9a29418ae401f3a39544863`. Each identity below is directly
tree-reachable at that commit, and every SHA-256 is over the raw
`git cat-file blob` payload. HCEA supplies readable intent and layout
corroboration; January PC COFF alone governs acceptance.

| Clean c168 HCEA evidence | Git blob | Bytes | Raw payload SHA-256 |
| --- | --- | ---: | --- |
| `src/device_group_get_value.c` | `d498334807632dfb2f84d27a57acaca2ff55f419` | 373 | `666965f8803146f65014c38f81161a50b62abf1c29f3bed80566438e20c00537` |
| `src/device_touched.c` | `c926452dcc4ceb93cde0e6d6c6b6a688e65811cf` | 899 | `797db778a827f72d0c657d62f1305ba7df197bd25a540c9acef8182ebbc53002` |
| `src/device_new.c` | `43518ab9c5ddab42afd860cf7a8936b060972c59` | 794 | `2bb5576da8609f5274e0fc4d97af766f88bd6c0e3b8413d5eb3b428009cc49b1` |
| `src/headers/device_datum.h` | `815a0b9b009e85e280c4c6b72a6adf4c1310e8d0` | 985 | `1a25f7013f0266b6a30896d7970cf75dd642e5fed3a0569577593ee420f5e318` |
| `src/headers/device_group_datum.h` | `608ebc2ffd7058316debb9c182c60fa2ad5c00df` | 336 | `d793ca6c310bef765ef3c12631e4b9283d0ebc65d1044829c2cd6cd37a1b75ad` |

The current PC type owners were pinned before emission:

| Current typed dependency | Git blob | Bytes | Raw payload SHA-256 |
| --- | --- | ---: | --- |
| `source/objects/objects.h` | `738c831f77a73e2dc3a1e7e9443144857c480ce8` | 16,905 | `94a6abe8acd6c1bc0d807d15a11457d5cea7c6a2cb84f6a9cd9fa71c564a32b0` |
| `source/objects/object_types.h` | `6b29a8466d240167157259a33d8e9b67cb6e7ef8` | 5,324 | `ef1fcf7deb10a6663be50faab5a713caa56751b9dc8201eb58e339e161505462` |
| `source/devices/device_definitions.h` | `24f0aa172ef84e0fa2b728e48d39894853f09cb6` | 2,563 | `bfa178754f07f8c56f0d819ee3d3ef4ec7e097aad121a9a88a029bd6fa4d11f9` |
| `source/memory/data.h` | `4c1f445a0358c3962c4dc96341c1873af1ebd135` | 1,925 | `cb7a93de367ee0debcbf184889ad7662c99db4a72d4bd77a44c8db9bc95f59c7` |
| `source/devices/device_controls.h` | `340d169e3011b90dcec5d5051f64cf50cfcb01d4` | 2,391 | `8c7684b9feacf722603a26d4a98564098f384daff0fa8be440c37146bb39ddd9` |
| `source/devices/device_machines.h` | `caebbc2840050d6c504eaac18efd46e104556855` | 1,229 | `9d33b98bdd7f9125739e68f128be8965e3ea060c16c1814f74a78aa4ca83b4ee` |

All-revision history finds no earlier committed definition of any fixed body.
Earlier devices waves touched disjoint lifecycle, typed-leaf, old-map, and
position-setter scopes. Repository-wide branch/worktree collision checks found
no competing implementation of this trio. The January target defines each
screened external symbol once. `_device_group_change_only_once_more_set` and
every other devices body were explicitly excluded before emission.

## Typed ABI, callers, and PC layout

The fixed public contracts are:

```c
real device_group_get_value(short group_index);
void device_touched(long device_index, long unit_index);
boolean device_new(long device_index);
```

On the 32-bit target ABI, `short` is signed 16-bit, `long` and `real` are
32-bit, and `boolean` returns in AL. Each body consumes ordinary stack formals
and returns with plain cdecl `ret`; no hidden or register-carried parameter is
present. These C identifiers naturally decorate to the exact leading-
underscore COFF names.

The complete target use census is one `REL32` caller of the getter from
`hs.obj:_code_000aef20` at caller `+0x24`, one `REL32` caller of touched from
`players.obj:_code_000ab440` at `+0x47`, and one `DIR32` address-taken
reference to new from the `object_types.obj` function table at `.data+0x704`.
Reconstructed source has the corresponding HS evaluator, players call, and
object-type callback. `players.c` already declares touched as
`void(long,long)`, and `object_types.c` already declares new as
`boolean(long)`. No shared-header edit is required.

The target getter begins by sign-extending its 16-bit stack formal (`MOVSX`),
proving the parameter is `short`, not the donor-like unsigned form formerly
declared in `hs.c`. Its source-local typed group prefix is exactly eight bytes:
`short identifier` at `+0`, `word flags` at `+2`, and `real actual_value` at
`+4`; both total size and `actual_value` offset are compile-time asserted.
It passes the signed index to the declared `datum_get` contract and returns
the typed `real` field. Its exact relocations are `DIR32 +0x09` to
`_device_groups_data` and `REL32 +0x10` to `_datum_get`.

The retained constructor uses the current complete PC `device_datum` and
named fields. Its checked `device_get` call uses `_object_mask_device`; the
typed composite places `definition_index` at `+0`, object flags at `+4`,
power-group index at `+0x1A8`, and position-group index at `+0x1B4`.
January's order is preserved: call `device_definition_get` for the required
`tag_get` side effect, clear position group then power group to `NONE`, OR
named `_object_shadowless_bit` (bit 18) into the object flags, and return
`TRUE`. The exact relocation schedule is `REL32 +0x0E` to
`_object_get_and_verify_type` and `REL32 +0x1D` to `_tag_get`.

The rejected touched body was likewise public ordinary cdecl. January obtains
the checked device at `+0x0E`, dispatches target object type 8 to
`_control_touched` at `+0x27`, and target type 7 to `_machine_bumped` at
`+0x37`. The typed natural C preserved that semantic boundary but did not
reproduce January's instruction schedule, so no part of it was retained.

## Signed HS declaration correction and controlled A/B

Before candidate emission, the sole reconstructed declaration mismatch was
preselected for ABI correctness: the source-local prototype in `hs.c` changed
only from `unsigned short device_group_index` to
`short device_group_index`. The evaluator call expression, runtime body, and
all storage remain untouched. This was not a body-support repair or candidate
retry.

The pristine canonical `hs.obj` was 123,259 bytes with SHA-256
`a9b3359853a5f5dc54202b55f580c24ac8dfdbb4668a1d5d839fc72758c58126`.
The corrected-declaration `hs.obj` is 123,257 bytes with phase-specific
SHA-256
`386b16365f04e185966b7eff120bca1aa2135d51ff9619edcb569baa78d445d3`.
A controlled complete runtime fingerprint A/B evaluated all 448 functions and
all 38 runtime non-code sections: no function or runtime section vanished,
appeared, or changed. Both sides have identical runtime fingerprint SHA-256
`265ee09d45a354caf662925a79dab7433b323ef0df5838c5e27bb40bbff8971d`.
The only object delta is `.debug$S|anonymous=0`, reflecting corrected prototype
type metadata. Thus every non-owner consumer is runtime-identical while the
source contract now agrees with the target callee's signed load.

## Preserved exact inventory and ownership boundary

Direct hardened January comparison proves the complete final nine-body
inventory:

| Exact function | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_devices_dispose` | `1 / 16` | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_devices_dispose_from_old_map` | `13 / 16` | 2 | `9dedd614496101c84c04e338b0fb2167843b37380b3f0e2a50007ce9c2bc7edc` |
| `_device_set_never_appears_locked` | `64 / 64` | 1 | `e180e3b5196b04ad2c72967cdc93bdc207a23cc33b12d406864567ff38d55e36` |
| `_device_one_sided_set` | `59 / 64` | 1 | `37b297855afc1da1f38223444572043ab773a86d1831a3c828a24175f687bfb6` |
| `_device_set_actual_position` | `53 / 64` | 2 | `d0e53196194b9cab6dd3e748e0b9c4d81902b02b465dfd019ed632d338c9d836` |
| `_device_set_desired_position` | `57 / 64` | 2 | `46aed102030b8a7a6ca51498138ebba7ce8952c3f05d2b9ea73affce0ade5b4f` |
| `_device_set_power` | `67 / 80` | 2 | `77bb40070eb2a2a67b2d36bc1bae62ffce1cf94b7a3babc9cd3eacd37972b7be` |
| `_device_group_get_value` | `28 / 32` | 2 | `14d2c7728226358605c598e4f2a5a78223c4ed901c5c313792e88985254fd65c` |
| `_device_new` | `69 / 80` | 2 | `a82077d5a860161cb67250de07a5a1a827b0c5a07883a4464672a38b8216a901` |
| **Final strict total** | **`411 / 480`** | **14** | |

The final object defines exactly these nine external code COMDATs. It defines
no runtime `.rdata`, writable `.data`, `.bss`, or COMMON owner. The group data,
object/tag accessors, and inherited helpers remain undefined value-zero
dependencies. All 208 target non-code bytes remain unclaimed; compiler and
target literals receive zero standalone data credit.

The retained implementation is readable typed defined C. It contains no
assembly, raw address or byte-offset dereference, pointer/integer
reconstruction, cast overlay, pointer or union pun, inactive-union access,
undefined signed overflow, data-dependent representation trick,
private/register ABI, volatile scheduling device, optimizer pragma or barrier,
synthetic anchor, or byte-forcing expression.

## One-shot discipline and pre-commit validation

The local typed record, includes, signed declaration correction, and all three
fixed bodies were frozen before emission. Exactly one VC7 `/Zs` parse-only
check passed without creating an object. Exactly one natural three-body
`devices.obj` candidate compile followed. Getter and new were strict in that
immutable object. Touched missed and was removed wholesale. The subsequent
production build only reproved the unchanged retained pair.

Only `source/devices/devices.c`, the declaration-only `source/hs/hs.c`
correction, and this new Jonas-owned ledger are tracked changes. No shared
header, configuration, storage owner, semantic exception, parked record,
completion label, pre-existing Markdown, or tracked deletion is changed. The
five protected sources remain untouched: `source/units/vehicles.c`,
`source/ai/ai_debug.c`, `source/ai/ai_script.c`, `source/ai/actions.c`, and
`source/units/units.c`.

- Complete `halobetacache_build` plus `libcmt_build`: pass. The controlled
  `hs.obj` build was followed by all 568 remaining production graph edges;
  only inherited repository warnings were emitted.
- Direct hardened final comparison: all nine emitted devices functions are
  strict, including padded bytes and every relocation identity, order, type,
  address, and addend.
- Semantic audit: 470 units, 4,158 functions evaluated, 3,571 missing-base
  symbols, 4,018 semantic exact, 101 hidden exact / 61,876 hidden code bytes,
  36 ordinary-only, 35 structurally accepted, one rejected, 12 locals skipped,
  4,079 accepted exact, and zero unit errors.
- Campaign progress: 375/833 objects, 4,066/11,060 exact functions,
  482,927/2,198,102 meaningful code bytes, and 1,835,088/4,176,062 matched
  data bytes.
- Halo progress: 273/468 objects, 3,899/7,574 functions,
  470,013/1,770,166 code bytes, and 1,829,896/3,923,451 data bytes. Libcmt
  remains 102/212 objects, 167/476 functions, 12,914/55,015 code bytes, and
  5,192/8,637 data bytes.
- `devices.obj`: 9/30 exact functions and 411/4,463 meaningful code bytes;
  it remains nonmatching and claims no data.
- Object admission: zero candidates and zero revocations; only the inherited
  unrelated `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active entries, zero stale, zero invalid.
- Complete tooling suite: 179/179 tests pass. `git diff --check`, changed-path,
  deleted-path, protected-five, source-policy, and storage checks are clean.

The clean committed-state replay is recorded additively after the
implementation-and-ledger commit. No push, amend, or history rewrite is
performed.

## Clean committed-state replay

Pending implementation-and-ledger commit and two-unit forced replay.
