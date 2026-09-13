# Weapons historical packet reconciliation (2026-09-12)

## Scope and result

This packet audits the historically stronger `weapons.c` source blob
`45d791510` from Fable commit `f9dd42218b3325e1b3a63d7860e5309f69fd3d8e`.
That donor gates at 52 exact / 3 residual / 24 unwritten functions, while the
current translation unit started at 49 exact / 2 residual / 28 unwritten.
The three extra donor exacts are `animation_update`, `weapon_aim`, and
`weapon_place`; its additional written residual is `weapon_can_be_fired`.

Only `weapon_can_be_fired` is retained.  It is a complete, typed, ordinary-C
reconstruction supported by January's control flow and the independent HCEA
semantic reconstruction.  It remains a residual and receives **zero exact
credit**.  The current gate is 49 exact / 3 residual / 27 unwritten: no
inherited exact function changed.

## Retained fuzzy owner

`weapon_can_be_fired` uses `weapon_get`, `weapon_definition_get`,
`TAG_BLOCK_GET_ELEMENT`, project `boolean`/`real` types, and the existing public
declaration in `items/weapons.h`.  `weapons.c` now includes the genuine
`game/game_engine.h` owner and removes its consumer-local declaration of
`game_engine_running`.

The retained source follows January's observable decision sequence: reject a
fully aged weapon; allow firing when the game engine is inactive, no magazine
exists, or the first magazine has no positive loaded-round capacity; otherwise
allow firing only while loaded or reserve rounds remain.  Unlike the historical
donor's combined condition, the retained sequential form reflects the target's
early exits and is independently corroborated by HCEA.  It is still an honest
control-flow/code-generation residual:

| Measurement | January | Retained candidate |
| --- | ---: | ---: |
| Meaningful bytes | 132 | 137 |
| Padded bytes | 144 | 144 |
| Relocations | 5 | 5 |
| Normalized SHA-256 | `ba8b3806624808498c9815871e5b6e94428c0dac363ca1543591f6530687cedc` | `a0706edaa4f925be59bbbd739403e62a5a21407645eaeea1670d7dc4c38ff4e3` |
| Objdiff similarity |  | 84.69% |

The relocation destinations and order agree; the tag-block call moves from
`+0x5c` to `+0x62` because VC7 lays out the shared true-return block earlier.
No further branch-spelling search is authorized without new original-source or
same-compiler evidence.

## Rejected or withheld historical exacts

- `animation_update` (27 meaningful / 32 padded) is byte-exact as a short
  passthrough, but January has no incoming relocation and leaves EAX as callee
  residue.  Credible cross-build evidence supports both `short` and `void`.
  Exact emission cannot resolve the ABI, so no guessed public owner is added.
- `weapon_aim` (257 meaningful / 272 padded) has a credible exact typed body,
  but the public declaration belongs in `weapons.h`.  The prior coherent
  owner-header packet regressed the protected 1,920-byte Units sentinel.  This
  audit does not evade that boundary with a local prototype or an invented
  narrow header.
- `weapon_place` (193 meaningful / 208 padded) is rejected in the historical
  exact form.  The object-type callback and all sibling placement callbacks are
  `void`; the donor changes it to `long`, returns `weapon_index` only to retain
  EAX, uses a surrogate scenario layout, and private renamed flag values.  That
  is fake ABI matching.  A future ordinary-void reconstruction must use the
  genuine scenario weapon record and correct owner declaration and will receive
  no exact credit unless it independently closes.

## Verification

The required whole-TU gate was run before and after the packet:

```text
before: 49 exact / 2 residual / 28 unwritten
after:  49 exact / 3 residual / 27 unwritten
```

Both runs passed the `_point_from_line3d` emitted-symbol guard.  The final
source passes `tools/fake_match_scan.py` and `git diff --check`.  A containing
batch must rebuild `weapons.obj` before validating the parked-function ledger;
no scratch object, historical object, or donor-wide exact total is published as
canonical credit.
