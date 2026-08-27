# `glow.obj` nonuniform spline pair recovery (Jonas, 2026-08-27)

## Result

Both adjacent spline owners are byte-exact and relocation-exact:

- `_nonuniform_cubic_spline`: 179 meaningful / 192 padded bytes, four
  relocations, normalized SHA-256
  `18c7ed7375f6f62197fa3d7e528a0ba1c2d091c73a79b2fad4875513ef4619a8`.
- `_nonuniform_cubic_spline_vector3d`: 171 meaningful / 176 padded bytes,
  three relocations, normalized SHA-256
  `c212fefb1ed52b780faaaebccfd1dec1d09bca9ee0665f29d4bdafe522dd0f76`.

`glow.obj` advances from 5/25 to 7/25 exact owners. All five inherited exact
owners remain exact, so `lost_exact` is empty. The wave adds 350 meaningful
code bytes and two functions without changing the completed-object count.

## Frozen evidence

- Target object raw SHA-256:
  `593DB0E7FDE065D7426231E9E08AAED6360B790033077481E1D70044E54A638A`.
- Pre-edit source raw SHA-256:
  `DC88B571F923D45583530FE25DDBD8DB6F7215757D2B813A76A3D0284924DF3F`.
- Frozen baseline:
  `build/audit/glow_nonuniform_spline_pair_baseline_20260827.obj`.
- Baseline raw SHA-256:
  `2C1B461953F78372C2E8969DAE84B67646E8472B771ABA27185AC37C25767D58`.
- First-shot candidate:
  `build/audit/glow_nonuniform_spline_pair_first_shot_20260827.obj`.
- First-shot raw SHA-256:
  `6F390DE54A2BD12BA3CF26A4D80F9528B968774696982D6D1E241C90CBA7AC3C`.
- Target-spelling-corrected candidate:
  `build/audit/glow_nonuniform_spline_pair_whitespace_corrected_20260827.obj`.
- Corrected raw SHA-256:
  `06156410CFE6E4266209F4CAC340EEDFC8997F68700A1A7CABAFD3B9420A36A7`.
- Retained source raw SHA-256:
  `EE81BBAEF75CC51E2B750D0629A192FB53EA5407B1EB445C0803438C6E3003DD`.

## Source authority and topology

The January scalar owner is the in-place Newton divided-difference topology:
three first differences, two second differences, one third difference, and a
nested evaluation at `t`. Its disassembly also proves the bounds assertion at
`real_math.h:1530`. The vector owner consists of three ordinary scalar calls
in `i`, `j`, `k` order.

The repository's inline bodies in `source/math/real_math.h` preserve the same
expression family, and the HCEA files
`build/audit/refs/halocea/src/blam/effects/nonuniform_cubic_spline.c` and
`nonuniform_cubic_spline_vector3d.c` independently confirm the mathematical
semantics. The January Xbox target remains the authority for in-place source
topology, assertion identity, and the three-call vector shape.

An include-time rename hides the incomplete header-inline spellings while
leaving the shared header untouched. `glow.c` then supplies the natural scalar
and vector owners with house-style parameter lists and explicit returns.

## Evidence waves

The first code-producing candidate already matched the scalar's entire padded
code hash and made the vector owner fully exact. Its sole scalar mismatch was
the stringified assertion token spelling:

- candidate: `t >= t0 && t <= t3`
- target: `t>= t0 && t <= t3`

That relocation-identity result was genuine new evidence. A second frozen wave
changed only this whitespace spelling. The corrected candidate matches every
padded byte and all seven relocation addresses, types, identities, addends,
and owners. No exception or waiver was used.

## Strict comparison and containment

`python -m tools.coff_compare` reports `all_equal: true` for the corrected
pair. The pre/post ordinary exact-owner census is 5 to 7, with additions
limited to the two spline owners and `Lost: []`.

## Repository gates

- `build\local-tools\ninja.exe`: pass.
  - 470 semantic units scanned.
  - 4,370 functions evaluated.
  - 4,225 semantic-exact and 4,285 accepted-exact functions.
  - zero unit errors.
  - Campaign: 377/833 complete objects, 4,259/11,060 exact functions,
    512,736/2,198,102 code bytes.
  - Halo: 275/468 complete objects, 4,092/7,574 exact functions,
    499,822/1,770,166 code bytes.
- `python -m tools.audit_object_admission`: pass, 0 candidates,
  0 contradicted, 0 revoked.
- `python -m tools.parked_functions`: pass, 3 active, 0 stale, 0 invalid.
- `python -m pytest -q`: 179 passed; only the existing cache-directory
  permission warning remains.
- `git diff --check`: pass.
