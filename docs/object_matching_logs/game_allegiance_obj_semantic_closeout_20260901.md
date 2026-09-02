# `game_allegiance.obj` semantic closeout

This 2026-09-01 closeout removes the last address-derived code and data names
from `source/game/game_allegiance.c`, preserves all 13 strict-exact sibling
functions and both exact non-code owners, and records the two remaining
ordinary-C register-allocation fixed points as evidence-backed fuzzy parks.
The object remains `NonMatching`: no exact, semantic, configuration, or data
credit is claimed for either residual.

## Scope and frozen inputs

- Isolated base: `ccc148aa9`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split object SHA-256:
  `005dc3acdefcc3bbb2c041087494c54531400a18babab990fb405a27172ed9cf`.
- Prior production source blob: `954efb10e5e08e930d33cf550dd88d11ae4b6a58`.
- Retained source blob: `07d457818d2ee27e34a6e6730021847902336435`.
- XDK `CL.Exe` SHA-256:
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
- The complete earlier lifecycle, public-leaf, public-continuation,
  private-cluster, and freshness ledgers were read before this closeout. Their
  bounded source-shape exhaustion remains authoritative and was not repeated.

## Recovered private and global names

The private state-transition function is **`game_allegiance_broken`**, not an
inferred descriptive substitute. The exact January cachebeta atlas at
`research/halo-symbol-atlas/symbols/halo_ce/4cc87b45...jsonl` maps image
address `0x496300` to `_game_allegiance_broken` in `game_allegiance.obj`.
That atlas has SHA-256
`851802ac508bbcf7cd2896c6cfddd1e0f81510b7b9ef9ea02f39f913c123578e`.
Multiple Gearbox Halo CE maps independently preserve the same name. Later
HCEA PDB evidence records
`static void game_allegiance_broken(struct game_allegiance *, unsigned char, unsigned char)`
in the same compiland, and the HCEA/Pastudan donor logic maps one-to-one to
January's four callers and bit-matrix transition. The HCEA donor blob has
SHA-256
`d2408f9e98c980360912f7f5040a6e06a508fcf2038d6c816a00349331f84903`.

The globals pointer is now named `game_allegiance_globals`. This is the named
HCEA data owner and the pointer used by every recovered allegiance routine;
the old `_bss_0043e490` spelling carried no semantic information. The source
defines the pointer directly with `= NULL`, retaining the January four-byte
`.bss` owner instead of hiding it behind a macro alias. `config/symbols.json`
now records both semantic names and marks only the helper static, matching the
PDB linkage evidence. The cross-subsystem AI callback declaration was also
moved out of the using `.c` file into the narrow owner header
`source/ai/ai_runtime.h`, which is included by both `ai.c` and
`game_allegiance.c`; its owner and consumer gates are unchanged. Finally, the
three recovered HCEA incident constants
`_allegiance_incident_accident`, `_allegiance_incident_betrayal`, and
`_allegiance_incident_forgive` replace raw switch labels without changing the
already-exact incident routine.

Regenerating the split target after these symbol corrections proves the rename
does not conceal a regression: the focused gate still reports 13 exact and two
residual functions. The 40-byte `_global_game_team_names` table remains exact
with all ten relocations, and the renamed four-byte
`_game_allegiance_globals` owner remains exact with no relocations.

## Current residual boundary

| Function | Target / candidate padded bytes | Relocations | Objdiff | Fixed point |
| --- | ---: | ---: | ---: | --- |
| `_game_allegiance_broken` | `416 / 416` | `2 / 2` | `94.296875%` | All 142 decoded instructions and both relocation identities preserve the authenticated transition. January keeps `permanently_broken` in DL and the bounded team words in CX/SI; VC7 uses CL and SI/DI and extends the ESI save through the tail. |
| `_game_allegiance_create` | `224 / 224` | `5 / 5` | `83.69334%` | All 82 decoded instructions and all five relocation identities preserve the create-or-replace flow. January assigns the globals base to EDI and the hot team word to DX; VC7 assigns them to EDX and a shrink-wrapped DI. |

The ordinary source remains the simplest coherent typed reconstruction. The
prior packet already exhausted index widths, predicate and declaration order,
alias scopes, direct-global versus local access, pointer/indexed traversal,
field-store order, bit-index locals, and helper spelling. This closeout also
censused every currently registered worktree: 1,646 copies collapsed to ten
unique source blobs, and none displaced the retained 13/15 complete frontier.
Claude, Fable, HCEA, Pastudan, Stian/symbol-atlas, and reachable local material
therefore supply no newer lawful exact body.

Both entries are parked as `register-allocation`. No `register` hint,
`volatile`, compiler barrier, pragma, inline assembly, artificial carrier,
fake dependency, undefined behavior, raw-address access, or nonsensical
control flow is present or was admitted.

## Validation

- focused gate: 13 exact, two measured residuals, zero unwritten;
- full `all_source` Ninja compile: pass (570 compiled translation units);
- strict board: 278/619 objects, 715,107/1,922,413 padded code bytes,
  5,013/8,245 strict functions; parks remain excluded from strict totals;
- semantic report: 472 units, 5,161 functions evaluated, 5,037 accepted
  exact, zero unit errors;
- Units gate: 189/189 exact;
- parked validation: 70 active, zero stale, zero invalid, including both
  allegiance residuals;
- AI owner gate: unchanged at 13 exact, zero residual, 33 unwritten;
- fake-match scan: zero findings in the changed source/header surface;
- tool tests: 255 passed;
- object-admission audit: zero contradictions or revocations; the sole
  unrelated `player_rumble` ownership candidate is unchanged;
- `_game_allegiance_broken` and `_game_allegiance_create` retain ordered
  relocation identity equality, including the renamed globals and private
  helper destinations;
- no `point_from_line3d` symbol or COMDAT appears in either allegiance object;
- `git diff --check`: pass.
