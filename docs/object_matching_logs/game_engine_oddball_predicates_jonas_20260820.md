# `game_engine_oddball.obj` Jonas predicate wave

This Jonas-owned checkpoint records one bounded semantic attempt on the two
small oddball predicates. Neither predicate was retained: the first candidate
compile produced the correct section sizes and relocation identities but not
the January instruction bytes, so both bodies were removed without tuning.
The independently authenticated typed oddball BSS was strict-exact and is the
only retained recovery.

## Scope and provenance

- Authoritative integration base:
  `6efbc916e2f34b9ad35e4d4d7dacf5aec68f9748`.
- Baseline `game_engine_oddball.c` blob:
  `92372f1750b71edff0c23d0700a97d4123911028`.
- January split `game_engine_oddball.obj` SHA-256:
  `70aa145fd07f88ee32489a708e7b01f521555bb2751621bc19d4919373a856e1`.
- HCEA semantic-oracle commit:
  `0fd88176fe19df400576d2cbb447d3363080cb52`; the
  `player_has_ball.c` and `ball_available.c` blobs are respectively
  `4a01b3cd200aef5c52d41b47f060cf083b872f7f` and
  `ce7ee614e721b2ba644629571a3fda34225257f3`.
- The independently recovered oddball layout is also present at HCEA
  C-series commit `570c83fd9c365dad6f2a3e7041705d5b84c7847c`;
  `oddball_globals.h` has blob
  `6f8c59ec879099db3a70b0d6f8487286fecf5117` in both local oracle copies.
- HCEA supplied names, types, and behavior only. The January PC object was the
  sole byte, relocation, and section authority.
- Only `source/game/game_engine_oddball.c` and this new ledger are changed.
  No shared header, configuration, frozen object source, or prior Markdown is
  changed.

## Bounded predicate result

One readable typed-C candidate used the existing `long` game-variant field at
offset `0x60` as the ball count. It scanned the named
`current_ball_owner[16]` array for `_player_has_ball`, and scanned the paired
`ball_spawn_timer[16]` and `current_ball_owner[16]` arrays for
`_ball_available`.

The single candidate compile was rejected in full:

| Rejected function | Target meaningful / padded | Relocations | Target normalized SHA-256 | Candidate normalized SHA-256 | Difference |
| --- | ---: | ---: | --- | --- | --- |
| `_player_has_ball` | 45 / 48 | 2 | `ba65ac574b19936b0b398f5e51fd591377c0108e2d5e36ff45dcd5c44f1af3da` | `d5a8541925f867c120b587fe29e38aa48ebae455dadc85781cdc529bbf6e6361` | Target relocation offsets `5,26`; candidate `4,23` |
| `_ball_available` | 49 / 64 | 3 | `1887f97956fc1d4a680f5005ed1d1a83ede9ca10ac7f8f1158e86dbbc3cdb4ef` | `6f2cced71ac5409acfdcbd0a22555c382d31b50b8a9197df23ff77df13cc661b` | Target relocation offsets `2,20,31`; candidate `2,21,32` |

The relocation destinations were semantically correct in both candidates:
`_game_engine_get_variant`, `bss_0043eba8+0x84`, and
`bss_0043eba8+0xC4` as applicable. The mismatch was the compiler's natural
prologue/register/loop shape, not missing semantic evidence. No declaration,
control-flow, scheduling, or code-generation tuning pass followed. Both
function symbols are absent from the retained object.

## Strict data result

The January symbol `_bss_0043eba8` is one zero-filled `.bss` section of size
`0x104`. Independent layout evidence names the complete storage:

- `score_to_win` at `+0x00`;
- `team_score[16]` at `+0x04`;
- `individual_score[16]` at `+0x44`;
- `ball_spawn_timer[16]` at `+0x84`;
- `current_ball_owner[16]` at `+0xC4`.

The retained local structure uses those typed fields and compile-time size and
offset checks. Its natural zero initialization emits the same `260`-byte
`.bss` section, with zero relocations and normalized SHA-256
`9c0095c04ef53d9df41602f3783c90ef3c3e27cc9d0b38262d23930db6313f5a`.
Direct fail-closed COFF comparison reports the target and base sections equal.

The object remains `NonMatching` at 11/38 strict functions. Its matched data
advances from 0/744 to 260/744 bytes. All 11 pre-existing exact functions were
replayed by direct semantic COFF comparison and remain equal.

## Policy and gates

The retained source is a defined-C typed data declaration with no raw-offset
access, integer-address reconstruction, union aliasing, assembly, volatile
scheduling, forced inline, optimizer directive, synthetic anchor, or
byte-forcing device.

The complete Halo and libcmt builds pass. Semantic audit reports 470 units,
3,966 functions evaluated, 3,826 semantic exact, 3,887 accepted exact, and zero
unit errors. Campaign progress is 375/833 complete objects, 3,874/11,060 exact
functions, 471,564/2,198,102 exact code bytes, and 1,835,068/4,176,062 matched
data bytes. The checkpoint therefore contributes exactly 260 matched data
bytes and no function or code credit.

Admission reports zero candidates and zero revocations, with only the inherited
`source/shell/shell_xbox` contradiction. Park validation reports three active,
zero stale, and zero invalid entries. The complete tooling suite passes 179/179
tests. No GitHub push is performed.
