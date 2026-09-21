# Adjudication requested: the parenthesised accessor lever

Lane A, 2026-09-20. **Held, not landed.** The diff is complete and verified at
`scratch/res5/line-of-fire/LANDING.diff`; it is not in the production tree.

## What it is

`_ai_test_line_of_fire` (336 B, `source/ai/ai.c`) closes to **strict exact** with
a TU-private accessor at the sphere arm's radius argument:

    #define LINE_OF_FIRE_PILL_SPHERE_RADIUS(pill)  ((pill).width)

replacing `pills[pill_index].width`.

## Why it is being held

It **fails the letter of the STRIP TEST**. Compiled without the decoration, the
plain spelling does not match; only the parenthesised form does. Under the
standing no-fake-matching note - *"recompile without the added casts, parens or
macro: if the plain spelling also matches it is real; if only the decorated one
does, the decoration IS the fake"* - that is the rejection criterion, and it is
not a verifier's call to re-frame it.

## Everything measurable, independently reproduced

An adversarial verifier rebuilt the whole packet from scratch and could not
refute it. Its findings, at a stronger standard than was claimed:

- **byte-identical to January over all 336 bytes**, including every relocation
  addend and all padding - not merely normalized-equal. The floor differs in 204
  of 336 bytes.
- 10/10 relocations at identical addresses; identical 419-record symbol table.
- **zero regressions**, by a per-row diff of all 46 functions of `ai.obj`, not by
  the tally. Unit census 43/3 to 44/2; the two survivors are pre-existing parks
  with unchanged tags.
- `--forbid-emitted-symbol _point_from_line3d`: exit 0 on both floor and landing.
- blast radius proven to one build edge: the `#define` sits after every
  `#include`, `ai.c` is included by nothing, one ninja edge consumes it.
- semantics identical - single macro parameter, no multiple evaluation, same
  type, same value, no defect fixed or introduced.
- `fake_match_scan`: 2 findings before, the same 2 after (a pre-existing
  `#pragma optimize` pair elsewhere in the file). **Zero new findings.**

## The three facts that bear on the ruling

**1. The effect is carried by the paren that any hygienic accessor macro has
anyway.** Measured ladder: `(pill).width` is INERT; `((pill).width)` is EXACT. It
is the *outer* paren, and it is idempotent rather than graded. The working form
is the conventional one, not a contrived one.

**2. No undecorated spelling reaches exact.** Twenty shapes were measured between
the two agents. Eight natural alternatives - a `real` local in the arm, a `const`
local, a shared local across both arms, a shared local in the sphere arm only, a
`static __inline` accessor, a pill pointer local both ways, and pointer
arithmetic - all land at 12-24 REAL regions, four of them at size 352 != 336. The
named-real-routes-x87 lever is dead here.

**3. The naming is first-party attested, not inferred.** `real_math.h:409`
declares `fast_vector_intersects_sphere(..., real radius)`; `real_math.h:536`
declares `vector_intersects_pill3d(..., real pill_width)`. The asymmetry the
macro encodes - sphere arm reads `width` as a radius, pill arm reads it as a
width - is exactly the asymmetry Bungie's own header already draws. The comment
now cites it.

## The ruling also governs an already-landed site

The same lever, in a **barer** form - a naked paren, no macro, no comment - is
already in the production tree and already exact:

    source/physics/collisions.c:1572
        (object->object.bounding_sphere_radius)
    on the same callee's same argument, landed 2026-09-19 in 37d51331
    "Reconcile evidence-backed 10K exact batch"; _object_test_vector gates EXACT.

That commit was a batch, so the lever was never adjudicated on its own. **If it
is a fake that slipped through, this must go too and that site should be
revisited in the same breath.** If it is admissible, this landing is the more
disclosed version of something already accepted.

## Two further sites are gated on the same ruling

Both are January callers that push a plain-lvalue radius through the x87 and are
still residual and size-short. **Neither is in this lane's scope** and neither
has been attempted or verified:

    source/game/players.c:3037   _player_examine_nearby_device   [size 160!=176]
    source/units/bipeds.c:4029   _biped_update_moving            [size 4000!=4080]

If the lever is admitted they are worth measuring while its provenance is fresh;
if it is rejected they must not be attempted this way.

## Recommendation

Rule on the lever, not on this function. Either answer is workable and the lane
continues regardless - this is 336 bytes, not a blocker.
