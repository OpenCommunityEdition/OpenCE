# `source/camera/orbiting_camera.obj` exact-match record

## Environment and scope

- Compiler: XDK 3911 `CL.Exe` 13.00.9254.1.
- Flags: `/O2 /Oy- /DDEBUG /Dxbox`.
- January target: pristine `cachebeta.exe`, regenerated with `csplit`.
- Complete object inventory: two functions, one 20-byte constants section,
  and one 40-byte owned path-string section.

## Final strict evidence

| Symbol | Size T/B | Relocs T/B | Normalized SHA-256 | Result |
|---|---:|---:|---|---|
| `_orbiting_camera_new` | `0x20/0x20` | `1/1` | `5e1d0e29bf599e8dc421610e5deb29869e72075acd624ee1794e9b48bf723984` | exact |
| `_orbiting_camera_update` | `0x470/0x470` | `50/50` | `908d34ea2543c87642a508552b21907f5aa137815b6abd711edf4816ff17188c` | exact |
| `_rdata_0025724c` | `0x14/0x14` | `0/0` | `860534d4a115070bc9d1c9cf3863cea78704541d317d85fb697fb4b44f9474e4` | exact |
| owned source-path string | `0x28/0x28` | `0/0` | `4947aefac83506a01ca314594c170532ef4437715f7eb8175022fc2f72cdab63` | exact |

The hardened comparator reports `all_equal: true`. The constants owner has
COFF type 0 and storage class 3 on both the regenerated target and rebuilt
object.

## Closing source and ownership evidence

The prior external declaration made `orbiting_camera_update` differ only in a
nine-byte scheduling window. The compiler delayed the two loads from
`rdata_0025724c` until after the zero-vector and depth stores. Declaring the
constants table `static const` gives VC7 translation-unit ownership and
non-aliasing knowledge; it then emits the January load/store interleaving
exactly, without changing source semantics or any other instruction.

This is also the evidence for the original linkage:

- no other translation unit references `rdata_0025724c`;
- the object owns the complete 20-byte table;
- `static const` alone reproduces every function byte and relocation address;
- `config/symbols.json` now marks the csplit symbol static, producing storage
  class 3 on both sides.

## Rejected experiment and do-not-repeat list

- Expanding `result->offset = *global_zero_vector3d` into three scalar
  assignments regressed the update to 52 relocations and introduced repeated
  global loads. It was reverted.
- Five older assignment/declaration-order permutations failed to improve the
  external-linkage baseline and were reverted.
- Do not revisit barriers, `volatile`, assembly, undefined behavior, byte
  forcing, compiler pragmas, or flag changes. None is needed.

## Disposition

Complete and strict-exact. The stale parked entry was removed and the object
is marked `Matching`.
