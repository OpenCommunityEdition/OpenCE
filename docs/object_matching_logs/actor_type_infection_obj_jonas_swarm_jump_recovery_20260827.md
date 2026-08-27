# `actor_type_infection.obj` swarm jump recovery (2026-08-27)

## Outcome

`source/ai/actor_type_infection.obj` remains `NonMatching`, but its previously
absent `_infection_swarm_aim_jump` body is now recovered in ordinary typed C.
The object now emits all five January target functions. Three remain strict
exact, and the new routine is a same-relocation near match rather than an
admission.

The retained change also corrects the public actor-type callback from three
opaque pointers to the proven ABI:

```c
long actor_index,
long unit_index,
float jump_magnitude,
union real_vector3d *jump_velocity
```

The dispatcher `_actor_type_swarm_aim_jump` remains strict exact after that
type correction.

## Scope and provenance

Tracked source scope is limited to:

- `source/ai/actor_type_infection.c`;
- `source/ai/actor_types.c`;
- `source/ai/actor_types.h`;
- this ledger.

The reconstruction was frozen from four independent evidence classes:

1. January split COFF `build/split/source/ai/actor_type_infection.obj` and its
   complete 688-byte `_infection_swarm_aim_jump` COMDAT;
2. relocation-annotated January x86 disassembly for RVA `0x00028290`;
3. the typed HaloCEA reconstruction at
   `C:\Users\isabe\Documents\Codex\2026-07-13\i-w\research-cache\halocea-cseries-20260820\src\blam\ai\infection_swarm_aim_jump.c`;
4. existing in-tree actor, swarm, component, prop, unit, object, math, and
   callback layouts and typed access macros.

The Stian Claude/agent recovery and tool documentation under
`build/audit/refs/stian-halo` was audited for source-recovery, call-site,
compiler, and verification guidance. It targets build 2276 and therefore was
used as methodology, not as January build-2342 byte evidence. The HaloCEA
routine was the only public donor with the complete behavior; its raw object
header access and vector pointer casts were not copied.

Target raw object SHA-256 is
`e4e984644dcb3ed6fc72ebdc73005321c6b17596b6f7e7f34d29bfefa0ed7a6d`.
The retained rebuilt object SHA-256 is
`278b690b33a28d9cf9a0d13f6a5030cf8b62a71912e3f6c9e47a42c493855a58`
and is preserved at
`build/audit/actor_type_infection_swarm_jump_retained_20260827.obj`.

## Recovered behavior

The routine now:

- obtains the actor, swarm, unit, component, and prop through typed subsystem
  accessors;
- finds the requested unit in the swarm's unit/component pair arrays;
- clamps ballistic jump magnitude with the January `MAX` semantics;
- invokes `projectile_aim_ballistic` with the proven fourteen-argument ABI;
- normalizes the horizontal aim, falling back to actor facing and then the
  global forward vector;
- clamps non-flying vertical launch velocity to `0.075f`;
- limits the resulting 3D velocity magnitude without an external helper call;
- handles targeted obey-jump fallback from the unit's forward/up vectors; and
- clears the component's pending-jump flag on the obey path.

The local `projectile_aim_direction` gives the ballistic output a legal typed
horizontal view. It avoids the donor's cast from `real_vector3d *` to
`real_vector2d *`. Unit access uses `unit_get`, which naturally reproduces the
target `_object_get_and_verify_type` relocation and mask `3`.

## Frozen candidate waves

All hashes below cover relocation-normalized padded function bytes.

| Wave | Candidate padded / relocs / SHA-256 | Evidence and disposition |
| --- | --- | --- |
| baseline | absent | January target was 688 bytes and 22 relocations |
| typed first shot | `640 / 23 / a7a6e6fcb3eec745...` | recovered behavior; exposed a duplicated empty-swarm check, external `_limit3d`, and non-January clamp topology |
| target-shape correction | `704 / 22 / 5875b1a56ed5b576...` | removed the duplicate guard, used `MAX`, and expanded the proven limiter body |
| typed helper topology | `704 / 22 / ceb8bb43749a44bc...` | matched the target instruction stream except the legal vector-copy boundary; helper emitted an unwanted candidate-only COMDAT and was reverted |
| retained direct typed form | `704 / 22 / ceb8bb43749a44bc50ee47c84b8698ed24fcdc6969ac69d7aa14b840e71697e6` | retained; no pointer/union pun or synthetic codegen device |

The first-shot raw object is
`build/audit/actor_type_infection_swarm_jump_first_shot_20260827.obj` with
SHA-256
`821d585c3a9be8b1cf9eb7371b5cf2bbd052255e1b0ba7c66f67a8995f655816`.
The retained raw object is byte-identical to the final narrowed-scope replay.

## Strict result and fixed boundary

| Function | January padded / relocs / normalized SHA-256 | Candidate padded / relocs / normalized SHA-256 | Result |
| --- | --- | --- | --- |
| `_infection_decide_action` | `304 / 26 / eb35b33e81b16d91...` | identical | inherited exact |
| `_code_000273b0` | `96 / 5 / b4122501a9598b4c...` | identical | inherited exact |
| `_code_00027410` | `96 / 5 / 2b2b9cfbfa978783...` | identical | inherited exact |
| `_code_00027470` | `3616 / 104 / 64eacc5c1e2a60ca...` | `3360 / 103 / 8efce2e0c5b5bde1...` | prior typed support fixed point |
| `_infection_swarm_aim_jump` | `688 / 22 / 5c2698e07de29daa...` | `704 / 22 / ceb8bb43749a44bc...` | recovered, nonexact |

The target's meaningful routine is 686 bytes; the candidate is 689 bytes.
Both have the same 22 ordered relocation identities, types, and addends. The
instruction sequences match across the routine except for the actor-facing
fallback copy, after which the candidate is shifted by three bytes. The
January/HaloCEA topology copies a `real_vector3d` as one aggregate and then
normalizes its two-float prefix. Expressing that literally requires the
forbidden cross-type pointer view. Direct typed field copying is retained.

Reopen this three-byte boundary only with an original-source declaration that
provides a legal common horizontal-vector member or another ordinary typed
topology. Do not use a raw pointer cast, union overlay, inactive-union access,
`memcpy` codegen probe, qualifier/barrier, compiler flag, or object patch.

## Validation

- XDK `/Zs` parse-only checks pass for all actor-type consumers plus
  `actors.c`, `projectiles.c`, `cinematics.c`, and protected `units.c`.
- `_actor_type_swarm_aim_jump` remains exact at
  `112 / 7 / 7f29d20181c6dad1...`.
- The three inherited exact functions in `actor_type_infection.obj` remain
  exact; `_code_00027470` is unchanged from its prior typed measurement.
- Halo and libcmt build graphs pass.
- Semantic audit: 470 units, 4,399 functions evaluated, 4,254 semantic exact,
  4,314 accepted exact, and zero unit errors. The new routine increases the
  evaluated set by one but receives no exact credit.
- Campaign progress remains 377/833 objects, 4,287/11,060 functions, and
  515,333/2,198,102 code bytes.
- Halo progress remains 275/468 objects, 4,120/7,574 functions, and
  502,419/1,770,166 code bytes.
- Admission audit reports zero candidates, contradictions, and revocations.
- Parked audit reports three active, zero stale, and zero invalid entries.
- Tooling tests pass: 179/179, with only the known unwritable pytest-cache
  warning.
- `git diff --check` and the consumer-wide parse gate pass. No protected
  source, configuration, semantic exception, object status, or compiler flag
  changed.

This wave claims source recovery and ABI correctness only. It does not claim a
new exact function, data owner, or completed object.
