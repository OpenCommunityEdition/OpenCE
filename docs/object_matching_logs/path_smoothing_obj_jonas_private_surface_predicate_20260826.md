# `path_smoothing.obj` private surface-predicate recovery (2026-08-26)

## Result

The cross-build reconstruction recovers one independently strict private
helper while preserving the five remaining bodies as source-backed private-ABI
scaffolding:

| Function | Meaningful / padded | Relocations | Target normalized SHA-256 | Result |
|---|---:|---:|---|---|
| `_code_00051190` | 116 / 128 | 5 / 5 | `a58fa4f6cf9598874c2a0d1fb60f226f65dea7f5cbc7182a682b2faefc62580f` | strict exact |

The exact helper evaluates the per-surface pathfinding walkability byte and,
when applicable, verifies and consults the runtime breakable-surface bit
vector. Its compiler-private ABI is anchored by the reconstructed
`_code_000516a0` caller; it is not tuned in isolation.

`config/config.json` remains `NonMatching`. Only `_code_00051190` is added to
`config/semantic_matches.json`; no object label or data credit changes.

## Frozen baseline

- cumulative parent: `fe0baf6b300de5239549ef9bec1b6fa201fb06be`
- isolated branch: `jonas/path-smoothing-crossbuild-20260826`
- pristine target object:
  `DE1BBACD9453A3DB7FE3DC9CB97DC2A48B83B61FF7C7796F2A06A9A264C1D0FF`
  (6,907 bytes)
- pristine empty base object:
  `E0FA021EFF82379630C7206F61B3E516252928FB90F11DE9CEFF44C312D4B54E`
  (481 bytes)
- clean baseline regression manifest:
  `outputs/path_smoothing_evidence_20260826/regression-baseline.json`

The January target contains six code owners, 3,021 meaningful code bytes and
348 data bytes. None had a base definition before this wave.

## Cross-build evidence and symbol map

The later HaloCEA reconstruction supplies behavior and parameter roles; the
January COFF target supplies all accepted machine shape, private ABI,
relocation, padding, literal ownership, and source-line evidence.

| January symbol | Recovered role |
|---|---|
| `_code_00051190` | pathfinding-surface walkability / breakable predicate |
| `_code_00051210` | tangent point on an obstacle-radius circle |
| `_code_00051360` | avoidance-point selection |
| `_code_00051480` | clockwise/counterclockwise turning-point choice |
| `_code_000516a0` | collision-edge graph walk for a turning point |
| `_path_smooth` | public raw-path smoothing driver |

Relevant local provenance:

- `research-cache/halocea-cseries-20260820/src/blam/ai/path_smooth.c`
- `research-cache/halocea-cseries-20260820/src/blam/ai/choose_turning_point.c`
- `research-cache/halocea-cseries-20260820/src/blam/game/find_tangent_point.c`
- `research-cache/halocea-cseries-20260820/src/blam/game/find_avoidance_point.c`
- `research-cache/halocea-cseries-20260820/src/blam/game/find_turning_point.c`
- `research-cache/halocea-cseries-20260820/src/blam/ai/path_state_build_path.c`
- `research-cache/stian-halo-cseries-20260820/src/halo/ai/path_smoothing.c`
- `research-cache/stian-halo-cseries-20260820/CLAUDE.md`
- `research-cache/stian-halo-cseries-20260820/AGENTS.md`
- `research-cache/stian-halo-cseries-20260820/docs/vc71-byte-accuracy-playbook.md`
- `docs/exact_match_acceleration_playbook.md`

The January target corrected one later-build donor ambiguity: the inner edge
walk reads `surface_indices[!matches_end]`, proven directly by the target's
`sete`/`test`/`sete` sequence and indexed load.

## Bounded evidence waves

### Wave 1: semantic reconstruction

- source Git blob: `230b700134a01cf661724a974deec7071af9099e`
- physical source SHA-256:
  `DB68476053FEA212365F5665201B81B7B3386D8F6EB9562AC1FFFC832C7D68EF`
- first object SHA-256:
  `19B3C4566ED30C596AF6A43C312A2E4E9E26FCEC3A937696BE50C526C662A85E`

All six bodies were emitted once. `_code_00051190` already had the exact 128
padded bytes and exact five relocation addresses/types, but its assert
relocation named the macro-expanded expression instead of January's literal
`TEST_FLAG(collision_surface->flags, _collision_surface_breakable_bit)`.
The other five bodies were nonexact. Nothing from this emission was credited.

### Wave 2: literal-identity correction

- source Git blob: `d275ea5f6a6235eb99187ae949fe853e33f79cd3`
- physical source SHA-256:
  `0161BBB7F9C90EE0B40CB5CFBE75E36CCE14E2E940E711DA613ED740CB89E0D9`
- first object SHA-256:
  `734C03F1C471EB40D9F70E6FF59F57A81AF653E05D133489FFAF584A34016BD5`

Using `match_assert` preserved the source-authentic literal spelling and made
`_code_00051190` strict. Ownership audit then found five candidate-only inline
helper COMDATs (`square_root`, `scale_vector2d`, `magnitude_squared2d`,
`magnitude2d`, and `normalize2d`). This object was preserved as evidence but
was not admitted.

### Wave 3: ownership correction

- source Git blob: `a6c617a574654ea0f2adada72f4221aa45c87899`
- physical source SHA-256:
  `E5235C7DEE6FC90FC389BC1F455B3534F8B41B409ED47BFCAF77CA92C707B09C`
- first and retained object SHA-256:
  `2F8DBB35B913CE1DAB7D6238294877652B351B0AFEB02DCCA15A1F7C2117FBBA`
  (7,590 bytes)

This new-evidence wave expands the normalization idiom in the five nonexact
support bodies. The retained candidate owns exactly the six expected code
sections and no candidate-only code owner. `_code_00051190` remains strict at
128/128 padded bytes, 116 meaningful bytes, five relocations, and the target
normalized hash above.

The remaining one-shot measurements are retained only as scheduling evidence:

| Function | Target / candidate padded | Target / candidate relocations | Candidate normalized SHA-256 prefix |
|---|---:|---:|---|
| `_code_00051210` | 336 / 336 | 5 / 6 | `a8aef761703c` |
| `_code_00051360` | 288 / 272 | 6 / 6 | `69e2033cd9b8` |
| `_code_00051480` | 544 / 544 | 16 / 16 | `adee06cad9ad` |
| `_code_000516a0` | 896 / 864 | 30 / 30 | `29e824e1995c` |
| `_path_smooth` | 864 / 864 | 30 / 30 | `2abdcd17ab56` |

No source-shape retry followed Wave 3.

## Validation

- strict `tools.coff_compare`: `_code_00051190` equal, including padding and
  ordered relocation identity/addends
- candidate code-owner census: exactly the six target code owners, no extras
- `ninja halobetacache_build`: no work to do, exit 0
- `ninja progress`: 470 units, 4,294 evaluated functions, 4,211 accepted exact,
  zero unit errors
- `tools.audit_semantic_matches`: same 4,211 accepted exact, zero unit errors
- `python -m unittest discover -s tools -p 'test_*.py' -v`: 179/179 passed

Campaign movement from cumulative `fe0baf6b`:

- all code: `507503 -> 507619` meaningful bytes,
  `4184 -> 4185` functions
- Halo code: `494589 -> 494705` meaningful bytes,
  `4017 -> 4018` functions
- data unchanged at `1836756 / 4176062`

The pre-wave regression manifest correctly reports the new function and the
newly reconstructed non-code/symbol shape as changes from the empty skeleton;
it does not indicate an inherited exact regression because the unit had no
accepted baseline function. A clean post-commit snapshot/check is required
before integration.

## Policy record

The retained source is C89 and uses one parameter per line, explicit final
`return;` in every void function, typed tag-block access, and no asm,
register forcing, volatile/barrier workaround, pragma, raw address,
pointer/integer conversion, or undefined type-punning. The five nonexact bodies
receive no semantic credit and `path_smoothing.obj` remains incomplete.
