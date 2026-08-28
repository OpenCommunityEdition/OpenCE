# `items.obj`: angular-velocity helper and `item_accelerate`

Date: 2026-08-27

## Result

Recovered the private angular-velocity cache helper exactly and restored the
full typed `item_accelerate` implementation. The unit advanced from 14/18 to
15/18 exact functions with no loss of an existing exact owner.

- `_code_000e6370`: 192/192 padded bytes, 3/3 relocations, strict exact.
- `_item_accelerate`: 944/944 padded bytes, 47/47 relocations, one residual
  commutative multiply-load ordering tie.
- Remaining unwritten owners: `_code_000e6900` and `_item_update`.
- Strict exact gain: 192 padded bytes and one function.
- Ordinary progress gain: 177 code bytes and one function.

## Provenance

The named HCEA reconstructions supplied the semantic bodies:

- `../work/halocea-reference/src/item_adjust_for_angular_velocity_change.c`
- `../work/halocea-reference/src/item_accelerate.c`

January disassembly remained authoritative for the Halo CE topology. It
established all of the following differences from the later donor:

- the private helper receives the item index through VC7's EAX local-function
  convention;
- `item_accelerate` pushes and pops `_collision_user_items`, with the original
  source-line asserts at 536 and 651;
- the BSP path calls `global_collision_bsp_get()` once and reuses the result;
- the two magnitude-squared uses remain separate, making the first inline and
  the second an external call;
- the gentle-spin path uses `real_random_range` and a separate scaled-axis
  temporary;
- the normal-selection fallback tests the normalized vector against positive
  zero using the January branch topology.

The reconstruction uses `item_get`, `item_definition_get`, typed datum fields,
typed tag-block access, and the public collision/math/object APIs. It adds no
raw object or tag cast, raw address, assembly, pragma, intrinsic, barrier,
volatile/register steering, undefined pointer punning, or byte forcing.

## Exact helper evidence

`python -m tools.coff_compare` reports `_code_000e6370` equal:

- target size / relocations: 192 / 3;
- candidate size / relocations: 192 / 3;
- target and candidate normalized SHA-256:
  `2854e75b7817c900a1ced886e137551e38dd4767f8942a6b671253b5ce6a8e68`.

The helper resolves the item through `_object_get_and_verify_type`, computes
the angular-velocity magnitude, caches a normalized rotation axis while the
object is moving, and caches sine/cosine. With zero angular velocity it clears
the item flag and stores sine zero / cosine one. Its exact EAX ABI is emitted
because the restored `item_accelerate` caller is present in the same unit.

## `item_accelerate` boundary

The reconstructed public function has the exact target envelope:

- target and candidate padded size: 944 bytes;
- target and candidate relocation count: 47;
- all relocation addresses, types, targets, addends, and ownership match;
- target normalized SHA-256:
  `ffc8298751362382e01e7f9a45ac29450ec2e761080c067f48fe077a9342ba69`;
- candidate normalized SHA-256:
  `356412c3963001b55bd2e43f14ddb5bd64c2d2c5dcc9e7b4830a09c194952511`.

All control flow and 310 of 311 decoded instructions align. The only remaining
difference is a commutative multiply at target offsets `0x2C5`/`0x2C7`:

```text
target:    fld  dword ptr [edi]
           fmul dword ptr [eax + 8]
candidate: fld  dword ptr [eax + 8]
           fmul dword ptr [edi]
```

Both calculate `acceleration->i * global_up3d->k`. A bounded legal-C matrix
covering direct expansion, scalar lifetimes, and inline-helper parameter order
either preserved this tie or disturbed the otherwise exact x87 schedule. The
shared typed `cross_product3d` form is retained as the smallest honest source.

Final unit census:

```text
UNWRITTEN   560  _code_000e6900
residual    944  _item_accelerate  [sha]
UNWRITTEN  2352  _item_update
== exact 15  residual 1  unwritten 2  (of 18 listed)
```

## Artifacts and gates

- January target object SHA-256:
  `D7C23F2AC171FBEDC9C2FBDFA3053D663145D69B3DC19D402BB8080379C4314C`
- Production candidate object SHA-256:
  `614CEFCC2D010E79404CE07DCCFD63A253E7A59888C733D8F319ACB5CA1169F0`
- Source SHA-256:
  `76C9718564C99BB904A631F120BE4E2A919FFE0A2191F3D8BD6D24A2DB774C5A`
- Source Git blob:
  `30b6780c4155beae68f5609a4ba100dd875b584d`

The frozen production target was compiled exactly once:

```text
[1/1] CL build\base\source\items\items.obj
```

- Full Ninja build: pass.
  - 470 semantic units scanned;
  - 4,472 functions evaluated;
  - 4,349 semantic-exact and 4,377 accepted-exact functions;
  - zero unit errors;
  - campaign progress: 377/833 objects, 4,350/11,060 functions,
    520,613/2,198,102 code bytes;
  - report SHA-256:
    `D20BE39B180132E3BA1784A83DC4FB296357441BC2DD8262942C91015FF56BE8`;
  - semantic report SHA-256:
    `03A96729026516BDB5E4A894CEC71EA6B507F5FDA7E0CE77310598D98C25AE26`.
- `python -m tools.audit_object_admission`: pass, 0 candidates,
  0 contradicted, 0 revoked.
- `python -m tools.parked_functions`: pass, 3 active, 0 stale, 0 invalid.
- `python -m pytest -q`: 194 passed.
- Policy scan and `git diff --check`: pass; only the repository's normal
  line-ending notice was emitted.
