# `collision_usage.obj` start-timer wrapper recovery — 2026-08-27

## Outcome

This bounded evidence wave recovers `_collision_log_start_time` as a new
strict-exact function.  `collision_usage.obj` advances from 6/14 to 7/14
exact functions and remains `NonMatching`; no completion label, semantic
exception, data exception, or parked-function record changes.

The accepted function contributes 14 meaningful / 16 padded code bytes and
one relocation.  Its normalized SHA-256 is
`63bf78a6f07971df04769bae2ba18a6b90615e87a71be92f83beb002afa345bf`.
The sole relocation is an `IMAGE_REL_I386_REL32` at offset 8 to
`_QueryPerformanceCounter@4`, addend zero.

## New evidence and reopen basis

The earlier collision-usage checkpoint measured and rejected a natural
three-step implementation: query into a local `LARGE_INTEGER`, copy its
`QuadPart` into the public `__int64` output, and return.  That body compiled to
48 padded bytes against January's 16.  The checkpoint also rejected the
historical direct call because it required an incompatible
`(PLARGE_INTEGER)` cast over `__int64 *` storage.

The current exact-shape atlas supplied genuinely new evidence: January's
function shares a 16-byte, one-relocation wrapper shape with an existing exact
donor family.  Direct target disassembly then resolved the complete body:

```text
push ebp
mov  ebp, esp
mov  eax, [ebp+8]
push eax
call _QueryPerformanceCounter@4
pop  ebp
ret
```

This proves there is no local counter or copy.  The missing source fact was
the API/storage type, not an optimizer schedule.  The wave therefore reopens
on new target-independent shape evidence plus target relocation identity; it
does not retry the rejected 48-byte source family.

## Typed source closure

The retained body is ordinary typed C and follows the house format:

```c
void collision_log_start_time(
	LARGE_INTEGER *start_time)
{
	QueryPerformanceCounter(start_time);

	return;
}
```

`collision_usage.h` includes the canonical XDK `<xtl.h>` type surface and
declares the parameter as `LARGE_INTEGER *`.  The two storage owners are also
made genuinely typed rather than cast:

- `collision_bsp.c` stores its vector and sphere timers as `LARGE_INTEGER`
  and passes `.QuadPart` to `collision_log_end_time`; and
- `collision_models.c` stores `collision_model_test_vector_time` as
  `LARGE_INTEGER` and likewise passes `.QuadPart` to the end timer.

The union's `QuadPart` member is the documented XDK representation written by
`QueryPerformanceCounter`.  There is no incompatible pointer conversion,
union overlay invented by this project, pointer/integer reconstruction, raw
address, or aliasing assumption.  The two timer aggregates retain their
original eight-byte member sizes and addresses.

## Frozen build and strict comparison

The pre-wave objects were preserved with these SHA-256 identities:

- `collision_usage.obj`:
  `CC790E885BCFEBF2C71DDB55908371E8AA77C887CB81097DC1BBCBB96423A978`;
- `collision_bsp.obj`:
  `7F3713222A9A1E333E3D3FD10D868661F8781F1E14BA0397661375A096C76699`;
  and
- `collision_models.obj`:
  `4EAC62F49BA9416265157EC3853408B6EE2A28654602E8BD6BFF562E447889C3`.

All three edited translation units passed pinned XDK 3911 `/Zs` checks before
the first production build.  One normal Ninja build then compiled the complete
typed closure.  The first-shot objects have SHA-256 identities:

- `collision_usage.obj`:
  `57C6A7DE9C7ACA66CB2FA48CF948D5AA678232E07693B0260F5A15C58664EBDC`;
- `collision_bsp.obj`:
  `04089C48F8B0C8D0DF6737ACE389598945E6A1D1A6CA76BB6BFCBB41884F9ED2`;
  and
- `collision_models.obj`:
  `F27E3FA139F6F8EFF97A7278322C9B596B5CD8AFDFF14859F9D4CECE6C6D8128`.

The semantic audit rose by exactly one accepted function, from 4,255 to
4,256.  Hardened comparison proves the new body exact in padded bytes and
ordered relocation identity, address, type, destination, and addend.  The six
inherited exact `collision_usage.obj` functions remain exact.

A frozen baseline/candidate exact-set comparison also proves zero losses in
the two callers:

- all 12 previously exact `collision_bsp.obj` functions remain exact,
  including `_collision_bsp_test_sphere` and `_collision_bsp_test_vector`;
  and
- all seven previously exact `collision_models.obj` functions remain exact,
  including `_collision_model_test_vector`.

The only post-measurement change moved the public function above the private
code marker.  A retained replay reproduced the same strict function identity;
the final retained `collision_usage.obj` raw COFF SHA-256 is
`366D1B24D4AEC636A162FB0BFEEA9E096A456814F5C41C47504EC1AFF17C3060`.

No assembly, register or volatile control, pragma, intrinsic, compiler flag,
attribute, synthetic anchor, undefined behavior, byte forcing, object patch,
comparator exception, or admission exception is introduced.  Existing SDK
and project header pragmas and the unrelated pre-existing volatile local in
`collision_bsp.c` are unchanged.

## Validation and progress

- full Ninja build and progress generation: pass;
- semantic audit: 470 units, 4,342 functions evaluated, 4,196 semantic exact,
  4,256 accepted exact, and zero unit errors;
- campaign progress: 377/833 objects, 4,230/11,060 functions, and
  510,745/2,198,102 meaningful code bytes;
- Halo progress: 275/468 objects, 4,063/7,574 functions, and
  497,831/1,770,166 meaningful code bytes;
- object admission: zero candidates, zero contradictions, zero revocations;
- parked functions: three active, zero stale, zero invalid;
- tooling suite: 179/179 tests pass; and
- `git diff --check`: pass.

The January target object SHA-256 remains
`E829727ABB548E1A649425732E4C12B60A4EE50D60060FB60BB4F68EF95CB6A4`.
No protected Units/vehicles/AI/game-engine source is changed, and nothing is
pushed.
