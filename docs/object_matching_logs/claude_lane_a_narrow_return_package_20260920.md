# The narrow-return package: approved, attempted in full, and PROVEN IMPOSSIBLE

Worktree `C:\halo-worktrees\claude-lane-a-ai-core-20260920`, branch
`claude/lane-a-ai-core-20260920`.

**OWNER RULING:** *"Approve the complete (a)+(b)+(c) package. Require full
header-consumer sweep, ABI review, and zero regressions. Do not land a partial
version."*

The package was attempted in full. **Part (c) is impossible**, so under the
owner's own instruction nothing is landed and the tree is reverted. This
document records the proof, because it closes off 8,160 bytes with certainty
rather than leaving it as an open lead.

## The package

- **(a)** `source/ai/actor_perception.c:1388`,
  `long actor_get_perception_knowledge(` -> `short`, plus the now-redundant
  `long result` / `(short)` cast simplified.
- **(b)** `source/ai/actor_perception.h:16`, the matching declaration.
- **(c)** a `short`-return spelling of `_actor_get_perception_knowledge` that
  emits `mov eax, 3` at 0x99 instead of `mov ax, 3`.

Without (c), (a)+(b) trades a byte-perfect 240-byte function for two larger
ones - which rule 1 and the standing byte-match rule both forbid.

## Header-consumer sweep - COMPLETE

Every mention of the symbol in the tree:

| site | role |
|---|---|
| `actor_perception.h:16` | the declaration |
| `actor_perception.c:1388` | the definition |
| `actor_perception.c:4264, 4337, 5127, 5224` | call, result -> `actor_visibility_at_point` |
| `actor_perception.c:6621` | call, result -> `short knowledge_type` |
| `ai_communication.c:2367` | call, result -> `actor_visibility_at_point` (the ONLY cross-TU consumer) |

Files including `actor_perception.h`: `action_charge.c`, `action_fight.c`,
`action_flee.c`, `actor_firing_position.c`, `ai_communication.c`. Only
`ai_communication.c` calls the symbol; the other four include the header without
using it.

## ABI review - CLEAN

`actor_visibility_at_point`'s eighth parameter is declared
`short perception_knowledge`, and site 6621 assigns to a `short`. **All six
consumers already narrow to `short`**, so no caller reads the high half of EAX
and the width change discards nothing anywhere.

The function's return values are `3`, `3`, `3`,
`(orphan->definitely_located != FALSE) + 2` (2 or 3), `2`, and
`actor->combat_status >= 3` (0 or 1) - range 0..3. Truncation is impossible.

## (a)+(b) measured

`source/ai/actor_perception` goes **36 -> 37 exact**:
`_actor_perception_update` (4,720) and `_prop_status_refresh` (3,680) both
become **EXACT**, and `_actor_get_perception_knowledge` (240) drops to residual
with exactly **one** real differing region:

    January  0x99  mov eax, 3      (5 bytes)
    ours     0x99  mov ax, 3       (4 bytes)

`ai_communication` and `actor_firing_position` are unchanged at 45/48 and 27/27.

## (c) is IMPOSSIBLE - two independent proofs

### 1. The return-register width is fixed by the declared return type

A standalone lab compiled with the same compiler and optimisation settings, one
function per spelling, each returning the constant 3 from two arms so the
epilogue merges exactly as the real function's does:

| spelling | constant materialisation |
|---|---|
| `short f(…) { return 3; }` | `mov ax, 3` |
| `short f(…) { return K_DEFINITE; }` (enum constant) | `mov ax, 3` |
| `short f(…) { return (long)3; }` | `mov ax, 3` |
| `short f(…) { long k = 3; return k; }` | `mov ax, 3` |
| `short f(…) { return 3u; }` | `mov ax, 3` |
| `long  f(…) { return 3; }` | **`mov eax, 3`** |
| `int   f(…) { return 3; }` | **`mov eax, 3`** |

Seven spellings; the width tracks the declared return type and nothing else.
Four further spellings were tried on the real function itself - named
enumerators throughout, a cast to `long` at each return, a block-scoped `long`
per arm, and a function-scope `long` assigned in each arm - and all four left
`mov ax, 3` (the last also collapsed the function to 160 bytes).

### 2. The two halves are mutually exclusive in one translation unit

Header `short` with definition `long` compiles, and gives the mirror image:
`_actor_get_perception_knowledge` **EXACT** while `_actor_perception_update` and
`_prop_status_refresh` both go residual. The in-TU calls bind to the
**definition**, which precedes them at line 1388.

And they precede them in January too. COMDAT emission order tracks source
order, and in **January's own object** the callee is COMDAT #2 while its callers
are #40 and #44 - the same relative order as ours (#31 against #102 and #118).
So January's definition also came before its callers, and January's in-TU calls
bound to the definition exactly as ours do.

## What this means for the law

January's `_actor_get_perception_knowledge` emits `mov eax, 3`, so **its
declared return type is wide**. Yet January's call sites are cleanup-first,
which the narrow-return law associates with a *narrow* callee. Those two facts
are both true of January and cannot both be explained by the law.

So the law is real - the census behind it is 46/46 and 4/4 in January and
46/46 and 6/6 in our build - but `_actor_perception_update` is **the one
disagreeing row in January**, and the `short` change reproduces January's caller
order by a different route while breaking the callee. It is not the mechanism
January used.

**The remaining question is what makes that one January site cleanup-first with
a wide-returning callee.** That is now the precise open question, and it is a
better one than the lane started with.

## Disposition

Nothing landed. `source/ai/actor_perception.c` and `actor_perception.h` are
reverted to HEAD; `git status` is clean; the unit is back at 36/44 exact with
`_actor_get_perception_knowledge` EXACT at 240 bytes. Per the owner's
instruction, no partial version was landed.
