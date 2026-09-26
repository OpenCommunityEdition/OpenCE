# Astra breakthrough research: current evidence and next attacks

Baseline: `20d0563d508f7836670db62514ffc3d1f1e2217e`, `jonas/exact-pilots`.
Research only: **zero new credited code bytes, zero object admissions, no production
source/config/header changes, no policy exceptions**. Preserved candidates remain
research, not approved source. The original program was compilable; a failed
finite search is not proof that a residual is unreachable. Conversely, matching
bytes alone do not establish that every matching source trick is authentic.

## What is new

1. **A repaired frame census finds a previously invisible class.** Checking only
   `sub esp,N` misses large frames allocated by `mov eax,N; call __chkstk`. The
   new relocation-verified decoder finds **28 allocation mismatches covering
   59,512 meaningful residual bytes**, versus 25 / 43,854 with the old method.
   The extra three are `_decal_new_from_collision` (-4 bytes),
   `_biped_update_physics` (+8), and `_actor_move_vector_avoidance` (+4).
   These are measured allocation differences, not promised gains or new source
   discoveries. Thirteen synthetic controls test the detector. See `census/`.

2. **The scout's final four-byte frame difference is localized to value
   materialization.** Independent left-vector and forward-vector aggregate-copy
   controls show that only the computed local left-vector copy changes the
   frame. The scalar form keeps values on x87; the aggregate form materializes
   them and copies integer words. VC7's assembly listing ties this to different
   sharing of the left/desired-velocity/forward/alpha slots. Seven current-tree
   variants preserve the other 38 exact functions. A fresh Ghidra read of the
   later debug build corroborates the copy instructions, not January's complete
   source. The remaining x87 leaf-order difference is separate. The load-bearing
   aggregate view remains held. See `scout/`.

3. **The decal clip's remaining load swap is now tied to an allocator web.**
   An observation-only bridge correlates register-allocation web symbol pointers
   with the final frame/type records. The edge-iteration counter is seen on the
   spill/retry route, not the normal register-choice route in that captured
   pass. A further live probe identifies the register-web-to-memory operand
   rewrite: its instruction and list links remain unchanged. The standalone
   reload must be explained downstream, not by that global CHOOSE operation.
   Stock and observed complete objects compare equal after masking only the
   COFF timestamp. See `decal_clip/` for the trace, qualifiers and exact next
   debugger question; no first January-versus-candidate IR divergence is claimed.

4. **One common four-cell runner replaces one-off test scripts.**
   `coupled_probe.py` compiles BASE, A, B and AB through the existing whole-TU
   gate, preserving all outputs. It requires a hash-pinned source, explicit
   predictions, exact edit counts and commuting changes; checks the unmodified
   target-owner function sections against production; and reports every
   inherited exact regression in that population.
   A genuine fix can need both factors even when neither works alone. The
   runner does not invent factors, award credit, or replace the full-board,
   ownership, provider-link and house-rule checks.

5. **A current, positive-controlled negative worklist.** The return-contract
   scanner redetects the historical actor-perception bug in its saved pre-fix
   object but finds **no new signature in 43,445 paired call observations**.
   It skips 62 incompletely decoded functions and makes no claim about all
   prototypes. Independent review caught and corrected stale-register and
   high-byte false positives. See `abi/`. Do not respend a wave on the old
   caller-spelling theory without new evidence.

## Actual remaining work

The current accepted Halo ledger is **1,583,050 / 1,770,166 meaningful code
bytes**: **187,116 remain**. The 122 nonaccepted report rows consist of 119
strict residual function owners, two missing same-name owners, and one interior
jump-table label. The table is not an independently missing function.

**388 / 468 Halo objects** are complete. The object queue has 80 open objects,
including 13 with no report function gap. Exact functions alone do not settle
data layout, private/static ownership, surplus COMDAT selection, whole-program
linking or held source forms. Libraries and the synthetic COMMON pool are not
new Halo function credit. The census reproduces the production accounting;
it does not redefine it.

The largest ten report residuals contain 72,415 meaningful bytes; the largest
twenty contain 105,388. Allocation mismatches are an overlapping subset, **not
additional bytes to add**. Current `_ai_debug_render_actor` already has the
right 0x810 frame: its old +12-byte hypothesis is obsolete.

## Run / reuse

From this checkout:

```powershell
python -m pytest -q tools research/astra_breakthroughs_20260925
python research/astra_breakthroughs_20260925/census/census.py
python research/astra_breakthroughs_20260925/abi/return_contract_census.py --output research/astra_breakthroughs_20260925/abi/current.json
python research/astra_breakthroughs_20260925/coupled_probe.py research/astra_breakthroughs_20260925/runner_control.json --output scratch/astra_coupled_control_new_run
```

The runner's output directory must be new and under `scratch/`; it never deletes
previous experiments. `runner_control.json` is deliberately two inert comment
edits, **only a harness test**, not proposed source reconstruction. Its measured
BASE/A/B/AB all reproduce the production target-function sections with zero
gained/lost functions. This is not a noncode/storage/COMDAT metadata comparison.
For actual research, replace the manifest with two independently evidenced
source hypotheses and test their joint prediction. Headers need an isolated
full-consumer experiment; this source-only runner does not support header edits.

The final project+research test result and any additional tracer qualifications
are recorded in the closeout section below. No full production rebuild is
needed solely because documentation changes; a fresh baseline build and strict
snapshot are included in the census.

## Dispatch

`NEXT_MODEL_TASKS.md` contains scoped research assignments, success criteria,
known negatives and non-negotiable admission boundaries. Read the independent
review when an older worker proposal disagrees with it. Do not turn a research
candidate into canonical code just because its gate row is exact.

Historical starting points worth reading with their corrections:

- `docs/object_matching_logs/claude_lane_a_narrow_return_package_20260920.md`:
  the "impossible" conclusion was retracted after a coupled type/body fix.
- `research/fifty_objects_20260925/results/wave1/REVIEW__source_physics_collisions.md`:
  callee-body visibility/escape mechanism affirmed; focused-header and scope
  workaround rejected.
- `research/fifty_objects_20260925/results/wave1/REVIEW__source_units_vehicles.md`:
  a homed floating temporary does not uniquely prove a helper call.
- `tools/c2dbg32/IR_LAYOUT.md` and `BACKEND_MODULE_MAP.md`: several old compiler
  field/function interpretations were corrected. Never use an assertion stub
  as an allocator entry point.

## Closeout

- Production tracked diff: empty at research closeout; all additions are under
  this research directory.
- Tests: **1,187 passed, 5 skipped, 26 subtests** (1,161 existing + 26 research).
- Parks and accounting: baseline reproduced, no admissions or park changes.
- IDA Free installation exists, but the new isolated probe failed with a
  registry-access error; do not claim successful IDA analysis. Ghidra's ordinary
  live endpoint was unavailable; the scout packet records the successful
  read-only standard-launcher export and its script.
- No original EXE/PDB/compiler binaries are included in this packet. Existing
  local toolchain/reference paths are needed to replay compiler/debugger runs.
- Archived unified patches retain their required context-prefix whitespace;
  dependency source snapshots retain their hashed original trailing blank lines.
  They therefore trigger raw `git diff --check` artifact warnings. Newly authored
  tools/docs pass when those two evidence classes are excluded. Local attributes
  preserve these evidence bytes across Windows checkouts; production whitespace
  checks and all matching/admission tests are unchanged.
