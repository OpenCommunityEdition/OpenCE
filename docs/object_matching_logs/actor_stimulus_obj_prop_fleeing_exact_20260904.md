# Actor Stimulus friend-fleeing closure, 2026-09-04

## Result

Starting from published canonical
`a5c2c50ab1ce6aa42696cee6fc23c77e8fbaebbc`, the final unwritten Actor
Stimulus function is reconstructed as natural typed C on its first scratch
compile. `_actor_stimulus_prop_fleeing` is **strict exact: 263 meaningful /
272 padded bytes, 14 ordered relocations**, normalized SHA-256
`13df393dee019d54339f963e17edd5d85610d11ce48fe5c2e357240b22f65da1`.

The focused object advances from **20 exact / 1 residual / 1 unwritten** to
**21 exact / 1 residual / 0 unwritten**. All 20 inherited strict functions and
the existing prop-acknowledgement park remain byte/relocation-identical.
This does not complete the whole object: the acknowledgement residual and
inherited extra-helper ownership still require their separate review.

## Evidence and why the old rejection is not a new tuning campaign

The complete prior Actor Stimulus ledgers were read, including the August 29
friend-fleeing first-shot rejection and the subsequent combat, small-function,
heard-shooting, weapon-detonation and acknowledgement work. That old policy
discarded an otherwise credible 272-byte body for one scheduling difference.
Current house rules permit retaining natural fuzzy bodies, so this pass
reconstructs the source for admission without repeating its expression,
declaration-order or register-allocation experiments.

January remains authoritative. Its entire target function was independently
disassembled before writing the current body. The open HCEA function at
`src/blam/ai/actor_stimulus_prop_fleeing.c`, from the previously authenticated
`570c83fd9c365dad6f2a3e7041705d5b84c7847c` evidence set, corroborates
the semantic name and behavior, not January byte or ABI details.

The current source resolves actor, character definition, then prop in the
January order. It requires a friendly prop, the named panic-in-groups flag
and an expired cooldown; obtains the definition's friend-killed panic chance;
then accepts either the emotion helper or the global-seed probability test.
It preserves a stronger panic, starts with the actor's current target, and
only records the friend-fleeing stimulus when the observed prop has an actor,
as January actually does.

The old candidate cached the observed actor's flee-prop member before testing
its action. The retained natural short-circuit expression tests
`state.action == _actor_action_flee` first and only then accesses the flee
member of `action_data`. This directly follows January's guarded read at
`+0xC1..+0xCE`, respects the active action variant and avoids an unnecessary
cached value. It was selected for those semantic reasons before compilation,
not through an A/B spelling search. In current canonical context, this first
implementation is exact; the result is not a claim that the historical
compiler-context difference has been isolated experimentally.

When the fleeing actor has a valid flee prop, the source translates that
prop's unit through the genuine `prop_get_active_by_unit_index` API. January's
panic-type-then-prop store order is retained. The seemingly restrictive
`prop->actor_index != NONE` enclosing both stores is target-established,
not an invented branch.

## Ownership and house rules

All public declarations already belong to the existing owner headers:
`actors.h`, `props.h`, and the game-time/random owners already used by this
TU. No shared header, caller-local prototype, storage definition, compiler
flag, comparison alias or semantic exception changes. The new named panic
constant is TU-private alongside the existing panic enum values. The symbol
inventory comment is also corrected to the already-authenticated semantic
`actor_stimulus_combat` name.

The body uses `real`, typed actor/prop/tag accessors, named action/panic/flag
constants, `TEST_FLAG`, same-line initialization where natural, one parameter
per line and an explicit final return. It contains no raw offset, ambiguous
owner facade, forced inline, assembly, volatile/register device, optimizer
directive, fake branch, synthetic lifetime, emission anchor or representation
pun.

The complete owner comparison finds exactly one new code owner: the target
friend-fleeing function. Seven existing candidate-only math helpers
(`dot_product3d`, `magnitude3d`, `magnitude_squared3d`, `normalize3d`,
`scale_vector3d`, `square_root`, `vector_from_points3d`) remain unchanged;
they are not newly introduced or waived by this function-level packet.
The hard emitted-symbol guard finds no `_point_from_line3d`.

## Reproduction

The first scratch edit manifest and output are:

```text
scratch/actor-stimulus-prop-fleeing-natural-20260904.json
scratch/actor-stimulus-prop-fleeing-natural-20260904.obj
```

The actual production-body replay, without scratch includes or aliases, is:

```text
python tools/campaign/gate.py source/ai/actor_stimulus --all --forbid-emitted-symbol _point_from_line3d --out scratch/actor-stimulus-prop-fleeing-canonical-20260904.obj
```

It independently repeats **21 exact / 1 residual / 0 unwritten** and the same
full 272-byte/14-relocation strict proof. This is verification of the one
retained body, not a second source candidate. Full-build, stable-sweep and
publication evidence belongs to the admitting batch checkpoint.
