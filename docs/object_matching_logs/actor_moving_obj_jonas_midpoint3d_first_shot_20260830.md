# `actor_moving.obj` midpoint3d first-shot boundary (2026-08-30)

## Outcome

The sole natural candidate for `source/ai/actor_moving.obj::_midpoint3d` is a
strict miss and has been removed in full.  Target and candidate are both 56
meaningful / 64 padded bytes and have the same three ordered relocations at
`+0x12`, `+0x20`, and `+0x2F` to `__real@3f000000`.  VC7 selected a different
three-pointer register assignment for the donor's `void` ABI, so the complete
padded bytes are not equal.

There was no return-type retry, expression permutation, declaration change,
compiler control, or byte tuning.  Production source and the baseline object
were restored exactly; only this additive boundary ledger is retained.

## Frozen evidence

January's split `source/ai/actor_moving.obj` has SHA-256
`d693781be5186ab1f618cc7765826856e679f27a40fc94802686c338c60520fa`.
Its target packet has normalized SHA-256
`712499e7f9fc6e985d3e3ddfe0b03e70d4a8e59c6eba96567b5cc281a52bc963`.
January fixes the three ordinary component calculations, constant-owner
identity, padded extent, and this register schedule:

```text
ECX = p0
EDX = p1
EAX = result
result[0..2] = (p0[0..2] + p1[0..2]) * 0.5f
```

Claude's independently recovered later source at
`C:/halo-worktrees/claude-ai-conversation-2/src/halo/ai/actor_moving.c`
provides the same three calculations with a public cdecl `void` signature.
The current typed math header independently proves the two const point inputs,
the point destination, and the component expressions, but spells the inline
helper as pointer-returning.  The existing BSP ledger also records historical
return-value sensitivity for the shared inline helper.  Those are material
signature tensions; January remains the only byte authority.

All applicable Claude campaign, ABI, compiler, prototype-inference, and
verification documents and all three existing actor-movement object ledgers
were reviewed before mutation.  Repository-wide ledger and history searches
found no earlier candidate for this function.  The four inherited exact
functions were frozen as regression sentinels.

## Sole production candidate

The candidate locally renamed the inline helper while including the existing
typed headers and emitted the authenticated later-source ABI as readable C89:

```c
void midpoint3d(
	real_point3d const *p0,
	real_point3d const *p1,
	real_point3d *result)
{
	result->x = (p0->x + p1->x) * 0.5f;
	result->y = (p0->y + p1->y) * 0.5f;
	result->z = (p0->z + p1->z) * 0.5f;

	return;
}
```

It contained no assembly, volatile/register/pragma/intrinsic/attribute,
compiler control, raw address or raw offset access, pointer reconstruction,
pun, undefined behavior, byte forcing, storage owner, or comparator exception.
Exactly one production VC7 object was emitted.

The candidate normalized SHA-256 is
`17f7556757e387cf1c3762ce25bcd2c85406d641b28af1478b2292fd42c1e808`;
its raw object SHA-256 is
`806760e0e64cd9ea3abebf18e605dac98eb1e85e4de7685161dc712360bbef10`.
It assigns `EAX=p0`, `ECX=p1`, and `EDX=result`.  All arithmetic instructions,
constant-relocation offsets, and operation order otherwise correspond, but the
different registers change each parameter load and memory operand.  Hardened
comparison therefore reports `all_equal: false`.

The four inherited functions remain independently direct-exact in the sole
candidate object:

- `_actor_move_keep_moving_past_destination`;
- `_actor_move_animation_busy`;
- `_actor_path_has_path`;
- `_arccosine`.

## Boundary and restoration

The target leaving the result pointer in `EAX` is consistent with a
pointer-returning ABI, while the authenticated later donor is `void`.  Changing
the return type after observing that register allocation would be byte-led
tuning, so it was not attempted.  Reopen only with newly preserved January or
independent ordinary same-compiler source that fixes the out-of-line return ABI
before compilation; do not retry from this mismatch.

The candidate body and local macro rename were inverse-reverted.  The restored
source is exactly Git blob
`75e9d2e5b4797bc9aa9f12ad3f5ca371afb2d629`, with an empty scoped diff.  The
preserved 1,342-byte baseline object was restored with SHA-256
`fbcabdb5d2562efb80d0617a249ef0cf3357a8835df67e06d3955bfb6871ae16`.
