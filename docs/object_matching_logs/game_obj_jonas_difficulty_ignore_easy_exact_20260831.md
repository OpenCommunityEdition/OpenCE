# `game.obj`: difficulty-ignore-easy natural exact recovery (2026-08-31)

## Result

`game_difficulty_level_get_ignore_easy` is recovered as an ordinary public
Game function.  The object moves from 13 exact / 14 target-only owners to 14
exact / 13 target-only owners; there are no emitted residuals.  This admits 32
padded code bytes and does not claim any data or BSS ownership.

| Function | Target / candidate padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_game_difficulty_level_get_ignore_easy` | `32 / 32` | `1 / 1` | `06bd18562c13eeba6c7fdfe4829b8b33334d519be501ee157786c818f59814dc` |

## Selection and provenance

The requested `source/bungie_net`, `source/networking`, and `source/game`
frontier was rebuilt from isolated canonical commit
`7935fbb0a3820b585e68f21ac7a3894a0a92dc9e`.  It measured 24 units, 485
shared exact functions, 34 shared residuals, and 319 target-only owners.
Fresh fixed-point or rejection lanes were excluded rather than retried:

- `player_rumble` (the 2026-08-30 exhaustive census);
- `prime_numbers` and `key_agreement` (2026-08-30 credible parks);
- `game_allegiance` (2026-08-30 freshness audit);
- `network_connection` and the Winsock endpoint first-shot/rejection logs;
- existing fresh public-batch/rejection boundaries for the server/client
  network managers and the game-engine variants.

The selected remaining seam is small, externally named, and has independent
semantic evidence.  January's split COFF is the byte authority.  Its body
loads the owned runtime-global pointer, reads the signed 16-bit difficulty at
`+0x0E`, clamps values at or below Normal (`1`) to Normal, and returns.  The
single relocation at `+0x01` is the typed external runtime-global owner
`_game_globals`; no address literal or anonymous address owner is retained.

The owner header already established a `short` difficulty contract.  That is
also consistent with the field's existing offset assertions in `game.c` and
with the HCEA semantic donor
`../../research-cache/halocea-full-blobs-20260830/src/blam/game/game_difficulty_level_get_ignore_easy.c`.
The Stian/Pastudan lifts use a 32-bit return spelling, but describe the same
signed 16-bit field and clamp.  January's `movsx eax, ax` is therefore treated
as compiler result materialization, not evidence to change the established
owner ABI.  The retained `short` definition emits the target exactly.

No historical candidate exists: `git log --all -S
game_difficulty_level_get_ignore_easy -- source/game/game.c source/game/game.h`
reaches only the initial owner/header introduction.

## Retained source

The function is intentionally just the natural operation:

1. read the named signed difficulty field once;
2. return it when above the named `_game_difficulty_level_normal` enum value;
3. otherwise return that enum value.

It uses no helper, cast, alias, compiler directive, assembly, volatile,
address arithmetic, raw field offset, synthetic dependency, or byte-oriented
control.  Its public declaration stays in `source/game/game.h`.  `hs.c` now
includes that owner header and drops its redundant local declarations for the
two difficulty accessors, so callers use the owner declaration rather than a
separate cross-translation-unit prototype.

## Validation

- focused `game.obj` compile: pass;
- strict `section_infos_equal`: true, including the one relocation;
- `python -m tools.residual_frontier --unit source/game/game`: 14 exact,
  928 exact padded bytes, 0 residual, 13 target-only, and 0 errors;
- `python tools/fake_match_scan.py source/game/game.c source/game/game.h
  --fail-on-findings --format json`: zero findings;
- full configured Halo and libcmt builds: pass;
- locally generated objdiff report plus `python -m tools.audit_semantic_matches`:
  470 units, 5,001 functions evaluated, 4,890 accepted exact, and zero unit
  errors;
- tool tests and `git diff --check`: recorded after the retained-source replay.

## Canonical naming correction

The integration review rejected the agent branch's direct use of
`bss_0043e48c`. Existing assertions and the typed runtime prefix support the
descriptive external pointer name `game_globals`. Canonical source and
`config/symbols.json` now use that name directly, with no address alias macro;
all five affected exact accessors were rebuilt after the rename.
