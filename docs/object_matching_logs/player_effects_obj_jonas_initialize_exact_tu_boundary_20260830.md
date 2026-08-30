# `player_effects.obj` initialize exact-packet / TU-owner boundary (2026-08-30)

## Outcome

The sole natural C89 candidate for `_player_effect_initialize` reproduced the
complete January function packet: 59 meaningful bytes, 64 padded bytes, all
seven ordered relocations, and normalized SHA-256
`f078c0200431da0bb4c3e0c1f2f106b27404f1d7c12479c6685b56be351905f1`.
Its typed four-byte `player_effect_globals` BSS owner also matched all bytes
and zero relocations, with normalized SHA-256
`df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119`.

Retention nevertheless failed the mandatory whole-translation-unit owner
gate.  The candidate was placed at its January source position before the
four inherited exact bodies.  That inserted its `.text`, three `.rdata`
owners, the BSS owner, and CodeView record ahead of those bodies, changing
their section/debug fingerprints even though their complete function bytes
and relocation packets remained exact.  The frozen one-shot rule forbids a
second code-producing source-order variant.  Source, configuration, target
label, and built object were therefore inverse-restored exactly.  This ledger
is the only retained path and no matching credit is claimed.

## Frozen authority and provenance

- Canonical starting commit:
  `58ab7f6b90e6b6c7ef7f238362f4fdebdebb4c8e`.
- Isolated branch: `jonas/player-effect-initialize-20260830`.
- Untouched source blob:
  `53da95da2331ef3878e74361f0ddaa46940632fa`.
- Untouched rebuilt object SHA-256:
  `23232c80a2837f0699df5e0137e02cfd3e5e327d242166d6856c2042e73a7710`.
- Untouched January split object SHA-256:
  `9a4ab25f9b58407433ecc9b04f235e5f3f3ae28bbeaf0023f3212bd5b789daeb`.
- Frozen candidate source blob:
  `4b9216075676d6fc81d40990b80d081f52a306f2`.
- Frozen candidate object SHA-256:
  `b5529e976dfe309ba0461ad67d06410aed3a992f92f80ccf31f531866a6e80f4`.
- Label-normalized January split object SHA-256:
  `df675976d69fbad1f06509ac47ea5187a42bc8a86f95071b0fd0ae2197daa6bb`.
- Compiler: Microsoft VC7/XDK 3911 production edge, version
  `13.00.9254.1`, compiler SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`,
  with the repository's unchanged `/O2 /Oy- /DDEBUG /Dxbox` flags.

All campaign, object, Claude, tooling, and prior Player Effects ledgers were
read before mutation.  Neither prior Player Effects ledger attempts this
owner: the 2026-08-21 ledger measures the damage-indicator/new-map group, and
the 2026-08-28 public-donor ledger freezes the other public residuals.
Repository-wide history, ref, registered-worktree, and ledger searches found
no earlier production candidate for `_player_effect_initialize`.

Three independent semantic sources agree on the natural allocation topology.
The Pastudan and Stian PC reconstructions allocate `0x3EC` bytes under the
name `"player effects"` and assert `player_effect_globals`; the HCEA headers
independently name `player_effect_globals_definition`,
`screen_fade_definition`, `scripted_player_effect_definition`, and the
`0xEC` per-player datum.  HCEA's Xbox layout has two local-player slots;
January PC disassembly and the PC donors establish four slots and total size
`0x3EC`.  January's own assertion literal and the four-byte target-owned BSS
slot at image address `0x43D58C` authenticate the file-static typed pointer
name.  All direct callees are public cdecl owners:
`_game_state_malloc`, `_display_assert`, and `_system_exit`.

## Exact function packet

The attested target-only symbol correction renamed provisional
`_bss_0043d58c` to `_player_effect_globals` at file offset `4445580` before
comparison.  It changed no executable or data byte.  The sole candidate used
a named `0x3EC` aggregate containing four typed `player_effect_datum` slots,
a typed screen fade, a typed scripted effect, flags, and reference time; a
file-static typed pointer; one natural allocation; and the January assertion
at source line `0x7F`.  A `/Zs` syntax-only pass emitted no object and left the
baseline object hash unchanged.  The production Ninja edge then ran exactly
once.

| Offset | Type | Strict destination |
| ---: | --- | --- |
| `+8` | `DIR32` | `"player effects"` COMDAT |
| `+13` | `REL32` | `_game_state_malloc` |
| `+23` | `DIR32` | `.bss:_player_effect_globals+0` |
| `+34` | `DIR32` | `c:\halo\SOURCE\effects\player_effects.c` COMDAT |
| `+39` | `DIR32` | `"player_effect_globals"` COMDAT |
| `+44` | `REL32` | `_display_assert` |
| `+51` | `REL32` | `_system_exit` |

Hardened direct comparison reports `all_equal: true` for the new owner and
all four inherited exact functions:

| Inherited owner | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_player_effect_dispose` | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_player_effect_dispose_from_old_map` | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_scripted_player_effect_set_rumble` | 16 | 1 | `6fde8ac9ac0799766d64cc971c152dad841aeba3fc8176bdd2cffc27383aed51` |
| `_player_effect_clear_damage_indicators` | 32 | 2 | `cdc2d7a082a24b44d427bd17d716078e3d4ffa8833c8cefc13907b2fabef3e66` |

## Fail-closed translation-unit boundary

The clean pre-source manifest records inherited function section indices
`3`, `5`, `7`, and `8`.  The candidate's target-position insertion moved
those owners to indices `9`, `11`, `13`, and `14`.  The fail-closed manifest
therefore reports changed accepted-function evidence for all four owners,
changed anonymous CodeView records, the three newly materialized assertion/
allocation `.rdata` sections, the new BSS section, and a changed symbol set.
It identifies `_player_effect_initialize` only as `NEWLY_EXACT`; that warning
does not grant credit.  This is an ownership/topology rejection, not a
function-byte or relocation mismatch.

The exact candidate body and layout are now frozen.  Do not retry spelling,
casts, constants, declaration order, body order, or a tail-appended body from
this ledger.  Reopen only if authenticated historical source independently
proves a different original translation-unit definition order, or if the
campaign's reviewed whole-owner admission protocol changes.  A compiler-
motivated source-order retry without that new evidence would be tuning.

## Restoration and validation

Inverse `apply_patch` restored the source and symbol map to their exact
starting blobs.  The base and split objects were restored to the untouched
SHA-256 identities above without a second production compilation.  The
original whole-TU manifest then passes with all four inherited owners
`still_exact`, no changed nonexact or newly exact function, no failure, and no
warning.  The focused campaign gate reports four exact, zero residual, and 25
unwritten functions.

- Full Halo/libcmt build, report, semantic report, and progress targets: pass.
- Semantic report: 470 units, 4,954 functions evaluated, 4,837 semantic
  exact, 4,847 accepted exact, 131 hidden exact / 78,940 hidden bytes, and
  zero unit errors.
- Progress: 384/833 linked objects, 4,808/11,060 functions, and
  610,370/2,198,102 code bytes; Halo remains 282/468 linked objects,
  4,641/7,574 functions, and 597,456/1,770,166 code bytes.
- Strict board: 277/619 objects, 4,819/8,246 functions, and
  667,543/1,922,669 padded bytes.
- Object admission: zero candidates, contradictions, and revocations.
- Parked validation: 12 active, zero stale, zero invalid.
- Python tooling suite: 212/212 pass.
- `build/report.json` SHA-256:
  `fd2d950866b443b6c2d66c93c767505d8e8b25e05ebd21382fee29840d1cc466`.
- `build/semantic_report.json` SHA-256:
  `3c51c7e32625c13fe2f70adde7cfdcc895f7e69390c7cd5e883243e512370936`.
- Protected `_unit_preprocess_node_orientations`: 1,920 padded bytes,
  87 ordered relocations, normalized SHA-256
  `5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
- Final `git diff --check`, JSON parse, scratch cleanup, and restored-source
  review: pass.

The rejected candidate was readable typed C89 with `void` on its own line,
one parameter per line, and an explicit terminal `return;`.  It introduced no
assembly, `volatile`, `register`, pragma, intrinsic, attribute, optimizer
barrier, raw address/field access, pointer/integer reconstruction,
representation pun, undefined behavior, byte forcing, object patch, compiler
flag change, semantic exception, comparator exception, or tool exception.
No production source, header, configuration, symbol label, compiler setting,
semantic exception, parked record, or candidate object is tracked.  Nothing
was pushed.
