# `items.obj` valid-real validator pair first shot (Jonas, 2026-08-27)

## Result

The first and only code-producing candidate exactly recovers both adjacent
validators:

- `_valid_real_vector3d_axes3`: 183 meaningful / 192 padded bytes, seven
  `REL32` relocations, normalized SHA-256
  `4ff4f15d7e2835b5d6fd06a2f312c80c14e0153be8bbf34c5d244729b7055613`.
- `_valid_real_matrix4x3`: 81 meaningful / 96 padded bytes, two `REL32`
  relocations, normalized SHA-256
  `09149bbb8fe8ff06774a5972f52f622f3f809e0bf59718a3cf2b019ac289b488`.

`items.obj` advances from 8/18 to 10/18 exact owners. All eight inherited
exact owners remain exact, so `lost_exact` is empty. The wave adds 264
meaningful code bytes and two functions without changing the completed-object
count.

## Frozen evidence

- Target object raw SHA-256:
  `D7C23F2AC171FBEDC9C2FBDFA3053D663145D69B3DC19D402BB8080379C4314C`.
- Pre-edit source blob: `53c3d935`.
- Pre-edit source raw SHA-256:
  `C533156D4F8E3080EE37EBBCA5E68D28AF46BF30C3DE1B32CB2ABC4D6E91FBC9`.
- Frozen baseline:
  `build/audit/items_valid_real_pair_baseline_20260827.obj`.
- Baseline raw SHA-256:
  `CE9DFDC13FF4EC6AC2FDF05290D75EBEB77C77177C75C01965DBA4DEB163B270`.
- First-shot candidate:
  `build/audit/items_valid_real_pair_first_shot_20260827.obj`.
- First-shot raw SHA-256:
  `5C694F1E477211E83AB649D1E1E9A85BF5BEF494A2CFAB17D40903A0C92C467A`.
- Retained source raw SHA-256:
  `CAE4A7CA55A3C88878F425AF9720761041FC1E4ABD6F0C376A745E340166F90E`.

The axes target calls `valid_real_normal3d` three times, `valid_realcmp`
three times, and `dot_product3d` once. The other two dot products are inlined.
The matrix target inlines the finite-scale test, then calls the recovered axes
validator and `valid_real_point3d`. The candidate reproduces every relocation
address, type, identity, addend, and owner.

## Source authority and topology

The canonical bodies already exist as inline helpers in
`source/math/real_math.h`. The HCEA implementation at
`build/audit/refs/halocea/src/blam/structures/valid_real_vector3d_axes3.c`
independently confirms the three normal checks and pairwise orthogonality
semantics. The January Xbox disassembly fixes evaluation order and proves the
matrix's short-circuit finite-scale, axes, and position sequence.

An include-time rename keeps the header's inline helpers available under
translation-unit-private spellings while allowing `items.c` to provide the
two natural external owners. The source uses the canonical short-circuit
expressions directly; it adds no casts, raw-address access, or compiler-facing
workaround.

## Strict comparison and containment

`python -m tools.coff_compare` reports `all_equal: true` for both owners.
The pre/post ordinary exact-owner census is 8 to 10, with additions limited to
the two validators and `Lost: []`.

## Repository gates

- `build\local-tools\ninja.exe`: pass.
  - 470 semantic units scanned.
  - 4,368 functions evaluated.
  - 4,223 semantic-exact and 4,283 accepted-exact functions.
  - zero unit errors.
  - Campaign: 377/833 complete objects, 4,257/11,060 exact functions,
    512,386/2,198,102 code bytes.
  - Halo: 275/468 complete objects, 4,090/7,574 exact functions,
    499,472/1,770,166 code bytes.
- `python -m tools.audit_object_admission`: pass, 0 candidates,
  0 contradicted, 0 revoked.
- `python -m tools.parked_functions`: pass, 3 active, 0 stale, 0 invalid.
- `python -m pytest -q`: 179 passed; only the existing cache-directory
  permission warning remains.
- `git diff --check`: pass.
