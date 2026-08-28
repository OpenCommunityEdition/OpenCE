# `actions.obj` exact-match log

## Scope and standard

This lane reconstructs `source/ai/actions.obj` from the January 14, 2002 Xbox
object.  The HCEX decompilation and Fable correspondence packets are source
topology hints only.  January code bytes, relocation identities, data bytes,
and symbol ownership decide every result.  The lane uses the established XDK
3911 compiler and unchanged `/O2 /Oy- /DDEBUG /Dxbox` flags.  Assembly,
volatile, inline-control keywords, optimizer barriers, undefined behavior,
byte forcing, flag changes, and comparator exceptions are out of scope.

## Fresh baseline (`fc8e1b07`)

The target contains 63 functions and 19,172 meaningful padded code bytes.
The fresh candidate has zero exact functions: 62 functions are absent and
`actor_action_debug_color` is 160 bytes with 15 relocations versus January's
176 bytes and 15 relocations.  January also owns the 888-byte/96-relocation
`global_action_functions` constant block and the 16-byte/3-relocation
`global_actor_mode_names` data block; neither is emitted by the baseline.

Fable's exact-unique name map covers 55 of the 63 functions (17,484 January
bytes), and the local HCEX tree has readable C for those names.  That coverage
does not grant match credit.  It only gives this lane a much better starting
shape than decompiling every body from raw instructions.

## Strategy

The first wave is deliberately small: callback dispatchers, names, class and
default-state accessors, plus the existing debug-color function.  These are
the low-risk functions that establish the action table's exact layout and
common dispatcher shapes.  Header-inline helpers are not duplicated in this
translation unit; they should be emitted naturally once real callers require
them.  The entire object is rebuilt and compared after every coherent wave so
an apparent local win cannot hide a sibling or ownership regression.

## Experiments

### Wave 1, natural HCEX/January topology

The six dispatcher bodies were written as ordinary callback-table lookups with
the January action-index assertion restored at its original file and line.
The two name getters and default-state getter use a single initialized result
and one range guard.  `actor_action_debug_color` retained the existing body and
restored the missing signed lower-bound check.  The first compile made nine of
the eleven functions exact.  The only two misses were otherwise-identical
conditional branches: `jae` in the candidate versus January's `jge` in
`actor_mode_name` and `actor_action_get_default_state`.

That difference came from using `NUMBEROF(array)`, whose unsigned result
converted the short index before the upper-bound comparison.  Replacing the
two unsigned counts with named enum constants kept the code readable and
restored the original signed comparison.  The second compile made all eleven
functions exact.  `_global_actor_mode_names` also matches as a 16-byte data
owner with the exact three string relocations and null final entry.

Strict result after Wave 1: **11/63 functions**, **1,088/19,172 code bytes**,
and **16/16 bytes of the actor-mode-name data owner** are exact.  No existing
exact function regressed and no shared header was changed.

### Waves 2-4, panic and berserk leaves

The next twelve leaf functions were recovered from the exact-unique HCEX map,
then checked individually against January.  The HCEX bodies supplied names and
high-level intent, while January supplied the missing Xbox-only details.  Two
general source-shape findings mattered:

* The redundant sign extension in the attached-melee panic handler comes from
  applying the ordinary `MAX` macro directly to a short field and an enum.  An
  explicitly typed temporary removes that promotion and does not match.
* January has a local-game debug bypass in the panic-from-damage threshold
  which is absent from HCEX.  Naming byte +8 in `ai_debug_state` recovered the
  condition without changing that structure's layout.  January only applies
  the bypass to a local connection; spelling the connection test in the other
  direction leaves one branch different.

The projectile and burning handlers also proved the responsible-unit and
active-prop call chains.  January's relocation identities, not just the call
count, were used to distinguish those helpers.  At the end of these waves the
object was **23/63 functions exact, 2,688/19,172 code bytes**; all earlier
matches remained exact.

### Wave 5, action stopping and transition denial

`actor_action_can_stop_guarding` first compiled to the correct logic but used
four-byte integer result materialization.  Casting the two comparison
expressions to the real one-byte `boolean` type produced January's `setge al`,
`xor al,al`, and shared one-byte true exit exactly.  This is a type recovery,
not a code-forcing cast: the API genuinely returns the project's byte-sized
boolean.

`actor_action_deny_transition` initially nested `encounter_get` inside
`encounter_get_squad`.  That changed argument evaluation and cleanup order.
Binding the encounter datum first, as a readable named local, reproduced
January's call order and naturally emitted the 144-byte
`encounter_get_squad` header helper.  Spelling the timer branch as
`combat_status >= 5` followed by expiration (with denial in the `else`) fixed
the remaining branch direction.  Both functions and the helper are strict
exact.

For `actor_action_can_stop_conversing`, an initialized `TRUE` result preserves
the no-conversation exit.  A compound logical expression and three explicit
early returns both placed the shared true block at the end.  The readable
`if/else if/else` result chain places the first true block where January does
and makes later true branches jump backward to it.  The result is strict exact,
including the scenario tag-block access and all six relocation destinations.

Strict result after Wave 5: **27/63 functions**, **3,280/19,172 code bytes**,
plus the exact actor-mode-name data owner.  No exact sibling regressed.

### `actor_action_allow_cover_seeking` live residual

The semantic body is recovered and January proves that the damage comparison
uses `recent_body_damage` (+0x1c0), despite the HCEX reconstruction naming it
as shield damage.  Direct definition-field accesses recover January's x87
loads and avoid a spurious float local.  Four declaration/scope variants for
the short visibility duration and the long-lived `allow` boolean have been
measured.  They preserve 8/8 relocation destinations but our current source
keeps the duration in `BX` and spills `allow`; January keeps `allow` in `BL`
and reuses the dead second-parameter home for the duration.  Declaration order
alone did not move this allocation.  The readable best body remains in source
as a nonexact reconstruction; no credit is granted, and register-forcing or
volatile tricks are barred.

### Waves 6-7, setup leaves and transition handlers

`actor_action_try_to_seek_cover` and `actor_action_try_to_panic` use a complete
`action_state_data` union, not a member-sized scratch buffer.  The full union
is part of the original readable contract even though each setup helper only
writes its own member; it also reproduces January's stack frame and argument
layout.  `actor_action_handle_pending_command_list` likewise needed the full
union and the original ordering: perform the immediate action first, then
clear the command-list reference.  All three compiled strict-exact on the
first measured natural shape.

`actor_action_handle_done_fleeing`, `actor_action_handle_combat_failure`, and
`actor_action_handle_berserk_transition` were recovered from the mapped source
and January control flow.  The fleeing handler retains the original `success`
assert and source line 2681.  `actor_action_handle_combat_transition` also has
an Xbox-only tail absent from HCEX: when already in combat with status zero it
calls `actor_action_class` and deliberately discards the value.  That call is
not dead trivia--it preserves the original range assertion.  These four
handlers are all strict-exact.

`actor_action_handle_exit_pursuit` exposed a useful source-shape rule.  Assigning
each action's done flag directly to the return variable made VC7 reuse `AL` for
the flag tests instead of January's `DL`.  Calling lost-contact directly from
each successful case fixed the registers but allowed the compiler to combine
the preceding cdecl cleanup in the wait arm.  The exact readable topology is:
test each action flag directly, set a one-byte `reported = TRUE` marker after
any perception notification, then make one common lost-contact call when the
marker is set.  This preserves separate cleanup, keeps flag tests in `DL`, and
leaves the called result naturally in `AL`.

Strict result after Wave 7: **33/63 functions**, **4,208/19,172 code bytes**,
plus the exact actor-mode-name data owner.  Every previously exact sibling
remained exact.

### `actor_action_handle_grenade_throwing` live residual

The January semantics and all seven relocation destinations are recovered.
Readiness requires a target type of at least five and rejects a fleeing actor
whose panic type is positive; the variant definition is fetched before those
checks.  The two grenade-stimulus cases use combat status or the prop's swarm
state, and an already-armed throw is attempted after consideration.  The best
readable body is 224 bytes versus January's 208.  January keeps the result in
`BL`, reloads the actor index for calls, and emits a sign-extended short switch;
the current compiler keeps the actor index in `EBX`, spills the result byte,
and retains word comparisons.  Moving the result and short declarations did
not change that allocation.  The function remains nonexact and uncredited;
broader semantic work continues before any tie classification.

### Wave 8, vehicle and grenade leaves

`actor_action_try_to_enter_vehicle` initially gave VC7 a separate working
pointer for either the caller's seat list or the local fallback array.  That
extra lifetime changed the January register assignment.  Reusing the optional
parameter itself as the working pointer is both ordinary C and the natural
contract: when the pointer is null it is assigned the local array, otherwise
the caller-supplied count is used.  Keeping the loop count short and sharing
the final false return then reproduced all 208 bytes and six relocations.

`actor_action_try_to_throw_grenade` required four semantic details that the
later HCEX body obscured.  The damage rejection is spelled `!(damage > 0)` so
the original NaN path is preserved; the normalized direction must be positive;
the facing rejection is `!(dot < cosine)`; and the throw order sets the combat
flag before clearing the pending-plan flag.  The minimum cosine is named from
its January float bits rather than replaced by a decimal approximation.  The
result is strict-exact at 288 bytes and twelve relocations.

The neighboring `actor_action_consider_grenade` is reconstructed and has the
exact 288-byte size and all eleven relocations.  Its remaining bytes are only
three references to one stack slot: January colors the base-chance temporary
at `[ebp-4]`, while this build colors it at `[ebp-8]`.  Tested shapes include a
single combined expression, three named float temporaries, moving those
declarations outside the check, moving them before the last-check local, and
scoping the last-check local inside a nested block.  The best readable shape
retains the named base/difficulty/result calculation and is not credited.

### Wave 9, exact vehicle-exit cluster

The mapped HCEX vehicle-exit topology incorrectly returned immediately when
the actor had no vehicle.  January always clears `stimuli.vehicle_eviction`,
including that path, so the search is a guarded body followed by unconditional
cleanup.  The first corrected shape reached exact size and relocation count,
but kept the successful `exited` value live in `BL` across `game_time_get`.
January calls `game_time_get`, stores the re-entry deadline, and only then sets
`exited = TRUE`.  That source statement order lets VC7 return constant `AL=1`
on the successful path while retaining the false result slot for both failure
paths.  The complete handler is strict-exact at 336 bytes and seven
relocations.

The adjacent private `code_0000b140` is the vehicle re-entry cooldown check.
It takes the actor index in the compiler-selected `EAX` private convention and
the vehicle index on the stack, returning false only while the requested
vehicle equals `vehicle_ignore_index` and the deadline is still in the future.
The direct readable predicate compiled strict-exact on its first build: 64
bytes and three relocations.

### `actor_action_handle_vehicle_entry` checkpoint residual

The full 960-byte January behavior is reconstructed, including both candidate
sources: friendly player-driven vehicle props and the 32-entry scripted
enterable-vehicle registry.  The source-local registry layout is proven as
0x28 bytes per entry at `ai_globals+0x3B8`; no shared AI header was expanded.
The helper calls, object types, distances, team/type masks, packed AI-index
matching, and action setup give the exact 22 relocation destinations.

The first natural form was 960 bytes but mirrored the long-lived actor pointer
and actor index between `EBX` and `EDI`.  Making the result a real named
success/failure value restored January's register roles.  Further measured
changes were semantic rather than register forcing:

* spell the best-distance test as `!(distance < best)` to reject NaNs exactly;
* compare the packed encounter component through a dword xor plus `& 0xFFFF`,
  rather than letting VC7 narrow the xor to a word;
* express AI scope selection as a switch, producing January's decrement chain;
* increment the long loop variables normally while comparing their short
  views, avoiding extra sign extension; and
* assign the prop-derived squared distance before its vehicle index, allowing
  the independent integer store to fill the x87 multiply window.

Those changes make the body instruction-identical over its substantive search
windows.  The best checkpoint form is 944 bytes with all 22 relocations and
differs in the false-return topology: January materializes a zero byte in
`DL` for the three early guards and retains a separate late false epilogue;
this compiler folds both blocks despite equivalent named-result and explicit
label spellings.  No barrier, volatile, register forcing, or control-flow byte
trick was attempted.  The readable body stays as a nonexact reconstruction
and receives no credit.

## Checkpoint status before canonical reconciliation

The independent wave ends at **37/63 strict-exact functions** and
**4,760/19,172 meaningful January code bytes (24.83%)**.  Those functions
occupy 5,104 padded section bytes; keeping the meaningful and padded measures
separate avoids overstating progress.  The 16-byte/3-relocation
actor-mode-name data owner is also strict-exact.  The 888-byte action dispatch
table remains deliberately disabled because callbacks in other unfinished
action translation units are not yet linkable; it receives no data credit.
The only shared-header changes are prototypes in `actions.h` and a
layout-neutral name for the already-existing byte at `ai_debug_state+8`.
`actors.h`, `ai.h`, `ai_communication.h`, `encounters.h`, `path.h`, and the
other headers concurrently used by the AI-debug lane were not modified.

## Canonical reconciliation and final checkpoint gates

The checkpoint was rebased onto canonical `a0bad121` before final validation.
The Actions object was then forcibly deleted and rebuilt with the unchanged
January campaign flags.  The hardened semantic audit still finds exactly
37 strict functions (4,760 meaningful bytes), while direct `coff_compare`
confirms `_global_actor_mode_names` at 16 bytes, three relocations, and
normalized SHA-256
`374708fff7719dd5979ec875d56cd2286f6d3cf7ec317a3b25632aab28ec37bb`.
The object remains `NonMatching`; no function, data, semantic, or parked
exception was added to configuration.

The fail-closed regression gate took a fresh Actions snapshot, deleted and
rebuilt the object again, and reported all 37 functions `still_exact`, with
zero changed nonexact functions, newly exact warnings, failures, ownership
warnings, or other warnings.  Full `halobetacache_build` and `libcmt_build`
both passed after the rebase, so the layout-neutral header name and Actions
prototypes compile across every consumer.  `progress` and `semantic_progress`
completed with 470 units scanned, 3,473 semantic-exact functions, 93 hidden
exact functions, and zero unit errors.  The admission audit produced no new
completion candidate; this checkpoint intentionally receives no object-level
credit.

All **179/179** tool tests passed.  `git diff --check` is clean, the config
diff is empty, and the added production code contains no assembly, volatile,
force/no-inline annotation, optimizer barrier, undefined-behavior trick,
byte forcing, compiler-flag change, or comparator change.  Original behavior
that may look surprising is retained and explained above rather than silently
"fixed".  This is therefore an honest readable-C foundation checkpoint, not
a claim that `actions.obj` is finished.
