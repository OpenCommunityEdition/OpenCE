# `projectiles.obj` Jonas ballistic-leaf wave

## Result

This bounded natural-C wave starts from authoritative integration commit
`8e3d82ed442155edb9e6f5221661f2bde4d52135`. The two permitted HCEA-backed
projectile helpers were emitted together in exactly one natural candidate
compile. `_projectile_get_ballistic_acceleration` was independently strict and
is retained. `_projectile_estimate_time_to_target` missed and was removed
immediately without a declaration, expression, control-flow, or compiler-control
retry. The pre-existing five exact bodies remain unchanged.

`projectiles.obj` advances from 5/30 to 6/30 exact functions and remains
`NonMatching`. It gains 22 meaningful and 32 padded code bytes without data
credit.

| Retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_projectile_get_ballistic_acceleration` | 22 | 32 | 1 | `fad894e7c8db43f1b5232ae8bf5c916219b1b8e7e3e3f428c454fb4da7242c0c` |
| **Wave gain** | **22** | **32** | **1** | |

The hardened COFF comparator proves equal padded bytes and exact relocation
address, type, destination, and addend. The unit now reports 27/11,950 matched
meaningful code bytes and six exact functions. Its 2,092 target data bytes and
remaining 24 functions stay outside this wave's credit.

## Provenance and signatures

- Baseline `source/items/projectiles.c` blob:
  `d2e6ded18ddb8c79fa039038b1c3a7b31490eda3`.
- Final source blob/committed-blob SHA-256:
  `a30cd38809f89617b470c3ed73acfe0744905902` /
  `4276ab6e156a184bb664114a89010f0e393c5076d61831d2aa43e35a9e57cf31`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `projectiles.obj` SHA-256:
  `0f558b623efe404e1c803b8fab03338c4f2b5b12af8ce14c4634c3ad59d6c8da`.
- January PDB2.00 symbol payload SHA-256:
  `7a02e56d90c680ccc9e95325f04c3406d39c197b22e7d2023b1bfb69da3aa118`.
  It labels the retained helper at RVA `0xE7570` and the rejected helper at
  RVA `0xE7590`.
- Clean HCEA oracle commit:
  `c168af2e747d3095d9a29418ae401f3a39544863`. The two donor body blobs are
  `a6d73b6de826ee7c47a69ea536aa407d070ef991` and
  `542cc66ed018e457d0b6f1d794092ce07084b0b1`. Its composite and nested
  projectile-layout blobs are `6171292053cd5daf5945095208735c8dd7882b0f`
  and `4aad15cd35c45f19e2fda8521008a74270cb0817`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86, SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
  with the repository's unchanged XDK 3911 command and flags.

The two authenticated signatures are:

```c
real projectile_get_ballistic_acceleration(
	struct projectile_definition const *definition);
real projectile_estimate_time_to_target(
	struct projectile_definition const *definition,
	real target_distance);
```

The clean donor layout places the embedded projectile definition at `+0x17C`,
`air_gravity_scale` at nested `+0x50`, and `initial_velocity` at nested
`+0x68`. The resulting composite offsets `+0x1CC` and `+0x1E4` agree with
January disassembly and the existing typed `projectile_definition` layout.
Existing `physics.h` independently owns the typed `real global_gravity`
declaration. No header or storage repair is needed.

## Authentication and rejection record

The retained body returns the negated product of the projectile definition's
air-gravity scale and `global_gravity`. January uses the ordinary cdecl pointer
at `[ebp+8]`, loads the named field at `+0x1CC`, and returns through x87. Its
sole relocation is `DIR32 _global_gravity` at section offset `+5`.

Both candidate bodies were compiled together once. The time-estimate helper
was then removed without tuning:

| Rejected function | Target meaningful / padded / relocs | Target hash | First natural candidate |
| --- | ---: | --- | --- |
| `_projectile_estimate_time_to_target` | `44 / 48 / 2` | `9c587ac970f3b77073e44a755654f4e7f086a3cde1a9d1bc7ef3fb40c82dd079` | `44 / 48 / 2`, SHA `0c9010c6db640284ffee91c20e7c6ba8e2fd87a995382c7f9f7ce302420b537b`; target zero-float relocations are at `+8/+20`, while the natural candidate places them at `+14/+27`. |

The rejected symbol is absent from the final COFF. Reopen it only with newly
preserved January source or an independently authenticated ordinary-C contract;
do not tune from the rejected object.

`_random_vector_in_cone3d` was excluded before compilation. Its canonical
`real_math.h` inline ownership would require a conflicting redefinition or
artificial out-of-line emission, neither of which is admissible in this wave.

## Policy, scope, and validation

Only `source/items/projectiles.c` and this new Jonas-owned ledger are changed.
No header, storage definition, data section, configuration, semantic exception,
parked record, completion label, protected/frozen source, pre-existing Markdown,
or Claude-owned file is edited.

The retained source is readable typed C. It contains no assembly, raw address
or byte-offset access, pointer/integer reconstruction, pointer or union pun,
inactive-union access, undefined overflow, volatile scheduling device,
force-inline annotation, optimizer pragma/barrier, synthetic anchor, or
object-byte/code-generation forcing. The final candidate object defines only
the five baseline functions and the retained helper, owns no `.bss`, `.data`,
or runtime `.rdata`, and leaves `global_gravity` as a typed external.

- Complete 569-edge `halobetacache_build` and `libcmt_build`: pass.
- Direct hardened comparison: the retained helper and all five baseline exact
  functions pass; the rejected time-estimate helper and excluded cone helper
  are absent from the final COFF symbol table.
- Semantic audit: 470 units, 4,067 functions evaluated, 3,927 semantic exact,
  101 hidden exact / 61,876 hidden code bytes, 3,988 accepted exact, and zero
  unit errors.
- Campaign progress: 375/833 complete objects, 3,975/11,060 exact functions,
  476,447/2,198,102 meaningful code bytes, and 1,835,088/4,176,062 matched data
  bytes. This wave adds one function and 22 meaningful bytes with no data
  increase.
- Admission audit: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass; `git diff --check` is clean.

A clean committed-state snapshot followed by deletion and forced rebuilding of
`projectiles.obj` is the final same-path reproducibility proof. The post-rebuild
hardened comparison remains strict for the retained helper and all five baseline
functions. No push is performed.
