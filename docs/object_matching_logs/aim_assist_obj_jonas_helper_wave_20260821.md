# `aim_assist.obj` Jonas helper wave

## Result

This bounded source-only wave starts from authoritative integration commit
`6a00c3a6d6b40f42001889e403dbcb9802e4ec31`. Six authenticated helper
candidates were emitted together in the first and only natural code-producing
candidate compile. Two were independently strict and are retained. The four
misses were removed without a source-shape, spelling, declaration, or compiler-
control retry. `aim_assist.obj` advances from 0/16 to 2/16 exact functions and
remains `NonMatching`.

| Retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_set_real_euler_angles2d` | 19 | 32 | 0 | `9e3ef3d2df5fda4a7490d040cce70db1c6b5ef4587dd82b14c9a9a0e8b10fdfc` |
| `_aim_assist_clear_line_of_sight` | 239 | 240 | 18 | `3dad84c61ea4cdfcd2c4ec13c8400a09197d6a57917cf8d3e3d4690f0085de54` |
| **Wave gain** | **258** | **272** | **18** | |

The hardened comparator proves identical padded bytes and exact relocation
addresses, types, names, destinations, and addends for both retained COMDATs.
The remaining fourteen target functions and all 40 target non-code bytes stay
outside this wave's credit.

## Provenance and signatures

- Baseline `source/game/aim_assist.c` blob:
  `71bf6802449990b7398a5e838b680e4a1c008a05`.
- Retained source blob/SHA-256:
  `3c302f49afda70162213149c8a2f4e5dcc50e540` /
  `a922889f5ff732f5bb5fbff105d8d6d5deb544412a9546b79ce998e394e63f7a`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split-object SHA-256:
  `a3202c27ff6a8dfb282ea34e72049ab6f0fac34704a6d24d90779b160e9ed199`.
- Extracted Release PDB symbol JSON SHA-256:
  `7a02e56d90c680ccc9e95325f04c3406d39c197b22e7d2023b1bfb69da3aa118`.
  It independently fixes all six exact labels at January RVAs `0x93C00`,
  `0x93C50`, `0x93E10`, `0x93E20`, `0x93E80`, and `0x93EA0` in candidate
  order. The companion type JSON is SHA-256
  `cff80fa1670b0aafe2f34cc3b696d56b6e5738b5e495ac76d99341f321c0aff3`.
- Clean HCEA semantic donor commit:
  `c168af2e747d3095d9a29418ae401f3a39544863`. Candidate source blobs are,
  in order, `477ef590f0a5ed8f2802389672b0bc69bd05baec`,
  `f987b956fdf731242a0ed03707b07922430f8f2c`,
  `a1e8e73ebd4e0099c43893bddb54b5b0d15e8283`,
  `41ea64b2e58ed0016a98704f4d56ab8afdf198e5`,
  `ec22338ab9877431c59532c31dafcea2b4a3a140`, and
  `ac0da3d80fb27f144da06ab4958c0d5a8f1cae88`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` for
  80x86, with the repository's unchanged XDK 3911 command and flags.

HCEA supplies the natural typed behaviors and parameter meanings. Release PDB
labels authenticate PC symbol identity; January stack access and exact COFF
authenticate the PC widths, layouts, return ABI, code extent, bytes, padding,
and relocation ownership. The screened retained signatures are:

```c
real_euler_angles2d *set_real_euler_angles2d(
    real_euler_angles2d *angles,
    real pitch,
    real yaw);
boolean aim_assist_clear_line_of_sight(
    real_point3d const *point0,
    real_point3d const *point1,
    long ignore_object_index,
    long target_object_index);
```

## Authenticated behavior and layouts

`set_real_euler_angles2d` writes the 32-bit `pitch` at `+4`, writes the 32-bit
`yaw` at `+0`, and returns the original record pointer. HCEA's two-float angle
record independently establishes the same order.

The line-of-sight helper uses ordinary typed calls to
`object_get_ultimate_parent` and `collision_test_vector`. Its local collision
record is compile-time checked at `0x50` bytes with the `long object_index`
field at `+0x38`; the point and vector records are three 32-bit reals. January
COFF independently proves the `0x5C` local frame, three component
subtractions, flags `0xC2AD`, collision-result object discriminator `3`, and
the complete comparison of ultimate parents. It also proves collision-user
type `6`, the depth limit `32`, and the source-authentic begin/end assertions
at lines 350 and 367.

The helper returns clear only when the vector test hits nothing or when its
object hit and the intended target resolve to the same ultimate parent. The
collision-user stack is pushed before the test and decremented after the
closing assertion on every ordinary return path.

All referenced globals are extern declarations from the existing collision-
usage API. The candidate defines no writable global, `.data`, `.bss`, or
COMMON storage. Assertion literals are naturally emitted read-only COMDATs
and compared as relocation destinations where required; this partial object
makes no data-matching or ownership claim.

## One-shot rejection record

| Rejected function | Target meaningful / padded / relocs | Target hash | First natural candidate |
| --- | ---: | --- | --- |
| `_compute_attenuation` | `80 / 80 / 3` | `5fd0d02f2dbea44522a26dde39adb5713d10f5113ed75b67a1f72c9cf1fa9ccf` | `80 / 3`, SHA `50b13439e7ef77d16e96b7c88647d0ff5804959870ae1351385ad5b66b924718`; natural control flow produced different bytes and constant-relocation addresses/order. |
| `_compute_composite_attenuation` | `43 / 48 / 2` | `89bd0c181712705720c166f9084ab12aad8438a3148a36a7cb8e3a041e9f4863` | `144 / 6`, SHA `9970a7c6f46f248be526962e24507913389769a9222e1744e3a07b0bc28deab1`; VC7 naturally inlined both attenuation calls while January owns two call relocations. |
| `_reciprocal_square_root` | `16 / 16 / 1` | `6675eafcdc04bcb2a3a70b55f061d18302beb6e07425aed3f4d15d23eb038ad6` | `16 / 1`, SHA `906f8f7bca15728e546f66ecb98b02f83b493ab97aab1bdb169531edea356482`; the ordinary portable expression owns a double `1.0` relocation while January owns the single-precision constant. No PPC or forcing intrinsic was introduced. |
| `_limit3d` | `88 / 96 / 0` | `4ba268790768577ef786d5f1c5b25a180d6468be03fcc0571e294eadd7f48e21` | `96 / 0`, SHA `0bec7e2bc780dbd91942a28f125f0700527961aad377e652e085db54305ca7dc`; equal section shape but different normalized x87 bytes. |

All four rejected definitions are absent from the final COFF symbol table.
Reopen them only from independently preserved January PC source or a newly
authenticated ordinary PC API/source contract; do not retry inline controls,
PPC `__fsqrts`, source-order matrices, or code-generation devices.

## Policy, scope, and validation

Only `source/game/aim_assist.c` and this new Jonas-owned ledger are changed.
No shared header, configuration, semantic exception, parked record, existing
Markdown, target data, or frozen large translation unit is edited.

The retained source is readable typed C. It contains no assembly, raw address
or byte-offset dereference, pointer/integer reconstruction, cast or union pun,
inactive-union access, undefined overflow, volatile scheduling device, force-
inline annotation, optimizer pragma/barrier, synthetic anchor, or object-byte
forcing.

- Complete `halobetacache_build` and `libcmt_build`: pass (569 actions).
- Direct hardened comparison: both retained functions pass; all four rejected
  symbols are absent from the final object.
- Semantic audit: 470 units, 4,052 functions evaluated, 3,912 semantic exact,
  101 hidden exact / 61,876 hidden code bytes, 3,973 accepted exact, and zero
  unit errors.
- Campaign progress: 375/833 complete objects, 3,960/11,060 exact functions,
  475,825/2,198,102 meaningful code bytes, and
  1,835,088/4,176,062 matched data bytes. This wave adds two functions and
  258 meaningful bytes with no data increase.
- Object admission: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` completion-label contradiction remains.
- Parked validation: three active, zero stale, and zero invalid entries.
- Complete tooling suite: 179/179 tests pass.

A clean committed-state snapshot/check followed by deletion and forced
rebuilding of `aim_assist.obj` and its reports closes the current section and
literal-ownership surface before handoff. No push is performed.
