# Actor Combat sine/cosine owner cleanup boundary (2026-09-05)

## Verdict

**WITHHOLD the standalone cleanup.** Removing the two local identifier aliases
and duplicate ordinary `sine`/`cosine` bodies is source-authentic ownership
cleanup, but the current partial translation unit has no genuine caller that
causes the real `real_math.h` inline owners to be emitted. The one natural
compile therefore loses two inherited strict functions. No production source,
header, configuration, or build file was changed.

The untouched replay gates at 19 exact / 6 residual / 9 unwritten. The cleanup
gates at 17 exact / 6 residual / 11 unwritten. `_sine` and `_cosine`, each 16
padded bytes and zero relocations, become unwritten. Every other listed
function retains its prior verdict, and the exact emitted-symbol guard for
`_point_from_line3d` passes.

This is not a request to keep the duplicate bodies. It proves that their
removal must be coupled to a real caller reconstruction. A synthetic anchor,
false external export, dummy call, forced-inline setting, or copied wrapper
would merely replace one unsupported owner with another and remains excluded.

## One-shot source trial

The scratch manifest
`scratch/actor_combat_sine_cosine_owner_cleanup_20260905.edits.json` removes
only:

- `#define cosine cosine_inline` / `#define sine sine_inline`;
- the corresponding two `#undef` lines; and
- the two ordinary out-of-line wrapper definitions.

It leaves `tangent`, every other local inline-schedule alias, all declarations,
all gameplay bodies, and source order untouched. Each replacement matches the
normalized current source exactly once. The manifest is SHA-256
`b1cc35c3ab2fa4c737ce70ddb90a112f5b3d71d048c33ddae65ad12862168ef8`.

The historical 2026-08-26 trig-wrapper ledger correctly authenticates the
January machine bodies and cdecl/real API:

| Owner | size | relocations | normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_sine` | 16 | 0 | `27de3b5b74384f112d211e1de10bc593564ddce9b3a20ceacad4896c36351c33` |
| `_cosine` | 16 | 0 | `5bc95755d7aff66043725a1d5d6814017dd7de095dd63db1a7d7ccad88b8c6e0` |

What is no longer accepted is its ownership mechanism: macro-renaming the
actual `real_math.h` definitions and then writing duplicate external wrappers.
The owner header already contains the natural typed definitions. The cleanup
trial asks only whether the present source's real calls retain those owners.
The answer is no.

## Complete frozen-object comparison

`scratch/actor_combat_sine_cosine_owner_cleanup_audit_20260905.py` compares
every defined runtime code and non-code symbol, including value, linkage,
symbol type, section name/flags, COMDAT selection, size, normalized bytes, and
ordered relocations. The baseline has 29 code and 15 non-code owners; the
candidate has 27 code and the same 15 non-code owners. The only removed owners
are `_sine` and `_cosine`. There are no added owners, no changed surviving
owners, no COMMON in either object, and no point helper.

Frozen artifacts:

| Artifact | SHA-256 |
| --- | --- |
| `scratch/actor-combat-sine-cosine-owner-before-20260905.obj` | `b8cff7f5d1659c783655634151f4d27b8f811a69b4b926851a51fe5e3a4c4c82` |
| `scratch/actor-combat-sine-cosine-owner-cleanup-20260905.obj` | `cc85dfb930feeb3bf6be328186411e129bad39f23b4ab539cb0788792f29fb99` |
| `scratch/actor_combat_sine_cosine_owner_cleanup_audit_20260905.py` | `0d2319b0f14d646f5a51a58bd04a26625cf7920009ca13985850af95d689629f` |
| `scratch/actor_combat_sine_cosine_owner_cleanup_audit_20260905.json` | `0b12d3a798871037c1f00dbdcea264de5c8ab43f72bc3d8be282be406290ff31` |

## Smallest genuine caller closure

Current `actor_combat.c` contains no direct `sine()` or `cosine()` use after
the include block. Its visible grenade routines pass the named 0.5 and
0.8660254 constants to `rotate_vector_about_axis`; they do not retain the two
inline owners.

The smallest authenticated source path is the previously paused private
`actor_start_burst(long actor_index)` at January `_code_000109a0`. It is 1,968
padded bytes, has 67 relocations, and normalized SHA-256
`cf16e6e39069e23e172999f70ec96d6b05686cf4a86f1f841587d9ee59ff0fc2`.
January/PDB evidence records it as static and gives it one real caller,
`actor_combat_update+0xB4A`. Its burst-sweep construction performs two sine
and two cosine evaluations. The later HCEA reconstruction corroborates that
algorithm, but is binary-reconstruction evidence rather than original-source
authority.

Because `actor_start_burst` is private, its 4,672-byte public parent must be
present; it must not be externalized merely to force emission. The child also
has two confirmed private dependencies:

- `_code_0000f880`, 160 bytes, `actor_combat_get_firing_parameters`; and
- `_code_00010620`, 176 bytes,
  `actor_combat_allow_special_fire_situation`.

That confirmed parent/child/private dependency closure is already 6,976
padded bytes. The parent additionally calls the still-unwritten 144-byte
special-fire enable helper, 176-byte first-burst-delay helper, and 208-byte
pause helper. Their semantic HCEA counterparts exist, but their January
linkage/owner declarations must be completed before deciding whether the
practical same-TU closure is the full 7,504 bytes. This is not a small owner
cleanup packet.

The paused handoff at `scratch/actor-start-burst-handoff-20260904.md` records
the remaining authentic prerequisites:

- correct `ai_communication_event` owner ABI; the current declaration's final
  boolean conflicts with the January seven-argument information-data shape;
- owning enums for special-fire situation `strafing` (3) and combat status
  `visible` (7), rather than private magic values;
- typed vehicle-object velocity access and the full salted prop handle;
- distinct conversion behavior: the initial burst duration uses January
  `__ftol2`, while two later timer extensions use the existing `fast_ftol`;
  and
- real includes for console, tag-file, and AI-communication APIs.

The same genuine caller cluster is likely to resolve more than sine/cosine:
its natural source also exercises tangent, random-boolean, and `fast_ftol`
paths whose isolated wrapper/owner attempts were previously withheld. That
potential is a reason to reconstruct the coherent cluster, not permission to
admit any helper independently.

## Practical recommendation

Do not spend another compile on the two-line cleanup alone. Preserve the
manifest and artifacts as the negative ownership proof. Reopen this work only
as a dedicated Actor Combat parent/child wave that first authenticates all
private/public linkage and shared API declarations, then reconstructs the
complete real caller graph and performs one ordinary owner-clean gate. The
existing 2026-09-04 burst handoff is sufficient to resume that larger wave;
no gameplay fragment was written in this bounded audit.
