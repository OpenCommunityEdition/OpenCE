# `real_math.obj` global typed-boundary audit (2026-08-30)

## Result

`source/math/real_math.obj` remains `NonMatching` at **79/84 strict-exact
functions**. The five legal residual bodies own 2,896 target padded bytes. No
production source, header, configuration, object, admission, semantic, or park
entry changed in this audit, and no candidate spelling was compiled.

The authoritative production source is unchanged at SHA-256
`63eb2d9c9f19b6934d173cf0f307f5e8676e24fc25e5db9eb98cd0ee5f3a3f9c`.
Two clean forced baseline compiles reproduced the same 79/84 frontier and
preserved every inherited exact owner.

| Residual owner | Target/base padded bytes | Target/base relocations | Disposition |
| --- | ---: | ---: | --- |
| `_angular_accelerate_to_position` | `576/576` | `13/13` | 12 legal scheduling bytes; historical barrier family remains prohibited |
| `_pill_intersects_triangle3d` | `832/848` | `12/12` | historical `outside[4]`/barrier spelling remains prohibited |
| `_pill_test_vector3d` | `448/464` | `9/9` | typed 2D helper/copy-out boundary remains one call byte and one padding quantum apart |
| `_point_in_triangle3d` | `432/432` | `9/9` | 20 stack-slot ModRM bytes; exact historical body uses representation casts |
| `_sphere_intersects_triangle3d` | `608/624` | `6/6` | exact-size scalar and forced-inline third-edge families remain rejected |

## Instructions and provenance read

Before the audit, the matching methodology, acceleration playbook, regression
gate doctrine, campaign-tool documentation, prior complete Real Math ledger,
and applicable repository/external `CLAUDE.md` and `AGENTS.md` instructions
were read. Matrix Math was treated strictly as read-only provenance: no
Matrix Math source, object, build edge, or candidate was edited or compiled.

The prior safe-owner ledger is still authoritative for the semantic bodies and
closed experiment families:

- `docs/object_matching_logs/real_math_obj_jonas_safe_owner_restoration_20260826.md`;
- authenticated HCEA commit `570c83fd9c365dad6f2a3e7041705d5b84c7847c`;
- authenticated Stian corpus commit
  `e638cf5fa17ed5308ac5c101ef65e9c69ced65f7`; and
- historical closeout commit `3b8b707e`, used as negative provenance only.

The current Fable/Claude/Matrix worktrees and their retained headers were also
checked read-only. The finalized Matrix review copy has the same Real Math
source and header hashes as this branch. Older Matrix trees contain incomplete
prototype and inline-math experiments, not a typed prefix or a closer Real Math
body.

## Global source and artifact census

The physical source census covered the shared workspace and
`C:/halo-worktrees`: **1,660 `real_math.c` files and 27 SHA-256 source groups**.
The unique dedicated Real Math worktrees, Fable/Claude copies, public-donor
mirrors, dirty files, and all ref-reachable path history were included. No new
legal source body exists beyond the current 79/84 reconstruction and the
already rejected historical families.

The registered-worktree object census covered **1,578 worktrees**. Of those,
**1,147** contained a standard `build/base/source/math/real_math.obj`; their
raw bytes formed **1,124 SHA-256 groups** because debug records encode checkout
paths. Hardened per-section comparison found no 82/84-or-better artifact and no
fully exact object.

The strongest artifact is the single historical 81/84 object at:

`C:/Users/isabe/Documents/Codex/2026-07-13/i-w/work/real-math-closeout/build/base/source/math/real_math.obj`

Its raw SHA-256 is
`79641d4bcba1160268a1993f5009a2d0339499418f7edbab7eef4b015613aab8`.
Its two additional exact owners are exactly the unsafe bodies already recorded:

- `_point_in_triangle3d` uses point/vector representation-pointer casts; and
- `_pill_test_vector3d` casts the caller's `real_vector3d *` output to
  `real_vector2d *`.

That source also carries `_ReadWriteBarrier`, forced-inline, `volatile`, and
additional representation-cast families elsewhere. The only other artifact
above the legal frontier is a single 80/84 object with the same two unsafe
owners. No artifact supplies a legal source-level control.

## Fresh typed-topology question

The only plausible reopening premise from the prior ledger was a genuinely
typed way for `_pill_test_vector3d` to normalize the first two components of
its `real_vector3d *normal` output in place while retaining the January
`_fast_normalize2d` call.

The complete source search closes that premise:

- `fast_normalize2d` has exactly one production call site, this residual;
- its public and defining parameter type is `real_vector2d *`;
- `real_vector2d` and `real_vector3d` are distinct unions;
- `real_vector3d` contains no `real_vector2d` subobject; and
- no exact same-compiler donor demonstrates an embedded typed 2D prefix or a
  legal adapter with the January one-argument call ABI.

The target disassembly proves the helper argument is the caller's output
pointer. The current legal body instead passes a true `real_vector2d` local and
copies its two normalized scalars out afterward, producing the sole 16-byte
size delta. Reaching the target with the present types therefore requires the
forbidden pointer cast. Adding an overlapping union view merely moves the same
representation pun into the type definition and is not a typed solution.
Changing the helper ABI, globally rewriting the vector type, using a raw
prefix address, or adding an intrinsic/compiler control would not be faithful
January source and would violate campaign policy.

Because the fresh premise was disproved before production mutation, the
one-candidate evidence rule correctly forbade a speculative compile. No
declaration-order, barrier, cast, forced-inline, scalar-third-edge, or
`outside[4]` family was repeated.

## Protected inventory and disposition

The forced baseline gate reports:

`exact 79, residual 5, unwritten 0`

All 79 exact function sections, target-owned data, and ordered relocations are
unchanged. `matrix_math.obj` and the protected Units subsystem were not built,
edited, or used as candidate surfaces.

Reopen Real Math only for an authoritative January local/type record, a real
same-compiler donor that embeds a true `real_vector2d` subobject in the public
3D output model, or a new defined-C control outside every family recorded here
and in the 2026-08-26 ledger.
