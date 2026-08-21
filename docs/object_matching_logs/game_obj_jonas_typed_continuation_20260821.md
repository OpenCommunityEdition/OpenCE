# `game.obj` Jonas typed continuation

This Jonas/Codex ledger records one bounded continuation for
`source/game/game.obj` from authoritative campaign commit
`e1e95eecc4dcb2d0d9946f4af868b1c277d470da`. The object advances from four
to five strict functions out of 27 and remains `NonMatching`. No runtime-data
or object-completion claim is made.

## Scope and provenance

- Baseline `source/game/game.c` blob:
  `edfb16b969da5e50222e377497649fac73dddfee`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split-object SHA-256:
  `fd703ba666f13a5f1311dfbe1383eba452cf49a0a9a7fab47b1cffd06c1cfd0`.
- Local HCEA source oracle commit:
  `c168af2e747d3095d9a29418ae401f3a39544863`.
- Relevant HCEA source blobs are `756e35d71b6d1509bda89a51c03a9751e86bf9c5`
  (`game_map_loading_in_progress`),
  `b10153a7e2b555688242277f3e16e8774cc71636`
  (`game_difficulty_level_get_ignore_easy`),
  `690c5c1f60705c00df6f5b6fd4e1f3d940adc7b9` (`game_all_quiet`),
  `a2106c296c52c22798e54a3f687f893f34365284`
  (`game_safe_to_speak`),
  `4be48f769ae15dd01a9d3f4edef4a61ff24c9427`
  (`game_is_cooperative`), and
  `975cb5e20888a98f25110d2288b53232820912bc` (`set_random_seed`).
- The HCEA June 24, 2011 Release PDB has SHA-256
  `0fd11b084fd2169cafe071fd8c3e4083d6e9564f01e7964f6c26e5a03eee4dff`.
  DIA records corroborate the public signatures; in particular,
  `game_is_cooperative` returns a Boolean and takes no arguments, while
  `set_random_seed` is an explicitly inline function taking an unsigned long.

HCEA supplies names, signatures, field roles, and high-level predicate intent.
The January x86 split object remains the sole byte and branch authority. For
the retained function it proves a signed-word comparison of the external
`player_spawn_count` against one, returning true exactly when the count is
greater than one. This intentionally does not copy HCEA's platform-specific
branchless expression.

## Bounded compile result

Six functions were ranked. `set_random_seed` was excluded before compilation:
the current tree already defines it as a shared `__inline` function in
`math/real_math.h`, and January's out-of-line `game.obj` owner was emitted by a
real neighboring consumer. Recovering that owner in isolation would require a
shared-inline semantic change or a synthetic emitter, so neither was attempted.

The other five natural bodies were compiled together once. Exactly one was
strict and retained; the four nonexact bodies and their unused declarations
were removed without a source-shape retry.

| Result | Function | Target padded / relocs / hash | Candidate padded / relocs / hash |
| --- | --- | --- | --- |
| **Retained** | `_game_is_cooperative` | `16 / 1 / dc1664259fae...` | identical |
| Removed | `_game_map_loading_in_progress` | `32 / 1 / cce4c6135d4c...` | `48 / 2 / 5b44d7d0d05d...` |
| Removed | `_game_difficulty_level_get_ignore_easy` | `32 / 1 / 06bd18562c13...` | `32 / 1 / 37a54a539678...` |
| Removed | `_game_all_quiet` | `64 / 5 / d312ece76a01...` | `64 / 5 / 5bffd4611e5e...` |
| Removed | `_game_safe_to_speak` | `32 / 2 / 9db25bf430e9...` | `32 / 2 / e811dfdcc765...` |
| Precompile exclusion | `_set_random_seed` | `16 / 1 / f7ba85f0d673...` | not emitted |

The retained function contributes 14 meaningful bytes, 16 padded bytes, and
one `DIR32` relocation. Target and candidate have identical normalized bytes,
padding, relocation address/type/destination/addend, and one unique external
function owner:

`dc1664259fae636950aeaf08e39776766f544d0b4ac207d6912031ef2a9dceea`.

## Typed external storage boundary

The only new declaration is `extern short player_spawn_count`. Repository
`main/main.h`, HCEA data records, target callers, and the January relocation
all corroborate the signed 16-bit type. The retained body is the direct,
readable predicate `player_spawn_count > 1`.

The candidate records `_player_spawn_count` as an undefined external symbol
with value zero, external storage class 2, and null type. The pre-existing
runtime-prefix pointer `_bss_0043e48c` also remains undefined and external.
The candidate emits no `.bss`, `.data`, or `.rdata` section, so neither symbol
nor any January runtime-data span receives credit.

## Policy and scope

The retained body is ordinary defined C, has an explicit `void` parameter
list, and ends with an explicit return expression. It contains no assembly,
volatile scheduling device, force-inline annotation, optimizer pragma or
barrier, raw address reconstruction, pointer/integer cast, union access,
signed-overflow match, synthetic anchor, or byte/code-generation forcing.

Only `source/game/game.c` and this newly created Jonas ledger are changed. No
header, configuration, frozen-object source, pre-existing or Claude-authored
Markdown file is edited; no tracked file is deleted.

## Validation

- Complete `halobetacache_build` and `libcmt_build`: pass (568 actions).
- Direct hardened COFF comparison: all five retained exact functions pass;
  `_game_is_cooperative` is the sole new exact owner, and all four baseline
  functions preserve their normalized bytes and relocation evidence.
- Direct report: `game.obj` is 5/27 functions and 69/3,217 meaningful code
  bytes exact. Its 2,492 target non-code bytes remain wholly uncredited.
- Strict semantic report: 470 units scanned, 3,991 functions evaluated,
  3,851 semantic exact, 101 hidden exact / 61,876 hidden code bytes, 3,912
  accepted exact, and zero unit errors.
- Progress: 375/833 complete objects, 3,899/11,060 exact functions,
  472,154/2,198,102 exact code bytes, and 1,835,088/4,176,062 matched data
  bytes. Halo contributes 3,732/7,574 exact functions and
  459,240/1,770,166 exact code bytes.
- Object-admission audit: zero candidates and zero revocations, with only the
  inherited `source/shell/shell_xbox` completion-label contradiction.
- Parked-function validation: three active, zero stale, zero invalid.
- Tooling suite: 179/179 tests pass.

The pre-wave fail-closed manifest reports `_game_is_cooperative` as the only
`NEWLY_EXACT` function, all four baseline functions as `still_exact`, and
`changed_nonexact: []`. Adding the intended function COMDAT necessarily adds
compiler-debug records and one symbol owner, which the generic object-shape
guard reports for review. No adjudication or configuration exception is added.
A clean committed-state snapshot/check and forced object rebuild close the
enlarged structural surface before handoff. No push is performed.
