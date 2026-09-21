# Deferred header prerequisite: `game_allegiance_get_incidents`

Lane A, 2026-09-20. **Nothing is landed and nothing is proposed for landing
without a ruling.** The lane brief requires header prerequisites to be deferred
and documented rather than worked around with a consumer-local surrogate
prototype, so this records the case completely enough to be decided.

## The gap

    source/game/game_allegiance.c:264   short game_allegiance_get_incidents(
                                            short our_team_index,
                                            short other_team_index,
                                            short *incident_threshold)

The function is **non-static**, is **declared in no header in the tree**, and is
**called from nowhere in the tree**. `source/game/game_allegiance.h` declares
every one of its siblings - `game_allegiance_provoke`,
`game_allegiance_notify_change`, `game_allegiance_remove`,
`game_allegiance_incident` - in exactly the style this one would need. It is a
genuine omission from the owning header, not a declaration we would be
inventing, and adding it satisfies the house rule that prototypes belong in the
object's own header.

## What it blocks

`_ai_communication_event` (8,064 B target; ours 7,888, so **176 bytes short**,
and 330 relocations against 336).

January's block at 0x3c1-0x409 is 72 bytes and carries **five** of the six
genuinely missing relocations:

    _game_allegiance_get_incidents                         0x3d8
    ??_C@_0CL@IOMHDOAM@allegiance?5?$CFs?0?5?$CFd?5incid   0x3fe
    ??_C@_0M@IDKAPGFO@still?5holds?$AA@                    0x3f9
    _console_printf         (January has 2, we have 1)     0x405
    _ai_debug               (January has 30, we have 29)   0x3c2

Read directly from January's bytes:

    3c1  mov al, byte ptr [_ai_debug+0x95] ; test al,al ; je 0x40c
    3ca  test bl,bl ; jne 0x40c
    3ce  mov edx,[ebp-0x20] ; lea ecx,[ebp-8] ; push ecx ; push esi ; push edx
    3d7  call _game_allegiance_get_incidents
    3dc  mov cx,[ebp-8] ; add esp,0xc
    3e3  cmp cx,-1 ; jne 0x3f0 ; mov ecx,0x3e7 ; jmp 0x3f3
    3f0  movsx ecx,cx
    3f3  push ecx ; movsx eax,ax ; push eax
    3f8  push "still holds" ; push "allegiance %s, %d incidents (thr..." ; push 0
    404  call _console_printf ; add esp,0x14

so the source shape is a debug print guarded by an `ai_debug` flag and a second
condition, whose fourth argument is the out-parameter with a sentinel
substitution - `threshold == NONE ? 999 : threshold` (0x3e7 is 999). The
out-parameter `&threshold` is precisely why the prototype is required: without
a declaration the call cannot be written at all.

## What it does NOT explain

The allegiance block is 72 of the 176 missing bytes. The remainder is a
**separate** missing block, visible as one extra `_actor_data` (6 against 5),
one extra `_actor_type_get_race` (3 against 2) and one extra `_datum_get`
(8 against 7) - i.e. one more `actor_get(...)` plus `actor_type_get_race(...)`
sequence. That second gap is independent of this prerequisite and is ordinary
lane work.

Two apparent relocation gaps are measurement traps and are **not** differences:
the eleven `_ai_communication_event` rows are the function's own symbol and its
jump table, which our build spells as `$L*` internal labels (traps 2 and 4).

## The proposed edit, if approved

One line group appended to `source/game/game_allegiance.h` beside its siblings:

    short game_allegiance_get_incidents(
        short our_team_index,
        short other_team_index,
        short *incident_threshold);

## What must be measured before it lands

Not yet measured - it cannot be, because measuring it requires the edit. On this
campaign a header edit has repeatedly broken **other objects that name none of
its types**, through definition position and declaration count, so approval
should carry the same conditions the earlier approved header package did:

1. a full-board rebuild with a per-function row diff, not a tally, since an
   aggregate count hides swaps;
2. a declaration-count check - this adds one declaration to a widely included
   header, and +4 has broken units before;
3. the whole-object admission guard on every unit that includes
   `game_allegiance.h`, compared against each unit's own floor exit code;
4. zero regressions, with any lost exact sibling an automatic rejection.

## Recommendation

Worth a ruling, but **not** worth landing on its own arithmetic: at best it
closes 72 of 176 bytes and `_ai_communication_event` stays residual, so it banks
**zero bytes** by the standing rule. Its value is that it removes the only
genuine blocker in the portfolio - every other residual is reachable from source
we are already allowed to edit - and it fixes a real omission in
`game_allegiance.h` independently of this campaign. It should be bundled with
the second missing block so the function can actually close, not landed alone.


## UPDATE 2026-09-20 - the value was understated, now MEASURED

The recommendation above - not worth landing alone, "banks zero bytes" - was
**wrong**, because it counted code only. See
`claude_lane_a_data_section_credit_20260920.md`: objdiff credits a data section
all-or-nothing, and `ai_communication .rdata` (11,600 B) sits at 99.5% because of
exactly the two string literals this block emits.

Measured, not inferred, on a scratch candidate
(`scratch/res8/allegiance-measure/cand.c`) that writes the decoded block. It uses
a consumer-local prototype **purely to take the measurement**; that surrogate is
forbidden for landing and is labelled so in the file.

    .rdata    January 224 symbols
              floor:        2 missing (55 B), 0 differing
              measurement:  0 missing,        0 differing   <- COMPLETE

    _ai_communication_event
              floor:        [size 7968!=8064, relocs 330!=336]   96 B short
              measurement:  [size 8048!=8064, relocs 335!=336]   16 B short

    ai_communication unit census 45/3 before and after - no sibling lost

The block, decoded from January's bytes (0x3c1-0x409) and reconstructed:

    if (ai_debug.print_allegiance && !broken)
    {
        short incident_threshold;
        short incidents = game_allegiance_get_incidents(
            cause_team, subject_team, &incident_threshold);

        console_printf(FALSE,
            "allegiance %s, %d incidents (threshold %d)",
            "still holds", incidents,
            incident_threshold == NONE ? 999 : incident_threshold);
    }

It sits inside `if (observed)`, after the notify block, where `broken` is in
scope - January tests `bl`, which holds `game_allegiance_incident`'s result. The
argument order is fixed by January's own reuse of `[ebp-0x20]` (cause_team) for
`game_team_is_enemy` at 0x40c.

Only two objects in January's whole split tree mention
`_game_allegiance_get_incidents`: `game_allegiance.obj`, which defines it, and
`ai_communication.obj`, which calls it. So January's ai_communication genuinely
made this call.

### Blast radius facts for the ruling

- **12 files** include `game_allegiance.h`: five in this lane (actors.c, ai.c,
  ai_communication.c, ai_script.c, encounters.c) plus actor_stimulus.c and six
  outside source/ai (aim_assist.c, game.c, game_allegiance.c, game_statistics.c,
  motion_sensor.c, damage.c).
- The header has **12 declarations**; this adds one. The recorded
  declaration-count oracle found +2 safe and +4 breaking, so +1 is inside the
  known-safe range - but header edits have regressed distant objects before, so
  a full-board per-row diff is still required before landing.
- This is a **genuine omission** from the owning header, not an invented
  declaration: every sibling function of `game_allegiance.c` is declared there
  in exactly this style.
