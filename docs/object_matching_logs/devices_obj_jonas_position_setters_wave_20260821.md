# `devices.obj` Jonas position-setter wave (2026-08-21)

## Result and fixed boundary

This Jonas-owned wave starts from exact canonical commit
`63c0252e3d088d52daaa59f45d2320555dac790f` on isolated branch
`jonas/devices-position-frontfacing-wave-20260821`. The dispatched boundary
contained exactly three public ordinary-C bodies. All three were present in
the first and only natural code-producing candidate compile. Both position
setters were independently strict immediately and are retained.
`_device_frontfacing` missed and was removed wholesale together with its
front-only include and named flag, without any expression, declaration,
control-flow, layout, compiler-control, or retry pass.

`devices.obj` advances from 5/30 to 7/30 exact functions and from 204/4,463
to 314/4,463 meaningful code bytes. It remains `NonMatching`. The strict net
gain is 110 meaningful bytes, 128 padded bytes, and four exact relocations.
No data credit is claimed.

| Fixed candidate | January RVA | Meaningful / padded | Relocations | Normalized SHA-256 | First-candidate decision |
| --- | ---: | ---: | ---: | --- | --- |
| `_device_set_actual_position` | `0x000856B0` | `53 / 64` | 2 | `d0e53196194b9cab6dd3e748e0b9c4d81902b02b465dfd019ed632d338c9d836` | **strict exact, retained** |
| `_device_frontfacing` | `0x00085810` | `119 / 128` | 4 | target `7f3f6dab74aed4c5c3a37eb7f916ba4fc0a44b3bf018e2e58f3bc340f5fed74d`; candidate `a995bbf1b3fc750e23878cfd54b9c61a14592cb833cf2e6c326c0a992f67fbfb` | rejected and removed |
| `_device_set_desired_position` | `0x00085890` | `57 / 64` | 2 | `46aed102030b8a7a6ca51498138ebba7ce8952c3f05d2b9ea73affce0ade5b4f` | **strict exact, retained** |

## Immutable target and source provenance

- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January csplit `source/devices/devices.obj`: SHA-256
  `c7cb2a7ec7efa3a2999a68c25d0a52e83ae1d3aa409ece76bf4611469c7c0b26`.
- Baseline `source/devices/devices.c` Git blob:
  `55ac691dd3a5372a7dce264183e43d8029ba1aa3`, 3,696 payload bytes, with
  Git-blob payload SHA-256
  `58deb2d3e929ef2e8430b18019d23b0c4b78bf737f74aff30c6bafa29677487f`.
- Retained translation-unit Git blob:
  `1a8cd2fe9c7f021e359220fea3fdfd4d39267500`, 4,442 payload bytes, with
  SHA-256 of the exact staged Git-blob payload
  `cf09bc580e6152e802e8c3f9dea737fdc327762bbb5d6fead502a0c0e5c17a5f`.
- The shared `source/devices/devices.h` remains baseline blob
  `638221fd4dfd7ff7196b528798845ce0d194d997`, 2,135 payload bytes, with
  payload SHA-256
  `6389caad1eefa0bb1ee1d3972b2b32bb6bde1a14150525db823be84a9382be5f`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86. `xbox/bin/vc7/CL.Exe` is 81,920 bytes with SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
  The natural flags remain `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` with the
  repository's unchanged XDK 3911 include graph.

The clean HCEA semantic reference is exact commit
`c168af2e747d3095d9a29418ae401f3a39544863`. Every identity below was read
directly from that commit's tree in the clean local reference worktree;
payload hashes are over raw `git cat-file blob` bytes.

| HCEA path | Git blob | Bytes | Payload SHA-256 |
| --- | --- | ---: | --- |
| `src/device_set_actual_position.c` | `7d6366cb6c6d6f7266b1e6c47830ac39b0dccac9` | 878 | `744f29d41b7cc8c0793cf3ca738b800bbee134bf2c839717a8bf558fa49f1019` |
| `src/device_set_desired_position.c` | `bad23b049bad6df5921e17a45e0502b1af2c4326` | 878 | `fc64f5e8728eb7e3abe5289a00dabb77d9670b872d10c77c40edef3ee86a859a` |
| `src/device_frontfacing.c` | `1d84995f6eee645f3685da96417f33d82eec47c7` | 1,619 | `d4f52e107e7cc5ec92cdd953755746f8dcb84a90c30ac1344dd3416b43cf38d2` |
| `src/headers/device_datum.h` | `815a0b9b009e85e280c4c6b72a6adf4c1310e8d0` | 985 | `1a25f7013f0266b6a30896d7970cf75dd642e5fed3a0569577593ee420f5e318` |
| `src/headers/control_datum.h` | `0c4617d5dff08ac749da936bd6cf5bea2d310da4` | 678 | `2009a47048f4d6b5ea66dc99f68c75b0ae91a9e8b444aa3232d0eb32c2811c07` |
| `src/headers/object_marker.h` | `8d4df1a9a601894c01c585222f95da5cf5debdd0` | 546 | `a9e6dd319839adf62865f639356e35e79a0b8f2e473335a9f7ce0ea46d44fdb4` |

HCEA supplies readable public intent only. Its CE/Xbox live-object layouts
place the position group at composite `+0x204` and control flags at `+0x214`;
those offsets are not PC-compatible and were not promoted. January is the
authority for the PC signatures, composite offsets, constants, call
identities, code extents, padding, and relocation order. The January setters
load the signed position-group field at `+0x1B4`, while front-facing tests the
control flags at `+0x1C4`. The repository's current PC structures and the
inherited exact `_device_set_power` body independently corroborate the device
layout. No Stian donor is used.

All-revision source history finds no earlier committed definition of any of
the three fixed bodies in `devices.c`. The prior Jonas devices wave attempted
only `_device_get_position`, `_device_get_power`, and
`_device_operates_automatically_set` in addition to its three retained leaves;
the later wave added only old-map disposal. The present three-body boundary
therefore did not collide with an integrated or previously attempted devices
body.

## Typed ABI, caller, and layout proof

The retained public contracts are:

```c
void device_set_actual_position(long device_index, real position);
boolean device_set_desired_position(long device_index, real position);
void device_group_set_actual_value(short group_index, real actual_value);
boolean device_group_set_desired_value(short group_index, real desired_value);
```

On this 32-bit C ABI, `long` and `real` are 32-bit, `short` is 16-bit, and
`boolean` is an 8-bit return in AL. The source names decorate naturally as
`_device_set_actual_position` and `_device_set_desired_position`. Both target
bodies push their two arguments and clean the stack as ordinary cdecl calls;
neither uses a hidden or register-carried formal. The actual-value helper is a
public ordinary-C dependency already declared with the same signature in
`source/hs/hs.c`; the owner TU uses an identical source-local declaration and
does not define or reconstruct that helper.

January's complete direct-caller census is:

| Caller object / owner | Callee relocation |
| --- | --- |
| `source/devices/device_machines.obj`, `_machine_try_to_open_with_damage` | actual setter, REL32 `+0x36` |
| `source/hs/hs.obj`, `_code_000aeee0` | actual setter, REL32 `+0x27` |
| `source/hs/hs.obj`, `_code_000aee50` | desired setter, REL32 `+0x2F` |

The reconstructed-source census finds the corresponding machine call and both
HS wrapper calls with the same typed signatures. The already present public
header declaration covers the actual setter. The desired setter self-types its
definition while `hs.c` carries its existing identical local declaration. No
shared-header change is required or retained.

Each retained setter first rejects `NONE`, obtains a checked device through
the existing `device_get` macro, reads the signed `short`
`position_group_index`, and rejects an unassigned group. The actual setter
then calls the void helper; the desired setter returns the boolean helper
result and otherwise returns `FALSE`. January's checked access mask is
`0x380`, exactly `_object_mask_device`. No cast, unsigned donor narrowing,
raw offset, private helper, or donor-only layout enters the final source.

The complete exact relocation proof is:

| Function | Offset | COFF type | Destination |
| --- | ---: | --- | --- |
| `_device_set_actual_position` | `+0x12` | `REL32` | `_object_get_and_verify_type` |
| `_device_set_actual_position` | `+0x2C` | `REL32` | `_device_group_set_actual_value` |
| `_device_set_desired_position` | `+0x12` | `REL32` | `_object_get_and_verify_type` |
| `_device_set_desired_position` | `+0x2C` | `REL32` | `_device_group_set_desired_value` |

## One-shot front-facing rejection

The natural typed front-facing candidate used the current complete
`control_datum`, the checked `_object_mask_control` accessor, bit zero of the
PC control flags, a 108-byte `object_marker`, the compiler-owned `"front"`
literal, and the existing `dot_product3d` inline. It preserved January's
unused `position` formal and returned `TRUE` on a missing control, the
both-sides flag, a missing marker, or a nonpositive/unordered dot product.

The sole three-body candidate object was 3,011 bytes with SHA-256
`bbf0c7bc0a094f3e84285d9c694408084db0252c823e8fc6c7bf3c1489f894e4`.
Front-facing emitted 114 meaningful / 128 padded bytes and four relocations,
but its normalized SHA-256 was
`a995bbf1b3fc750e23878cfd54b9c61a14592cb833cf2e6c326c0a992f67fbfb`.
Its relocations appeared at `+0x11`, `+0x2C`, `+0x32`, and `+0x59`; January
requires `+0x12`, `+0x30`, `+0x36`, and `+0x5C`. The body, the
`device_controls.h` include needed only by it, and its source-local flag name
were removed immediately. There was no retry or attempt to force January's
five-byte-longer schedule. The baseline public header declaration and external
players caller remain untouched.

## Preserved exact bodies and ownership boundary

Direct hardened comparison proves the complete final seven-body inventory:

| Exact function | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_devices_dispose` | `1 / 16` | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_devices_dispose_from_old_map` | `13 / 16` | 2 | `9dedd614496101c84c04e338b0fb2167843b37380b3f0e2a50007ce9c2bc7edc` |
| `_device_set_never_appears_locked` | `64 / 64` | 1 | `e180e3b5196b04ad2c72967cdc93bdc207a23cc33b12d406864567ff38d55e36` |
| `_device_one_sided_set` | `59 / 64` | 1 | `37b297855afc1da1f38223444572043ab773a86d1831a3c828a24175f687bfb6` |
| `_device_set_actual_position` | `53 / 64` | 2 | `d0e53196194b9cab6dd3e748e0b9c4d81902b02b465dfd019ed632d338c9d836` |
| `_device_set_desired_position` | `57 / 64` | 2 | `46aed102030b8a7a6ca51498138ebba7ce8952c3f05d2b9ea73affce0ade5b4f` |
| `_device_set_power` | `67 / 80` | 2 | `77bb40070eb2a2a67b2d36bc1bae62ffce1cf94b7a3babc9cd3eacd37972b7be` |
| **Final strict total** | **`314 / 368`** | **10** | |

The final pre-commit object is 2,301 bytes with phase-specific SHA-256
`ab1a641c1e15e455006c3a77a4846ddb9c6ace7f5e2408ec383a6e1aa5cb45c0`.
Its only defined external runtime owners are the seven code COMDATs above. It
defines no `.rdata`, writable `.data`, `.bss`, COMMON, compiler literal, or
aggregate storage. `_device_groups_data`, both group helpers, both object
accessors, and `__fltused` remain undefined value-zero dependencies.
`_device_frontfacing` and its two compiler literals are absent. All 208 target
non-code bytes remain unclaimed, and this wave receives zero data credit.

The retained implementation is readable typed defined C. It contains no
assembly, raw address or byte-offset dereference, pointer/integer
reconstruction, object-representation cast, pointer or union pun,
inactive-union access, undefined signed overflow, private/register ABI,
volatile scheduling device, force-inline annotation, optimizer pragma or
barrier, synthetic anchor, or byte-forcing expression.

## Scope and pre-commit validation

Only `source/devices/devices.c` and this new Jonas-owned ledger are tracked
changes. No header, configuration, semantic exception, parked record,
completion label, storage owner, pre-existing Markdown, or tracked deletion
is changed. The five protected sources remain untouched:
`source/units/vehicles.c`, `source/ai/ai_debug.c`,
`source/ai/ai_script.c`, `source/ai/actions.c`, and
`source/units/units.c`.

- The sole natural three-body candidate compile produced two immediate strict
  accepts and one fail-closed wholesale removal, with no candidate retry.
- Complete 569-action `halobetacache_build` plus `libcmt_build`: pass.
- Direct hardened final comparison: both retained setters and all five
  inherited bodies are strict; rejected front-facing is absent.
- Semantic audit: 470 units, 4,142 functions evaluated, 4,002 semantic exact,
  101 hidden exact / 61,876 hidden code bytes, 4,063 accepted exact, 36
  ordinary-only, 35 structurally accepted, one rejected, and zero unit errors.
- Campaign progress: 375/833 matched objects, 4,050/11,060 exact functions,
  481,620/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 matched data bytes.
- Halo progress: 273/468 matched objects, 3,883/7,574 exact functions,
  468,706/1,770,166 meaningful code bytes, and
  1,829,896/3,923,451 matched data bytes. Libcmt remains 102/212 objects,
  167/476 functions, 12,914/55,015 code bytes, and 5,192/8,637 data bytes.
- `devices.obj`: 7/30 exact functions and 314/4,463 meaningful code bytes;
  it remains nonmatching and claims no data.
- Object admission: zero candidates and zero revocations; only the inherited
  unrelated `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass. `git diff --check`, changed-path,
  deleted-path, protected-five, and retained-source policy checks are clean.

## Clean committed-state replay

Implementation-and-ledger commit
`c64c0b34e66c6fc186cb4aadd09e4cf569a4b8ff` was clean before replay and is
authored and committed by Jonas Volman. Re-reading its committed source blob
produced exactly `1a8cd2fe9c7f021e359220fea3fdfd4d39267500`, 4,442 bytes,
and payload SHA-256
`cf09bc580e6152e802e8c3f9dea737fdc327762bbb5d6fead502a0c0e5c17a5f`.
The shared header remained baseline blob
`638221fd4dfd7ff7196b528798845ce0d194d997`.

A one-unit regression snapshot was captured directly from that clean commit
with status `SNAPSHOT_WRITTEN`, commit
`c64c0b34e66c6fc186cb4aadd09e4cf569a4b8ff`, and sole unit
`source/devices/devices`. The generated candidate path resolved to
`build/base/source/devices/devices.obj` inside this isolated worktree. Before
deletion it was 2,301 bytes with SHA-256
`ab1a641c1e15e455006c3a77a4846ddb9c6ace7f5e2408ec383a6e1aa5cb45c0`.
The resolved path was verified to remain under the worktree, deleted, and
confirmed absent. Its untouched normal Ninja edge then performed exactly one
`[1/1] CL` action with the repository's natural flags and no source change or
retry.

The immediate regression check returned `ok: true`, no failures, no warnings,
`changed_nonexact: []`, and `newly_exact: []`. Its `still_exact` inventory is
exactly the seven functions in the table above. Direct hardened January
comparison again returned equality for all seven padded COMDATs and every
relocation address, type, destination, and addend. In particular, the rebuilt
actual setter remains 53 meaningful / 64 padded bytes with two relocations and
hash `d0e53196194b9cab6dd3e748e0b9c4d81902b02b465dfd019ed632d338c9d836`;
the desired setter remains 57 meaningful / 64 padded bytes with two
relocations and hash
`46aed102030b8a7a6ca51498138ebba7ce8952c3f05d2b9ea73affce0ade5b4f`.

The replay object is 2,301 bytes with phase-specific whole-file SHA-256
`4eabbd93203d3c48f0e60b79ee8bb95d54e09ce6a410449e695c8050068e284d`.
It defines exactly the seven accepted external code owners.
`_device_frontfacing` remains absent, as do the `"front"` and zero-float
runtime COMDATs. The object has no runtime `.rdata`, writable `.data`, `.bss`,
or COMMON owner; its remaining non-code sections are compiler directive/debug
material only. Both position helpers, both object accessors,
`_device_groups_data`, and `__fltused` remain undefined externals. The
worktree was clean immediately after replay. No push, amend, or history
rewrite is performed.
