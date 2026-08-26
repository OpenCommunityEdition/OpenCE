# `lightning.obj` marker-offset helper fixed point (2026-08-26)

## Scope and result

This bounded wave opened the previously undefined private
`_code_00124c70`, identified from two cross-build donors and January call-site
evidence as the marker-position jitter helper. The reconstruction recovers the
complete behavior, exact private register ABI, exact padded size, and all 22
relocations, but misses January by two independent instruction-scheduling
swaps. No production source or semantic-match metadata is retained.

`source/objects/widgets/lightning.obj` therefore remains 7/9 strict-exact
functions. The seven inherited lifecycle functions remain strict in the first
candidate; `_lightning_submit` remains absent from accepted source.

## Frozen baseline and provenance

- cumulative baseline commit:
  `4864a2a2593357956fbcbad81c824262246dc003`
- January target object SHA-256:
  `85AB5029070191A1C1B22FA398E56EC71932EB7D96E34B099C68DEB50EA3058F`
- cumulative baseline object SHA-256:
  `C88489E7AC87D5EB17DFB3D32D764668948E68BE9EA7AC67B1BFDE37A541E69C`
- frozen baseline source SHA-256:
  `187D6D24EBBC9833C3900858FA30C1DC797C3CEC7E889F0222DC1E267E0F0F16`
- clean pre-wave regression manifest:
  `outputs/lightning_offset_helper_evidence_20260826/regression-baseline.json`

Relevant source and tooling evidence:

- `research-cache/halocea-cseries-20260820/src/blam/objects/widgets/lightning_offset_marker_position.c`
- `research-cache/halocea-cseries-20260820/src/blam/objects/widgets/lightning_submit.c`
- `research-cache/stian-halo-cseries-20260820/src/halo/objects/objects.c`
- `research-cache/stian-halo-cseries-20260820/CLAUDE.md`
- `research-cache/stian-halo-cseries-20260820/AGENTS.md`
- `research-cache/stian-halo-cseries-20260820/docs/vc71-byte-accuracy-playbook.md`
- `docs/matching_methodology.md`
- `docs/exact_match_acceleration_playbook.md`

The later builds supplied semantic roles only. January COFF remained authority
for the function signature surface, assertion literals/lines, private ABI,
padding, instruction order, relocation identity, and ownership.

## January target shape

The helper receives its three source parameters through the compiler-private
same-TU ABI:

- `ESI`: output position;
- `EBX`: marker matrix;
- `EDI`: random-position bounds.

The target asserts those pointers at source lines `0x74`, `0x75`, and `0x76`,
draws three values from the local random seed, reverses their component order,
maps each from `[0, 1]` to `[-1, 1]`, scales by the corresponding bound,
transforms the offset in place, and adds it to the position.

Target measurements:

- meaningful size: 237 bytes;
- padded size: 240 bytes;
- relocations: 22;
- normalized SHA-256:
  `2fe97798519acba821968a88f76c5a3ac64a62c86a9919327b6ed89c8599f17d`.

The 22 ordered relocations cover the three assertion literal groups, three
`get_global_local_random_seed_address` calls, three `real_seed_random` calls,
three references to `1.0f`, and the final
`matrix4x3_transform_vector` call. Both candidates reproduce every address,
type, destination, and addend.

## Bounded candidates

Both source candidates used a typed, source-backed first-marker
`lightning_submit` scaffold with the target's 4,096-element intermediate-point
array. That same-TU caller naturally reproduced the private `ESI/EBX/EDI`
surface; the helper was not compiled as an external cdecl leaf.

| Wave | Post-transform spelling | Source SHA-256 | Object SHA-256 | Helper hash |
|---|---|---|---|---|
| 1 | HaloCEA compound assignments | `5C3FEFF3B7B27006DC84F32B95EC359729AAAB90E246B2BCA69D71CD505DDB6D` | `CFA3B6D0FFBFB810F33DA49B60DD0293BE7421B4B67BA69ED718005C3D1F6A3D` | `f31a3152a95637d3f7e9f1bbada7d9c01bbcb9a88193ccf7c741c3fde5e3dbaa` |
| 2 | Stian explicit `offset + position` assignments | `CC5837114CD471F2410586D9AC020162992F48DCB743DD3F9B513D7E87780EC2` | `21CC0FDC631B3DE49C496309F1B942826E3015B52B31549F9A37A275C1800FA0` | `f31a3152a95637d3f7e9f1bbada7d9c01bbcb9a88193ccf7c741c3fde5e3dbaa` |

The two donor spellings compile identically. Relative to January, the only
instruction differences are:

1. January issues `push ebx` before the first component `fmul`; the candidate
   issues that independent `fmul` before `push ebx`.
2. January performs the first post-transform `fadd` before `add esp, 0x18`;
   the candidate performs the independent stack cleanup before the `fadd`.

All remaining instructions, padding, and relocation identities agree. The
project classifier confirms equal size and exact relocation semantics but
returns `UNKNOWN`, because its conservative scheduling STOP rule accepts only
one adjacent transposition rather than two separated transpositions. No parked
classification or match credit is claimed.

## Regression and ownership boundary

The first candidate keeps all seven inherited functions strict:

- `_lightnings_initialize`
- `_lightnings_dispose`
- `_lightnings_initialize_for_new_map`
- `_lightnings_dispose_from_old_map`
- `_lightning_new`
- `_lightning_delete`
- `_lightning_render`

The candidate emits only the two target-owned residual code functions in
addition to those inherited owners. However, neither residual is strict, so
the wave grants no code credit. Retaining the incomplete submit scaffold and
nonexact helper would expand the production translation unit without advancing
the strict campaign; both are removed.

## Do not repeat / reopen

Do not retry compound versus explicit position addition, alter assertion
spelling, compile the helper as external cdecl, or vary the 4,096-point caller
scratch topology. Those surfaces are measured: assertions, private registers,
frame size, padded size, and all relocations already agree.

Reopen `_code_00124c70` only with new provenance explaining one or both
independent scheduler choices, or after a faithful `_lightning_submit`
reconstruction changes the same-TU global scheduling context naturally. Do
not use asm, register forcing, volatile/barriers, pragmas, or undefined aliasing
to exchange the two instruction pairs.
