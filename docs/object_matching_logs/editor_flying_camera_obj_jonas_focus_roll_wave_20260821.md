# `editor_flying_camera.obj` Jonas focus/roll wave

## Result

This bounded source-only wave starts from exact canonical commit
`fd4cb20f52fd9df0b66d07c0dee225475a40c5e3`. Five authenticated natural
bodies were emitted together in the first and only code-producing candidate
compile. Three are independently strict and retained; the two misses were
removed immediately without a source-shape or code-generation retry.

`editor_flying_camera.obj` advances from 5/21 to 8/21 exact functions and
from 45/5,065 to 335/5,065 meaningful code bytes. It remains `NonMatching`.
The wave gains 290 meaningful and 304 padded code bytes, 29 relocations, and
no data credit.

| Retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_editor_camera_get_focus` | 122 | 128 | 13 | `fcbc16d17ca58ad82807d47a380e2a35b3557c6313a61587e109170e335a07e4` |
| `_editor_camera_set_focus` | 128 | 128 | 13 | `8b7fe295b0ab83638e0b9d4998ca0452f75d8b61233aed6023609e4b64e4068e` |
| `_editor_camera_use_roll` | 40 | 48 | 3 | `6cdf9034713ee5447c6ef31128d250ebaa736ef45c9b7551b22050a376081f76` |
| **Wave gain** | **290** | **304** | **29** | |

The hardened COFF comparator reports `all_equal: true` for all three retained
bodies. Their padded bytes and every relocation address, type, destination,
and addend are identical to January. The other 13 unmatched functions and all
520 target non-code bytes remain outside this wave's credit.

## Provenance and signatures

- Baseline `source/camera/editor_flying_camera.c` blob:
  `a4e2b3bf173247ea69b7dacf4fca24b635886470`.
- Retained source Git blob / SHA-256 of its committed Git-blob payload:
  `f256fc56db8f65b3c96aed0c74b9ff8d72a502fa` /
  `d38dff089102cfdf30a0b9cd1621f2a70804c36412384fb6266978e3f49a7b88`.
  This digest is over the stable Git-blob payload, not a platform checkout's
  line endings.
- Unchanged `editor_flying_camera.h` blob:
  `af8949834a0ae788325f02c13d8665f393eb192b`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `editor_flying_camera.obj` SHA-256:
  `5e0066e8f28b6c2c79e9e546075922b263342dea5a50d9ef84e8818792d99945`.
- Clean HCEA semantic donor commit:
  `c168af2e747d3095d9a29418ae401f3a39544863`. The screened body blobs, in
  target order, are `92d6a11bf5d17409eb881109b4605fd9f088458b`,
  `3c0f7c4eb6c226a23f8c1c261add083a7062f6e9`,
  `1d1679366ca0ccbcdda763f33bc24a42690c64fe`,
  `cf81c356d02acdac76549a6b5d8f13270aa66bd3`, and
  `1ef2c0a62e994c2b0f15f132abeb9ac3e4f7543b`. Its independently typed
  `flying_camera.h` and `editor_camera_focus_t.h` blobs are
  `add0cab77bac8ddbedb7f280fa3b25e58b8c75e8` and
  `1c4032fc3642ad1e670a7dbda8aa4a74c880fc50`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86. `CL.Exe` SHA-256 is
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
  using the repository's unchanged XDK 3911 command and flags.

The January PDB, assertion strings and line metadata, disassembly, and HCEA
donor fix the screened signatures as:

```c
void editor_camera_get_focus(
	real_point3d *position,
	real_euler_angles2d *angles);
void editor_camera_set_focus(
	real_point3d const *position,
	real_euler_angles2d const *angles);
void editor_camera_set_position(
	real_point3d const *point,
	real_euler_angles2d const *angles);
boolean editor_camera_use_roll(boolean new_use_roll);
void editor_camera_move_to_point(real_point3d const *point);
```

HCEA supplies readable natural bodies and independent layout evidence;
January remains authoritative for the PC assertions, `set_position` fallback
call, public vector helper, 2.5-real constant, symbol identity, extent,
padding, bytes, and relocation ownership.

## Typed layout and retained behavior

The prior source already exposed the scripted byte at `+0x00` and mode at
`+0x2C` through an external grouped-BSS prefix. This wave naturally refines
the intervening typed fields without defining storage: use-roll at `+0x01`,
initialized at `+0x02`, the 20-byte persistent focus at `+0x04..+0x17`, and
the editor flying-camera pointer at `+0x18`. The local camera type follows the
independently preserved HCEA layout: position, two-angle orientation, roll,
and field of view. This avoids a cast or overlapping-type view while naming
the exact January offsets.

`editor_camera_get_focus` and `editor_camera_set_focus` assert their two
pointers with January's source path and lines 120/121 and 129/130, then copy
the complete typed point and two-angle records. Each has eight assertion
relocations plus five `DIR32 _bss_0031d438` relocations to `+4`, `+8`,
`+0xC`, `+0x10`, and `+0x14`.

`editor_camera_use_roll` returns the prior byte, stores the requested byte,
and clears the camera's roll at `+0x14` when roll use is disabled and a camera
exists. Its relocations are `DIR32 _bss_0031d438+1` at code offsets `+9` and
`+0xF`, and `DIR32 _bss_0031d438+0x18` at `+0x17`.

The source defines no `.data`, `.bss`, or COMMON storage. The retained
assertions necessarily reference compiler-folded string literals, but this
incomplete wave does not authenticate or claim any of the target object's
520 non-code bytes.

## One-shot rejection record

| Rejected function | Target meaningful / padded / relocs | Target hash | First natural candidate |
| --- | ---: | --- | --- |
| `_editor_camera_set_position` | `146 / 160 / 11` | `b4b5dc5d6f88c8d32309b8019c9c1f00e7a9607fc097812f57bc7e412219e1a3` | 160 padded / 13 relocs, SHA `aa90ee75950f6b36f3e2ff1665940895b6d914ddb028d24f4b89ce76e97eca34`; the defined typed body reloads the external camera pointer around the member copies, unlike January's single retained pointer value. |
| `_editor_camera_move_to_point` | `120 / 128 / 10` | `a9a76937ef6744f3ed0a1dece0ad93e77619b3e70e22f23f0e8e2c9409396958` | 144 padded / 12 relocs, SHA `1a277bb6cc12186f5f91470cd900b3afb0a83f126d65f7acc8a1e11441296133`; repeated defined typed global member access does not reproduce January's post-helper pointer reuse. |

Both candidates had the authenticated semantics: `set_position` asserted its
arguments, called `editor_camera_set_focus` and set initialized when no camera
existed, otherwise copied the point and two-angle orientation; `move_to_point`
asserted its point, called public `vector3d_from_euler_angles2d`, and placed the
camera 2.5 units behind it. Their code-generation shapes were not exact. Both
definitions and symbols are absent from the retained COFF. They may be
reopened only with newly preserved January source or equivalent independent
ordinary-C topology evidence; this wave does not tune from the rejected
bytes.

## Policy, scope, and validation

Only `source/camera/editor_flying_camera.c` and this new Jonas-owned ledger
change. No header, storage definition, configuration, semantic exception,
parked record, protected/frozen source, pre-existing Markdown, or Claude-owned
file is modified. The five inherited exact getters remain strict.

The retained implementation is readable typed C with house formatting and
explicit terminal returns. It contains no assembly, `volatile` scheduling
device, force-inline annotation, optimizer pragma/barrier, raw address or
byte-offset access, pointer/integer reconstruction, cast or pointer/union pun,
inactive-union access, undefined overflow, synthetic anchor, or object-byte
forcing.

- Complete 569-action `halobetacache_build` and `libcmt_build`: pass.
- Direct hardened comparison: all three retained functions pass; both
  rejected symbols are absent from the final COFF.
- Semantic audit: 470 units, 4,102 functions evaluated, 3,962 semantic exact,
  101 hidden exact / 61,876 hidden code bytes, 4,023 accepted exact, and zero
  unit errors.
- Campaign progress: 375/833 complete objects, 4,010/11,060 exact functions,
  478,890/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 matched data bytes. Halo contributes 3,843/7,574
  functions and 465,976/1,770,166 meaningful code bytes.
- Admission audit: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass; `git diff --check` is clean.

A clean committed-state snapshot/check followed by deletion and forced
same-path rebuilding of `editor_flying_camera.obj` is the final
reproducibility proof. The post-rebuild hardened comparison must retain all
three exact bodies with both rejected symbols absent. No push is performed.
