# `players.obj` credibility recovery — 2026-08-30

## Result

This change keeps coherent typed Players logic and removes reconstruction-only
code-generation steering.  It does **not** treat an exact byte sequence as
evidence that implausible source was original.

The current focused measurement, rebuilt from this lane, is:

| Measure | Result |
|---|---:|
| shared exact functions | 52 / 70 |
| shared exact code bytes | 7,744 / 15,904 |
| shared residual functions | 17 / 70 |
| residual code bytes | 8,144 |
| target-only functions | 1 |
| comparator errors | 0 |

This supersedes the earlier `59/9/2` claim.  That claim included code emitted
by a dead `if (0)` path and was not a valid source-credit census.  The dead
emitter is not restored.

## Credibility decisions

Removed constructs include volatile reinterpretation reads, `noinline`,
`_ReadWriteBarrier`, two artificial inline distance helpers, and dead emission
paths.  The retained implementation uses direct typed data, `distance3d`,
`TEST_FLAG`/`SET_FLAG`, action-result enums, and ordinary calls.  A focused
fake-match scan of `players.c` and `players.h` reports zero review leads.

`players_update_before_game` and `players_update_after_game` are retained as
normal game-loop callers.  They supply meaningful player spawning, control,
telefrag, powerup, interaction, and BSP-switch behavior; they were not added
as a byte-emission device and currently remain fuzzy where the comparator says
they do.

The former address labels in the bounded Players block are replaced with
January-atlas names.  Target storage class decides linkage: file-local helpers
are `static`; `player_teleport_internal` and `player_handle_powerup_equipment`
remain external and are declared in
`players.h`.  Declarations for other subsystems live in their owner headers.

`player_examine_nearby_unit` is deliberately **target-only**, despite its
one-byte January target body being `ret`. The atlas and two independent donor
trees authenticate a boolean two-handle ABI, but the no-argument C no-op that
would reproduce `ret` is a contradictory fake. Donor source disagrees on
behavior: one has a full interaction implementation while another records the
XBE as a stub. We retain only the header ABI and do not grant matching credit
until first-party provenance explains the stub.

The local vehicle projection intentionally remains private to `players.c`.
`VEHICLES.C` still has a larger independent runtime definition, so publishing
an incomplete global vehicle type would be false ownership rather than a
useful header cleanup.

## Data boundary

The `0x00453408` BSS symbol is the external
`machine_to_player_table[4][4]` (0x40 bytes).  The following screen-flash and
debug state is represented by separate meaningful statics; no invented 0x5C
aggregate or padding global is claimed.  `players_static_data` at `0x002DEE08`
is an external-linkage object whose full profile-storage layout remains only
partially inferred, and `adjustment_weights` is a static read-only table. Data
ownership beyond those observed bounds is deliberately
not credited by this lane.

The current project report records 3,096 matched data bytes out of 5,349
Players data bytes. That aggregate is a progress measure, not an ownership
admission: this lane did not rerun a per-symbol data comparator and therefore
does not claim an exact section census.

The telefrag fields are named from their observed game-engine transition:
collision marks a blocking player and increments the timeout; the after-game
update decays or resolves it.  The Players global packs the local triggering
player in bits 0–3 and the recursive-switch tick count in bits 4–7, which is
the VC7/x86 declaration order used by the target.  These names and fields are
semantic inferences, not proof of strict object matching.

## Residual frontier

Sixteen residual functions (7,904 bytes) are unclassified; one,
`_player_handle_powerup` (240 bytes), has a control-flow/return-shape blocker.
The natural forms are parked rather than tuned with aliases, barriers, inert
branches, raw byte emission, or forced inlining.

## Validation

```text
ninja halobetacache_build
python -m tools.residual_frontier --unit source/game/players
python tools/fake_match_scan.py source/game/players.c source/game/players.h --fail-on-findings
git diff --check
```

All commands above pass in this lane (the full build retains unrelated legacy
warnings only).  Reopen a fuzzy function only with source/provenance that
explains its remaining schedule or lifetime naturally.
