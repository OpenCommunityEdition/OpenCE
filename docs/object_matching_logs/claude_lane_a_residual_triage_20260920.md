# Lane A residual triage, ranked by how close each function ACTUALLY is

Worktree `C:\halo-worktrees\claude-lane-a-ai-core-20260920`, branch
`claude/lane-a-ai-core-20260920`.

Regenerate at any time with `python tools/campaign/residual_triage.py`.

## Why the old ranking was wrong

`alndiff`'s region count **systematically overstates the closest functions**. It
prints a region whenever two normalized instruction texts differ, which includes
pure addressing noise: the relocation-target spelling
(`target=symbol:X` vs `target=defined-noncode:.rdata:X`), the relocation SITE
address (`rel+0x...`, which shifts whenever anything upstream changes length),
branch immediates, and trailing alignment NOPs. None of those is a divergence -
`relocdiff --allow-structural` scores them `=`.

Stripping them changes the picture completely. `_actor_path_refresh` reads 12
regions and has **0** real ones. `_actor_look_idle_find_prop` reads 16 and has
**1**. A triage pass ranking by the raw count buries exactly the cheapest work.

The table below ranks by REAL regions, with the raw count in parentheses.

## The table

```
function                                     unit                   bytes frame j/ours     refs  real code j/o  REGIONS
------------------------------------------------------------------------------------------------------------------------------
_actor_path_refresh                          actor_moving            1440 -/-                 0  1428/1440 +12    0 (12)
_actor_look_idle_find_prop                   actor_looking            608 0x30/0x30           0     602/599 -3    1 (16)
_actor_perception_friend_prop_is_attacking   actor_perception         432 0x18/0x18           0        430/430    1 (3)
_actor_situation_update                      actor_perception        1264 0x1c/0x1c           0   1262/1263 +1    2 (23)
_code_00039990                               ai_debug                 848 0x20/0x20           0        845/845    2 (11)
_ai_communication_get_player_rating          ai_communication         832 0xb4/0xb4           0        826/826    2 (12)
_actor_emotion_update                        actor_perception        1664 0x50/0x50           0      1652/1652    3 (12)
_ai_communication_actor_talk_weight          ai_communication         912 0x10/0x10           0        904/904    3 (9)
_prop_get_active_by_unit_index               props                    144 -/-                 0        144/144    7 (7)
_ai_test_ballistic_line_of_fire              ai                       944 0x594/0x594         0        930/930    8 (15)
_actor_perception_aiming_vector_test_blockag actor_perception         400 0x18/0x18           0     385/380 -5    9 (16)
_actors_spawn_from_unit                      actors                   672 0xac/0xac           0    668/684 +16   11 (37)
_actor_create_for_unit                       actors                   624 0xc/0xc             0     622/614 -8   12 (17)
_ai_test_line_of_fire                        ai                       336 0x508/0x508         0     327/321 -6   12 (14)
_actor_perception_update                     actor_perception        4720 0x4f4/0x4f4         0      4720/4720   14 (30)
_actor_move_test_avoidance_vector            actor_moving             752 0x438/0x438         3   743/541 -202   23 (30)
_actor_input_update                          actors                  2384 0x24/0x24           0   2380/2382 +2   24 (53)
_actor_destination_update                    actor_moving             976 0x214/0x214         0     968/967 -1   24 (38)
_ai_communication_finished                   ai_communication        1584 0x650/0x650         1  1583/1565 -18   34 (72)
_encounter_create                            encounters               752 0x14/0x10 !         0        740/740   43 (54)
_actor_customize_unit                        actors                   640 0x98/0x98           0    638/659 +21   43 (52)
_actor_perception_refresh_danger_zone        actor_perception        1472 0x58/0x58          10  1472/1498 +26   47 (64)
_actor_emotion_unopposable_retreat           actor_perception        1264 0x1e0/0x1e0         0   1263/1261 -2   47 (57)
_ai_test_line_of_sight                       ai                      1008 0x80/0x8c !         0   996/1030 +34   49 (75)
_prop_status_refresh                         actor_perception        3680 0x44/0x44           0   3674/3671 -3   52 (86)
_actor_move_update                           actor_moving            3136 0x44/0x44           0  3128/3106 -22  107 (142)
_actor_look_update                           actor_looking           4720 0x58/0x58           0  4714/4690 -24  120 (197)
_actor_move_vector_avoidance                 actor_moving            4144 -/-                11  4130/4177 +47  209 (236)
_ai_communication_event                      ai_communication        8064 -/-                12 8064/7856 -208  341 (497)
_ai_debug_render_actor                       ai_debug               24976 0x810/0x81c !       0 24964/24976 +12  544 (1477)

30 residuals, 75392 padded bytes
cheapest-first: the top rows are the fewest REAL differing regions with the most bytes behind them.
```

## Reading it

- **`frame` flagged `!`** is structural by campaign law - the set of homed
  locals differs. Three rows carry it.
- **`refs`** is the external reference multiset difference. Non-zero means a
  missing or extra program reference; five rows carry it.
- **`real code j/o`** is where real code ENDS on each side with padding
  stripped. This is the number that survives the padding trap: a section size
  equal to the target can still be a dozen code bytes short, and a size
  *mismatch* can be an improvement if it only removes padding.
- **`0 (12)` with a non-zero code delta** means every instruction's TEXT matches
  and the gap is encoding length - usually a short-versus-near branch, which is
  a cross-jump binding question, not a missing instruction.

## What the top of the table says

A cluster of eight functions sits at three or fewer real regions, totalling
roughly 8,200 bytes. Two of them had never been probed and are decoded here:

**`_actor_path_refresh`** (1,440 B, **0 real regions**, +12 code bytes). The
entire residual is three branches at 4 bytes each: January encodes
`jne/jp 0x542` short to the NEAR copy of the return epilogue, we encode near to
the FAR copy 0x267 bytes back. Both copies exist in both builds - only the
binding differs. Source site and a suggested lever are in
`scratch/orch/pathrefresh/NOTES.md`.

**`_actor_look_idle_find_prop`** (608 B, **1 real region**). January emits
`cmp ebx, dword ptr [_ai_debug+0x38]`; we emit `mov eax, [_ai_debug+0x38]` then
`cmp ebx, eax`. The source is already the natural form
(`if (actor_index == ai_debug.selected_actor_index)`) at both of its two sites,
and the split appears where the scheduler is filling an `fcos` latency gap - so
this is instruction selection under x87 scheduling, not a source shape. Note the
function is also 3 bytes SHORT overall, so a second, branch-width difference is
hiding behind the normalisation.

**`_actor_situation_update`** (1,264 B, **2 real regions**) is one five-
instruction test block that January places OUT OF LINE at 0x21b with a `jmp`
back, and we place inline at 0xe9. That is a block-placement question, and the
`_actor_perception_refresh` precedent - "give the arm its own block" - is the
lever to try.

**`_actor_perception_friend_prop_is_attacking`** (432 B, **1 real region**) is
an AL/CL two-colouring: January loads `known_enemies` into CL, keeping
`attacking` live in AL so both early exits return it directly; we load into AL
and must reload. Three guard spellings measured, all dead -
`scratch/orch/fpia/NOTES.md`.

## Scope note

`tools/campaign/residual_triage.py` scans a whole directory, so run bare it
covers all of `source/ai` (51 residuals, 96,032 bytes) rather than just Lane A's
eleven translation units (30 residuals, 75,392 bytes). The table above is Lane A
only. Nothing outside the eleven was edited.
