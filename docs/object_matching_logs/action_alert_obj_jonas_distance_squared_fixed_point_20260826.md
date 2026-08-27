# `action_alert.obj`: `distance_squared3d` wrapper fixed point (2026-08-26)

## Outcome

No production source change and no matching credit. One evidence-backed
candidate disproved the locally renamed inline-wrapper topology for
`_distance_squared3d`: the candidate emitted 48 padded bytes, while January
and every exact donor emit 64. The source was immediately reverted without a
tuning compile, the committed five-function baseline was rebuilt, and its
whole-TU manifest is green.

`action_alert.obj` therefore remains 5/12 exact functions and 177/2,120
meaningful code bytes. The retained production commits remain:

- `020449c3b3518fd6bbc1dfbe739ab3a15762befa`
  (`Match action alert geometry wrappers`);
- `5196b8e4b409c571abc6be5634d0d09bc90ac9e5`
  (`Record action alert geometry validation`).

## Frozen target and baseline

The clean committed translation unit was frozen at `5196b8e4` in:

- `build/audit/action_alert_distance_squared_baseline_20260826.json`;
- manifest SHA-256
  `f1ceaca1e7bd5c97bbac65192214909d4068cb9532af1f0cd29973938a3ec30c`.

The January target is:

- 49 meaningful / 64 padded bytes;
- zero relocations;
- normalized SHA-256
  `f42fa39f289bfa50174aa61e77b345b092df8329d46f43f88ca127227ad1e9c8`;
- split-object SHA-256
  `bb7a70aae881cdf5672ae48c3b9c5b7d0cdcc9a30a90730672f04b71fa1cab5f`.

The baseline source SHA-256 is
`0682c4b9fbf896d8434a1a8f6f78b355c2fabf20669601be3d25f64541fee93b`
and its Git blob is `8de8debb9ac7108acc7352c53e36e305f75d06f7`.

## Same-name donor census

The canonical nested body is owned by `source/math/real_math.h`:

```c
__inline real distance_squared3d(
	real_point3d const *a,
	real_point3d const *b)
{
	real_vector3d v;
	return magnitude_squared3d(vector_from_points3d(a, b, &v));
}
```

The read-only census script
`build/audit/find_action_alert_distance_donors_20260826.py` has SHA-256
`c8989f86e767ca76965766eace71c8f8a9d326d96b32545df14e219fd3a32e72`.
It found twelve rebuilt objects defining the same-name section, and hardened
comparison reports every one `all_equal: true` against the action-alert
target:

- `source/ai/action_fight.obj`;
- `source/ai/action_guard.obj`;
- `source/ai/action_search.obj`;
- `source/ai/action_uncover.obj`;
- `source/ai/actor_perception.obj`;
- `source/effects/material_effects.obj`;
- `source/game/game_engine.obj`;
- `source/objects/objects.obj`;
- `source/physics/breakable_surfaces.obj`;
- `source/physics/collisions.obj`;
- `source/physics/physics.obj`;
- `source/units/units.obj`.

Direct hardened comparisons against both `action_fight.obj` and `units.obj`
independently prove the 64 padded bytes and zero-relocation inventory. The
donors obtain the COMDAT from a real call to the canonical header inline;
none supplies an ordinary out-of-line source definition that can be
transplanted verbatim.

## Sole candidate and rejection

The candidate locally renamed `distance_squared3d` across the header include
and added the ordinary wrapper:

```c
real distance_squared3d(
	real_point3d const *a,
	real_point3d const *b)
{
	return distance_squared3d_inline(a, b);
}
```

This is ordinary C89 and obeys all formatting and return rules. Exactly one
normal VC7 compile was spent. The preserved first candidate is:

- `build/audit/action_alert_distance_squared_first_current_tu_20260826.obj`;
- raw SHA-256
  `c20dfa1ae881e1fae04d85b0a1f8e8911c837ece78b17bc33f64bcac6d5734c6`.

Its `_distance_squared3d` section is:

- 48 padded bytes;
- zero relocations;
- normalized SHA-256
  `3e3811d46757035eda49d4b9374d60a716646e589c018726d37a8e0ae2e3564a`.

The wrapper therefore changes the compilation context enough for VC7 to
collapse the canonical nested operation into a shorter form. It is not the
same emission topology as the donor COMDAT. The candidate whole-TU gate also
fails closed on the additive symbol/debug inventory and records
`_distance_squared3d` in `changed_nonexact`. Direct comparison proves the five
accepted bodies themselves remained byte-exact, but the manifest failure is
independently sufficient to reject the candidate.

No second spelling, hand expansion, compiler option, object permutation,
allowlist, adjudication, exception, synthetic caller, or byte-forcing form was
compiled.

## Restoration

The candidate source was removed with `apply_patch`, returning
`source/ai/action_alert.c` byte-for-byte to the committed blob. The generated
object path was resolved inside the authoritative worktree, that single object
was deleted, and one restoration build was run. Its nondeterministic raw
SHA-256 is
`28105c5cf2ead523cf5e98dfb87afb09330164d4e5cb233f3c776167345f92b3`.

The frozen manifest then reports:

- `ok: true`;
- no failures or warnings;
- `changed_nonexact: []`;
- `_action_alert_begin`, `_action_alert_control`,
  `_vector_from_points3d`, `_magnitude_squared3d`, and
  `_real_random_range` all `still_exact`.

Direct hardened comparison of the same five functions remains
`all_equal: true`. No protected path changed, no tracked path was deleted,
and nothing was pushed.

## Reopen criterion

Do not retry the renamed-inline wrapper. Reopen `_distance_squared3d` only
with genuinely new evidence that reproduces the donor COMDAT emission context
without a synthetic caller—for example an authenticated ordinary definition,
a grounded VC7 inline/COMDAT topology explanation, or another natural
translation-unit use that makes the canonical header definition emit under
the function's real ownership. A direct ordinary copy of the canonical body
is a distinct unmeasured hypothesis, but it is not justified for another
production compile merely by the failed wrapper result.
