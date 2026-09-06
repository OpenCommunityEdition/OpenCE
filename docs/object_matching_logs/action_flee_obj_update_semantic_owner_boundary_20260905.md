# `action_flee.obj` update semantic and owner boundary, 2026-09-05

## Result

`action_flee_update` is **withheld**. This review admits neither exact nor
fuzzy code and creates no park. The current production object remains at its
published two strict functions; the attempted update remains unwritten.

Two independent blockers were demonstrated:

1. The required public-API ownership repair changes two inherited strict Race
   functions and the existing AI speech-timer park in a full 84-object header
   trial. The entire four-file API packet was restored; there is no
   caller-local declaration or header-position workaround.
2. The first natural 160-byte body writes the positive-panic timestamp to the
   wrong, adjacent semantic member. January writes actor `+0x39C`; the
   candidate writes `+0x398`. This is not an ordinary scheduling residual and
   is not eligible for a fuzzy park.

The member discrepancy is now resolved by a complete neighboring-use census:
the existing layout is correct. Actor `+0x398` is
`emotions.last_flee_failed_time`, while `+0x39C` is
`emotions.flee_with_friends_disable_time`. A new source manifest containing
only that semantic member correction is preserved pending a future genuine
API context. It was deliberately not compiled against restored, incorrectly
owned declarations.

## January behavior and ABI

The authoritative January `_action_flee_update` is 152 meaningful / 160
padded bytes, has four relocations, and contains 54 decoded instructions.
Its normalized SHA-256 is
`c5e5c557ca1dc9c70ea3fe4dbfff11451fc6a5b7a1b71921f839386588fbe2d8`.
Its call/data relocations, in order, are `_actor_data`, `_datum_get`,
`_unit_start_running_blindly`, and `_game_time_get`.

The target establishes the following source contract:

- `actor_index` is a complete 32-bit datum handle. The prologue loads the
  full dword argument and passes it to `datum_get`; the HCEA donor's
  `uint16_t` parameter is not the January ABI.
- `actor->meta.unit_index` at actor `+0x18` is also consumed and passed as a
  complete 32-bit handle.
- `forced_flee_ticks` at `+0x9C`, `flee_stationary_ticks` at `+0x9E`, and
  `panic_type` at `+0xA8` are signed 16-bit values.
- `flee_timer` at `+0xB4` increments every call with 32-bit machine wrapping.
- A positive forced-flee countdown decrements once. A positive stationary
  countdown decrements once; only its `1 -> 0` transition can start blind
  running, and only when the unit is not `NONE` and the signed panic value is
  in the inclusive `[9, 12]` interval.
- The blind-running decision naturally calls the already strict
  `action_flee_blind_panic(short)` source helper. The compiler may inline its
  range test; the helper must not be manually expanded or artificially
  retained.
- Independently of the unit/countdown path, any signed `panic_type > 0`
  calls `game_time_get`, adds 750 ticks (`25*TICKS_PER_SECOND`) modulo 32
  bits, and stores the result at actor `+0x39C`.
- When both calls occur, `_unit_start_running_blindly` precedes
  `_game_time_get`. The function returns `void` and preserves its callee-saved
  state and stack.

## The adjacent fields are not a missing-layout gap

Three independent January-backed uses distinguish the two adjacent dwords:

- In `_action_flee_perform`, after setting the flee state's
  `unable_to_flee` byte, the target calls `_game_time_get` and at function
  offset `+0x1C3` stores EAX to actor `+0x398`.
- `_actor_action_handle_panic_transition` tests actor `+0x398` against
  `NONE`, calls `_game_time_get`, reloads `+0x398`, adds seven ticks, and
  compares it with the current time. Current source names that value
  `last_flee_failed_time`.
- The strict January/canonical `_actor_stimulus_prop_fleeing` calls
  `_game_time_get` and at function offset `+0x58` compares it with actor
  `+0x39C`. Its natural typed source names that value
  `flee_with_friends_disable_time`; the function is strict exact at
  263 meaningful / 272 padded bytes and 14 relocations, normalized SHA-256
  `13df393dee019d54339f963e17edd5d85610d11ce48fe5c2e357240b22f65da1`.

The canonical `actor_emotion_data` already declares those members in that
order. Its base is actor `+0x350`, so relative offsets `+0x48` and `+0x4C`
produce the observed actor offsets `+0x398` and `+0x39C`. The HCEA
`actor_emotion_data.h` independently corroborates both names and offsets, but
its `action_flee_update.c` incorrectly selects `last_flee_failed_time` and is
not authoritative. The extracted January PDB census contains no member-level
type record for these fields, so no direct PDB-name claim is made.

The behavior is also coherent: a live positive panic refreshes a 25-second
cooldown that suppresses group/friend-flee propagation, while a failed flee
attempt records its separate, short-lived failure time at `+0x398`. No
padding, field insertion, or four-byte schema shift is proposed.

## First natural body and executable behavior review

The first natural source used `actor_get`, the existing typed flee-state and
emotion layouts, the already strict blind-panic helper, and owner APIs. Its
focused gate was `2 exact / 1 residual / 10 unwritten`; both inherited strict
functions and the hard `_point_from_line3d` guard were preserved. The new
function is also 160 bytes with four relocations and 54 instructions. Its
normalized SHA-256 is
`6e9a8d4d811187492ebf30f9a1942589321ce1e5beedf2c30045564be563bb28`.

Aligned disassembly finds exactly one instruction difference:

```text
target    +0x8F  mov dword ptr [esi + 0x39c], eax
candidate +0x8F  mov dword ptr [esi + 0x398], eax
```

The bounded Unicorn harness executes only `_action_flee_update` and stubs
only its three genuine call boundaries: `_datum_get`,
`_unit_start_running_blindly`, and `_game_time_get`. It verifies exact API
arguments, event order, all actor bytes outside target-written fields,
callee-saved registers, and stack balance. Its 47 cases cover:

- signed countdown boundaries `-32768, -1, 0, 1, 2, 32767`;
- `NONE` and full-width `0x89ABCDEF` unit handles;
- panic boundaries `-32768, -1, 0, 1, 8, 9, 10, 12, 13, 32767`;
- actor handles `0x00000007`, `0x12340007`, and `0xFFFF0007`;
- zero, sign-boundary, and all-one flee counters; and
- timestamp addition across 32-bit wrap.

The frozen January target passes **47/47**. The first candidate passes
**27/47** and fails exactly the 20 cases with positive panic. Every failure
has the same adjacent-field difference spanning actor bytes
`[0x398, 0x3A0)`: the candidate overwrites `last_flee_failed_time` and leaves
`flee_with_friends_disable_time` unchanged. Countdown behavior, complete
handles, calls, call order, and counter behavior agree in the remaining
observations. This is direct behavioral rejection of the candidate, not a
request to adapt the expected offset.

## Required owner-header repair and failed blast

A source-faithful production packet requires these ownership changes as one
coherent boundary:

- declare `void action_flee_update(long actor_index)` in `actions.h` and
  remove its consumer-local declaration from `actions.c`;
- declare `void unit_start_running_blindly(long unit_index)` in `units.h`
  and remove its owner-C local declaration from `units.c`; and
- include `game/game.h` from `action_flee.c` for the already owned
  `game_time_get` API.

Root tested the pure owner-header migration across the complete 84-object
consumer union before admitting the body. It materially changed
`_race_touch_flag` (336 padded bytes) and
`_race_engine_player_update` (160 padded bytes), losing both inherited Race
stricts. It also changed the normalized body of the existing
`_ai_communication_update_speech_timers` park. The packet was therefore
withheld without header-position permutations, local-prototype fallback, or
shim headers.

After restoring all four API files, the full build returned to 6,217 strict
owners out of 8,245 with zero losses, and all 281 parks validated. Production
`action_flee.c`, the owner headers, and configuration remain unchanged by
this review.

## Frozen evidence

| Artifact | SHA-256 | Disposition |
| --- | --- | --- |
| `scratch/action-flee-update-target-frozen-20260905.obj` | `5D6E667103C4B42824A5E1C69FB047860A60D901EDA36153162CF19672EFF59E` | January target |
| `scratch/action-flee-update-natural-api-trial-20260905.obj` | `FA607D970E3E97D39519086C1FFA09008E98659211C35E2D2BF760D485E8B6D9` | rejected wrong-member candidate |
| `scratch/action_flee_update_natural_20260905.edits.json` | `2D9D0F48A831E2B2E9FF8EA1D1B8C1E6C2BAA797C79CDECA8CB8CD5B241B3197` | immutable first-source manifest |
| `scratch/action_flee_update_semantic_corrected_pending_20260905.edits.json` | `14D98FCD9964F932CDF92AB99364B974EEE009E4B75CB0B6A97C0577F980C58E` | one-member source correction, intentionally uncompiled |
| `scratch/action_flee_update_behavior_review_20260905.py` | `927D2B29FAF1A400B7429F60FE24146E8DC520CC71D617C156FD45E76DDAE1C2` | bounded behavior harness |
| `scratch/action-flee-update-target-behavior-reviewed-20260905.json` | `910C06F6630F38AC25B3B2E2F3A013BDC8CC577C717235515C98776FF620DE69` | target 47/47 pass |
| `scratch/action-flee-update-candidate-behavior-reviewed-20260905.json` | `EB6EC2F6A66739A828B6C2A2DCA431A9185EEF45015B427930DB1AB8CFF0D4C0` | candidate 27/47 pass, 20 semantic failures |
| `scratch/action-flee-api-failure-summary-20260905.json` | `6C68F7B76ACC6120F8D50A121FEAA206918763BCE5EEE83B69BACDEF168A88BD` | 84-object owner-header failure census |
| `scratch/action-flee-api-restored-20260905.json` | `5EF39E7AD811893F6E1FFAA2F46F960D06592AEDB144E958FF646E386B72CFB7` | restored stable census |
| `scratch/action-flee-api-parks-restored-20260905.json` | `92ED21DB874053C53684CDD519F70A2D1ACAAD96F76F9E0E8E0CD866879DB739` | restored 281-park validation |
| `scratch/actor-stimulus-prop-fleeing-canonical-20260904.obj` | `2FE9919CFE7DF2B7E8A7C6A895EB2DEECBA75FF12856BEB162E6377F1B731764` | strict `+0x39C` consumer |

Harness reproduction uses frozen objects only:

```text
python scratch/action_flee_update_behavior_review_20260905.py scratch/action-flee-update-target-frozen-20260905.obj --output scratch/action-flee-update-target-behavior-reviewed-20260905.json
python scratch/action_flee_update_behavior_review_20260905.py scratch/action-flee-update-natural-api-trial-20260905.obj --output scratch/action-flee-update-candidate-behavior-reviewed-20260905.json
```

## Reopen condition

Reopen only as a coherent owner-header trial after the Race and speech-timer
collateral can be reconciled by independently authenticated source/API
evidence. At that point compile the preserved corrected manifest once in the
genuine header context and repeat the behavior, full owner, no-point, park,
and whole-tree stable checks. Do not patch the old object, shift the actor
layout, restore caller-local externs, permute declarations, or classify the
wrong-member candidate as fuzzy progress.
