# The hoisted-block lever has worked examples - Lane A, 2026-09-20

Wave D's second board-wide census found that January and our build place
hoisted short-circuit blocks identically in 7,716 of 7,729 functions, leaving
thirteen divergences - all residuals. That census said *how rare* the divergence
is. It did not say whether the divergence is reachable from source.

This note answers that, and the answer is **yes**.

## Method

`scratch/orch/hoistexact.py` censuses the hoisted-block idiom in BOTH builds and
keeps every function where January hoists a **value-producing** block and our
build hoists a block with the **same instruction mnemonic sequence**. The test
is the mnemonic sequence, not an objdiff percentage - objdiff is diagnostic
only and never credit.

## Result

    January functions containing a hoisted block   131
    REPRODUCED value-producing hoists              117  in 96 functions

**The hoist idiom is overwhelmingly reproducible.** This is the opposite of what
the cross-jump census found: there, `_actor_path_refresh` was the only function
on the board where the binding was the sole difference, so there was no closed
example to learn a spelling from and the route was empty. Here there are
ninety-six.

Three of them are inside Lane A's own files, so their source is ours to read:

| exact function | file | hoisted block |
|---|---|---|
| `_actor_perception_refresh_test_object` | actor_perception.c | `mov fld test je fcomp fnstsw test jne mov jmp` |
| `_actor_move_calculate_controlled_by_aiming` | actor_moving.c | `test jne fcom fnstsw test jne mov fstp mov mov jmp` |
| `_actor_situation_combat_status_update` | actor_perception.c | `mov inc cmp mov jl inc jmp` |

All three are byte-identical to January at the identical address, hoist site and
join - verified individually, not inferred from a percentage.

## What the worked examples spell

### `_actor_move_calculate_controlled_by_aiming` - a ternary controlling an `if`

    if (best_direction == NONE ||
        (aim_dot > best_aim_dot ?
            (facing_dot > best_facing_dot || best_facing_dot < 0.5f) :
            (facing_dot > best_facing_dot && aim_dot > 0.5f)))
    {
        best_direction = direction;
        best_aim_dot = aim_dot;
        best_facing_dot = facing_dot;
    }

The hoisted block is the `:` arm of the ternary **plus a duplicate of the if
body**. So a conditional expression used as an `if`'s controlling expression,
with short-circuit chains in its arms, hoists one arm out of line and duplicates
the body into it.

### `_actor_situation_combat_status_update` - the counter idiom

    if (actor->state.combat_status == _actor_combat_status_none)
        actor->state.in_combat_timer = 0;
    else
        actor->state.in_combat_timer++;

    if (actor->state.combat_status >= _actor_combat_status_certain)
        ...

January hoists the `else` arm, and **the hoisted block swallows the NEXT `if`'s
test** (`cmp cx,4 ; jl`) - which is why it is a hoist and not a plain else. We
reproduce it byte for byte.

Note the discrimination: the same function contains an immediately preceding
`if (mode < _actor_mode_combat) timer = 0; else timer++;` which is **not**
hoisted in either build. The idiom alone is not sufficient; the tail-merge with
the following test is what carries it out of line.

## Why this matters for the remaining divergences

`_actor_move_vector_avoidance` (4,144 B, actor_moving.c) has a January-only
hoist whose block is:

    ce6  inc eax
    ce7  mov word ptr [ebx + 0x5f0], ax
    cee  jmp 0xbe4

and whose in-line counterpart in our build is:

    bd5  jne 0xbe2
    bd7  mov word ptr [ebx + 0x5f0], 0
    be0  jmp 0xbea
    be2  inc eax
    be3  mov word ptr [ebx + 0x5f0], ax
    bea  <join>

Identical instructions, pure placement - and it is **the same source idiom as
`_actor_situation_combat_status_update`**, which we already reproduce. January's
then-arm falls through into the join with no `jmp` at all, which is the whole
point of the hoist.

Our source in the avoidance function contains no `++` anywhere in its body, so
the increment is currently spelled some other way. That is the first thing to
test, and it is a source question, not a backend tie.

## The correction this forces

A January-only hoist must **not** be recorded as a backend placement tie on
sight. Ninety-six functions prove the transformation is source-reachable, and
two of the three in-file worked examples are FP cascades of exactly the kind the
remaining divergences contain. The standing conclusion for
`_actor_situation_update` is unaffected and remains correct for its own reason -
alone among the nine January-only hoists its block carries no value, so there is
nothing for a source-level assignment to express.

In-scope work list, with the lever now attached:

    _actor_move_vector_avoidance                   4,144 B  counter idiom, worked example in hand
    _actor_perception_aiming_vector_test_blockage    400 B  FP cascade, `mov ecx,1`
    _actor_look_update                             4,720 B  we hoist, January does not
    _actor_emotion_unopposable_retreat             1,264 B  we hoist, January does not

## January's decode of the 400-byte cascade

Read completely from January's bytes; real code ends at 0x181 with 15 nops.

    if (A > -0.5f && A < 0.9f)        result = 2;
    else if (A > -0.8f && A < 1.2f)   result = 1;     <- HOISTED to 0x13f
    else                              return 0;
    d = dx*dx + dy*dy;
    if (d < 0.36f)  return result;
    if (d < 1.21f)  return 1;
    return 0;

Constants confirmed: `bf000000` -0.5, `3f666666` 0.9, `bf4ccccd` -0.8,
`3f99999a` 1.2, `3eb851ec` 0.36, `3f9ae148` 1.21. The hoisted arm sits directly
above the shared `xor eax,eax ; ret` at 0x162, and the `else` return is that
same block - so the cascade's failure paths are already merged in January.
