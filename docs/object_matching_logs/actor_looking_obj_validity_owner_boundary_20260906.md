# ActorLooking validity owner boundary, 2026-09-06

## Decision

**HOLD.** The natural `source/ai/actor_looking.c` validity graph is credible,
but it is not a header-free packet. Its public root needs the shared
`actor_mode` domain, and the first complete-owner trial regressed a protected
Units function. The entire owner trial was restored. No ActorLooking body was
compiled or admitted, and no exact-byte gain is claimed.

The proposed graph was:

| January symbol | Recovered identity | meaningful / padded bytes |
|---|---|---:|
| `_code_000163e0` | private `actor_look_valid_aim_vector` | 124 / 128 |
| `_code_00016460` | private `actor_look_valid_look_vector` | 233 / 240 |
| `_actor_looking_test_validity` | public root | 161 / 176 |

Total: 518 meaningful / 544 padded bytes. The frozen Fable intake remained at
2 exact, 0 residual, and 14 unwritten functions; its report contains no trial
result.

## Authentic graph and ABI evidence

January authority is
`build/split/source/ai/actor_looking.obj`, SHA-256
`443e269f49461dcaf5ff3804fc8770cc5579798427289c60f56a1ce533a226de`.
The three sections form a genuine same-TU graph. The public root obtains the
actor and its definition through the existing datum/tag APIs, then calls only
the two private helpers. The aim helper otherwise references literals; the
look helper otherwise references literals and `_normalize2d`. The existing
AI Debug caller is `source/ai/ai_debug.c:5253`, and the genuine public
declaration is already in the `ACTOR_LOOKING.C` region of
`source/ai/actors.h`:

```c
void actor_looking_test_validity(
	long actor_index,
	real_vector3d const *vector,
	boolean *aiming_valid,
	boolean *looking_valid);
```

Direct HCEX DIA evidence gives the private parameter widths and arities as:

- `static unsigned char aim(float, real_vector3d *, real_vector3d *)`;
- `static unsigned char look(float, float *, real_vector3d *,
  real_vector3d *, real_vector3d *)`.

The ordinary source therefore needs three-dimensional formal inputs and
typed `real_vector2d` locals (`looking_vector2d` and `aiming_vector2d`) for
the planar calculation. The short HCEA reconstruction's two-dimensional
formal casts are not admissible source evidence and must not be copied.

January instructions independently establish the fields used by the public
root:

| record | offset | field |
|---|---:|---|
| actor meta | `0x58` | definition index |
| actor state | `0x6a` | mode |
| actor input | `0x174` | facing vector |
| actor input | `0x180` | aiming vector |
| actor definition looking block | `0xb4`, `0xb8` | noncombat angular limits |
| actor definition looking block | `0xbc`, `0xc0` | combat angular limits |
| actor definition runtime | `0x12c` | maximum aiming cosine |
| actor definition runtime | `0x134` | maximum looking cosine |

These are field accesses through the current real actor and actor-definition
owners, not a proposal for shadow layouts or raw-offset access.

Pinned natural-source corroboration:

The `_research_cache` paths below are relative to
`C:/Users/isabe/Documents/Codex/2026-07-13/i-w`, not this canonical worktree.

| file | SHA-256 |
|---|---|
| `_research_cache/halocea/src/blam/ai/actor_look_valid_aim_vector.c` | `4fce8f5a792e2e4d455db12f0ad0239826d84512ee989b6862bfc4b6590c6899` |
| `_research_cache/halocea/src/blam/ai/actor_look_valid_look_vector.c` | `5c726ba3f70e1aa479d5d0a88678dbe2e776d25c1fddc8e21ef1b6c527c12c94` |
| `_research_cache/halocea/src/blam/ai/actor_looking_test_validity.c` | `c6a59c54768783ab27f4de0da97d79369ecaf16ea40169acb5461d65d87fb932` |

The disassembly is reproducible with:

```powershell
python tools/audit/dump_coff_disasm.py build/split/source/ai/actor_looking.obj _code_000163e0
python tools/audit/dump_coff_disasm.py build/split/source/ai/actor_looking.obj _code_00016460
python tools/audit/dump_coff_disasm.py build/split/source/ai/actor_looking.obj _actor_looking_test_validity
```

## Required shared owner

The public root selects combat versus noncombat limits using
`_actor_mode_combat`. At intake time that constant was not owned by a header:
partial or overlapping C-local definitions existed in `actors.c`,
`actor_stimulus.c`, `actor_type_flood.c`, and `ai_communication.c`, while
`actions.c` separately owned `NUMBER_OF_ACTOR_MODES`. Adding another local
constant to ActorLooking would perpetuate incompatible ownership.

The authenticated complete domain is:

```c
enum
{
	_actor_mode_braindead = 0,
	_actor_mode_asleep = 1,
	_actor_mode_alert = 2,
	_actor_mode_combat = 3,
	NUMBER_OF_ACTOR_MODES = 4,
};
```

The genuine owner is `source/ai/actors.h`. The bounded trial added that one
complete domain there and removed the five overlapping C-local definitions
or constants from `source/ai/actors.c`, `source/ai/actor_stimulus.c`,
`source/ai/actor_type_flood.c`, `source/ai/ai_communication.c`, and
`source/ai/actions.c`. The later-source owner corroboration is
`_research_cache/halocea/src/headers/actor_mode.h`, SHA-256
`50842064859a42947ae4696b37587a1fb343e9be508c4a46bcd6751b2535e268`;
January/HCEX evidence remains authoritative for the values. Direct root DIA
queries independently returned all four named modes and the count with values
0, 1, 2, 3 and 4 as an anonymous enum. The actual trial used that anonymous
declaration, not the reconstructed friendly `actor_mode` tag.

## First-owner trial and restoration

The owner-only trial rebuilt the actual 47-consumer closure. It changed no
ActorLooking implementation, yet strict equality fell from 6,261 to 6,260:
`source/units/units::_unit_preprocess_node_orientations` lost its protected
match (1,919 meaningful / 1,920 padded bytes). This is a real frontend/header
blast boundary, not permission to retry declaration placement, qualifiers, or
partial local enums. The six-file owner change was immediately inverse-patched.

The initial compile exposed the existing duplicate count in `actions.c`;
removing that duplicate was an ownership correction, not a source-shape retry.
Its unchanged source was separately frozen before editing as
`scratch/actor-looking-validity-actions-before-20260906.c`, SHA-256
`df430c426ffacf6c7872bc9b8a6b45f325950c7fe2e79477930d39c9673c5975`.
Each capture manifest records the capture script version used for that phase.

Restoration is complete:

- strict total returned to 6,261 with zero gains and zero losses;
- all 3,165 runtime records across all 47 consumers are unchanged;
- no new `_point_from_line3d` definition or reference appeared;
- no ActorLooking validity implementation was compiled.

| frozen artifact | SHA-256 |
|---|---|
| `scratch/actor-looking-validity-before-20260906.json` | `0d4d5d9de0ee45ad15413899b0155c29eac2be760b53093a9f8b22c77552847c` |
| `scratch/actor-looking-validity-owner-20260906.json` | `b4fcfc33e618ff2035aee933b80906e5db232de467d1577bf27f00cfcea0f6b3` |
| `scratch/actor-looking-validity-restored-20260906.json` | `79c6f47f0ebaa45c24a448415602cbd49448185d37c2cf193a166136a9d2c3ee` |
| before stable inventory | `b13c8826c3323d4a472bf46c94af12b818cc7b8d67fd402e0eb13aafe200b1a6` |
| owner stable inventory | `2c4b9c1d25b9ecb230cd31367540465591b8bf52676a80fd0faf8192f0ff1f5b` |
| restored stable inventory | `b13c8826c3323d4a472bf46c94af12b818cc7b8d67fd402e0eb13aafe200b1a6` |

The independent restored runtime comparison is
`scratch/actor-looking-validity-restored-runtime-20260906/summary.json`; it
reports `unit_count: 47`, `section_classification_totals: {unchanged: 3165}`,
and no review-required units.

Fable provenance is frozen in
`C:/halo-worktrees/fable-50k-small-families-20260904/scratch/facts/actor_looking.md`
(SHA-256 `b60d1151c0f57ea71c21077ae11258181068016b2f340c7b34d8016819ff1ed3`)
and the corresponding report (SHA-256
`cc2e4593ca88ccf411d66fa5d9c045cb4fd0198ac94b3aebb3ba0d8bcf3ba0d5`).

## Reopen condition

Do not admit any of the three validity bodies independently by duplicating
`_actor_mode_combat` locally. Reopen only with a campaign-approved way to own
the complete actor-mode domain without losing the protected Units section, or
with a broader policy decision that explicitly accepts that source-correct
collateral. Any later body proposal must retain the real private signatures,
typed 2D locals, same-TU private linkage, and the existing public prototype.
