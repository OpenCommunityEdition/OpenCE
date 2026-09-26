# Final independent research-packet review

Scope: read-only review of root `coupled_probe.py`, `README.md`, `NEXT_MODEL_TASKS.md`, and updated `abi/return_contract_census.py`; cross-checks against current census, ABI JSON, four-cell runner receipt, scout ledger and decal-clip findings. No root code or production files edited.

## Result

The packet is credible research with correctly bounded zero-credit claims. It does not claim a new exact function, cleared hold or object admission. The 122-row population bridge, byte totals, allocation subset and current frame facts match the generated census. Scout distinguishes emitted materialization/slot differences from an unproved internal compiler explanation and retains the aggregate-view hold. Decal clip distinguishes a witnessed spill operand rewrite from an unproved final reload-order rule; opcode `0x21` has not yet been mapped to the final MOV/INC/store sequence. The new node-link observation supports following downstream integer materialization rather than asserting a global register-choice explanation.

Independently executed:

```powershell
python -m pytest -q tools research/astra_breakthroughs_20260925
```

Result at review: **1,186 passed, 5 skipped, 26 subtests passed**. This is 1,161 existing tests and 25 research tests. A later regression test added for the issue below will increase the total; refresh the root closeout count if so.

ABI artifact cross-check: 467 compared units, 7,440 exact owners, 119 residual owners, 43,445 paired calls, 62 incomplete-decoding skips, zero findings/errors. The saved positive control reports exactly `_actor_perception_update` / `_actor_get_perception_knowledge`, with opposite ESP-adjustment/EAX-copy ordering before the 16-bit comparison.

## Two corrections sent to the root owner

1. **Incoming LOOP joins need collecting.** The updated observer correctly stops when it encounters LOOP directly, but `calls()` collects incoming block entries only through `CS_GRP_JUMP`. Current Capstone reports LOOP/LOOPE/LOOPNE in group 7 (`branch_relative`), not group 1 (`jump`). Therefore an incoming LOOP edge can bypass the join guard. A complete byte stream demonstrating the uncollected target is `e800000000900fbec831c0e2f8`: call; NOP at +5; MOVSX ECX,AL; XOR EAX,EAX; LOOP +5. Collect explicit LOOP immediate targets as well as ordinary jump targets and add a regression test. This does not invalidate the research-only disclaimer, but is a small fail-closed correctness gap in the scanner.

2. **Runner baseline wording should name target-function sections.** The root README originally said BASE/A/B/AB all reproduce "the production object." `compare()` checks January target `.text` function-start owners only. Its BASE drift check does not compare full raw objects, non-code sections, production-only extra functions, storage classes or COMDAT selection metadata. The supplied control receipt has no target-owner drift or gained/lost exact rows in any cell, which supports "reproduce production target-function sections," not whole-object equality. Keep the more precise wording and document this baseline boundary.

## Common runner limitations (not new admission blockers)

- Only one `.c` TU and textual source factors are supported. No header edits, config changes, `.cpp` owners or cross-TU provider packets are modeled.
- Full-TU target-function controls are not full-object ownership, full-board regression, final-link, source-authenticity or admission checks.
- `extra_function_owners` means extra relative to January target owners. The list includes pre-existing surplus, not only newly introduced surplus; compare it with BASE before calling a helper new.
- The receipt pins source, source-header tree, Ninja graph, target and base-object hashes. It does not pin external SDK headers, compiler DLLs, gate/comparator code or every command dependency. It is a useful local receipt, not a hermetic build proof.
- BASE function drift stops the experiment, and each source transformation must apply exact occurrence counts and commute. Those are sound mechanical checks but do not themselves authenticate the proposed source factors.
- Compile/gate success does not make a policy-held source body admissible; all four cells are retained as diagnostics even when their exactness differs.

The three runner unit tests cover text substitution/count requirements; the saved real BASE/A/B/AB inert-comment control additionally exercises the actual compiler/gate path. More mocked runner tests for missing outputs and baseline drift would improve robustness, but are not required for the current diagnostic result.

## Updated ABI assessment

The earlier overwrite, high-byte, trap, incomplete-decoding and omitted-call pairing concerns were addressed. Tests cover explicit, implicit and partial register kills; copied-register kills; low versus high aliases; LOOP/trap/explicit-join boundaries; and repeated-name pairing. Remaining limits are appropriately disclosed: linear rather than complete CFG recovery, heuristic unique-callee pairing, source casts versus declaration inference, and delayed argument cleanup. Zero findings is a narrowly tested negative worklist, not a claim that all return declarations are correct.

No policy hold should be widened as a result of this review. The only requested changes are the small scanner join guard and precision of the runner's claim.
