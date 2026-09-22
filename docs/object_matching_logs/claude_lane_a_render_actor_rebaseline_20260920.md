> **CORRECTION 2026-09-20 - the "shortening fixpoint" section below is WRONG.**
>
> Wave H's branch-fixpoint research (`scratch/res8/branch-fixpoint/NOTES.md`)
> refutes it on every point:
>
> - **There are no seed branches.** The "span 128" figures were distances
>   measured from the start of the NEAR form. The short `rel8` for those
>   branches is 122-123, so they fit. All 27 of our near-but-fits branches fit
>   on their own, with short displacements from 93 to 124.
> - **It is not a fixpoint over the final layout.** No final-bytes model
>   reproduces our object (the best gets 25-30 wrong), while a pessimistic
>   relaxation reproduces all 526 of January's widths and its code end at
>   0x6000. Our object looks as though it was emitted against an address
>   estimate running roughly 12-38 bytes ahead of the final layout.
> - **Width is non-local and chaotic.** Deleting one inline call flipped 29
>   branches, 18 of them with byte-identical spans; across 330 single-statement
>   deletions the near-but-fits count ranged from 0 to 32. Null edits are inert.
> - **The frame is not "paying for the branch tax".** Padding the frame to
>   January's 0x810 left the count at 27. The +39 measured when correcting the
>   frame alone may be real, but the causal story attached to it here is not
>   supported.
>
> Consequence: on this function **padded size (key 3) mostly measures this
> noise**. Judge candidates by width-normalised code length
> (`scratch/res8/branch-fixpoint/widthcensus.py`) instead. The colour-argument
> fix below stands - it was verified from January's bytes independently of any
> of this.

# `_ai_debug_render_actor` re-baselined on correct source - 2026-09-20

Owner ruling: *"Land it and re-baseline the fuzzy floor. Correct source outweighs
a misleading byte score; grant zero exact credit and document the backend
artifact."*

**ZERO exact credit.** The function remains residual at 24,976 padded bytes and
nothing here is scoreboard movement.

## The defect that was fixed

`source/ai/ai_debug.c:1100` and `:1110` had their colour arguments **swapped**
relative to January. Verified from January's own bytes before landing:

    5a7  mov ebx, _global_real_argb_white     <- January pins `color` in EBX
    5c5  mov ebx, _global_real_argb_yellow       for the whole loop body,
    5cd  mov ebx, _global_real_argb_aqua         selected three ways

    6ac  push ebx                             <- FIRST site pushes `color`

    704  mov ecx, _global_real_argb_blue      <- SECOND site pushes BLUE
    70a  push ecx
    711  push 0x3f800000                         (= the 1.f argument that
                                                  follows in render_debug_vector)

Our source passed `global_real_argb_blue` at the first site and `color` at the
second - exactly reversed. The fix takes a 480-byte window from 431 differing
bytes to 13.

## The new floor, and why it is numerically worse

    before   residual 24976  _ai_debug_render_actor  [sha]          495 REAL
    after    residual 24976  _ai_debug_render_actor  [size 25024!=24976, sha]
                                                                    529 REAL

Padded size was previously equal to the target and is now 48 bytes over. **That
equality was never a match** - it was the padding coincidence recorded in wave C
(January's real code ends at 0x5ffd with 12 bytes of trailing pad; ours ended
later with none), and the retraction of the "the code length comes back" claim
is on record. The new number is a more honest floor measured against correct
source.

No sibling moved: `ai_debug` is 59/60 before and after. The pre-existing
`--forbid-emitted-symbol _point_from_line3d` failure on this unit is unchanged
and remains accepted (see the danger-zone adjudication note).

## The backend artifact that makes the correct source look worse

The 48 bytes are **not** caused by the colour fix. They are an independent
branch-shortening fixpoint, and this is the wave's most reusable finding.

`scratch/res7/render-actor5/brcensus.py` lists every branch where January emits
the SHORT form and we emit NEAR, with its final span:

    O@27bd je  -> 2834  span 119     O@3208 je  -> 3288  span 128
    O@2ce0 jmp -> 2d60  span 128     O@378a jmp -> 380a  span 128
    O@2e49 jmp -> 2ec0  span 119     O@3fdb jmp -> 4056  span 123
    O@407d je  -> 40f9  span 124     O@4afa jmp -> 4b69  span 111

**Five of the eight fit comfortably inside +/-127.** Three sit at exactly 128.
For `O@2ce0`, January's span is 0x2d56-0x2cdb = 123 and ours is 0x2d60-0x2ce5 =
**123 as well** - identical distance, identical intervening byte count - and
January shortens where we do not.

So this is a **shortening FIXPOINT, not a distance threshold**: three seed
branches one byte over the line block the other five, each worth 3-4 bytes. It
is the largest single byte family in the function (+27 at the old floor, +85
now), it has no known source lever, and it is invisible to `real_regions.py`, to
the instruction count and to every region metric - measurement trap #6 at scale.

It also explains something earlier waves could not: the **non-additive sizes**
they measured (block 1 alone +48, block 2 alone +64, both together +48).

## A corollary that changes the plan for this function

**The 4-byte-short frame is currently paying for the branch tax.** Correcting
`sub esp,0x80c` to January's `0x810` in isolation makes the function **+39, not
+12**. Nobody should treat the frame gap as free, and no future wave should
attack it on its own.

## What is now known about the remaining gap

The +12 code-span surplus at the old floor decomposed as **+0 aligned** (branch
width +27 cancelled by frame displacement -27) **plus +12 over 202 divergent
regions**, and those 202 regions cluster into 40 clusters whose net sums exactly
to +12. Twenty clusters are net zero; six families explain everything.

Two censuses on this function are CLOSED and must not be re-spent: operand order
of multiplication is byte-inert here, and the whole-function immediate census has
zero differing rows apart from x87 stack-register operands, so there is no second
wrong literal to find.
