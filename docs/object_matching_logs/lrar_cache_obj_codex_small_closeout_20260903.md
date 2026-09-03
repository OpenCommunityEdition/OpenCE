# `lrar_cache.obj` small-object closeout audit (2026-09-03)

## Disposition

`source/memory/lrar_cache.obj` remains honestly `NonMatching`: 11 of 12
functions are strict exact and `_lrar_allocate` remains the single credible
fuzzy park at 67.61811%. No production source, header, symbol, status, or
parking entry changed in this closeout. The retained ordinary C is still the
best source-supported reconstruction.

This lane started from canonical commit `775cee0e2` in the isolated worktree
`worktrees/lrar-cache-small-closeout-20260903`. The dirty evidence checkout
`work/rasterizer-model-wrapper-alias-20260829` was read only. No applicable
`AGENTS.md`, `CLAUDE*.md`, or `TREE*.md` exists for this path.

## Strict baseline

`tools/campaign/gate.py source/memory/lrar_cache --all` reproduces:

| Function class | Count | January padded bytes accepted |
|---|---:|---:|
| strict exact | 11 | 1,344 |
| residual | 1 | 0 |
| unwritten | 0 | 0 |

The exact siblings and their padded sizes are `_lrar_new` (432),
`_lrar_flush` (320), `_lrar_deallocate` (112), `_lrar_block_address` (96),
`_lrar_verify_block` (96), `_lrar_get_block` (80), `_lrar_verify_cache` (80),
`_lrar_dispose` (64), `_lrar_unlock_block` (32), and the two 16-byte default
callbacks. The object total is 1,344/2,128 strict padded bytes.

| `_lrar_allocate` | January | Retained rebuild |
|---|---:|---:|
| padded size | 784 | 1,056 |
| relocation count | 27 | 47 |
| normalized SHA-256 | `a34c14dad0afc2a3462c738ca6153c3b46d2bdd0164b176dd208295594f86663` | `8a2fe453345af3d00eaf476f17eedbfc67629d8af6d6a036cc097d669dcc3652` |
| objdiff | - | 67.61811% |

The fail-closed codegen classifier remains `UNKNOWN` / `unclassified`, grants
no match credit, and reports unequal size, relocation count, identities, and
addresses.

## Aligned instruction evidence

A `difflib.SequenceMatcher` alignment over relocation-annotated, normalized
Capstone streams reports 262 January instructions versus 345 retained
instructions. The structural delta is narrowly localized:

- at target `+0x82`, January has a call to `_lrar_get_block` plus four result
  instructions; the rebuild expands the cache check, index assertion, block
  address, and block validation (target indices 47:52 versus base 47:93);
- at target `+0xCC`, January again calls `_lrar_get_block`; the rebuild expands
  the same validation path (target indices 72:75 versus base 113:155);
- the lookup after the ring-index increment is expanded in both objects;
- outside those holes, the only aligned body replacement is the expected
  `AX`/`SI` register choice at `+0x70`, followed by three trailing base NOPs.

Thus the extra 272 padded bytes and 20 relocations are direct consequences of
two additional helper expansions. This is evidence of a selective-inlining/TU
context boundary, not authorization to force a compiler schedule.

## Complete local source and artifact census

All 1,745 registered worktrees containing `source/memory/lrar_cache.c` were
hashed. They reduce to 12 raw working-file SHA-256 groups, including line-ending
representations, old skeletons, the historical reconstructed stages, the
rejected exact construction, and the retained source. Ref-reachable history for
the path contains seven C blobs and eight implementation commits from the
initial skeleton through `d655a7e99` (`Reject fake inline exactness`). No dirty
working copy contains a new allocator source topology.

The retained source has Git blob `65e9892a425dc2efc854ea6f57875f8c68314467`
and working-file SHA-256
`734e353e59901f2877d0712d68c7a49be5bf6a60ad7be57fdfeaa6304362037a`.
The owner header has Git blob `898e0f47ff873040ab3ccb794577f33010ba2367`
and working-file SHA-256
`9c7633010e9548501894948ac1e03ce65675503337a259236c613cea25b355ab`.

Of 1,267 available `build/base/source/memory/lrar_cache.obj` files, 379 are
skeleton-era objects without `_lrar_allocate`. The remaining 888 allocator
sections reduce to four normalized forms:

| Count | Padded bytes | Relocations | Normalized SHA-256 | Provenance/disposition |
|---:|---:|---:|---|---|
| 629 | 784 | 27 | `a34c14dad0afc2a3462c738ca6153c3b46d2bdd0164b176dd208295594f86663` | rejected forced schedule: `noinline` plus two copied helper bodies |
| 164 | 1,088 | 47 | `f95be472ea6794232a8d2bfcb9cdbfce98e7e594f6d6f2f31937582e8a5f3382` | cross-build `next_block_index` lifetime; farther from January |
| 94 | 1,056 | 47 | `8a2fe453345af3d00eaf476f17eedbfc67629d8af6d6a036cc097d669dcc3652` | retained natural helper-call source |
| 1 | 816 | 27 | `78b77484e0cb4a849f2203efac6a2c796f43a70fe00d1102ec9105c27fb85a20` | stale noinline-only experiment; no matching current source and no admission provenance |

The exact artifact count does not supply new evidence: those objects all descend
from the explicitly revoked construction in `14e4e8adf`. The sole 816-byte
artifact retains a third helper call where January expands the helper, has
different relocation identities/addresses, and is likewise not a source donor.

## Donor and documentation audit

- `claude/lrar-final-19` is already ancestral and supplies no branch-only body.
  Its exact result is the revoked forced schedule.
- `claude-hsmap-2` and the Stian/Pastudan HCE trees contain reverse-engineered
  lifts of a different executable's LRAR allocator inside `lruv_cache.c`. They
  are machine-topology analogues, not original Bungie C. Their separate
  `next_block_index` lifetime was recompiled through `gate.py --source` and
  reproduces the known 1,088-byte/47-relocation form, worse than the retained
  1,056-byte form.
- `claude/lra-cache-17` contains an old HCEX stage, not an LRAR donor.
- the Opus small-families tree and its reconciliation documentation contain the
  same retained LRAR source and park; no Opus-only candidate exists.
- the complete local Punpckhdq history contains only the HCEX skeleton and the
  same reachable reconstruction history. HalocEA, Baboon, demon, Nimbus,
  HaloPC Restored, xboxrecomp, PDB/symbol-atlas research, and the published
  Marathon 2/Infinity source contain no original LRAR implementation.

The only source form that reproduces January's selective schedule remains the
rejected combination of a `__declspec(noinline)` getter and two hand-expanded
copies of that getter. Restoring it would transcribe compiler output into C and
violate the campaign's source-credibility rules. No pragma, barrier, volatile
control, dead branch, undefined alias, assembly, or raw-byte steering was
tested or admitted.

## Header ownership

`source/memory/lrar_cache.h` has exactly one includer in the tree:
`source/memory/lrar_cache.c`. No external source references the six public LRAR
entry points. The typedefs and public prototypes already live in the correct
owner header and use the established vertical signature style. Because no
header edit was warranted, an aggregate header blast was not triggered; the
one owning translation unit was nevertheless covered by the strict gate and
the full build.

## Final validation

The final one-TU gate remained 11/12 and the aligned instruction result was
unchanged. A full 578-step `ninja` build completed successfully. Rename-stable
snapshots before and after the full build both contained 8,245 functions and
5,630 exact functions; the comparison reported zero gained bytes and zero
regressions. The independent units gate remained 189/189.

The repository test suite passed 261/261 tests when its temporary directory was
placed under the worktree. The targeted fake-match scan covered the LRAR source
and header and found zero review leads. Parked-function validation reported 161
active entries, zero stale entries, and zero invalid entries. The rebuilt LRAR
object has no `point_from_line3d` symbol. `git diff --check` was clean.

Since the closeout changes documentation only, the exact function/raw/padded
campaign delta is **+0 / +0 / +0** and the LRAR strict state remains 11/12 and
1,344/2,128 padded bytes.

Reopen `_lrar_allocate` only for original source or local/compiler provenance
that naturally explains the January selective-inline schedule. Do not restore
forced inlining controls or copied helper bodies for match credit.
