# `_actor_perception_refresh_danger_zone` - REJECTED, and the ai_debug audit

Lane A, 2026-09-20. **RULED: rejected on both counts. Nothing landed.** The candidate is complete and gates
STRICT EXACT; it is at `scratch/res6/danger-zone2/candidate_exact.c` and
`BLOCKED_CANDIDATE.diff`, and the production tree is untouched.

    floor    residual 1472  _actor_perception_refresh_danger_zone  [size 1504!=1472, sha]
    candidate  EXACT  1472  _actor_perception_refresh_danger_zone
    unit       38/6 -> 39/5, no sibling lost

The whole residual is **two inlining decisions**, and the byte arithmetic sums
exactly: `1472 + 80 - 48 = 1504`. Fixing either half alone moves relocation count
off 50 and padded size further from 1472, so it is **all-or-nothing**.

## Part A - our own reconstruction hack, whose removal trips a guard

`source/ai/actor_perception.c` carries a TU-private

    #define REAL_MATH_EXTERNAL_POINT_FROM_LINE3D
    ...includes...
    #undef REAL_MATH_EXTERNAL_POINT_FROM_LINE3D

that suppresses `real_math.h`'s existing `__inline point_from_line3d`. January
INLINES `point_from_line3d` here (0x0fc-0x12a: `fld[eax]` / `fmul __real@42340000`
/ `fadd[edi]` / `fstp[esi+0x2c8]` x3, and the following `midpoint3d` reads the
destination directly, proving the return value was folded). Deleting our macro
pair restores the natural state and is worth -48 bytes.

Deleting it also makes the TU emit a `_point_from_line3d` COMDAT, which trips
`gate.py --forbid-emitted-symbol _point_from_line3d`.

**I verified the case against that guard myself, independently of the agent:**

1. In January's split tree, `_point_from_line3d` is **defined by exactly ONE
   object**, `source/ai/action_charge.obj`, and **referenced as UNDEFINED by 17
   objects** - including `actor_perception.obj` itself and `ai_debug.obj`
   (`scratch/orch/comdatscan.py`).
2. The 48-byte COMDAT our candidate emits is **BYTE-IDENTICAL** to the one
   `action_charge.obj` carries (`scratch/orch/comdatbytes.py`):
   `558bec8b4d0cd94510d8098b55088b4514d802d918d94510d84904d84204d95804d94510d84908d84208d958085dc390`
3. A non-static `__inline` that is called out of line in a TU is emitted as a
   COMDAT **in that TU**. The linker folds the duplicates and keeps one. The
   splitter, working from the linked image, can attribute that single survivor to
   only one object - and it chose `action_charge.obj`. So January's own
   `actor_perception.obj` almost certainly DID emit this COMDAT; the split tree
   simply cannot show it.
4. **`source/ai/ai_debug.c` already emits it at HEAD and already fails this
   guard.** I checked the pre-landing floor: `HEAD~1`'s `ai_debug.c` exits 1 on
   the same guard. So one of this lane's files is already in the state the guard
   forbids, pre-existing and accepted.

On that evidence the guard looks like it is enforcing an artifact of COMDAT
folding plus split attribution rather than a property of January's object. That
is a splitter/attribution question, not a source question, which is why it is
being routed rather than worked around.

## Part B - a compiler directive, with a proof that no source spelling works

January CALLS `actor_get_perception_knowledge(actor_index, NONE)` out of line at
0x308 and 0x4e8 (`mov ecx,[ebp+8]; push -1; push ecx; call`). VC7's
argument-aware auto-inliner prunes the callee on the constant `NONE` and inlines
it for us at both sites: worth +80 bytes. The candidate stops it with

    #pragma auto_inline(off)   /* around actor_get_perception_knowledge */

A `#pragma` is on the reject-on-sight list. The agent's four probes argue nothing
else reaches it:

- `/Ob1` on the **unmodified** caller source yields a byte-exact function, so the
  caller's source is already right and no caller-side edit is the lever;
- January materialises the second argument as an immediate `push -1` with no
  stack traffic and an unchanged `sub esp,0x58`, so it is a compile-time constant
  in January's source too - a variable costs a slot and a 4-byte bigger frame;
- the auto-inline cost is taken **after** the constant prune, and the surviving
  tail is pinned byte-exact by `actor_get_perception_knowledge` being EXACT at
  240, so the post-prune cost is identical in both builds;
- definition position is inert.

Same compiler, same flags, same input, opposite decision. `/Ob1` per file would
do it but costs four currently-EXACT siblings and `config/config.json` is frozen;
the pragma does it surgically with zero sibling cost.

**Checked and cleared:** this is NOT a consequence of this session's narrow-return
landing. The function gates `[size 1504!=1472, sha]` identically at `b3223f36~1`
and at HEAD.

## What is being asked

Two decisions, and the package needs **both** or neither:

- **A.** Is `--forbid-emitted-symbol _point_from_line3d` correct, given that the
  symbol is folded, that our emitted copy is byte-identical to January's, that 17
  objects reference it, and that one lane file already fails the guard at HEAD?
- **B.** Is `#pragma auto_inline(off)` admissible on one callee to reproduce an
  inlining decision that is proven unreachable from source?

If either is refused the candidate is dead and the 1,472 bytes stay unbanked,
which is a workable outcome - the lane continues either way.


## RULING - 2026-09-20

> **A.** Guard stands; reject the candidate. January ownership matters.
> Separately audit and fix the pre-existing `ai_debug.c` COMDAT violation.
>
> **B.** Reject - no pragma. The candidate is already invalid on COMDAT
> ownership, and compiler-directive steering violates house rules.

The candidate is dead and the 1,472 bytes stay unbanked. `candidate_exact.c`
remains in `scratch/res6/danger-zone2/` as evidence only and must not be landed.
The standing interpretation is now explicit: **the split tree's attribution is
authoritative.** If January's object does not own a symbol, ours must not emit
it, regardless of whether the emitted bytes are identical.

Consequence for the lane: `--forbid-emitted-symbol _point_from_line3d` stands as
a hard constraint, and any future candidate that emits that symbol is dead on
arrival. The four `source/ai/` files that carry the
`REAL_MATH_EXTERNAL_POINT_FROM_LINE3D` suppression - `action_vehicle.c`,
`actor_moving.c`, `actor_perception.c`, `ai.c` - are therefore correct as
written and must keep it.

## THE ai_debug.c AUDIT, as directed

**Result: the violation is real, and fixing it costs seven exact functions. I
have not fixed it, and I recommend it is not fixed in `ai_debug.c`.**

What `ai_debug.c` does today:

    forbid-emitted-symbol guard   FAILS (exit 1), emitting a 48-byte
                                  _point_from_line3d COMDAT at section 29
    unit census                   == exact 59  residual 1  (of 60)

It is the only one of the eleven lane files without the suppression macro.

**The call pattern is not the problem - it already matches January exactly.**
Counting relocations to `_point_from_line3d` in each build's `ai_debug.obj`:

    JANUARY  2 relocations
    OURS     2 relocations

So both builds inline the same roughly forty call sites and both call out of
line at the same two. Our object emits the COMDAT purely because a **non-static
`__inline` that is called out of line is emitted as a COMDAT by that TU** - which
is correct VC7 behaviour, not a reconstruction defect.

**The only available fix is the suppression macro, and it is measurably worse
than the violation.** Adding the same `#define`/`#undef` pair the other four
files carry:

    forbid-emitted-symbol guard   PASSES (exit 0)
    unit census                   == exact 52  residual 8  (of 60)   -7 EXACT

It passes the guard by turning every inlined site into a call, which destroys
seven functions that are currently byte-exact. That fails lane rule 1 outright -
a change must not cost a sibling that is currently exact - so it is not a fix,
it is a trade of seven exact functions for one guard result.

Probe at `scratch/orch/ai_debug_suppressed.c`; nothing was landed.

**Recommendation.** Leave `ai_debug.c` as it is and record the guard failure as a
known, accepted exception for that one object. If the guard result matters more
than the seven functions, the remedy is at the splitter/attribution level -
giving the folded COMDAT a different or shared owner - and not in this source
file. That is outside this lane's scope and I have not attempted it.
