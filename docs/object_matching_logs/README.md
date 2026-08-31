# Object exact-match strategy logs

Every object that receives implementation or matching work must maintain one
Markdown ledger in this directory. The filename is the object name with
`_obj.md`, for example `players_obj.md`.

General techniques that apply across translation units belong in
`docs/exact_match_acceleration_playbook.md`; object-specific measurements stay
in the individual ledger.

The purpose is to make matching work cumulative. A new lane must be able to
distinguish an untried evidence-backed source shape from a spelling that has
already compiled to the same bytes.

## Required contents

Each ledger must include:

1. **Scope and provenance**
   - source translation unit and target object;
   - exact compiler identity and flags;
   - target-build provenance used for names, layouts, or source topology.
2. **Validated baseline**
   - exact/total function count and padded code bytes;
   - exact/total data sections and bytes;
   - for every residual: target/base size, relocation count, normalized hash,
     and first meaningful divergence.
3. **Accepted controls**
   - the source shapes retained in production;
   - why each shape is supported by binary or provenance evidence.
4. **Experiment matrix**
   - every meaningful source shape tested;
   - resulting size, relocations, normalized hash, first divergence, and
     sibling-regression result;
   - whether it was retained or reverted.
5. **Do-not-repeat list**
   - equivalent spellings or compiler-control attempts already exhausted;
   - unsafe or prohibited techniques.
6. **Residual classification**
   - source/topology, ABI/register, scheduling, ownership/relocation, data, or
     unresolved;
   - clearly separate measured fact from inference.
7. **Reopen criteria**
   - the exact new evidence that would justify spending another lane.
8. **Disposition**
   - strict exact and eligible for `Matching`, still active, or rigorously
     parked;
   - never grant credit from a fuzzy percentage, equal size alone, or semantic
     plausibility.

## Lane rules

- Update the ledger after every bounded experiment batch, not only at handoff.
- Rebuild with XDK 3911 and the repository flags after each retained change.
- Gate every function and owned data section with the hardened comparator.
- Run the whole-translation-unit regression sweep before committing.
- Revert failed source shapes immediately.
- Treat exact bytes as necessary but not sufficient: reject semantically
  nonsensical, historically implausible, or inert compiler-steering source,
  even when it happens to match. Park the best honest fuzzy reconstruction
  when a natural exact spelling is not available.
- Do not use inline assembly, volatile byte forcing, undefined behavior,
  object-byte patches, or compiler-flag changes.
- A parked residual must preserve enough evidence for another agent to
  reproduce the classification without redoing the full search.
- Label a defect `BUG (original)` only when January binary/source evidence
  proves the defective behavior belongs to the original game. A mistake or
  omission in our reconstruction is not an original bug. Preserve proven
  original behavior in the matching build and document the corrected-build
  alternative in a comment without changing the matching token/line shape.
- When retroactively documenting an old lane, state which experiment details
  were not preserved. Never manufacture source variants, hashes, or outcomes
  to make a historical matrix look complete.

## Minimal experiment row

```markdown
| ID | Source shape | Size T/B | Relocs T/B | Base hash | First divergence | Siblings | Decision |
|---|---|---:|---:|---|---|---|---|
| E01 | ... | `0x.../0x...` | `N/N` | `...` | `+0x...: ...` | `K/K exact` | reverted |
```
