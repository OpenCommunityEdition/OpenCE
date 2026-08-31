# `player_rumble.obj` symbol-backed `fast_ftol` exact-source boundary

## Disposition

All 12 target functions match for 1,385 meaningful / 1,488 padded code bytes,
and all 68 target data bytes match. There are no residual or target-only
functions. The recovered source is retained, but the object remains
`NonMatching`: its candidate-only `_fast_ftol` COMDAT fails the required
ordinary-link ownership proof described below.

This is not admitted from byte equality alone. The retained mixer has coherent
rumble semantics, its private name is recovered independently, and its only
inline assembly comes from an authenticated original math helper. No inert
branch, representation trick, volatile lifetime, compiler barrier, raw byte
emission, or nonsensical logic is present.

## Name and source provenance

- The January linker-map atlas records `_rumble_calculate` at image address
  `0x4A91A0` in `player_rumble.obj`, tier `exact`, in
  `../../research/halo-symbol-atlas/symbols/halo_ce/4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520.jsonl`.
  Multiple other Halo CE builds independently record the same private name,
  commonly with its EAX/fastcall decoration.
- `rumble_globals` is a descriptive private-global name corroborated by the
  Stian recovery knowledge base and cross-build `player_rumble.c`; it is not
  presented as a January PDB recovery. The allocation size and typed layout are
  independently constrained by the exact code: four 0x208-byte player slots
  followed by the three scripted `real` values at offsets 0x820, 0x824, and
  0x828.
- The historical project source at commit `5093ac1a167b3443eef9a77facdf5f3ee40fd422`
  defines `fast_ftol` in `source/cseries/cseries.h` as an explicitly inline
  `fld`/dword-`fistp` helper. HCEX identifies the same function in `cseries.h`
  and records it as explicitly inline and inlined. January's standalone
  `_fast_ftol` in `actor_combat.obj` supplies a binary positive control.
- The owner explicitly permits sparse inline assembly in helper/math
  functions. The helper is therefore restored once in its authenticated owner
  header, not copied as a player-rumble-specific codegen shim.

## Semantic-authenticity review

`rumble_calculate` starts with the player's continuous left/right values,
accumulates eight two-motor impulse definitions through the shared transition
function, adds the optional scripted contribution, clamps each motor to the
unsigned 16-bit hardware range, and converts each clamped value with
`fast_ftol`. The control flow, field offsets, loop bounds, and call sequence all
agree with the January function. The function is meaningful for ordinary input
states and does not contain an exact-only artifact.

The two former C casts were the only structural mismatch. VC7 lowered them to
`__ftol2` calls and seven excess relocations. The authenticated helper naturally
expands to January's two `fld`/dword-`fistp` pairs and removes those relocations.
This is evidence for the original source primitive, not an assembly
transcription of the target function.

## Exact inventory

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_rumble_initialize` | 32 | 3 | `103f515e7a431a7ff29ac132ed9b28df34f2cbee70b2152ffe5374417e1b84c2` |
| `_rumble_dispose` | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_rumble_initialize_for_new_map` | 32 | 2 | `6e7854da0f060763b7b573883e141cdc46428181bd3fb7396ef365857ceb2087` |
| `_rumble_player_set_scripted_values` | 32 | 1 | `c4ca59505929dace67b5cab00df410b88a194781127034926b1863ed5aa3619d` |
| `_rumble_player_set_scale` | 32 | 1 | `0d5ee8da441c7e93ebadf3bfca1d875e9cda9ef2300ebb81dad5729a87013755` |
| `_rumble_player_impulse` | 368 | 6 | `d8b80f2d254f157797593b7ff45e8fc14ce1239aec7aa6631eee02658dc5bb5f` |
| `_rumble_player_clear` | 48 | 2 | `35905262f46c8677a95ffc3e5355f6c6e80eeeb9a3cce5c1c21f4a7936a9475a` |
| `_rumble_clear_all_now` | 64 | 4 | `11332269e61bc046ce60fdc0843897d9cb151c670953093c4d53c38d7545f6d7` |
| `_rumble_player_continuous` | 48 | 1 | `5275e297797627258ecc3248dfaa053afac15c898f17366ecc197053bf047401` |
| `_rumble_dispose_from_old_map` | 96 | 5 | `cb49037ef265ad9a384bb66ce5d38beff52f48591f0545412d9cfab2b5e0f018` |
| `_rumble_calculate` | 416 | 12 | `ee97f132ea073644aada4775c0d5f96149754b0ad66a2926fb458e4ae4b30b7e` |
| `_rumble_update` | 304 | 16 | `0149be7a3b1ffa37395a640eec09e8d2fb6f2587ec69ef3aa3e5572b712efd07` |

The rebuilt TU also emits a candidate-only `_fast_ftol` COMDAT because the
header helper was expanded here. It is a 32-byte, zero-relocation code section
with flags `0x60501020`, COMDAT selection 2 (`IMAGE_COMDAT_SELECT_ANY`), and
normalized SHA-256
`8d76b4470cb80ce310b0c4d5255470b182cfa0a78d93b9df3cdcb6f56678f248`.
No relocation in any current rebuilt object refers to this out-of-line copy.
January's selected `_fast_ftol` owner is in the split `actor_combat.obj` and
has identical bytes but COMDAT selection 1.

Those measurements satisfy the section-shape, reference, and target-owned
exactness conditions, but byte equality is not enough. XDK 3911 `Link.Exe`
was run directly on the split actor-combat owner and rebuilt player-rumble
object in both input orders with `/dll /noentry /nodefaultlib
/force:unresolved`. Both orders fail before unresolved-symbol forcing can
produce an image:

```text
player_rumble.obj : error LNK2005: _fast_ftol already defined in actor_combat.obj
actor_combat.obj : error LNK2005: _fast_ftol already defined in player_rumble.obj
fatal error LNK1169: one or more multiply defined symbols found
```

Therefore the ordinary linker does not discard this particular pairing. The
candidate COMDAT receives no target credit, and `player_rumble.c` is not marked
`Matching` despite its complete target-owned inventory.

## Shared-header regression gate

Restoring `real` and `fast_ftol` in the widely included `cseries.h` rebuilt 343
Halo translation units. The semantic report then evaluated 5,002 functions,
reported 4,879 semantic exact / 4,889 accepted exact, and reported zero unit
errors. The definition-count-sensitive
`_unit_preprocess_node_orientations` remains strict exact at 1,920 padded
bytes. The only collateral artifact was a compiler-local Shell scope-table
owner rename; its payload and both `_main`-relative relocations were re-proved
before the fail-closed snapshot was updated.

## Validation and reopen criteria

- `tools/campaign/gate.py source/game/player_rumble --all`: 12 exact, zero
  residual, zero unwritten.
- Scoped fake-match scan: zero findings in `player_rumble.c`/`.h`; the only
  relevant repository finding is the reviewed `cseries.h` helper assembly.
- Object-admission audit deliberately reports one unlabelled candidate
  (`player_rumble`) with zero target function/data gap; the documented linker
  ownership failure prevents admission. The unrelated already-revoked
  `action_converse` label remains.
- Park validator: 24 active, zero stale, zero invalid.

Reopen admission only when an ordinary compatible build supplies a discardable
canonical `_fast_ftol` owner, or authentic source topology prevents this TU's
out-of-line copy without compiler steering. Stronger first-party evidence may
also revise a type, private/global name, or helper placement. A future exact
result from implausible or semantically invalid source must be rejected even if
its bytes compare equal.
