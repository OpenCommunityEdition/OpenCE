# Adjudication requested: `_actor_emotion_update`, a target-proven out-of-bounds read

Lane A, 2026-09-20. **Held, not landed.** The diff is at
`scratch/res7/emotion-update2/LANDING.diff`; the production tree is clean.
**This banks ZERO bytes unless and until it is admitted.**

    floor      residual 1664  _actor_emotion_update  [sha]
    candidate  EXACT    1664  _actor_emotion_update
    unit       38/6 -> 39/5, no sibling lost, relocations 38/38, 0 differing rows

## Why this is being re-raised after a refusal

An earlier wave proposed the same out-of-bounds read and the owner refused it.
The stated objection was to landing undefined behaviour on a function that
**stays fuzzy**. That premise no longer holds: with this edit the function is
**strict exact**, so January's own bytes prove Bungie's source performed the
access. That is the campaign's documented target-proven BUG category rather than
a guess, and it is the owner's call to re-make.

## The defect, proven from January's bytes

Our source scans `for (priority = NUMBER_OF_ACTOR_THREAT_TYPES - 1; ...)`, i.e.
from 8. January scans from **9**:

    f5   mov   eax, 9
    fa   lea   ebx, [ebx]                      <- 6-byte alignment nop; touches
                                                  neither EAX nor memory
    100  movsx edx, ax                         <- LOOP HEAD (16-byte aligned)
    103  cmp   byte ptr [edx + esi + 0x1ee], 0 <- reads specific_threats[9]
    10b  jg    0x113
    10d  dec   eax                             <- the decrement is AFTER the load
    10e  test  ax, ax
    111  jg    0x100

`source/ai/actors.h:535` declares `char specific_threats[9];` immediately
followed by `char cumulative_threats[9];`, so index 9 is one past the end and
aliases `cumulative_threats[0]`, i.e. `cumulative_threats[_actor_threat_none]`.

## Why the access is harmless in practice - verified independently

I checked this myself rather than accepting the claim. Every use of
`cumulative_threats` in the whole tree:

    _actor_threat_visible (x2), _actor_threat_visible_facing_me,
    _actor_threat_visible_aiming_at_me, _actor_threat_shooting,
    _actor_threat_shooting_near_me, _actor_threat_shooting_at_me,
    _actor_threat_extremely_close_to_me, _actor_threat_damaging_me,
    [5], and one variable index [threat_type]

Those enum values are 1..8. **Index 0 is never written anywhere in the tree**,
and the one variable-index use is a read-only `csprintf` argument inside the
`actor_debug_print_threat` debug macro, not a write. So the byte at
`specific_threats[9]` is always zero, the `cmp`/`jg` never exits on the stray
iteration, and the scan falls through to the intended starting index. Observable
behaviour is identical; the access is out of bounds in C but reads a real
adjacent member that is always zero.

## The byte ledger closes exactly, and both halves are needed

Nineteen raw differing bytes over the 1,664-byte section:

    A  relocation-addend convention (INERT by construction - coff_compare
       zeroes reloc sites before hashing)                              12
    B  loop-init immediate, 09 against 08                               1
    C  two swapped `test r8,r8 / jcc` pairs                             4
    D  their two incoming branch displacements                          2
                                                                     ----
                                                                       19

Verified additively rather than asserted: closing C+D alone leaves 13 differing
bytes, closing B alone leaves 18, closing both leaves 12 - all of them cause A -
and the gate then reports EXACT. **Neither half closes the function on its own**,
so the crouch restructure cannot be landed alone as a consolation.

The second half is a semantics-preserving restructure of two sequential `if`
blocks into an `if`/`else if`. I traced all four input combinations; the two
conditions are mutually exclusive in the original too, so behaviour is
unchanged.

## What is being asked

Admit the out-of-bounds read as a documented target-proven BUG, with the
evidence above carried in a source comment in the house style, or refuse it.

If refused, the honest outcome is a well-measured negative and **zero bytes** -
six in-bounds spellings were measured and none closes the function, so there is
no third option.
