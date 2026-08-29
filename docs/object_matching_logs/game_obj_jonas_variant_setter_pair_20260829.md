# `game.obj` variant setter pair recovery (2026-08-29)

## Result

This wave recovers `_game_set_game_variant` and
`_game_set_game_engine_index` in `source/game/game.c`. Both functions are
strict-exact, all eleven inherited exact owners remain exact, and the unit
advances from 11/27 to 13/27 functions with zero emitted residuals. The two
functions contribute 96 padded code bytes. `game.obj` remains
`NonMatching`; fourteen target owners are still unwritten.

| Function | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_game_set_game_variant` | 48 | 3 | `b8acc0db46873a9d3a0520edcf39b343807781c1579de3cf9b214dedc3ca3f86` |
| `_game_set_game_engine_index` | 48 | 4 | `8fee2968dc925e458b118ba2b49c9304d2641ab51ac4c30ee21d575c4bfca1a9` |

The strict whole-unit census is 13 exact, zero residual, and 14 unwritten
owners. Accepted padded code rises from 800 to 896 bytes.

## Evidence and source reconstruction

The existing `game.obj` ledgers, repository instructions, current target
COFF, and Claude near-complete, finish-HS, and untried-object records were
read before editing. The Claude branches contain only the original symbol
stub for these functions. Their residual inventories classify the functions
as target-only and provide no prior candidate or fixed-point experiment.

The HCEA public donor for `game_set_game_variant` supplies the typed semantic
operation: install the supplied `struct game_variant`, or clear the global
variant when passed null. January is the branch and call authority. It places
the null/clear path first, calls `csmemset` with exactly `0x68` bytes, and
uses a 26-dword `rep movsd` copy on the non-null path. The retained ordinary-C
body expresses that exact negative branch and typed structure assignment.
All three relocations match in address, type, destination, and addend:

- `DIR32 +0x0F` to `_game_variant_global`;
- `REL32 +0x14` to `_csmemset`;
- `DIR32 +0x25` to `_game_variant_global`.

The HCEA `game_set_game_engine_index` file is an empty later-build stub, so it
is not treated as a body donor. January independently proves that the short
parameter is unused and the function unconditionally executes the historical
fatal assertion at line `0x311`. The exact stringized assertion is:

```text
!"this is broken and should get updated for the variants, ask michael"
```

The retained `match_assert` expands to January's four pushes, `display_assert`,
`system_exit(-1)`, and terminal return. Its relocations match at `+0x08`,
`+0x0D`, `+0x12`, and `+0x19`, respectively targeting the source path, the
assertion expression, `_display_assert`, and `_system_exit`.

`game/game_engine.h` is included only to use the canonical complete
`struct game_variant` definition. No shared header is changed.

## Compile record

The first build stopped during type checking because `game/game.h` only
forward-declares `struct game_variant`; it emitted no candidate object. Adding
the canonical defining header allowed the first code emission. The assertion
function was exact immediately. The variant function matched its 48-byte
envelope and complete relocation inventory, but the later donor's positive
branch put the copy path first. January disassembly isolated that sole control
fact. Inverting only the source predicate to the target-authenticated null-first
form made the function strict-exact. No other spelling, declaration, compiler
option, layout, or scheduling experiment was attempted.

## Runtime data

The fatal assertion emits two target-owned, zero-relocation RDATA COMDATs that
also compare strict-exact:

| Owner | Bytes | Normalized SHA-256 |
| --- | ---: | --- |
| `c:\halo\SOURCE\game\game.c` | 27 | `356e6bfd9bbf1543470987f38ca6ba6e268e173b91b68632983f417373f8893f` |
| assertion expression | 71 | `9e5b7c208b5bc68bb92ddab7f35d4ca13d2d2f594e7bfa88ed434654e49508e6` |

No writable storage is defined. `_game_variant_global` remains a typed
undefined external owner, exactly as in the January split object.

## Policy and validation

The retained source is readable C89-style typed C with one parameter per line
and explicit terminal returns. It uses no assembly, `volatile`, `register`,
pragma, intrinsic, barrier, attribute, raw address, raw byte offset, pointer
pun, undefined behavior, compiler substitution, object patch, synthetic
anchor, or semantic/comparator exception. Protected Units, Vehicles,
`ai_debug.c`, `ai_script.c`, `actions.c`, and `game_engine.c` sources are
unchanged.

- Halo and libcmt builds: pass.
- Semantic report: 470 units, 4,784 functions evaluated, 4,660 semantic
  exact, 4,670 accepted exact, zero unit errors.
- Strict board: 275/619 complete objects, 636,871/1,922,669 padded code
  bytes, and 4,641/8,246 functions. No parked or assembly function is counted.
- Object admission: zero candidates, contradictions, or revocations.
- Parked validation: 13 active, zero stale, zero invalid.
- Tooling tests: 205/205 pass.
- `git diff --check`: pass.

Reproducibility identities before commit:

- source Git blob: `9f845c0774f40b2b02186d6d7cd72aa87be65d0a`;
- source SHA-256:
  `cc3e15bc208a56435b720da44174a49d0084d8b11a6f2eb59b28a0df5f1890b2`;
- rebuilt object SHA-256:
  `ac4c046200b4164d97ef4b6e56c3baca9869c4c928d843e4bb63425dfb037fe4`;
- January split object SHA-256:
  `fd703ba666f13a5f1311dfbe1383eba452cf49a0a9a7fab47b1cffd06c1cfd0`.
