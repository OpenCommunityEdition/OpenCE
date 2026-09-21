# Adjudication requested: `_actor_perception_refresh_danger_zone`, 1,472 bytes

Lane A, 2026-09-20. **Held, not landed.** The candidate is complete and gates
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
