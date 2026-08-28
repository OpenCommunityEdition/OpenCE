# `items.obj`: ground alignment and complete item update

Date: 2026-08-27

## Result

Recovered the private ground-alignment helper exactly and restored the complete,
typed `item_update` state machine. The unit advances from 15/18 to 16/18 exact
functions, with no unwritten owners and no regression of an existing exact
owner.

- `_code_000e6900`: strict exact, 560/560 padded bytes and 26/26
  relocations.
- `_item_update`: behavior-complete residual, 2,368 candidate padded bytes
  versus 2,352 target bytes and 110/110 relocations.
- `_item_accelerate`: retained at its prior 944-byte, 47-relocation boundary;
  its single commutative x87 load-order tie is unchanged.
- Final unit census: 16 exact, 2 residual, 0 unwritten.
- Campaign gain: one exact function and 550 code bytes.

## Provenance and reconstruction boundary

The semantic donors were read in full before adaptation:

- `../work/halocea-reference/src/item_align_to_normal_and_point.c`
- `../work/halocea-reference/src/item_update.c`

January disassembly and COFF metadata remained authoritative for the Halo CE
ABI, stack shape, source-line asserts, collision flags, relocation ownership,
and branch topology. The prior item ledgers and the repository campaign
methodology supplied the established compiler/tooling constraints:

- `docs/object_matching_logs/items_obj_jonas_angular_velocity_accelerate_20260827.md`
- `docs/object_matching_logs/items_obj_jonas_object_type_external_owner_20260827.md`
- `docs/object_matching_logs/items_obj_jonas_inventory_position_detonate_20260827.md`
- `docs/object_matching_logs/items_obj_jonas_valid_real_pair_first_shot_20260827.md`
- `docs/matching_methodology.md`
- `tools/campaign/README.md`

The reconstruction uses `item_get`, `item_definition_get`, typed datum fields,
typed tag-block access, and public collision, object, math, effect, sound, and
physics APIs. The 0xC8 `item_update` frame is modeled with target-backed typed
overlays: a 0x6C marker/collision-result live-range union followed by the
matrix/sound-location work union, plus the velocity/candidate pair. It adds no
raw address, raw object/tag cast, assembly, pragma, intrinsic, volatile/register
steering, undefined pointer reconstruction, synthetic anchor, byte forcing, or
comparator exception.

## Exact helper evidence

`python -m tools.coff_compare` reports `_code_000e6900` equal:

- target and candidate size: 560 bytes;
- target and candidate relocation count: 26;
- target and candidate normalized SHA-256:
  `d3c59756d09fa1593c64892405a8b0f47c5768d116b57262ecaa1a61464f78fa`.

The function resolves the typed item, obtains the `ground point` marker,
constructs the shortest-arc quaternion (with the antiparallel double-cross
fallback), builds and validates the destination matrix, aligns the object, and
returns the resulting object position. Its VC7 private calling convention is
emitted by the real `item_update` caller.

## `item_update` boundary

The restored function covers the complete January behavior:

- profile and `_collision_user_items` entry/exit accounting;
- z-up re-orthonormalization;
- gravity integration and the `0x1FF3E9` collision sweep;
- material effects, impulse sound, bounce and settling;
- structure and supporting-object rest tracking;
- breakable-surface/support invalidation and renewed acceleration;
- angular damping, marker-relative rotation, and axis normalization;
- detonation effect/deletion and last-owned-time maintenance.

The production candidate has the exact 0xC8 frame and all 110 target
relocation owners represented. Its actual return is six bytes beyond the target
boundary, crossing one 16-byte COFF padding bucket (2,368 versus 2,352 padded
bytes). The remaining differences are legal-C VC7 scheduling/control-shape
choices; the fully typed semantic body is retained rather than introducing a
non-semantic steering device.

## Artifacts and gates

- January target object SHA-256:
  `D7C23F2AC171FBEDC9C2FBDFA3053D663145D69B3DC19D402BB8080379C4314C`
- Production candidate object SHA-256:
  `50484BC9F71B990380A7A0F7B35C0A7EE8A214DBB38454F6EA866BA515BB400A`
- Source SHA-256:
  `FDFDB32AC6BB532EF1959737883291A35AD5219247081AC8C7769A48A1B81BEC`
- Source Git blob:
  `8ead2aa51b83e5ebc2328ca74f3ade98ff7f4210`

The frozen production target was compiled exactly once:

```text
[1/1] CL build\base\source\items\items.obj
```

- Full Ninja build: pass.
  - 470 semantic units scanned;
  - 4,474 functions evaluated;
  - 4,350 semantic-exact and 4,378 accepted-exact functions;
  - zero unit errors;
  - campaign progress: 377/833 objects, 4,351/11,060 functions,
    521,163/2,198,102 code bytes;
  - report SHA-256:
    `BB838270AD8DF6971C87F794B6C7AF66709809185255551F76B2BA074FAB177F`;
  - semantic report SHA-256:
    `B86B04004575AA190F2AD11A69C70B42CCE50CFA82E99E6FC835E45B275259EA`.
- `python -m tools.audit_object_admission`: pass, 0 candidates,
  0 contradicted, 0 revoked.
- `python -m tools.parked_functions`: pass, 3 active, 0 stale, 0 invalid.
- `python -m pytest -q`: 194 passed.
- `git diff --check`: pass; only the repository's normal line-ending notice
  was emitted.
