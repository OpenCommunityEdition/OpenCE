# Physics predicates and observer power materialization

Baseline: `9e851ed4e5cbe792b40f342fb5a4d8eb5847220f`.
This isolates two credible arithmetic corrections from older, larger held
packets. Neither correction creates an exact function or a completed object.
No held helper, pointer view, manual expansion, source-count instrument or
compiler-control change is included. The earlier four-site `fabs` correction
remains a separate commit and ledger.

## Physics: positive-height antigravity test

Both physics paths formerly used `height <= 0 ? 1 : 1-height/H`. January uses
the positive test, `height > 0 ? 1-height/H : 1`. These agree for ordered
values but not for NaNs. This is a behavior correction, not a cosmetic branch
inversion or a means of controlling register allocation.

January `_physics_compute_new` at `+0x8cd..+0x8ee` and `_physics_update_old`
at `+0xa9b..+0xabc` compare height to zero, then execute
`fnstsw ax; test ah,0x41; jne constant_one`. The previous compiled source uses
`jp arithmetic` after the same mask. The literals resolve to the actual zero
and one owners, not just unidentified float-looking dwords.

| Comparison with zero | Masked AH | January selection | Previous selection |
|---|---:|---|---|
| Greater | 0 | arithmetic | arithmetic |
| Less | 1 | one | one |
| Equal, including either zero sign | 0x40 | one | one |
| Unordered | 0x41 | one | arithmetic |

The supplied later `/Od` build corroborates the positive predicate with
`COMISS`/`JBE` at `0x7babfd/0x7bac04` and `0x7bf644/0x7bf64b`.
The proof concerns branch selection when execution reaches it; unmasked invalid
exceptions can trap earlier. It does not establish full-function numerical
equivalence, NaN payload behavior, or a map/runtime invariant.

Preregistered new-only, old-only and combined probes compose exactly. Only two
existing 22-byte diamonds and their contained literal-relocation positions
change: candidate new `[0x93e,0x954)` and old `[0xb8c,0xba2)`. Every byte and
relocation outside those ranges remains identical. The old diamond now equals
January's corresponding diamond; new retains its different height home slot.
The existing stale NonMatching size comment is also corrected without changing
its line count. No probe-length, scope, helper or other physics finding is added.

## Observer: explicit time powers before the coefficient loop

The old source computes only t2 and combines it with each coefficient as
`a*t2*t2*t`, `b*t2*t2`, `c*t2*t`. January instead materializes t2, t3, t4 and t5
before the coefficient loop. The correction restores initialized, meaningful
locals and consumes them in the existing source polynomial. No coefficient
reordering or other loop/control-flow change is made.

January `+0x18b..+0x1a3` multiplies successively by time, with FST dword at
`+0x191/+0x197/+0x19d` and final FSTP at `+0x1a3`. The coefficient loop reloads
all four single-precision homes. **FST does not round the continuing ST0 value**:
the next power can retain extended precision while the stored home is rounded
for later coefficient use. The candidate reproduces this exact distinction at
`+0x173..+0x18b`, followed by loads from the four homes.

Later `/Od` at `0x53046e..0x5304b7` independently supports the four locals and
their position, but its SSE sequence recursively rounds at each step. That is
not claimed as January's arithmetic. Original local names are unestablished;
the descriptive power names follow the existing neighboring routines.

The one preregistered probe preserves every exact sibling but grows by 32
padded bytes. Downstream sum scheduling still differs (candidate e,d,c,a,b,f;
January a,b,c,d,e,f), and unrelated structural differences remain. This is
deliberately reported as source fidelity, **not** an improved overall byte
score or an algebraically equivalent refactor. Rounding/overflow/exceptional
results can change. Every new local is initialized and genuinely consumed.
The larger F1 pointer-view and helper-expansion packets remain held.

## Measurements and review

| Function | Before padded bytes/relocs | After | January |
|---|---:|---:|---:|
| `_physics_compute_new` | 2992 / 49 | 2992 / 49 | 2944 / 49 |
| `_physics_update_old` | 5376 / 116 | 5376 / 116 | 5168 / 115 |
| `_observer_update_positions` | 1648 / 32 | 1680 / 32 | 1568 / 40 |

Physics remains 14/17 exact; observer remains 25/26. Independent reviewers read
the primary images and actual candidate sources/objects. Observer received four
additional independent control/candidate stock-/W3 compiles. Both TUs keep all
non-target code/data, named storage, COMMON, undefined references, section
flags/alignment and surplus providers unchanged. There are no new helper copies.

Stock diagnostics remain empty. Diagnostic `/W3` remains 16 warnings in physics
(including the four separately disclosed `fabs` conversions) and 13 in observer;
no new warning is introduced by this packet. Inputs, commands and outputs were
sealed before production integration. No supplied executable was run.

## Canonical verification

The final production objects equal their independently reviewed scratch
candidates across all nondebug content, including the comment-only physics
size correction. Only the three listed residual function sections change;
physics retains 83 sections/87 named definitions and observer 99/100.

- Full build/progress: pass, with frozen objdiff 3.3.1.
- Stable whole-board sweep: 8,252 rows, 7,641 exact, zero gains/losses.
- Parks: 72 active, zero stale/invalid. Exactly the three affected fuzzy
  measurements are rebaselined; every earlier evidence/hold/reopen note remains.
- Admission remains 12 candidates / zero contradicted / one rejected /
  zero revoked. The same 26 inherited fake-match leads remain.
- Tests: 1,161 passed, five skipped, 26 subtests passed.
- Scoped whitespace check passes; unrelated README/research dirt preserved.
- Halo totals remain 1,598,242 / 1,770,166 meaningful code bytes,
  7,469 / 7,574 credited functions, 390 / 468 complete objects and
  2,588,903 / 3,923,451 data bytes. No new credit or admission is claimed.

Final normalized hashes:

- new physics: `c629686ee285946e3985727ac9ef9d23bfbeabc1a84410c60f83647e72b6c5c2`;
- old physics: `30485cadbe38782890331c521302441305aec0b45856c30bebe59d2eca8b20cf`;
- observer: `fedbfb422193f7e38a3d8068215f252299b756ef86987dcceb5ccf456b9b595c`.

## Evidence and remaining work

Local immutable candidates, raw disassembly, negative controls and independent
reviews remain under `scratch/astra_frontier_sources_20260927/`:

- `math/polarity/`: preregistration, A/B/AB probe, primary truth-table evidence,
  full section/storage/warning comparisons and sealed verification;
- `physics_review/`: independent polarity review;
- `observer/`: one power-local probe, exact FST/FSTP distinction, whole-TU
  comparison, later-build caveat and retained larger-packet holds;
- `math/observer_review/`: independent rebuilds and primary/semantic review.

Canonical before/after receipts are under
`scratch/astra_numeric_topology_20260927/`. Private reference executables,
compiler/SDK files and object binaries are not committed.

The weapon-HUD read-only audit in `hud_weapon/REPORT.md` found no new eligible
source lever: known conversion/initialization repairs still emit helpers from
a nonexact caller, and its integer sentinel view remains separately held.
No such patch was tested again or landed. These bounded negatives do not prove
the residuals impossible; reopen on new primary source/lifetime/precision facts,
not repetition of the old count, name, grouping or helper experiments.
