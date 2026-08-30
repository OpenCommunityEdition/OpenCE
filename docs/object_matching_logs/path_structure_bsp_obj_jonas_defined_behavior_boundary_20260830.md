# `path_structure_bsp.obj` defined-behavior boundary — 2026-08-30

## Verdict

`source/ai/path_structure_bsp.obj` remains **3/6 strict exact** and is not an
eligible whole-object closeout under House C.  The three absent January owners
total 2,208 padded bytes:

| Owner | January padded bytes | Status |
| --- | ---: | --- |
| `_structure_test_ray2d` | 480 | historical exact body is inadmissible undefined behavior |
| `_structure_test_line2d` | 1,104 | legal-C scheduling fixed point |
| `_structure_test_pill2d` | 624 | legal-C local-colouring/scheduling fixed point |

No production source was changed and no normal VC7 candidate compile was
spent in this freshness pass.

## Instructions and prior work read

The campaign matching methodology, exact-match acceleration playbook, campaign
tooling README, repository instructions, and all four existing object ledgers
were read before the audit.  In particular, the 26-family line-test matrix and
18-family pill-test matrix already exhaust declaration, scope, alias, helper,
operand-order, return/CFG, barrier, and branch-copy spellings.  Those families
were not repeated.

The current gate reports:

```text
== exact 3  residual 0  unwritten 3  (of 6 listed)
```

The inherited exact owners remain
`_structure_surfaces_are_equivalent`,
`_clip_empty_interval_by_solid_interval`, and
`_structure_surface_index_from_point`.

## Fresh source and artifact census

The all-ref history contains ten distinct committed
`source/ai/path_structure_bsp.c` blobs.  No source change touching this unit is
newer than the safe salvage merged on 2026-08-27; the remaining committed
bodies are the already-measured August 12 atomic experiments.

A filesystem census across the Codex workspace and `C:/halo-worktrees` found
**1,565 source copies with 16 physical SHA-256 hashes**.  The two dominant
hashes are the empty upstream body (`32146cc4...`, 1,183 copies) and the current
three-owner safe salvage (`d1ced68e...`, 346 copies).  The remaining copies are
the documented August 12 experiments, the August 20 salvage, and one
line-ending-only Fable copy.  Claude's near-complete and untried-object trees
carry the empty upstream body.  Fable's current tree is content-identical to
canonical.  The authenticated punpckhdq tree is empty for this unit; the
authenticated Stian and Pastudan trees contain no implementation.  HCEA is
already the control-flow hypothesis recorded in the existing ledgers and adds
no new declaration-lifetime or scheduler evidence.

Sixteen preserved target/candidate object files in the dedicated path-BSP
worktrees have eleven raw hashes.  Strict comparison confirms the best atomic
artifacts reach four exact owners only by including the inadmissible ray body;
none makes `_structure_test_line2d` or `_structure_test_pill2d` exact.  No
unmeasured rebuilt exact owner was found.

## Hard whole-object blocker

January's `_structure_test_ray2d` loads
`pathfinding_surfaces[next_surface_index]` before testing
`next_surface_index != NONE` on both the enter and exit paths.  When the
sentinel is present this indexes one byte before the array.  The historical
480-byte exact candidate reproduces that out-of-bounds read and additionally
views byte-backed breakable flags through an incompatible `long const *`.
Both are forbidden by the defined-behavior and typed-access rules.  Moving the
sentinel test before the load is defined C but cannot reproduce January's
instruction order.  Therefore even a future exact solution to the two
scheduler residuals would still leave the object inadmissible without new
evidence for a defined typed source topology.

## Reopen boundary

Reopen only with at least one of:

1. authenticated January source/PDB lexical records proving a defined typed
   ray topology that emits the target load order;
2. a same-compiler exact donor that explains the line test's `fstps`/reload
   pair without a barrier or changes before the collision tail;
3. a same-compiler exact donor that explains the pill test's perpendicular
   lifetime and tail-hoist pattern without casts, aliasing violations, or
   byte-forcing.

Absent such evidence, another source-family compile cannot close this object
and must not be spent.
