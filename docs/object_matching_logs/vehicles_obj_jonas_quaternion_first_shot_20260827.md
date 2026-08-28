# `vehicles.obj` quaternion-setter recovery (Jonas, 2026-08-27)

## Result

`_set_real_quaternion` is byte-exact and relocation-exact on its first
object-producing candidate compile:

- 31 meaningful / 32 padded bytes;
- zero relocations;
- normalized SHA-256
  `9a46b00e8fe912ad0044759feb81101ce35e7914093416a4c31169719f10d199`.

`vehicles.obj` advances from 7/39 to 8/39 exact functions and from 112 to 144
strict padded code bytes. All seven inherited exact owners remain exact, so
`lost_exact` is empty. The object remains `NonMatching`; no data or whole-object
completion is claimed.

## Provenance and source authority

- January split object raw SHA-256:
  `F3DE171EF028B6254B0A2A53505D3CCBD47C5B3C80B5B7FE5251A917B9B91A81`.
- Production candidate object raw SHA-256:
  `9B22AEAA82736424D85109F97F208B396A9E755F9BE17E3393885572F85BEE28`.
- Pre-wave source Git blob:
  `0557da6c3437baf7922a6504c9565fcb54224603`.
- Retained source Git blob:
  `2f4d88e5afc049805d06ee944f0d3e1d066e3882`.
- Retained checkout source SHA-256:
  `4263EF440124CD12D423D4F0D6D416B3C2E5EF47C68D930DAB058B9A4650C6A6`.

January's PDB label, independent COMDAT boundary, five cdecl stack arguments,
and return value fix the PC ABI. The target writes the incoming `i`, `j`, `k`,
and `w` components to a typed `real_quaternion`, then returns the original
pointer. The repository's canonical inline body at
`source/math/real_math.h:1309` has the same field order and return value.
HCEA independently authenticates that topology in
`build/audit/refs/halocea/src/blam/math/set_real_quaternion.c`.

The translation unit uses an include-time rename to hide the header-inline
owner, then supplies the natural external owner required by January. This is
the same bounded technique already used for authenticated out-of-line owners
elsewhere in the repository. The retained function uses typed fields, one
parameter per line, and an explicit pointer return. It introduces no raw
address, cast, ABI attribute, register steering, compiler control, or synthetic
payload.

## Strict comparison and containment

The first scratch object is strict-exact. A second whole-unit scratch census
reports 8 exact, 0 residual, and 31 unwritten functions. The production board
repeats the 8/39 and 144/17,232-byte result. The retained function's padded
bytes and zero-relocation ownership match January exactly.

## Repository gates

- Complete Ninja build: pass.
  - 470 semantic units scanned and zero unit errors.
  - 4,477 functions evaluated, 4,354 semantic-exact, and 4,381
    accepted-exact.
  - Campaign: 377/833 complete objects, 4,354/11,060 exact functions, and
    521,200/2,198,102 code bytes.
  - Halo: 275/468 complete objects, 4,187/7,574 exact functions, and
    508,286/1,770,166 code bytes.
- `python -m tools.audit_object_admission`: pass, zero candidates, zero
  contradictions, and zero revocations.
- `python -m tools.parked_functions`: pass, three active, zero stale, and zero
  invalid entries.
- `python -m pytest -q`: 194 passed.
- `git diff --check`: pass.
