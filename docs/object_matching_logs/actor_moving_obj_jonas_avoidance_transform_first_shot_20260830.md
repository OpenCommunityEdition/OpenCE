# `actor_moving.obj`: avoidance-transform first-shot boundary (2026-08-30)

## Outcome

The one authenticated, readable C89 candidate for
`_actor_move_transform_avoidance_vector` is a strict miss and has been removed
in full.  The candidate and January target both have 144 padded bytes and one
ordered relocation, but 90 normalized padded bytes differ.  The mismatch is a
stable x87 operand-lifetime/code-shape boundary: January hoists each avoidance
vector component and duplicates it on the x87 stack, while VC7 compiling the
natural typed source reloads that vector component from memory for each basis
term.

There was no source permutation, retry, byte tuning, compiler-control change,
exception, or adjudication.  Production source is restored byte-for-byte to
its starting Git blob.  This ledger is the sole retained change, and
`actor_moving.obj` remains at its inherited four exact functions.

Starting commit:

- `47f4fb44f8e9a1724a570b99269a2b7014f35b53`.

## Frozen provenance and topology

- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `source/ai/actor_moving.obj` SHA-256:
  `d693781be5186ab1f618cc7765826856e679f27a40fc94802686c338c60520fa`.
- PDB/config symbol inventory SHA-256:
  `16620d6518733c0c46652aee80e1baaecdd9cbca6dc1832fe52d332ce17af821`.
- Canonical `source/ai/actor_moving.c` starting/restored Git blob:
  `75e9d2e5b4797bc9aa9f12ad3f5ca371afb2d629`.
- Canonical `source/ai/actors.h` Git blob:
  `3f1d86b7f2e0a42d454aa5d0d05d4ff86d9b3be2`.
- Clean HCEA donor commit:
  `570c83fd9c365dad6f2a3e7041705d5b84c7847c`; donor body blob
  `ba0936873775a317a5379d96b90b61e0a8118738`; checked-out file SHA-256
  `423976868d3edf4075fc3a145992edb6c4f5025bdd6d6b38239120cecc4c458f`.
- Stian corroboration commit `fc11d861`; actor-movement file blob
  `7d3d3e2c` and checked-out SHA-256
  `ddea52fa` (prefixes recorded because this source is corroboration only).
- Pastudan corroboration commit `918af885`; actor-movement file blob
  `9e908de7` and checked-out SHA-256
  `c46fbbb0` (prefixes recorded because this source is corroboration only).
- VC7 compiler SHA-256:
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.

Repository-wide history search found the canonical declaration but no earlier
production implementation or candidate for this function.  HCEA supplies the
ordinary named-field source.  Its `vector_avoidance_data` definition and the
canonical typed owner independently agree on `forward`, `left`, and `up` at
offsets `0x18`, `0x24`, and `0x30`.  Stian and Pastudan independently
corroborate the transform semantics.  January remains authoritative for the
ABI, code extent, relocation, and byte identity.

The frozen signature is:

```c
void actor_move_transform_avoidance_vector(
	struct vector_avoidance_data *avoidance_data,
	real_vector3d const *avoidance_vector,
	real_vector3d *direction_vector);
```

The target is at January RVA `0x19A10`.  It initializes the destination from
`global_zero_vector3d`, then adds the three `forward`, three `left`, and three
`up` basis contributions.  The canonical header already provides all named
fields and the exact prototype, so no declaration or layout edit was needed.

## Sole production candidate

The only candidate was a direct typed transcription of the authenticated HCEA
body: assign `*global_zero_vector3d`, then perform the nine named-field
multiply-adds in forward/left/up order, followed by the house-required explicit
`return;`.  Each parameter was on its own line.  The body was ordinary C89 and
contained no assembly, `volatile`, `register`, pragma, intrinsic, attribute,
barrier, raw address or offset access, pointer/integer reconstruction, cast or
union pun, undefined behavior, synthetic anchor, or byte-forcing form.

One production compile emitted the following strict packet:

| Packet | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| January target | 135 | 144 | 1 | `93d71f00d607670a2982916b90db0fd8e0fc20199745b031acc88de3aabb7cf9` |
| Sole candidate | 139 | 144 | 1 | `66196aa32906b028182afbcf67e95634ef8cc4a6a5828e90d02ff3770e6ec5f6` |

The candidate raw object SHA-256 was
`f0a4befc839564306dff0be14a67f0e20c66be1f1f68776663e8007afe12d6d9`.
Both packets have exactly one `IMAGE_REL_I386_DIR32` relocation at byte `+8`
to `_global_zero_vector3d`; ordered destination and address sequences match.
The hardened distance report counts 90 differing normalized padded bytes.

The initialization sequence matches through byte `+0x1F`.  January then loads
the first avoidance-vector component, pops the saved register, duplicates the
component with `fld st(0)`, and multiplies the duplicates by the three basis
fields.  The candidate instead loads a basis field first, multiplies it by the
component in memory, and reloads that component for the next basis field.  The
same distinction repeats for the remaining components.  The candidate's
natural source therefore cannot reproduce January's x87 value lifetime under
the frozen compiler and flags.

This is a fixed no-go boundary.  Reopening requires newly authenticated source
topology or an independently established ordinary-C donor that naturally
produces January's x87 schedule; byte-led expression permutations are outside
the campaign rules.

## Restoration and validation

The candidate definition was inverse-reverted completely.  After restoration,
`source/ai/actor_moving.c` hashes to its starting Git blob
`75e9d2e5b4797bc9aa9f12ad3f5ca371afb2d629`, and the rejected symbol is absent
from the rebuilt base object.  The four inherited exact siblings remain exact:

- `_actor_move_keep_moving_past_destination`: 48 padded bytes, two relocations,
  normalized SHA-256
  `00196458cf1a472a0be6f10a753144237ea5a61c12e951384f770990b9d68f74`;
- `_actor_move_animation_busy`: 64 padded bytes, three relocations, normalized
  SHA-256
  `9fcb00ab9e3bd2cd37c1df910a592601cb5464621f1eb4d320521479dd420f7f`;
- `_actor_path_has_path`: 32 padded bytes, two relocations, normalized SHA-256
  `53b10b14ff7ec9b3e1c07f2f997b14c8271ad5054136ae583232d5c749d09359`;
- `_arccosine`: 16 padded bytes, one relocation, normalized SHA-256
  `bbe139825c1f5012bc08ffc81ed8a634feef80fe809ac3c6959eee118ef7edf5`.

Final gate results after the clean baseline rebuild:

- focused actor-movement comparator: all four inherited siblings
  `all_equal: true`; rejected symbol absent; restored raw base-object SHA-256
  `6a8d16108c8a3a5999470458f028d7e8234c8842afeb40d28a7c0fd8c00237eb`;
- complete `halobetacache_build`, `libcmt_build`, `progress`, and
  `semantic_progress`: pass;
- campaign progress: 384/833 complete objects, 4,772/11,060 exact functions,
  606,426/2,198,102 code bytes, and 1,856,050/4,176,062 data bytes;
- semantic progress: 470 units and 4,918 functions evaluated, 4,801 semantic
  exact, 131 hidden exact / 78,940 hidden bytes, 4,811 accepted exact, and zero
  unit errors;
- report SHA-256:
  `b9cb28ea0bd32f8729fd7b241e83a6ab71e01b56f44350abc3796bc63ec844fb`;
  semantic-report SHA-256:
  `0f782f197d2e92d9837d8252e43b074b43bf1f2386da21a8d7211c81553ec542`;
- object admission: zero candidates, contradictions, and revocations;
- parked-function validation: 12 active, zero stale, and zero invalid;
- tooling tests: 212/212 pass (the sandbox-only pytest cache warning is
  non-functional);
- protected `Units` `_unit_preprocess_node_orientations`: strict exact at
  1,920 padded bytes, 87 ordered relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`;
- hardened comparator SHA-256:
  `d926e41b4d1b8eedd6ad8813615c32da452dcd8e95ad7fd3fa87ac5d82fe11a5`;
- `git diff --check`: clean.

No push is performed.
