# `recorded_animation_playback.obj` Jonas complete-object closeout (2026-08-28)

## Result

`source/cutscene/recorded_animation_playback.c` is now fully exact under the
strict target-owner campaign rule.  The unit advances from 11/14 to 14/14
exact functions and from 1,072/2,592 to 2,592/2,592 padded code bytes.  This
wave recovers three functions, 1,496 meaningful code bytes, and 1,520 padded
code bytes:

| Function | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_recorded_animation_apply_event_stream` | `489 / 496` | `34 / 34` | `4e8a5bd6f5b73b358df9e82beed6a1a08aa23ee6f84b353aa3026df37f1b441b` |
| `_code_00082290` | `500 / 512` | `19 / 19` | `9a6dfddddaa3cb4536223132a167ccfd0e6751faa228910209fa69697e16a2a2` |
| `_code_00082490` | `507 / 512` | `19 / 19` | `17ecb1a356a7c3f420296c1dbed9a7acddc665a886a4212b957458645efa6b02` |

For every row the target and rebuilt fingerprints are identical.  The final
`bijection_walk` has zero events for both vector handlers, and the dispatcher
has identical normalized bytes, relocation addresses, relocation types,
destinations, addends, and local jump-table topology.

All 24 target-owned runtime-data symbols are exact as well: 1,197 logical
owner bytes, or 1,228 bytes with COFF section padding.  The newly recovered
owners are the 272-byte `_data_002dcf20` aggregate and its seven 177-byte
definition-name COMDATs.  `config/config.json` index 348 therefore moves from
`NonMatching` to `Matching`.

Objdiff still reports the dispatcher's local jump table as a false negative.
The strict comparator does not: it proves all 496 bytes and 34 resolved
relocations exact.  `config/semantic_matches.json` records that UI/reporting
bridge so the fail-closed admission audit sees the same strict evidence; it
does not relax the comparator or create an exception to the byte gate.

## Frozen provenance

This closeout uses branch/worktree
`jonas/recorded-animation-playback-closeout-20260828`, based on published
integration commit `fd9c8eb0951e388f5e041ff3060aef90140b290c`.  The immutable
January split object and phase artifacts are:

| Artifact | Raw SHA-256 |
| --- | --- |
| `build/split/source/cutscene/recorded_animation_playback.obj` | `3808c85f1eec3cce0269b5ec3c94fab49c21893e49f28fa6f80ffef17274cd49` |
| `build/audit/recorded_animation_playback_baseline_20260828.obj` | `dfba7c37293f29307cf671dbc25579cdab107b75b1ad5e1ed03261646217ad10` |
| `build/audit/recorded_animation_playback_dispatch_data_first_shot_20260828.obj` | `3e15b9f9729d00b41a72a25f1cdf6e244c1dcff7546d34aad4a652362084c1ed` |
| `build/audit/recorded_animation_playback_dispatch_data_exact_20260828.obj` | `73ef6e7f2723f693e14515e7a76cba7cf0678c00886567080ddc33b2a5cb35bd` |
| `build/audit/recorded_animation_playback_handlers_exact_20260828.obj` | `eaf068340332f192bc5cb8f6ee68aef618950ed7058dc74b79e74dabe460c959` |

The final base object has the same raw SHA as the frozen handlers artifact.
The final source SHA-256 is
`4a333aa5fcc370943f6593f47c4a825f5289041229192e4c1fe38a53880eb283`.
Raw whole-object digests are artifact sentinels only; normalized target-owner
sections and their resolved relocations are the acceptance authority.

The compiler is XDK 3911 `CL.Exe` 13.00.9254.1 under the unchanged repository
`/O2 /Oy- /DDEBUG /Dxbox` edge.  No compiler flag, symbol manifest, target
split, comparator rule, or parked entry was changed to obtain the result.

## Data reconstruction

The later-build HCEA apply table established the semantic family, while the
January COFF established every retained owner, offset, width, relocation, and
string.  `_data_002dcf20` is one typed
`struct recorded_animation_playback_data` with this exact layout:

| Offset | Field | Logical bytes |
| ---: | --- | ---: |
| `0x00` | `apply_funcs[23]` | 92 |
| `0x5c` | animation-state code + definition | 24 |
| `0x74` | aiming-speed code + definition | 24 |
| `0x8c` | control-flags code + definition | 24 |
| `0xa4` | weapon-index code + definition | 24 |
| `0xbc` | throttle codes + definition | 28 |
| `0xd8` | char-vector codes + definition | 28 |
| `0xf4` | short-vector codes + definition | 28 |

The apply table contains two null entries, five scalar handlers,
`code_00082290` at entries 7 through 14, and `code_00082490` at entries 15
through 22.  The byte-swap code sequences are respectively `{ _1byte }`,
`{ _1byte }`, `{ _2byte }`, `{ _2byte }`, `{ _4byte, _4byte }`,
`{ _1byte, _1byte }`, and `{ _2byte, _2byte }`.

The aggregate is 272/272 bytes with 35/35 DIR32 relocations and normalized
SHA-256
`6d66d4cc2ed3769275f29000746ff8d94d7df5331aac3aa1ad357048163b7c1b`.
It has target flags `0xc0400040`, external owner value/type/storage
`0/0/2`, and no child symbols.  The seven associated string COMDATs are:

| Definition name | Bytes | Normalized SHA-256 |
| --- | ---: | --- |
| `animation_state_event_data` | 27 | `92ffb59bb479fcd3806522b9bc2f23385be51b9a4607dc6415f607401083d142` |
| `aiming_speed_event_data` | 24 | `e38f767c96c91a9fb8fc236a9364839db37bcda430d13c4a53dc0a4aa5139813` |
| `control_flags_event_data` | 25 | `e45f6ba9b7bcef9b7d17381d803a0144ffbadb3361f388c9a8a97ac79cb64350` |
| `weapon_index_event_data` | 24 | `7858874f115e7d7b9a2145a7596111b4184c55c8458fe50f99743f94d885c0f9` |
| `throttle_event_data` | 20 | `f2c5a41563b3a5f3877b5ff4b1c192319a72bcbc1241b6987b9165324a57d4bd` |
| `vector_char_difference_data` | 28 | `c10bdc3a174a13b6c11e8fd530c0e0becb5975bb5fdbd680ebe8e954fa0a4cb5` |
| `vector_short_difference_data` | 29 | `afb46061f4c090d32c05e1a63cacc4fcfe5a40409cac7eca49a6dc6c9c7076d4` |

The aggregate plus all seven strings were exact on their first typed-data
build.  HCEA wrapper-style fastcall streams were not copied because they are
cross-build ABI artifacts, not January source evidence.

## Dispatcher reconstruction

The dispatcher uses four initial assertions, a four-way two-bit time-delta
switch, the playback-end break rule, a typed 23-entry handler dispatch, tick
subtraction, and one final boolean result.  A standard `memcpy` of the
unaligned word delta folds to January's `mov bx, word ptr [esi+1]`.

The first semantically correct candidate was already 496 bytes with 34
relocations, but its normalized SHA-256 was
`f0cf69383b98fd53d399d064e25d92f9f7ae64a43bccf320b74ba7cddf498c70`.
It assigned DI to `time_delta`, BX to `header_size`, pushed EDI too early,
placed the switch table at `+0x1d8`, and moved the aggregate relocation to
`+0x18d`.  Declaration order, inner scope, and local-width variants did not
change that allocator choice.

The decisive ordinary-C lifetime is the apparently redundant
`header_size = 0;` immediately before the switch.  It corresponds directly
to January's `xor edi, edi` at `+0xa9`, assigns BX to `time_delta` and DI to
`header_size`, places the jump table at `+0x1d4`, and moves the sole aggregate
reference to exact offset `+0x189`.  The final 34 relocation records and four
internal table destinations are identical.

## Vector-handler reconstruction

The starting handlers were complete HCEA-derived legal C but retained a
separate `word update_facing` local.  Their normalized SHA-256 values were
`d1d2d7416f1b6414a8d0144b8eaef46100c8801109c01bc9ea1138674662a272`
and
`d664bc0a81ee00b073f7b510f1c5452980bc47c4b03f259e38053805c6e640f5`.

Target disassembly and Stian's same-lineage `FUN_00093c20`/`FUN_00093e20`
showed that the mask itself is the source topology.  A 32-bit
`update_facing` improved allocation but omitted three target `test ax, ax`
operations per handler.  Casting its six uses back to `word` collapsed the
allocation and shrank the handlers to 480 and 496 bytes.  Removing the local
and repeating
`event_type & FLAG(_control_vector_facing_bit)` at all three source tests made
the char handler exact immediately.

That same form left the short handler with only two independent reloads
transposed at `+0x167` and `+0x16a`; size and every relocation were already
exact.  Stian's donor records its mask as signed `short`, whereas the char
handler's is unsigned `word`.  Applying that final type boundary changed only
the scheduler choice and produced the exact short-handler hash above.

## Documentation and donor audit

Before editing, all five existing playback ledgers, `docs/matching_methodology.md`,
`tools/campaign/README.md`, and the relevant current/Stian `CLAUDE.md` files
were read in full.  Claude project/session artifacts were searched read-only;
they contained the residual inventory but no hidden exact candidate or
exclusion matrix.  Claude's active worktrees were not edited.

The principal behavior/topology donors were:

- `research-cache/stian-halo-current-20260827/src/halo/cutscene/recorded_animations.c`,
  `FUN_000940a0`;
- `research-cache/stian-halo-current-20260827/src/halo/cutscene/cinematics.c`,
  `FUN_00093c20` and `FUN_00093e20`;
- `work/halocea-reference/src/recorded_animation_apply_event_stream.c`;
- `work/halocea-reference/src/data/apply_funcs_0.c`; and
- the HCEA prototype PDB under
  `research/hcea_jun2011_prototype/payload/`, which confirms the later-build
  dispatcher family but retains no useful optimized locals.

These sources are provenance and hypothesis evidence only.  January's target
COFF admitted every retained spelling.

## Policy audit

All parameters remain one per line.  Every void function has an explicit
terminal `return;`; the dispatcher has an explicit terminal return.  Payload,
controller, event, byte-swap, and aggregate access is typed.  The result uses
no assembly, `volatile`, `register`, pragma, intrinsic, barrier, annotation,
raw address/offset access, pointer/integer reconstruction, pointer or union
pun, undefined behavior, synthetic anchor, object-byte patch, compiler flag,
comparator exception, semantic exception, or parked credit.

## Validation

- Production-flag XDK 3911 `/Zs`: pass with no diagnostic.
- `tools/campaign/gate.py source/cutscene/recorded_animation_playback --all`:
  14 exact, zero residual, zero unwritten.
- Baseline/final exact-set comparison: 11 to 14 exact, three newly exact,
  zero lost exact functions.
- Strict runtime-owner census: 24/24 symbols and 1,197/1,197 logical bytes;
  `_data_002dcf20` and all seven new definition strings compare exact.
- `halobetacache_build`, `libcmt_build`, and `semantic_progress`: pass.
- Semantic audit: 470 units, 4,592 functions evaluated, 4,461 semantic exact,
  4,488 accepted exact, and zero unit errors.
- Object-admission audit: zero candidates, contradictions, and revocations.
- Parked audit: 13 active (three compiler ties and ten asm/vendored-assembly
  exclusions), zero stale, and zero invalid.
- Tooling tests: 205/205 pass.
- Strict global board: 269/619 objects, 4,442/8,246 functions, and
  594,151/1,922,669 padded code bytes.  Parked and asm functions are excluded.

The tracked closeout is limited to the source file, this ledger, the one
`NonMatching` to `Matching` status change, and the dispatcher's strict
objdiff-reporting bridge.
