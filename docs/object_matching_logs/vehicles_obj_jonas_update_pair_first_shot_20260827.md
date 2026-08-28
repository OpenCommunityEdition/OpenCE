# `vehicles.obj` update-pair recovery (Jonas, 2026-08-27)

## Result

The two adjacent update callbacks are byte-exact and relocation-exact:

- `_vehicle_build_update`: 3 meaningful / 16 padded bytes, zero relocations,
  normalized SHA-256
  `1191f37eec740e495eda01ae2ac0969bf44c3c661ce4ae90583b5fdc05c0f707`.
- `_vehicle_parse_update`: 3 meaningful / 16 padded bytes, zero relocations,
  normalized SHA-256
  `1191f37eec740e495eda01ae2ac0969bf44c3c661ce4ae90583b5fdc05c0f707`.

`vehicles.obj` advances from 5/39 to 7/39 exact functions and remains
`NonMatching`. The five inherited exact lifecycle/delete owners remain exact,
so `lost_exact` is empty. This wave adds six meaningful and 32 padded code
bytes without claiming data or whole-object completion.

## Frozen evidence

- January split object raw SHA-256:
  `F3DE171EF028B6254B0A2A53505D3CCBD47C5B3C80B5B7FE5251A917B9B91A81`.
- Production candidate object raw SHA-256:
  `505B841DBB5E390C8B64949BAEC2F8D66E638141687511C64A58AC324303678C`.
- Pre-edit source Git blob:
  `4a01dc7c54717e1b063a0cac72b0178b4bb2bf04`.
- Retained source Git blob:
  `0557da6c3437baf7922a6504c9565fcb54224603`.
- Retained checkout source SHA-256:
  `03454F93A6C5C11157BD8852F5A6326B808CBBB733D4574A0534B455B181F20D`.

## Source authority and implementation

January's PDB labels, independent 16-byte COMDAT boundaries, and executable
code are the PC authority. Both functions consist of `mov al, 1; ret`, followed
by ordinary alignment padding. They read no argument, global, literal, or
other state and own no relocation. The natural typed C spelling is therefore
an ordinary `boolean` function that takes `void`, returns `TRUE`, and terminates
with that explicit return.

The translation unit previously had no project headers because its only
implemented owners returned `void`. The retained source adds the normal
`cseries/cseries.h` then `units/vehicles.h` include order so the canonical
`boolean` and `TRUE` names are available. It adds no ABI annotation, register
steering, cast, raw address, or synthetic payload.

Two preliminary scratch invocations produced no object because the required
typed header environment was initially absent and then incomplete. After the
normal project include order was supplied, the first object-producing VC7
compile made both functions strict-exact. No body-shape sweep or compiler
control experiment was performed.

## Containment and direct comparison

The whole-unit scratch census reports 7 exact, 0 residual, and 32 unwritten
functions. The exact set is:

- `_vehicles_initialize`
- `_vehicles_initialize_for_new_map`
- `_vehicles_dispose_from_old_map`
- `_vehicles_dispose`
- `_vehicle_delete`
- `_vehicle_build_update`
- `_vehicle_parse_update`

Direct production disassembly is identical for both retained owners, including
all 13 padding NOPs. The strict board reports `vehicles.obj` at 112/17,232
padded code bytes and 7/39 functions.

## Repository gates

- Complete Ninja build: pass.
  - 470 semantic units scanned and zero unit errors.
  - 4,476 functions evaluated, 4,353 semantic-exact, and 4,380
    accepted-exact.
  - Campaign: 377/833 complete objects, 4,353/11,060 exact functions, and
    521,169/2,198,102 code bytes.
  - Halo: 275/468 complete objects, 4,186/7,574 exact functions, and
    508,255/1,770,166 code bytes.
- `python -m tools.audit_object_admission`: pass, zero candidates, zero
  contradictions, and zero revocations.
- `python -m tools.parked_functions`: pass, three active, zero stale, and zero
  invalid entries.
- `python -m pytest -q`: 194 passed.
- `git diff --check`: pass.
