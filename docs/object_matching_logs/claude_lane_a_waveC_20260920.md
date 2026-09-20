# Lane A wave C - the remaining structural residuals

Worktree `C:\halo-worktrees\claude-lane-a-ai-core-20260920`, branch
`claude/lane-a-ai-core-20260920`.

Ten residuals decoded in parallel on disjoint scratch copies, every proposed
landing re-measured by an adversarial verifier and again by the orchestrator
against the pristine tree. Targets were chosen with
`tools/campaign/residual_triage.py`, which ranks by REAL differing regions
rather than the raw `alndiff` count.

**Every landing in this wave banks ZERO strict bytes.** All five functions
remain residual. A residual's padded size is its *potential*, not a realized
gain, and the record says so at every point - the wave-B verifiers were
emphatic about that distinction and it is kept here.

## Landed, all at explicitly zero credit

| unit | function | change | effect |
|---|---|---|---|
| `ai_communication` | `_ai_communication_event` | restores a re-fetched `actor_get(subject_actor_index)` January has | size 7856 -> 7888 (target 8064), relocs 328 -> 330 (target 336) |
| `actor_moving` | `_actor_move_vector_avoidance` | hand-rolled cross-product math -> the existing `cross_product3d` helper | regions 236 -> 227, differing instruction lines 670 -> 626 |
| `actor_perception` | `_prop_status_refresh` | three boolean stores put in `props.h` declaration order, plus the approved macro at its site | see below |
| `actor_perception` | `_actor_perception_update` | the approved macro at its two sites | **real regions 14 -> 2** |
| `ai_debug` | `_ai_debug_render_actor` | three target-proven reconstruction defects | frame `+12 -> -4`, real regions 544 -> 497 |

## The flagship: three reconstruction defects, all proven from struct offsets

`_ai_debug_render_actor` is 24,976 bytes, a third of what was left in the
portfolio. The agent decoded three genuine defects in our source, and each was
re-verified here against the struct layout and January's bytes.

**A wrong constant.** January pushes `3f800000` (1.0f) where we push `3fcccccd`
(1.6f). The whole-function immediate census settles it: January has 14 of the
former and 1 of the latter, we have 13 and 2, and the single surviving 1.6f is a
different site that already matches.

**A wrong field, plus a 2D-to-3D promotion.** January loads `[esi+0x6f0]` and
`[esi+0x6f4]` and stores `mov dword ptr [ebp-0x4c],0`; we `lea [esi+0x63c]` and
load three dwords. `struct actor_animation_orders` is
`{ short impulse; real_vector2d alignment_vector; }` and `+0x6ec` is the
`impulse` that **both** builds test, so `+0x6f0/+0x6f4` is
`output.animation.alignment_vector` and January promotes the 2D vector to 3D
with `k = 0`. `0x63c` is `control.current_fire_target_aim_vector`, a different
member entirely. The local is **already named `alignment_vector_3d`** - the name
corroborates the fix independently of the bytes.

**A second wrong field.** January does `lea eax,[esi+0x658]` then copies through
it; we read `[esi+0x5b0/5b4/5b8]` componentwise. `burst_initial_position`
(`+0x64c`, which both builds `lea` for the preceding line) and `burst_origin`
(`+0x658`) are adjacent `real_point3d` members in `actor_control_data`. `0x5b0`
is `control.desired_aiming_vector` - a unit **direction**, and plotting a
direction as a world point in a Burst Geometry block is simply wrong.

Measured here independently: `sub esp` `0x81c -> 0x80c` against January's
`0x810`, so the frame gap goes from **+12 to -4**; padded size **24976 = target**;
relocations **1905 = target**; real regions **544 -> 497**; census 58/2 with no
sibling lost.

**This retires an open question.** The `control_flag_names` NULL-deref fix - an
array sized `NUMBER_OF_UNIT_CONTROL_FLAGS` (15) with only 14 names, so
`control_flag_names[14]` is an implicit NULL handed to `strcat` - had been held
back because on its own it shrank the section to 24,960 and traded away the
size key. With these three fixes the code length comes back and the section is
24,976 again, so the NULL-deref fix now costs nothing on any key and needs no
adjudication.

## Held for an owner ruling

### `_actor_move_test_avoidance_vector` - house rule 13 against the byte evidence

The landing **hand-expands** `actor_move_transform_avoidance_vector` at both
call sites: eighteen lines of basis-transform math written out twice. House rule
13 is *"Avoid manual/forced inlining; retain ordinary helpers and natural
scheduling"*, and this lane's own handoff had already recorded the prohibition
for this very helper. On that reading it is a straightforward rejection, and it
does not reach exact either.

**But its adversarial verifier returned CONFIRMED, and I do not think a lane
orchestrator should resolve the conflict alone.** The verifier assessed the
fake-match question - is this logic that merely happens to match? - and answered
no: it is the *identical* computation, so a match is meaningful rather than
coincidental. It went further and showed the repeated `avoidance_ray->offset.i`
spelling is itself forced by the bytes: the `component`-temp spelling compiles
16 bytes WORSE and loses the size key, and January's own `[ebp-4]` traffic at
`fn+0xe0` is `fld [edi+8]` / `fst [ebp-4]`, a floating-point CSE spill rather
than the integer copy a source `real component = ...;` would produce.

So the two things genuinely conflict. House rule 13 exists to stop byte-chasing
by manual inlining; here the byte evidence says January's source did not call
the helper at these sites. The verifier answered the question it was asked and
**did not apply house rule 13** - which is dispositive if the rule is read
literally. I am recording both and landing neither.

Everything else about it is clean: 752/752 bytes, identity-equal relocations,
census 31/5 with no sibling lost, `fake_match_scan` clean.

The decode is kept in full, because it is the best evidence anyone has produced
on this family:

- January **open-codes** the transform at both sites - `fn+0x96..0x193` is two
  interleaved copies of the helper body; the two `*dest = *global_zero_vector3d`
  struct copies share **one** load of the global at `fn+0x9a`; and `offset.j`
  and `offset.k` are never stored to memory, because VC7 keeps them on the x87
  stack across the *second* transform and consumes them at `fn+0x1a5`
  (`fxch st(2)` / `fmul st(2)`) - only possible with both bodies in line. The
  79-instruction, 208-byte gap is exactly two helper bodies.
- **VC7 refuses to expand the helper at these flags** under every spelling
  tested - `__inline` on the definition, a fresh non-static `__inline` twin, and
  an explicit `/Ob2` on top of `/O2` - all byte-identical to the floor.
- **`__forceinline` reproduces it**: 768 bytes, 16 relocations, the relocation
  count snapping to January's.
- Independently, the `vector-avoidance` agent **refuted the obvious header
  hypothesis** from the other direction: MSVC ignores `__inline` on a definition
  that follows a plain prototype, so the natural guess was that January's
  `actors.h` declares it `__inline`. Its probe put `__inline` on **both** the
  prototype and the definition and VC7 still emitted a call.

**Disposition: a DEFERRED HEADER PREREQUISITE**, and a narrow one - the helper's
definition in its owning header would have to be a **macro or a forced-inline**.
Plain `__inline` is refuted.

## Rejected

### `_actor_perception_refresh_danger_zone` - fails a mandatory gate

**Reason of record, found by its adversarial verifier and confirmed here:** the
landing fails the lane's own mandatory guard,
`gate.py source/ai/actor_perception --forbid-emitted-symbol _point_from_line3d --all`
- exit 1 on the landing, exit 0 on the floor - by adding a 48-byte
candidate-only `_point_from_line3d` COMDAT. That is the exact construct
`config/object_admission_rejections.json` already records as a whole-object
admission failure under the explicit no-emission house rule, and the exact thing
this lane's R6 says it must neither add nor remove. Since the change banks zero
bytes, the trade is strictly negative: it costs whole-object admissibility and
buys nothing creditable. I had reached the same rejection on weaker grounds; the
verifier's reason is the better one and is the one of record.

The secondary objection stands as well. The landing adds `long prop_index = NONE;` in the projectile case, never
assigned anything else, purely so the argument to
`actor_get_perception_knowledge` is not a literal and VC7's `/Ob2`
argument-aware auto-inliner cannot const-fold the callee's guard away. That is a
dead local whose only purpose is to change codegen. Disclosure does not cure it:
the owner's ruling admitted an inferred **macro expressing a real computation**,
not a variable that exists to defeat an optimiser. The same trick is applied at
the vehicle site, laundering a literal `NONE` through a variable that is
provably always `NONE` on both paths into that branch. It does not reach exact.

**The defensible part does not stand alone.** Removing
`#define REAL_MATH_EXTERNAL_POINT_FROM_LINE3D` by itself measures worse: size
1552 against the floor's 1504 (target 1472) and relocations 50 -> 52
(target 50).

Research kept: the `/Ob2` argument-aware auto-inliner mechanism; an `/Ob1`
control that reaches target size and relocations on pristine source but
**regresses two exact siblings** (`_actor_perception_refresh` 2416 -> 1920,
`_actor_perception_refresh_test_object` 1744 -> 1280), refuting the flag
explanation; and the fact that the residual `lea edx,[ebp-0x58]` is independent
of the invented local, since the `/Ob1` build shows it with no source change.

## A bounded header prerequisite, precisely stated

`_ai_communication_event`'s gap A is worth **+96 bytes and 5 of its 8
relocations** and is blocked by exactly one thing:

    short game_allegiance_get_incidents(
        short our_team_index, short other_team_index, short *incident_threshold);

It is **already implemented** at `source/game/game_allegiance.c:264`, and a grep
over every `.h` in the tree returns **zero** hits for the name - so the
definition is its only declaration. Its sibling `game_allegiance_incident` is
already declared in `source/game/game_allegiance.h`. The agent measured the gap
with a throwaway local prototype and then discarded it rather than landing a
consumer-local surrogate.

## Clean negatives

- **`_actor_look_update`** (4,720 B, 8 shapes) - no landing.
- **`_actor_input_update`** (2,384 B, 23 shapes) - no landing.
- **`_actor_perception_update`'s** remaining divergence is one adjacent
  `add esp,8` / `mov edi,eax` swap, which 14 one-factor controls plus 2 from the
  prior wave all measure inert - consistent with the already-parked
  dependency-free scheduler-tie class.

## A tooling hazard found in passing

Under Git Bash, `--cflag /Ob0` is **silently swallowed** by MSYS path mangling
and the flag never reaches CL. It must be run from PowerShell as
`--cflag=/Ob0`. **Any earlier `--cflag /X` measurement taken from bash is void.**

## A measurement ladder worth keeping

From the `_actor_perception_update` verifier, with everything else held
constant:

     prop->distance * prop->distance        14 REAL   2 differing reloc rows
    (prop->distance * prop->distance)       14 REAL   0 differing reloc rows
    (prop->distance) * (prop->distance)      4 REAL   0 differing reloc rows
    ((prop->distance) * (prop->distance))    2 REAL   0 differing reloc rows
     actor_perception_distance_squared(...)  2 REAL   0 differing reloc rows

The effect is **graded**, not binary: an outer paren pair alone fixes relocation
identity while leaving all 14 regions, inner parens take regions to 4, and both
together reach 2. The macro is exactly the last spelling. That also means the
relocation-row key is not discriminating here, which corrects an
over-attribution in the proposer's own write-up.
