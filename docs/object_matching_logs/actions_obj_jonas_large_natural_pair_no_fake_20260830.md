# Actions large natural-C pair and no-fake audit (2026-08-30)

## Scope

This wave starts from canonical commit
`fa102b086bebdf6f874145025a05323c8f9d9d3f` and changes only the large
`source/ai/actions` reconstruction plus the headers that own its shared enums
and prototypes.  The pre-wave Actions gate was 55/63 strict-exact functions.

The source frontier came from the Fable history rather than a wholesale
cherry-pick:

- commit `79275995c484a2637aa5db570030d42a9e02685d`, source blob
  `d8d32fb630fb70e6bc8733efd97fcd69b31c397f`, supplied the combat-selection
  and first evasion candidates;
- commit `c8ff585ee941bd8454e991d7512b5b36ba0f161a`, source blob
  `92eb33aa4cd3e94a2dda31edf6bf9063faf59de1`, supplied the danger-avoidance
  experiment;
- the later house-rule commit
  `2a9ede1cc37821492fe56ddbecb5898f60073605`, source blob
  `bc1ceba344612a50a191e36f2fda1d8d47d0242f`, is controlling evidence for
  the no-fake review below.

No donor commit was transplanted wholesale.

## Accepted strict functions

`_actor_action_handle_combat_selection` is strict-exact:

- 1,776 padded bytes;
- 56 relocations with identical addresses, types, and destinations;
- normalized SHA-256
  `2fb7b29eea215e53086b6ce6fd971c3eb4ed361fa45f1c836d3e4833c2edf5ed`.

The unusual calls were reviewed rather than accepted merely because they
match.  The independent target lift at
`research-cache/stian-halo-current-20260827/src/halo/ai/actions.c` lines
2461-2713 proves the controlled-unit type-verification call and the discarded
`actor_has_ranged_weapon` call before melee charge setup.  Both calls therefore
express original behavior, including their validation/side effects; neither is
an inert compiler lever.  The same lift corroborates the vehicle cooldown and
lookup order, charge-state policy, and final assertion ladder.  Negated real
comparisons retain the January x87 unordered/NaN behavior and are not Boolean
noise.

`_actor_action_handle_evasion` is strict-exact:

- 800 padded bytes;
- 27 relocations with identical addresses, types, and destinations;
- normalized SHA-256
  `f38b2a0650790ee8d848ca0a4faee7a8cee3c00e2ad3048412b62c8db594370b`.

The admitted source is ordinary structured C: the first successful cover or
panic action sets `result`, and the remaining policy is guarded by
`if (!result)`.  The perceived-danger branch is positive, later successes only
set `result`, and the function has one terminal return.  There is no donor
`goto`, label, assembly, volatile state, wrapper, optimizer directive, object
patch, or undefined behavior.  Direct-return, compound-guard, and nested
single-exit forms were measured; the nested single-exit form is both natural
and uniquely strict.  `boolean result;` followed by `result = FALSE;` is kept as
an evidence-backed exception to the declaration/assignment preference: a
same-line initializer changed 657 bytes and the relocation schedule.

The accepted gain is therefore two functions and 2,576 padded code bytes:
55/63 to 57/63.

## Honest rejection of a coincidental exact result

`_actor_action_handle_danger_avoidance` is deliberately **not** accepted.
The donor's only strict form changed the ordinary condition

```c
if (!body_within_danger)
```

into an empty positive arm followed by the real work in `else`.  Fable commit
`2a9ede1cc` independently identifies that shape as an exact-only fake-match
generator and reverts it.  This wave keeps the coherent negative condition.
The result is honestly residual at 1,504 candidate bytes versus the 1,520-byte
January function, with 34 relocations on both sides and candidate normalized
SHA-256
`b1d5ecaf33b392f8b6071a515b226a8c41e112097105096e0216f9554e3b328f`.
No credit, exception, adjudication, or park is added for it.

## Private helper and inline ownership

The private `_code_0000c970` placeholder is renamed
`actor_action_find_escape_from_danger` and remains `static`.  HCEA PDB/cross-
build correspondence and the January-addressed function atlas both provide
that original name; this is not an address-derived invention.

An initial exact experiment manually reproduced the formal parameter aliases
of `point_from_line3d`.  It was rejected under the no-fake rule because those
aliases conveyed no policy and existed only to control compiler scheduling.
Production instead uses six plain coordinate assignments, an established
project idiom.  This ordinary form remains strict-exact at 928 bytes, 28
relocations, and normalized SHA-256
`ec0b698df2d40432b1b2a912ebc33eca0a989f2420cc69d3b184ab8131a34e11`.
The rebuilt object owns no symbol containing `point_from_line3d`, preserving
the January inline/COMDAT boundary without fake source.

## Type, enum, and prototype ownership

- `destination_type`, `actor_vehicle_driver_type`, and the danger-zone enums
  live in `actors.h`, their closest shared owner.
- `action_class` lives in `actions.h`.
- the complete communication type/hostility enums and
  `ai_communication_event` declaration live in `ai_communication.h`.
- local numeric switch/action/hostility values were replaced with the matching
  enum constants.
- the duplicate local danger enum in `actor_perception.c` and duplicate local
  communication prototype in `actions.c` were removed.
- all real-valued locals use `real`; typed object access uses the appropriate
  `unit_get`/`vehicle_get` paths.

## Verification

The final rebuilt Actions gate reports 57 exact, six residual, and zero
unwritten functions.  The six residuals are the honest danger-avoidance form
plus the five inherited residuals:

- `_actor_action_handle_lost_contact`;
- `_actor_action_handle_vehicle_entry`;
- `_actor_action_try_to_dive`;
- `_actors_searching_same_position`;
- `_code_0000b3b0`.

The full `halobetacache_build` and `libcmt_build` targets pass.  Consolidated
`progress` and `semantic_progress` pass with 470 units scanned, 4,986 functions
evaluated, 4,876 semantic-exact functions, 131 hidden exact functions, 4,886
accepted exact functions, and zero unit errors.  All 212 tool tests pass.
`tools.parked_functions` reports 13 active, zero stale, and zero invalid
entries.  `git diff --check` is clean.

Claude's prototype fake-smell scanner reports no smell in `actions.c`.  Its
three reports are pre-existing empty-then forms in `actor_perception.c`, outside
this Actions admission and unchanged by this wave.  They receive no new credit.

The old pre-wave regression manifest is not used to waive failures: moving
enums/prototypes to their owning headers and applying the supported private
symbol rename intentionally changes compiler-private/debug ownership.  Strict
function comparison proves exactly two accepted gains and no loss among the 55
pre-wave exact functions.  A clean committed-state Actions snapshot and forced
rebuild/check are captured after this ledger is committed.
