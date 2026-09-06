# Fable `actor_moving` independent intake — 2026-09-06

## Scope and decision

Read-only intake of the two Fable claims in `source/ai/actor_moving.c`; no source,
configuration, or build was changed and no compilation was run.

- **HOLD the two exact-match claims.** Both exact bodies are downstream of disclosed
  source-shape retries. `_actor_move_initialize` additionally brings a large,
  insufficiently owned data/COMMON packet.
- **A one-function, honestly fuzzy `_actor_aim_jump` packet is viable.** It must use
  the independently corroborated explicit three-component initialization, call the
  ordinary typed `real_math.h` helper (or spell out the independently sourced clamp),
  and make no attempt to reproduce the target's out-of-line helper relocation. It
  carries no presumption of exactness; classify the measured result under the normal
  strict-object and semantic rules.
- `_actor_move_initialize` remains held in every form in this intake.

This changes the earlier all-held recommendation only for a deliberately fuzzy,
semantics-first aim packet. It does not rehabilitate Fable's tuned exact aim body.

## Frozen inputs

Canonical tree:
`C:/Users/isabe/Documents/Codex/2026-07-13/i-w/work/rasterizer-model-wrapper-alias-20260829`

Donor tree:
`C:/halo-worktrees/fable-50k-small-families-20260904`

The donor is at `f9dd42218b3325e1b3a63d7860e5309f69fd3d8e`, parent
`96086c9c4c187dd539af13bdaa9252d6e46ad8f6`. The donor actor-moving file is
tracked and clean. Its tip blob is `286e8f1b86c91084724885c2886ef7e2a7c42784`;
its parent blob is `2959dc727ff2544d0520efd9cd585ead83789b1f`, identical to
the canonical actor-moving source.

| Artifact | Bytes | SHA-256 |
|---|---:|---|
| canonical `source/ai/actor_moving.c` | 21,742 | `12726F5E621227926457505C4A45198C63FD6FBC884C3A7FC63A47AF712507CD` |
| canonical `build/base/source/ai/actor_moving.obj` | — | `99CE0689F56803817E6EBCDDF02B521006D20F76E194B7A7F283F8AF6AF36C44` |
| frozen January `build/split/source/ai/actor_moving.obj` | 45,471 | `D693781BE5186AB1F618CC7765826856E679F27A40FC94802686C338C60520FA` |
| donor `source/ai/actor_moving.c` | 27,422 | `10699A157570893870EEB52110F1CC3EE0F24B1E6383A5C93BFCF4E0DDFC1B34` |
| donor current `build/base/source/ai/actor_moving.obj` | 12,859 | `07B8F91F9845F41368B9423F606C5FB3F573C9114C861787910128E882BCA7E4` |
| donor earliest saved `scratch/w_actor_moving.obj` | 12,829 | `2BFAAD3CC25AB300A5ABAC902C6DA13773DCD1FBC87E52EEC7B6A68365C6BB01` |
| donor final saved `scratch/ng_actor_moving.obj` | 12,844 | `94EB31CDB5B7A18BE19FEEB81156794C3E01355D581C7029357F28A7DAE03358` |
| donor `scratch/reports/actor_moving.md` | — | `FD48D50B798A99B3AC0084A1E630492D17E0285C060D92F8C8555723DA0ACC07` |
| donor `scratch/facts/actor_moving.md` | — | `2B43ADC6C8397536045297D3EAC136C60E2360303FDD62F3020348439C1CAACB` |

The canonical source defines neither claimed function. Its comment inventory names
both target symbols, and the canonical object gate was 20 exact / 0 residual / 16
unwritten before this donor packet. Thus these are unique candidates, not duplicate
implementations of already admitted functions.

## Exact function evidence

Both target and final donor symbols have external storage class 2, function type
`0x20`, and COMDAT selection 1.

| Function | Target/final padded bytes | Relocations | Normalized SHA-256 | Earliest saved state |
|---|---:|---:|---|---|
| `_actor_aim_jump` | 256 | 4 | `141b04fcbe71e0761c688c7a766b0791721fecc38d3d58eb14e6b319fa4f0e84` | already exact in `w_actor_moving.obj` |
| `_actor_move_initialize` | 336 | 15 | `4cb84b739dfc710ce3452d9c6e9819eef7c05a0a4722be7fdcbf128066c52fb2` | residual 320/15, normalized `056c9d85017bfbbbb43e5272fb3f33921007bd1712566ccbdd55762098e92695` |

The aim target relocations are `_actor_data` (DIR32), `_datum_get`,
`_actor_type_swarm_aim_jump`, and `_scale_vector3d` (REL32). The 488-byte constant
section used by initialize is byte-identical in the target, earliest saved object,
and final object; raw SHA-256
`432952c74db7f907a3692ed82f1a86aaf33ce7fea7bc6eccf464f06dd91da0b4`.

The earliest object timestamp is 2026-09-04 20:39:39 -07:00. The earliest saved
source variant, `scratch/w_actor_moving_init_A.c`, is later (20:41:43) and has
SHA-256 `046B417D67B79EA7401D8E71484549B40E70A4939EC00B69D0D193071977A304`.
It therefore cannot authenticate the source that produced the earliest object.

## Provenance adjudication

### `_actor_aim_jump`: tuned exact held; natural fuzzy viable

The donor report explicitly records two shapes: the first used three component
stores and was residual with the same 256-byte/4-relocation envelope; the second
replaced those stores with `set_real_vector3d`, whose right-to-left evaluation made
the object exact. No pre-retry source or residual object was preserved. The earliest
saved object already contains the exact second shape, and all saved initialize-source
files contain that same final aim body. That is source-shape tuning, so the exact
`set_real_vector3d` body is held.

There is, however, independent semantic support for an untuned fuzzy body:

- `C:/Users/isabe/Documents/Codex/2026-07-13/i-w/research-cache/halocea-full-blobs-20260830/src/blam/ai/actor_aim_jump.c`,
  SHA-256 `A16A5B92B46DC4EC4D35E6A6FAE5EA4F3502668FD55D42BA6949205D878BD30B`,
  uses explicit component stores for both construction and clamp.
- `C:/Users/isabe/Documents/Codex/2026-07-13/i-w/_research-stian-halo/src/halo/ai/actor_moving.c`, SHA-256
  `741A1B75881E8A3F288C3C7C3C359E1FA21CC1575A299E2D9B6FBE30433F84F7`,
  independently documents and implements explicit component construction; its
  disassembly-backed lift identifies the final in-place vector-scale call.
- Canonical typed fields already exist in `struct actor_datum`, and canonical
  `source/ai/actor_types.h` (SHA-256
  `886C1FEBA4589579D70C124F79ADC83CD8053CAAB68F7CB3D3120085AC6C29A8`)
  declares the genuine four-argument `actor_type_swarm_aim_jump` API.

The viable fuzzy source should preserve the typed control flow and explicit initial
stores, then use the ordinary `scale_vector3d` declaration/inline definition already
owned by `source/math/real_math.h`. `actors.h` includes that genuine math header.
It must **not** copy Fable's `#define scale_vector3d scale_vector3d_inline` import
schedule or add a C-local external prototype merely to force the target relocation.

Canonical currently has an exact-match precedent for this mechanism, not a public
API: `actor_moving.c` macro-renames `normalize3d` while importing headers and then
declares the external spelling locally; `action_charge.c` similarly emits external
math bodies. `action_charge.c` (observed SHA-256
`E8096E28A0A821AE5F97B0461C61AE6F828F51037B0CB62B75F5B1D8FB0BE366`)
defines `scale_vector3d`, but no genuine non-inline `scale_vector3d` prototype is
published by `real_math.h`. Therefore that external relocation boundary is an
exact-object convention, not a clean dependency to extend for this fuzzy packet.

Smallest fuzzy trial packet:

1. Add only the typed natural `_actor_aim_jump` implementation to
   `source/ai/actor_moving.c`.
2. Add the genuine `ai/actor_types.h` include for
   `actor_type_swarm_aim_jump`.
3. Add the public five-argument prototype to genuine owner `source/ai/actors.h`.
4. Use normal `real_math.h` `magnitude3d`/`scale_vector3d`; add no math aliases or
   local external math prototypes.
5. Add none of Fable's initialize constants, COMMON symbols, private ray type,
   enum rename, or unused `point_from_line3d` import.

This packet should be assessed for compile/semantic/runtime correctness and recorded
as fuzzy. It is not an attempt to recover the 256/4 target object.

### `_actor_move_initialize`: held

The donor report admits five tried forms (pointer ray, explicit stores, array forms,
and the final pointer/helper arrangement). The scratch directory preserves sixteen
distinct `w_actor_moving_init_*.c` whole files. The exact final body is the S4 body;
`scratch/w_actor_moving_init_S4.c` has SHA-256
`7010441502C26A5CFFB7B75AEDA98EF1B69222AD4DFDAE66DD5EE9E28AF0D6C4`.
That retry chain is direct evidence against first-natural exact provenance.

The independent HCEA source
`C:/Users/isabe/Documents/Codex/2026-07-13/i-w/research-cache/halocea-full-blobs-20260830/src/blam/ai/actor_move_initialize.c`
(SHA-256 `CA4349CD99D3A76C8E82215C5EEA52A834278247E18EC19B0C56E22137B8BD6A`)
uses materially different loop types, explicit array stores, and expression placement;
it does not authenticate S4.

Initialize is also not a leaf. It would add fourteen new external constants around
the one existing `avoidance_ray_angles` owner, replace the complete 488-byte January
constant layout, and introduce three external COMMON definitions:

- `sense_rays`: 252 bytes
- `avoidance_rays`: 448 bytes
- `avoidance_directions`: 96 bytes

The January split object leaves those three symbols undefined because linker-common
owns their BSS addresses. Fable privately types both ray arrays as
`struct vector_avoidance_ray`; HCEA has a 0x1C `vector_avoidance_ray` reconstruction,
but separately labels `sense_ray` a non-PDB reconstruction. Exact public type/name
ownership is therefore unresolved. This remains held even as a fuzzy packet until a
separate, authenticated data-owner/schema packet exists.

## Owner and dependency closure

Neither public function currently has a declaration in `source/ai/actors.h`. A
public aim trial requires this exact owner contract:

```c
boolean actor_aim_jump(
    long actor_index,
    long unit_index,
    boolean leap,
    real jump_magnitude,
    real_vector3d *jump_velocity);
```

There is no current consumer-side duplicate to remove. At observation time,
`actors.h` had SHA-256
`411A70414C9ED5605A6AB01D14503D7E4B82F2B04BE1D12865CD6D96F45D0B8A`
and was already dirty from separately owned work; it must be frozen afresh by the
owner before any trial. A text scan finds 43 direct source/header includes of
`actors.h`, but that is only a heuristic. Root's actual Ninja dependency capture for
the owner-header trial contains 47 consumers, which is the authoritative
rebuild/preservation closure.

The frozen January callers found by scanning all canonical split COFF objects are:

- `source/units/bipeds.obj::_code_00191b00` -> `_actor_aim_jump` at relocation +323.
- `source/ai/ai.obj::_ai_initialize` -> `_actor_move_initialize` at relocation +123.

There is no current canonical C call to either function because the relevant caller
body remains unwritten/address-named. Absence of a current source caller does not
make either public function TU-local or waive its owner-header prototype.

## Reopen conditions

- Reopen exact aim only with a preserved, independently dated pre-emission source
  that authenticates the helper form before any object comparison; the existing
  Fable report cannot do so.
- Reopen initialize only after independent source provenance and a complete genuine
  owner decision for all 15 constants, three linker-common arrays, and the distinct
  sense/avoidance ray types. Do not retry source arrangements.
- The fuzzy aim packet above is the only bounded actionable candidate from this
  intake. Any target-shape aliasing, forced out-of-line math import, or addition of
  initialize data converts it back to HOLD.

## Prior ledgers consulted

- `docs/object_matching_logs/actor_moving_obj_claude_admission_audit_20260902.md`
  — `42383C489F4C6235569924491C971728DE6CA741672ECE30C8266D4B7CC50151`
- `docs/object_matching_logs/actor_moving_obj_jonas_arccosine_20260826.md`
  — `373A4D91EAA81562D63B37E5DE7824364E8410BEFDEC9FFD1463B9BC60CCC447`
- `docs/object_matching_logs/actor_moving_obj_jonas_avoidance_transform_first_shot_20260830.md`
  — `560B7CC9546AD07CC6BFED5E371ABF68AE84C9352ED752B35A9C4AEA787ECAF8`
- `docs/object_matching_logs/actor_moving_obj_jonas_claude_exact_sextet_20260831.md`
  — `78F1BB152621C94AB6CD08E3A9ACAAF57092CEAC50A52B5B907D02C2E4D46F0A`
- `docs/object_matching_logs/actor_moving_obj_jonas_destination_packet_20260831.md`
  — `99EDE54DAD1A9BB2833C6AC250A5C6DC3B42B52944C51F35A1CAD615EBD93BF7`
- `docs/object_matching_logs/actor_moving_obj_jonas_midpoint3d_first_shot_20260830.md`
  — `7BE073F2C38A9EE7CD870DC4444E453DB930BD700F49292BC36FED431C47AA54`
- `docs/object_matching_logs/actor_moving_obj_jonas_three_leaf_20260821.md`
  — `AB6432AEF81DE0D807FB935EFBD3B5F456EE9A558C33D4693C8BF0FCB51A25CD`
- `docs/object_matching_logs/fable_50k_small_families_interrupted_lane_intake_20260905.md`
  — `788F18E1B6303604E8BE6CA8F82838F0F813E59D926073C3B18D14FD7FC07125`

The old ledgers consistently admit typed first-natural work and hold compiler-shape
permutations. That policy is the basis for the exact/fuzzy distinction above.
