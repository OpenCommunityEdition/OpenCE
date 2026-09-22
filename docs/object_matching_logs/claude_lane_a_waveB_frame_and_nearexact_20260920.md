# Lane A wave B - the frame-mismatch class and the near-exact residuals

Worktree `C:\halo-worktrees\claude-lane-a-ai-core-20260920`, branch
`claude/lane-a-ai-core-20260920`, baseline
`6130661c9585fdb2785b3ff6bdb2ad0cfef0520c`.

Ten residuals were decoded in parallel, each on its own scratch copy, and every
proposed landing was re-measured by an adversarial verifier and then a third
time by the orchestrator against the pristine tree. Targets were chosen by the
two board censuses added this session: the `+12` frame-mismatch class and the
near-exact rows.

## Landed

| unit | function | before | after | padded bytes |
|---|---|---|---|---:|
| `actor_firing_position` | `_pre_evaluator_attack` | residual | **EXACT** | 1,120 |
| `actor_perception` | `_actor_perception_refresh` | residual | **EXACT** | 2,416 |
| `ai_debug` | `_code_0003a910` | residual | **EXACT** | 704 |

`source/ai/actor_firing_position` closes at **27/27 - the whole object**.
`source/ai/actor_perception` goes 35/44 to 36/44 and `source/ai/ai_debug`
57/60 to 58/60.

Two further changes landed at **explicitly zero credit** because they make the
source more authentic without reaching exactness:

| unit | function | change | effect |
|---|---|---|---|
| `ai_debug` | `_ai_debug_render_actor` | a real off-by-one **corrected** | instruction error 40 to 37, normalized regions 255 to 253 |
| `actor_perception` | `_actor_perception_update` | an invented local macro replaced by the genuine `real_math.h` helper | differing regions 15/3 to 14/2 |

## `_pre_evaluator_attack` - the +12 frame class, solved and proven

January homes one 4-byte cell at `[ebp-0x34]`; we homed twelve more bytes, of
which `[ebp-0x40]` was **never referenced at all** - the elided-aggregate-member
signature. The callee multiset was already identical.

The cause was a hand-expanded library idiom:

```c
real_vector3d projection;

scale_vector3d(&attack_vector->vector, -along, &projection);
add_vectors3d(&projection, &direction, &direction);
```

January writes it Bungie's way, in place and with no temporary:

```c
point_from_line3d((real_point3d *)&direction, &attack_vector->vector, -along,
    (real_point3d *)&direction);
```

That is not a construction invented to move bytes. It is the **literal** shape
of `point_to_line_distance_squared3d` in Bungie's own math library
(`source/math/real_math.c:2536`), casts included, in a function whose entire job
is this computation. And it **removes** a local rather than adding anything.

Result: `sub esp,0x34` on both sides, `EXACT 1120`, `relocdiff` 39/39 with **0
differing rows**, and the unit at `exact 27 residual 0`.

The mechanism generalizes but **cuts both ways** - do not apply it blind.
`_actor_perception_refresh_danger_zone` in this same portfolio is the mirror
image: **we** emit a `point_from_line3d` call January does not, and January
references `45.0f` three times where we reference it none. Read the bytes per
site.

## `_code_0003a910` - target-proven averaging

January's bytes at 0x1ef:

    fild  dword ptr [ebp-4]                  ; (real)vertex_count
    fdivr dword ptr [__real@3f800000]        ; scale = 1.0f / vertex_count
    fld [ebp-0x38] ; fmul st(1) ; fstp [ebp-0x38]
    fld [ebp-0x34] ; fmul st(1) ; fstp [ebp-0x34]
    fld [ebp-0x30] ; fmul st(1) ; fstp [ebp-0x30]
    fstp  st(0)

**One** `fdivr` against 1.0 and **three** `fmul`. Three `x /= n` divides would
emit three `fdiv`s. So `scale = 1.0f / vertex_count;` followed by three
`*= scale` is January's source, not a rounding liberty taken to buy bytes - and
`vehicle_stuck` in `source/units/vehicles.c` writes the same average the same
way. Point accumulation likewise moved to `add_vectors3d` with the cast spelling
the tree already uses at `source/devices/device_machines.c:341` and
`source/physics/collision_features.c:139`.

`EXACT 704`, `relocdiff` 23/23 with 0 differing rows, no new compiler warning.

## `_ai_debug_render_actor` - a reconstruction bug we were carrying

Not a byte-steering change: a defect.

    switch (actor->target.target_type-1)

The enum is `_actor_target_none = 0`, `_actor_target_partial_enemy = 1`, and the
case labels are the named enumerators, so subtracting one dispatches every
target type onto the **previous** colour and sends `_actor_target_damaging_enemy`
into the `default` arm, which asserts. The guard above is
`target_type != 0`, so the subtraction has no protective purpose either.

January proves the compiler does the subtraction itself, because the lowest case
value is 1:

    January  0x2e07   dec eax     ; cmp eax,0xa ; ja ...
    ours     0x2e14   add eax,-2  ; cmp eax,0xa           <- subtracted twice

Removing the `-1` makes our build emit `dec eax` like January's. The function
stays residual - its `+12` frame is a separate, unsolved problem - but the
program is now correct and measurably closer.

**Rejected on the way there, and worth recording.** A `char string[108]`
buffer reproduces January's `sub esp,0x810` *exactly*. It was rejected as a
**fake match**: 108 is a tuned magic number, and matching the frame size broke
the array-block layout that the pristine source already reproduces (January's
array bases 0x2b0/0x310/0x410/0x510/0x710/0x810 appear in the pristine build at
exactly +0xc; the tuned buffer scrambles them). Matching `sub esp` is not
matching the layout.

## `_actor_perception_refresh` - EXACT, 2,416 bytes, landed on an owner ruling

This one **gates strictly EXACT** - `relocdiff` reports `sha equal`, the unit
goes 35/9 to 36/8, and every other section in the object is byte-identical to
the floor. It is the largest single win this wave found. **I have not landed
it**, because it needs a decision that is the owner's, not mine.

**OWNER RULING, 2026-09-20:** *"Land `_actor_perception_refresh` (+2,416) with a
TU-private, descriptively named inferred macro and documented evidence."*
Landed accordingly: the macro is translation-unit private (a `#define` in the
`.c`, no header touched), named to match its sibling
`actor_perception_distance_squared2d`, and carries a block comment that states
plainly that it is **inferred from January's bytes and not attested in any
surviving source**, reproduces the five-spelling table below, and explains why
the byte evidence is itself the argument for a macro. The second hunk carries
its own comment saying the two are required jointly.

It requires two hunks, and neither works alone - I measured each in isolation
and both are residual:

1. a new macro beside the file's existing `actor_perception_distance_squared2d`:
   `#define actor_perception_distance_squared(distance) ((distance) * (distance))`
2. `if (c) desire = TRUE; else desire = FALSE;` in place of `desire = c;`

### The strip test, run five ways

The campaign rule is: when a candidate adds incidental syntax, compile it
without. I did, on the squaring expression, with hunk 2 held constant:

| spelling at the call site | result |
|---|---|
| `prop->distance * prop->distance` | residual |
| `(prop->distance) * (prop->distance)` | residual |
| `(prop->distance * prop->distance)` | residual |
| binding a local first, `(distance * distance)` | residual |
| `((prop->distance) * (prop->distance))` | **EXACT** |

Only the **full canonical macro expansion** reaches January - inner parentheses
on each operand *and* outer parentheses on the product. Nothing simpler does.

### Why that cuts both ways, honestly

**For landing it.** `((a) * (a))` is exactly and only what a correctly written
squaring macro expands to. A human writing this inline would never produce it;
a macro produces it inevitably. So the byte evidence is itself an argument that
**January's source had a macro here**. The file already carries
`actor_perception_distance_squared2d`, whose body contains the identical
`(delta_x) * (delta_x) + (delta_y) * (delta_y)` spelling - same name stem, same
convention, same parenthesisation. And hunk 2 matches its three sibling arms in
the same `else if` chain, which all assign the literal `TRUE`/`FALSE`, and
matches the file's own branchy form on this very variable 25 lines above
(`if (distance_squared < 225.0f) { desire = TRUE; ... } desire = FALSE;`).

**Against landing it.** We would be **inventing** a macro on byte evidence
alone, and the only spelling that matches is the decorated one - which is the
exact signature the strip test exists to catch. Hunk 2 also leaves the function
internally inconsistent: two other sites keep the concise `desire = <comparison>`
form.

### What I did

I initially rejected an earlier shape from this agent that carried a redundant
`(real)` cast on a `real`-typed product. That rejection was right and that shape
is gone. The final shape is materially different - no cast - and reaches exact,
so I re-opened the question rather than letting my first call stand. Having
re-opened it, I do not think a lane orchestrator should invent a macro on byte
evidence unilaterally, and the owner has adjudicated this class before (the
`ai_script` BUG, the `_actor_emotion_update` hold).

The diff, all 76 gated shapes and my five-spelling table are at
`scratch/res/perception-refresh/`. Verified after landing: `EXACT 2416`,
`relocdiff` `sha equal` with **0 differing rows**, unit census 35/9 to **36/8**,
`fake_match_scan` 0 findings, no sibling lost.

One adjacent result from the same agent, worth keeping either way: the seed's
claim that `_ai_communication_get_player_rating` shares this interleave
mechanism is **refuted** - its residual is a three-instruction FP term
transposition originating in `magnitude_squared3d` in `real_math.h`, i.e. a
deferred header prerequisite.

## Found, verified, NOT landed: a live NULL-deref in `_ai_debug_render_actor`

This is a **correctness bug in our reconstruction**, target-proven, and it is
also the `+12` frame cause on the portfolio's largest residual (24,976 bytes).
I verified every part of it from January's bytes myself.

`source/ai/ai_debug.c:3043` declares

```c
const char *control_flag_names[NUMBER_OF_UNIT_CONTROL_FLAGS] = { ...14 strings... };
short flag_count = NUMBER_OF_UNIT_CONTROL_FLAGS;
```

`NUMBER_OF_UNIT_CONTROL_FLAGS` is **15** (`source/units/units.h:109`; the enum
runs `crouch` .. `swap_weapons`), but only **14** names are given -
`_unit_control_swap_weapons_bit` has none. So `control_flag_names[14]` is an
implicit **NULL**, the loop runs `bit < 15`, and
`strcat(temporary, control_flag_names[bit])` is reached with that NULL whenever
the swap-weapons control bit is set. `flag_count` is left dead and the
`"<unknown %d>"` arm is unreachable - the dead local is itself the tell that
January used it as the *inner* bound.

**January's bytes carry both constants, twice each:**

    January   0x422c  cmp bx, 0xe      0x4278  cmp bx, 0xf
              0x435c  cmp bx, 0xe      0x43a8  cmp bx, 0xf
    ours      0x424c  cmp bx, 0xf      0x4298  cmp bx, 0xf     <- the 14 appears nowhere

So January's source bounds the *name lookup* at `NUMBEROF(control_flag_names)`
= 14 and the *loop* at `NUMBER_OF_UNIT_CONTROL_FLAGS` = 15. Restoring that -
an unsized array plus `NUMBEROF`, which is the campaign's own house idiom -
reproduces January's `cmp bx,0xe` / `cmp bx,0xf` pattern in both blocks
(measured: our 0x423c/0x4288 and 0x436c/0x43b8) and:

| | January | ours now | ours + fix |
|---|---|---|---|
| `sub esp` | `0x810` | `0x81c` (**+12**) | `0x80c` (**-4**) |
| last non-pad byte | 0x6184 | 0x6190 | 0x6180 |
| relocations | 1905 | 1905 | 1905 |
| unit census | - | 58/2 | 58/2, no sibling lost |

It also makes the three `char string[72]` sprintf buffers coalesce into a single
frame object sharing `firing_decision_names`' base - one base, six `lea`s -
exactly as January does, which is the mechanism behind the `+12`.

**Why it is not landed.** The section becomes **24,960** instead of 24,976,
because our jump table then needs no padding, so the gate row gains
`size 24960!=24976` - it trades away key (3). That key was never real: the
24,976 match is a *padding coincidence*, January's code ending at 0x5ffd with 12
bytes of padding against ours at 0x600c with none. Surrendering a coincidence to
fix a NULL-deref and move the frame from +12 to -4 is right on the merits, but
it regresses a gate key on the portfolio's biggest prize, and that is the
owner's call rather than a lane orchestrator's.

The candidate is at `scratch/res/render-actor/FLAGCOUNT_candidate.c` and
`CAND4.diff`. **One ruling releases it.**

## Held, not landed: `_ai_test_line_of_sight`

An agent found, and proved from January's slot census, that January **never
initialises** `collision_fraction`: `[ebp-0xc]` has exactly three references, of
which only one is a write (`collision.t` at 0x11a), and the fog test at 0x139
can reach `classify_collision_distance` with a clear line of sight before any
write. Deleting our `= 1.0f` initialiser moves the function from 1040 to 1024
padded bytes against January's 1008, instruction error 333 to 332, differing
regions 75 to 66.

**It is not landed.** The owner's standing ruling on `_actor_emotion_update` is
*"do not land the out-of-bounds read while it remains fuzzy"*, and this is the
same class: an undefined-behaviour disclosure that buys **zero strict credit**
while the function stays residual. The `+12` frame here is a *separate* cause -
January lets `down` share `perpendicular`'s 12-byte cell, which only sibling
scopes can do - and it is untouched by the initialiser.

The diff, the BUG comment and the full evidence are preserved at
`scratch/res/line-of-sight/`. Reconsider it only as part of a change that takes
the function strictly exact.

## Negatives worth more than the landings

- **`_encounter_create`: 23 gated shapes, no landing.** The `-4` frame is
  **not** a missing local: it measures which pointer wins EBX. January spills a
  homed `short actor_type`; we keep it in SI. The previous wave's claim that
  `[ebp-0x14]` is a compiler spill slot is **wrong** - January does the same
  32-bit-store / 16-bit-read on the indisputably named local `count` at
  `[ebp-8]`, because VC7 rounds a `short` to a 4-byte granule and uses dword
  accesses on it freely. And that wave's negative "promoting `actor_type` to
  function scope was byte-identical" is uninformative: scope promotion only
  moves a variable that is *already* homed, and ours is not.
  **A metric trap was caught here**: one shape reaches January's exact
  instruction count (257) and would outrank the floor on the literal ordered
  key, while diverging from January 0x20 bytes *earlier* than the floor does
  (regions 58 versus 54). The count parity is a coincidence.
- **Declaration order is inert, again**, now measured on four more functions.
- **The declaration-count oracle is dead here**: 20 gated builds inserting
  1, 2, 3, 4, 5, 6, 7, 8, 16 and 31 verbatim prototypes across two basins each
  reproduced their basin's hash exactly.
- **Unused locals are eliminated and do not move x87 order; only live locals
  do.** Proven by adding unused `real`/`long` locals to a shape whose x87 order
  had just been fixed and seeing it stay fixed.
- **A frame-size match is not a layout match** - see the rejected 108-byte
  buffer above.

## New laws, all target-proven

1. **Local type shape sets x87 evaluation order, not just frame packing.**
   `real_math.h`'s `distance_squared2d` is `magnitude_squared2d(vector_from_points2d(a,b,&v))`
   with **one `real_vector2d` aggregate**; the invented macro it replaced used
   **two separate scalar `real`s**. Same instructions, but aggregate-versus-two-scalars
   flips which square VC7 evaluates first - deep-first against
   top-of-stack-first.
2. **Locals in sibling scopes can share one frame cell; two in the same lexical
   scope never can.** In `_ai_test_line_of_sight` January colours `down` into
   `perpendicular`'s cell; promoting either to function scope prevents sharing
   and costs another 12 bytes, which was measured (`sub esp,0x98`).
3. **A +/-4 frame delta can be a consequence of enregistration** - a local kept
   in a register on one side and homed on the other - rather than a missing
   local. A delta the size of an aggregate is far more likely a genuinely
   different local set. This tempers the `+/-4` rows of the frame census
   (23 functions, 24,496 bytes of the 101,056).
4. **One decoded lever becomes a board-wide worklist.** Counting the two x87
   square idioms (`d9c0d8c9`, `d9c1d8ca`) per function across both trees gives
   224 functions carrying it in both, 206 agreeing and **18 disagreeing** -
   across AI, physics, bitmaps, interface, structures and units. Generalized
   into `tools/campaign/idiom_census.py`.

## Independent verification of the two exact landings

Both were re-measured by an adversarial verifier that did not author them, and
then a third time by the orchestrator against the pristine tree. Both returned
**CONFIRMED WITH CORRECTIONS**, and both corrections are applied.

`_pre_evaluator_attack`. The verifier rebuilt the exact-set with `coff_compare`
directly rather than reading gate listings - `exact(baseline)` 26 symbols,
`exact(patched)` 27, regressed set **empty** - and confirmed the patched
function's **raw** section bytes are byte-for-byte identical to January's, not
merely equal under the normalized hash. It also traced the one semantic
difference (the operand order of a single float multiply, bitwise commutative
under IEEE-754) across signed-zero, denormal and inf/NaN edges. Its correction:
the notes' padding-trap sentence had the two addresses swapped. Re-measured
here: **January's real code ends at 0x459** (last instruction `ret` at 0x458)
with **seven** trailing NOPs; the floor ends at **0x45d** (`ret` at 0x45c) with
**three**. Both sections are 0x460 bytes and both carry padding. The conclusion
drawn - our code was 4 bytes longer - was right. Note corrected.

`_code_0003a910`. No reported number was wrong. The verifier's correction is an
unstated side effect worth recording: the object gains a new 48-byte
`_add_vectors3d` `.text` COMDAT, because routing the accumulation through the
`real_math.h` helper makes the translation unit use it. This is the known
systemic extra-COMDAT gap and costs nothing - the unmodified floor already
carries **18** such helper COMDATs that January's split object lacks, this makes
19; **15 objects in `build/base` already emit this exact symbol**, including the
closed `units.obj`; `gate.py` does not see it; no `--forbid-emitted-symbol`
guard names it; and the linker folds it.

That verifier also produced **a sharper fake-match test than the scanner**, which
is now campaign guidance: *when a candidate adds casts or other incidental
syntax, compile the version without them.* It did, and the uncast
`vehicles.c`-verbatim spelling **also** gates EXACT - which converts "this
spelling happens to hit the bytes" into "the most natural spelling matches
January". The same test is what condemned `_actor_perception_refresh` above,
where only the decorated spelling reaches the bytes.

## A park retired

`_pre_evaluator_attack` was parked `unclassified`. The board-wide frame census
contradicted that class - a differing `sub esp,N` is structural by campaign law,
and this one was `+12`. It was. The entry is removed, because the function now
compares EXACT and `tools.parked_functions` fails the build on a stale park,
which is exactly the behaviour you want. `config/parked.json` goes 184 to 183
entries; no other entry was touched and the file was never re-serialized.

This is the first park retired by the frame census, and the census says 35 more
parked functions carry the same kind of evidence.

## Verification, all reproduced after the landings

    ninja -j4 all_source progress semantic_progress       exit 0
      halobetacache  1,448,422 / 1,770,166   (7,295 / 7,574)
      overall        1,469,654               (7,562)
      Validated parked compiler ties: 183

    stable_verdicts diff (pre-wave -> post-wave)
      gained source/ai/actor_firing_position::section:27 _pre_evaluator_attack     1120
      gained source/ai/actor_perception::section:43      _actor_perception_refresh 2416
      gained source/ai/ai_debug::section:53              _code_0003a910             704
      gained 3, 4240 padded bytes; REGRESSIONS 0

    tools.parked_functions        183 active / 0 stale / 0 invalid
    audit_object_admission        0 contradicted / 0 revoked
    fake_match_scan source/ai     2 findings, both pre-existing (the ai.c
                                  #pragma optimize pair), none introduced here
    git diff --check              clean

**Net: +4,229 meaningful bytes, +3 functions, 0 regressions.** The two
zero-credit landings move no byte of the total and are booked as such - the
verifiers were emphatic that the workflow header's "worth N meaningful bytes"
is a residual's *potential*, not what a non-closing change realizes, and that
distinction is kept everywhere in this record.
