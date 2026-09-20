# `actor_looking.obj` evidence closeout (2026-09-19)

## Scope and strict result

- Unit: `source/ai/actor_looking.c`; January target:
  `build/split/source/ai/actor_looking.obj`.
- Final strict gate: **14 exact, 2 residual, 0 unwritten** out of 16 listed
  functions.  The `_point_from_line3d` owner guard passes.
- No production source or configuration was changed in this pass and no fuzzy
  function is counted as exact.

## New evidence census

1. The reachable Git history contains 12 distinct blobs for this translation
   unit; `tools/campaign/branch_sweep.py` compiled all of them against the
   current January target.  None exceeds the current 14 exact functions.
2. Unreachable-commit traversal was performed separately (`git fsck` commit
   roots fed to `git rev-list --objects --stdin -- source/ai/actor_looking.c`).
   It contributes no additional source blob beyond the already measured
   historical set.
3. Eight distinct checked-out `source/ai/actor_looking.c` contents were
   compiled.  A further 139 unique focused candidates from the 100K, 150K, and
   throughput-recovery scratch portfolios were gated specifically on the two
   remaining rows.  Thirty stale candidates no longer compile with the current
   headers; none of the other 109 closes either function.
4. The January `cachebeta.pdb` was queried through DIA.  It contains only the
   `_actor_look_update` public (RVA and 4,720-byte extent); the private
   `_actor_look_idle_find_prop` is absent.  No function type, argument, local,
   lexical-scope, or line record exists for update.  The legacy IDA database
   likewise contains the public name but no recoverable local identifier/type
   strings.
5. Existing Ghidra output was checked instruction-for-instruction.  Its stack
   names and types are decompiler guesses and were not treated as source
   authority.  The HCEA Xbox/PPC reconstructions confirm high-level semantics
   but have a different ABI, layout, and debug branch set, so they are used only
   as semantic corroboration.

## Residuals

### `_actor_look_idle_find_prop`

January and the reconstruction are both 608 bytes with 20 relocations.  A
register-bijection walk reports exactly two events:

- January at `+0x8d`: `cmp ebx, dword ptr [_ai_debug+0x38]`;
- reconstruction: `mov eax, dword ptr [_ai_debug+0x38]` followed by
  `cmp ebx, eax`.

The source already expresses the direct equality using the named
`selected_actor_index` member.  Both operand orders, boolean/control-flow
spellings, declaration scopes, and the missing `game_time_get` include were
measured in earlier candidates; the new 139-candidate census confirms none is
an overlooked closure.  Introducing a volatile access, inline assembly, or an
invented helper merely to pin this one instruction would be fake steering and
is rejected.

### `_actor_look_update`

January is 4,720 bytes; the current source is 4,688.  Both have exactly 159
relocations and the same external call/data graph.  The best preserved
alternative is 4,704 bytes, but it remains nonexact and obtains its size by
reordering initializations plus redundantly re-evaluating an equality already
held in `change_facing`.  The cleaner current spelling is retained.

The surviving divergence is an allocator/stack-home cascade beginning with the
byte locals for `looking_free`, `facing_free`, and the secondary-direction
validity state.  January's PDB cannot supply their authentic names, types, or
scopes; Ghidra cannot elevate its inferred `char`/`bool` locals to primary
evidence.  All recorded source-plausible scope, order, conditional, switch,
and saved-boolean probes are included in the 139-candidate sweep and remain
nonexact.

The permitted `/QIfist` diagnostic is byte-inert for this unit and closes
neither residual.

## Admission result

No new strict function or object was found.  The evidence gap is now narrow and
explicit: authentic January local/type/scope information, or a measured VC7
allocator law predicting the named byte-local homes, is required for update;
find-prop requires a non-steering source reason for the direct global-memory
compare.  More equivalent-spelling search is not matching evidence.

## Exact-January Ghidra second pass

A fresh analysis of the exact January executable is preserved at
`scratch/ghidra_actor_projectile_ui.c`; `_actor_look_idle_find_prop` begins at
`0x00416600` and `_actor_look_update` at `0x00417650`.  The export is new
**primary binary evidence** for CFG, stack offsets, instruction order, and
field access.  Its `local_*`, `char`, and `bool` declarations are decompiler
inference rather than recovered source records.

For update, that graph corroborates three source-topology facts without
requiring aliasing or invented steering: the free/locked byte states precede
the two cosine loads, the facing flags are cleared before the exact-facing
merge, and the decoded minor direction has an explicit validity state.  The
minimum safe packet is recorded in
`scratch/actor_update_primary_evidence_safe.json`; it remains 4,688 padded
bytes but improves the normalized aligned operand score over both canonical
and the earlier 4,704-byte v12 candidate.  It is evidence-quality fuzzy work,
not an exact landing.

A learning-only probe in
`scratch/actor_update_safe_duplicate_face.json` re-evaluates the secondary
priority equality and scores slightly closer.  It was rejected for production:
the current named `change_facing` value already holds that equality, and no
source record authenticates a redundant recomputation.  The stationary 2D
representation cast and pointer-local register probes also remain rejected or
worse.  Find-prop's sole direct-global-load scheduling difference is unchanged.
No production source or configuration was changed in this second pass.
