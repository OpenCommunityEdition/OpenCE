# `player_rumble.obj` Codex checkpoint

This is a Codex-owned checkpoint ledger for
`source/game/player_rumble.obj`. The object remains `NonMatching`: ten of its
twelve functions are strict-exact, while the private motor mixer and its
dependent update routine are deliberately absent. No pre-existing or
Claude-authored Markdown was modified.

## Scope and provenance

- Integration base: `60c1f7e6f4ce10ad3b1856357c32f5b0ffd56b49`.
- The untouched base rebuilt at 1/12 strict functions: `_rumble_dispose`, one
  meaningful byte, 16 padded bytes, and zero relocations.
- `git log --all -- source/game/player_rumble.c` found the historical
  reconstruction commits `c2a6d850` and `5dee3760`, plus the earlier one-
  function checkpoints. The substantive donor was co-authored with Claude.
- A filesystem census also hashed the actual `player_rumble.c` in all 116
  registered Claude/Jonas worktrees. It found five source hashes: the full
  `5dee3760` family and four skeleton/line-ending variants. There was no
  additional committed or uncommitted donor shape.
- Only independently strict readable C was reconstructed. No donor config,
  parked entry, compiler flag, existing ledger, or frozen-object path was
  imported.

## Retained reconstruction

The recovered global allocation is a typed `struct rumble_globals` of 0x82c
bytes: four 0x208-byte player records followed by the scripted left, right,
and scale values. Each player owns eight 60-byte impulse definitions, eight
timers, and two continuous-motor values.

`rumble_player_impulse` selects the impulse slot with the greatest elapsed
timer, copies the typed definition, applies the floor-to-one gain scale and
duration scale, and clears that slot's timer. The assertion uses the
target-backed parameter name, source path, and line so both owned diagnostic
strings are emitted honestly. The lifecycle, clear, continuous, and scripted
value routines use the same typed state.

The retained source contains no assembly, volatile scheduling device,
force-inline annotation, optimizer pragma, compiler barrier, undefined
aliasing, object-byte patch, or byte-forcing construct. Every emitted void
function ends with an explicit `return;`.

## Strict function evidence

The January target and a clean XDK 3911 rebuild agree in padded bytes,
relocation count and identity, and normalized SHA-256 for all ten accepted
functions.

| Function | Meaningful bytes | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_rumble_initialize` | 26 | 32 | 3 | `103f515e7a431a7ff29ac132ed9b28df34f2cbee70b2152ffe5374417e1b84c2` |
| `_rumble_dispose` | 1 | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_rumble_initialize_for_new_map` | 22 | 32 | 2 | `6e7854da0f060763b7b573883e141cdc46428181bd3fb7396ef365857ceb2087` |
| `_rumble_player_set_scripted_values` | 28 | 32 | 1 | `c4ca59505929dace67b5cab00df410b88a194781127034926b1863ed5aa3619d` |
| `_rumble_player_set_scale` | 19 | 32 | 1 | `0d5ee8da441c7e93ebadf3bfca1d875e9cda9ef2300ebb81dad5729a87013755` |
| `_rumble_player_impulse` | 358 | 368 | 6 | `d8b80f2d254f157797593b7ff45e8fc14ce1239aec7aa6631eee02658dc5bb5f` |
| `_rumble_player_clear` | 39 | 48 | 2 | `35905262f46c8677a95ffc3e5355f6c6e80eeeb9a3cce5c1c21f4a7936a9475a` |
| `_rumble_clear_all_now` | 58 | 64 | 4 | `11332269e61bc046ce60fdc0843897d9cb151c670953093c4d53c38d7545f6d7` |
| `_rumble_player_continuous` | 41 | 48 | 1 | `5275e297797627258ecc3248dfaa053afac15c898f17366ecc197053bf047401` |
| `_rumble_dispose_from_old_map` | 82 | 96 | 5 | `cb49037ef265ad9a384bb66ce5d38beff52f48591f0545412d9cfab2b5e0f018` |

Totals: 674 meaningful bytes, 768 padded bytes, and 25 exact relocations.
Relative to the integration base this is a gain of nine functions, 673
meaningful bytes, and 752 padded bytes. The baseline `_rumble_dispose` remains
strict-identical.

## Fail-closed residual boundary

| Function | Target padded bytes | Target relocations | Final candidate |
| --- | ---: | ---: | --- |
| `_code_000a91a0` | 416 | 12 | absent |
| `_rumble_update` | 304 | 16 | absent |

The historical static helper body makes `_rumble_update` exact because VC7
assigns the private helper an EAX argument convention. That helper itself is
not exact: the ordinary legal-C build has 19 relocations against the target's
12 because its float-to-integer conversions call `__ftol` and materialize
float constants. The historical `/QIfist` diagnosis is a flag experiment, not
an admissible source change, and still did not close the constant ownership
gap.

Declaring the helper externally and omitting its body removed the nonexact
section, but VC7 then used the public cdecl ABI. The resulting
`_rumble_update` remained 304 bytes with 16 relocations but was not exact
(`60ee934897c149aaab0cb2228e999c8bdecc5a35bccf069697042182ea6e92f3`
against target
`0149be7a3b1ffa37395a640eec09e8d2fb6f2587ec69ef3aa3e5572b712efd07`).
Both functions were therefore removed. Reopen this pair only with a legal-C
helper that is itself strict-exact; do not retain the old partial body or use
an ABI/codegen trick.

## Data and ownership audit

All four target-owned runtime data sections are strict-exact by owner, logical
size, flags, alignment, COMDAT selection, payload hash, and relocation
identity: 65/65 logical bytes total.

| Owner | Kind | Logical bytes | Normalized SHA-256 |
| --- | --- | ---: | --- |
| `_bss_00453404` | BSS | 4 | zero-filled, strict-equal |
| `??_C@_06OLEBFMCD@rumble?$AA@` | RDATA | 7 | `c20d980d6d7d475a9f47923742330afe37cd673eb5e4a5e931406889e4e790e7` |
| `??_C@_0BC@NEAPFALG@rumble_definition?$AA@` | RDATA | 18 | `eef7fb0cd233e1fd4ae0d1d902702497851c59c9730b367fdae1771082419c69` |
| `??_C@_0CE@KKOMCMLO@c?3?2halo?2SOURCE?2game?2player_rumbl@` | RDATA | 36 | `d70f75a61e90547ef1da8eef54628b84387db8f844f58ae99f17124dbf7d2bb1` |

The candidate additionally emits ordinary directive/debug metadata and the
discardable SELECT_ANY 4-byte `__real@3f800000` COMDAT referenced by the exact
impulse function. It does not replace or claim any target-owned section.

## Reproducibility and gates

- Source SHA-256:
  `3df8439a4eb8ab1ddd570070803d6f9d8777d67248fa5e7b6e151ec145a8c893`.
- January target object SHA-256:
  `c6f3cadcd7081dd4ffff4fa6ff9abbfb9c1badebac05c22811712c6daee97db5`.
- Clean full `halobetacache_build` plus `libcmt_build`: 569/569 edges passed.
- Semantic audit: 470 units, 3,709 functions evaluated, 3,567 semantic exact,
  3,650 accepted exact, and zero unit errors.
- Progress: 374/833 complete objects, 3,644/11,060 exact functions,
  439,850/2,198,102 exact code bytes, and 1,802,712/4,176,062 matched data
  bytes.
- Admission audit: zero candidates and zero revocations; only the inherited
  `source/shell/shell_xbox` contradiction remains.
- Parked audit: three active, zero stale, zero invalid.
- Tool suite: 179/179 tests passed.
- A clean post-commit forced-object regression snapshot/check is required and
  recorded in the final handoff.

The configuration remains `NonMatching`. The five user-frozen objects are
untouched, no tracked file was deleted, and no Claude-authored Markdown was
changed.
