# `actions.obj` Codex closure ledger

This file continues the Actions investigation without modifying the earlier
Claude-authored `actions_obj.md`.  January's object is always authoritative;
HCEX/PDB material is used only to recover names, types, and plausible source
topology.  Every experiment below changes one readable-C factor, rebuilds the
whole translation unit, and receives no credit unless the hardened comparator
accepts the complete function and its relocation identities.

## Verified starting point

After a fresh rebuild, 55 of 63 functions are strict exact.  Both owned data
sections are also exact: the combined 888-byte/96-relocation action table block
and the 16-byte/3-relocation actor-mode-name block.  The eight remaining
functions have the correct target section size and relocation count:

- `code_0000b3b0`
- `actor_action_handle_vehicle_entry`
- `actor_action_handle_combat_selection`
- `actor_action_handle_lost_contact`
- `actor_action_try_to_dive`
- `actors_searching_same_position`
- `actor_action_handle_evasion`
- `actor_action_handle_danger_avoidance`

The reconstructed `actor_action_handle_surprise` must retain
`boolean result = FALSE;`.  Removing the initializer loses an explicit January
zero materialization and regresses that previously exact function.

## Rejected closure experiments

### Vehicle entry: enclosing negative guard

The target has three result epilogues, so the setup tail was respelled as an
enclosing negative guard in an attempt to separate its late false exit from
the early named-result exit.  The function stayed 960 bytes with 22
relocations, but the allocator and control-flow graph changed broadly: 240
normalized byte positions differed instead of the prior best 39 and several
call relocations moved.  The prior readable best form was restored.

### Lost contact: HCEX local declaration order

The nested scalar declarations were reordered to match the later HCEX
CodeView record.  This did not recover January's private helper convention or
frame coloring.  `actor_action_handle_lost_contact` still had about 1,204
different positions, while `code_0000b3b0` stayed at 327; multiple relocation
sites shifted.  The January-best declaration order was restored.

### Evasion: mixed direct and named-result exits

The return topology was rewritten to mirror the apparent January split between
direct success exits and named-result exits.  VC7 instead cross-jumped the
blocks, shrinking the function from 800 to 784 bytes and worsening the overlap
to 513 differing positions with downstream relocation shifts.  The prior
800-byte/27-relocation form was restored.

### Try to dive: authentic vector aggregate by itself

HCEX CodeView names a `real_vector2d evade_vector`, so the two scalar alignment
components were replaced with that aggregate without altering expressions or
control flow.  Under external linkage this was codegen-neutral in the relevant
allocation: the function remained 720 bytes/29 relocations with 634 differing
positions and broad relocation-offset drift.  The proven scalar form was
restored.

### Try to dive: static linkage by itself

HCEX CodeView says the original function was file-local.  Making only the
definition `static` recovered January's important entry allocation exactly:
`actor_index` stayed in ESI and was copied to EDI for debug-array indexing.
However, the candidate then emitted only 704 bytes, retained 29 relocations,
and differed at 493 normalized positions.  Its first remaining mismatch was
the actor spill at `[ebp-0x20]` rather than January's `[ebp-0x18]`; the switch
also merged two cases that January keeps separate.  Static linkage was
therefore reverted.  This is useful diagnostic evidence, not a match claim.

The target's external COFF symbol versus a static candidate is not by itself a
permanent blocker: this same object already has strict-exact file-local
functions whose csplit target symbol is external.  The static experiment is
rejected because its bytes and section size miss, not merely because the COFF
storage-class numbers differ.

### Try to dive: scalar declaration order

This experiment retained external linkage and changed declaration order only.
January's stack homes are: best animation at `-0x8`, best score at
`-0xC`, best direction at `-0x10`, actor spill at `-0x18`, and the alignment
components at `-0x1C/-0x20`.  The best candidate previously put the alignment
components first at `-0x8/-0xC`, pushing the later values downward.  Therefore
the alignment declarations were moved after the three best-choice scalars.
A fresh whole-object build was byte-identical to the prior external baseline:
720 bytes, 29 relocations, normalized SHA beginning `e669dd20`, first mismatch
at `+0x0D`, and 634 differing overlap positions.  The entry still destructively
used EDI and the actor spill remained at `-0x20`.  VC7's coloring is invariant
to this declaration ordering, so the readable original order was restored.

### Combat selection: scalar declaration order

January places the cached prop distance at `[ebp-0x8]`, the melee charge delay
at `[ebp-0xC]`, and the actor-variant definition pointer at `[ebp-0x10]`.
The candidate instead cycles those values through `-0xC`, `-0x10`, and
`-0x8`.  To test whether this was ordinary declaration coloring,
`prop_distance` and `charge_delay` were declared before the variant pointer;
the `REAL_MAX` assignment and every call remained at their previous execution
points.  The rebuilt 1,776-byte/56-relocation function was unchanged in the
relevant allocation: the variant pointer remained at `-0x8`, prop distance at
`-0xC`, first mismatch at `+0x41`, and 1,013 normalized positions differed.
Later relocation sites also retained the broad allocation cascade.  The
original readable declaration order was restored; simple declaration order is
not a control for this three-value cycle.

### Combat selection: direct last-attempt field

January keeps the current game time in EDI and does not load
`last_melee_check_time` until the comparison, while the candidate's named
`last_attempt_time` local caused that field to be hoisted into EDI.  Removing
the local and spelling the two adjacent comparison reads directly from the
actor tested whether the field binding caused the register cascade.  Instead,
VC7 shrank the function from 1,776 to 1,760 bytes, kept 56 relocations, moved
the first mismatch to `+0x6A`, and increased the overlap difference to about
1,200 positions.  The desired local register change did not preserve the
surrounding control-flow/allocation topology.  The named readable local was
restored.

## Frozen 55/63 checkpoint

The last rejected experiment was fully restored and a fresh build again
reports **55 of 63 functions strict exact**.  This is the checkpoint source,
not a claim that `actions.obj` is complete.  Each of the eight remaining
functions still has January's exact padded section size and relocation count;
their normalized bytes and/or symbolic relocation placement remain different,
so none receives credit.  The combined 888-byte action-table/default-state
owner remains exact at 96 relocations, and the 16-byte actor-mode-name owner
remains exact at three relocations.

The bounded closing experiments above now answer the natural source questions
they were designed to test.  In particular, simple declaration reordering did
not control the residual stack coloring, direct-field spelling caused a broad
allocation cascade, and topology rewrites changed section size or cross-jump
behavior.  No independently supported readable-C factor remains ready for a
production experiment.  Freezing the verified source is therefore safer than
turning register-allocation symptoms into speculative source churn.

The object intentionally stays `NonMatching`: `config.json`, the parked
ledger, compiler flags, comparator, and semantic-credit files are unchanged.
No assembly, volatile access, force/no-inline annotation, optimizer barrier,
undefined-behavior trick, type-punning byte force, or compiler-flag change was
used.  January quirks are retained, including the original
`code_0000c970` assertion that does not validate
`escape_is_ledge_reference`.  The separate Claude-authored
`actions_obj.md` was not edited, renamed, or deleted; this Codex-owned file is
the only new object log.

## Final checkpoint gates

The restored source was rebuilt again with the unchanged campaign compiler and
flags before the checkpoint audit.  `research/actions_strict_status.py`
returned 55/63, and the hardened semantic report independently recorded those
same 55 functions at **10,906 of 19,172 meaningful January code bytes
(56.89%)** and 11,392 padded bytes.  The eight residual rows above retain
their exact January padded sizes and relocation counts.  Direct hardened COFF
comparison also reconfirmed both data owners: the 888-byte/96-relocation
combined block has normalized SHA-256
`b4bd37bc3750a724b625c0254a2b8bc3272c1aff70c71ae46952bac6acd4a9ab`,
and the 16-byte/3-relocation name block has normalized SHA-256
`374708fff7719dd5979ec875d56cd2286f6d3cf7ec317a3b25632aab28ec37bb`.

The full Halo and libcmt build targets pass.  Fresh progress and semantic
generation scanned 470 units with zero unit errors; the admission audit found
no Actions completion candidate and granted no credit.  All 179 tool tests
pass.  `git diff --check`, the config/parked/semantic-credit diff, the
forbidden-construct scan, and the Claude-Markdown preservation audit are
clean.

The formal regression-manifest snapshot is the sole administrative gate not
recorded in this worktree.  It correctly refuses to snapshot a dirty tree, and
the desktop Git metadata ACL/approval-service usage limit prevented creation
of the local checkpoint commit needed to make the tree clean.  The forced
Actions rebuild and strict whole-object sweep itself did pass at 55/63; the
lane is left commit-ready so the clean-commit snapshot/check can be run as
soon as Git metadata writes are available.  This limitation does not turn the
checkpoint into object-level credit.
