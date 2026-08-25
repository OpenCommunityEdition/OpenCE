# Units typed-declaration recovery pair (Jonas, 2026-08-24)

## Result and boundary

This bounded wave starts from clean cumulative Units commit
`0fab4bb1c991ddb114588e634150a65376bef9cc`. It revisits exactly two
first-shot misses whose earlier objects supplied concrete missing-declaration
evidence:

- `unit_test_spawning(long unit_index)`; and
- `unit_try_and_exit_seat(long unit_index)`.

The only support additions are the ordinary translation-unit-local external
declarations required to compile those already-authenticated bodies under
their real C interfaces:

```c
short actors_spawn_from_unit(long, long, short, real);
short animation_choose_random_permutation(long, short);
```

No shared header, protected source, configuration, storage owner, semantic
exception, parked-function record, completion label, or existing ledger is
edited. Exactly one natural code-producing Units compile was used for the
two-body classification. Both functions were strict first-shot matches and
are retained without a source-shape retry.

The retained source is Git blob
`3fd40fa3885b465b0169d86ae3ac506304a508c8`, 242,674 raw Git-payload bytes,
with payload SHA-256
`bac4b65e699f7c83155dbda832380bedc889f51382579e741039067e8ade84dc`.

## Authority and provenance

January PC remains byte authority. The split `source/units/units.obj` is
138,090 bytes with SHA-256
`e6a24ca597147dacff1f06806613d6bcfa4fa48f359dfbf542c34935f674b19f`.
The January executable is 6,584,672 bytes with SHA-256
`4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
The cumulative pre-wave object copied before editing is 133,253 bytes with
SHA-256
`9d6518991cbc0832704aa57dff0767b57471bb25f5a815459b28016e8e9debac`.

All Claude Units documentation and tooling was treated as reconstruction
evidence rather than a wholesale patch. This includes the consolidated
`units_obj.md` ledger, the damage/lifecycle and vehicle-exit ledgers that
recorded the two original misses, the final carrier at
`deb5554ab8d16da80da934e2aa7081bc7d1f594f`, the donor/caller census, the
compiler provenance/QFE survey, and the campaign and `c2dbg32` research. The
carrier source blob is
`124f29e32497c091c4163b5faafc58dbf8890161`. The two readable typed bodies
were copied without raw offsets, puns, unions, barriers, attributes,
register forcing, or other code-generation controls.

The clean HCEA semantic-donor tree is
`c168af2e747d3095d9a29418ae401f3a39544863`. Its directly authenticated
source payloads are:

| donor | Git blob | bytes | raw payload SHA-256 |
| --- | --- | ---: | --- |
| `src/unit_test_spawning.c` | `0eeb7b2193cefbe45796c9ca350ffb2b8e991537` | 2,026 | `64ce879f1dce04d6e335230d388d53d0a896e81d796ba7f74c2026cae591ad96` |
| `src/unit_try_and_exit_seat.c` | `a06a3ad50e41492790247ac8ce0b1e1458a5a226` | 5,014 | `820b8c46c02dba8f5a624ae39f0197631c771cf2a5ccdcf2d7c3bbf37b27117a` |

HCEA is semantic/type corroboration only. January disassembly, relocations,
and callers establish the PC stack ABI. The implementation of
`animation_choose_random_permutation` already exists in `weapons.c` with the
same `short(long, short)` signature. January's spawn body pushes one 32-bit
float slot and a promoted 16-bit count for `actors_spawn_from_unit`, proving
the `short(long, long, short, real)` declaration used here.

## Why the previous candidates missed

The earlier `unit_test_spawning` candidate had the correct 176-byte extent
and six relocation identities but normalized hash
`81e293c9b29245eb35a7747b523a0a22af672ef77bfb590fa4bc19c5a3b186e4`.
With no visible prototype, VC7 used C89 implicit-int/default-promotion rules:
it sign-extended the actor count and promoted the throw velocity to `double`.
January instead pushes a single `real` and the ordinary promoted `short`.

The earlier `unit_try_and_exit_seat` candidate was 288 padded bytes with 12
relocations and normalized hash
`53ae0908f2b62c959c4cc9a9c57d8e552ae1a6f50af5406cf7f5a0585ffe1480`,
versus January's 272 bytes. Every relocation after `_unit_open` was three
bytes late. The missing `short` return/argument declaration for
`animation_choose_random_permutation` caused the topology drift. These are
declaration defects with direct emitted-object evidence, not speculative
source-shape tuning.

## One-shot adjudication

The final two bodies and the two declarations were frozen before emission.
`git diff --check` passed. A real VC7 `/Zs` parse-only invocation succeeded
and left the copied baseline object byte-identical at SHA-256 `9d651899...`.
The next build action was exactly one normal production edge:

```text
[1/1] CL build\base\source\units\units.obj
```

The immutable first-shot result is 134,343 bytes with SHA-256
`6a91533db835182ecdd47b5716e2525063f9da0d7d494b3ba711a2f55442918b`.
Strict padded-byte, relocation-masked-hash, and complete relocation
identity/addend comparison produced:

| January owner | result | meaningful / padded | relocs | normalized SHA-256 |
| --- | --- | ---: | ---: | --- |
| `_unit_test_spawning` | retain | 166 / 176 | 6 | `d30a923ccc6986e4548167d5d9f2f323bdc08b25a1b1092b0a2c030e25535ccc` |
| `_unit_try_and_exit_seat` | retain | 272 / 272 | 12 | `6b6a6ef87ea90824a136314d30dafc9719b4cb879e26c5fb0b64fa4e07738c83` |

The strict gain is two functions, 438 meaningful bytes, 448 padded bytes,
and 18 exact relocations. No candidate was pruned and no retry or tuning
occurred.

## Whole-object preservation and gates

The complete hardened census advances from 156 to 158 exact functions of
189, with the same 11 emitted-nonexact functions and 20 absent functions.
Removing the two gains from the final exact set produces precisely the 156
pre-wave exact owners; there is no inherited function loss.

The direct target-data acceptance census remains 121 exact target sections.
All 121 inherited exact sections are preserved and there is no target-data
gain or loss. Runtime-owner comparison shows no writable `.data`, `.bss`, or
COMMON addition. The spawn body's `__real@3d088889` is a normal exact
select-any literal already represented by January's relocation identity; no
standalone campaign data credit is claimed.

Pre-commit validation passed:

- all 568 Halo plus libcmt compile edges;
- semantic audit: 470 units, 4,226 functions evaluated, 4,085 semantic exact,
  110 hidden exact, 4,146 accepted exact, and zero unit errors;
- campaign progress: 375/833 objects, 4,124/11,060 functions, and 500,231
  accepted code bytes;
- object admission: zero candidates and zero revocations, with only the
  inherited `shell_xbox` completion-label contradiction;
- parked validation: three active, zero stale, zero invalid;
- the complete tooling suite: 179/179 tests.

The compiler is Microsoft VC7 `13.00.9254.1`; `xbox/bin/vc7/CL.Exe` is
81,920 bytes with SHA-256
`483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
The natural `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` production rule and include
graph are unchanged.

## Committed replay

The implementation and initial ledger were committed together as
`4265f987837c8cd73413721dc4834dcf98a776af` (`Recover Units typed
declaration pair`). The committed source blob is
`3fd40fa3885b465b0169d86ae3ac506304a508c8` (242,674 bytes; payload SHA-256
`bac4b65e699f7c83155dbda832380bedc889f51382579e741039067e8ade84dc`).
The initial ledger blob is `4a2951ca3c9806bbcebd4b5d7ed4dbf8f5c8620f`
(7,167 bytes).

At the clean implementation commit, `tools.regression_gate` wrote the
5,845,001-byte ignored snapshot
`build/audit/units_typed_declaration_pair_replay.json`, SHA-256
`6679240f3c6759d545bff1ed1fe4f2f4415f58749506ca258098731c32519ee9`.
The manifest pins exact commit `4265f987...` and the full accepted Units
function, non-code, and symbol inventory.

The generated object resolved to
`build/base/source/units/units.obj` under this exact worktree. Its pre-delete
SHA-256 was `6a91533d...`; the resolved absolute path was checked to remain
inside the worktree, the single file was removed with literal-path semantics,
and absence was verified. One ordinary rebuild then ran exactly:

```text
[1/1] CL build\base\source\units\units.obj
```

The replay object is 134,343 bytes with phase-specific raw SHA-256
`4642f063446f09e96c28c0ac66d759363c5c40a533e9e07d4abd7873cf83c53b`.
The expected raw-object difference from the first shot is confined to COFF
timestamp metadata; hardened section comparison is unchanged.

The committed regression check returned `ok: true`, exactly 158
`still_exact`, zero failures, zero warnings, zero `newly_exact`, and zero
`changed_nonexact`. Independent strict comparison repeated both retained
bodies as `all_equal: true` and reproduced the complete 158 exact / 11
emitted-nonexact / 20 absent function census. The 121 inherited exact target
data sections and runtime owner inventory remain preserved. A final Ninja dry
run reported no work, and the tracked worktree was clean before this additive
ledger-only update.
