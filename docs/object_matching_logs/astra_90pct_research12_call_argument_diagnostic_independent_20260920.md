> Research-only packet12: zero retained production changes and zero admitted exact credit. Latest validated production remains wave9.

# Independent scanner review

**PASS as a narrow diagnostic locator, with the limits below. No admission authority, compiler run or source edit.** Reviewed final scanner SHA `6358abe1ac52b7d5cded97a66a5c0f481679220c5c5ce75240fe2bdd3611e016` and result SHA `083840c102dfd8bfef549638b87b40b0d15da0cc3487ed118220394665ca7534`. Reproduction and actual machine-byte fixtures are in `independent-review.py` / `.json`.

The final run covers 274 residual functions and 3,567 comparator-exact controls, visits 53,783 call sites across both sides, and excludes 44 differing call sequences. Controls contain 5,198 known argument words: 1,565 controls have at least one known word and 2,002 have none. The lone final hit is the already-adjudicated network error string. These figures do not imply that every call or argument in those functions is modeled.

The initial version had three reproduced unsafe assumptions: pending argument slots survived an unmodeled ESP reassignment; a 16-bit push occupied a modeled four-byte slot; and Capstone's incomplete x87 memory-access flags allowed `push 1; fldz; fstp [esp]` to retain a false known `1`. Root fixed these cases conservatively. Independent raw-byte fixtures now verify ordinary dword pushes, unknown-state invalidation for ESP changes and partial stack operations, x87/SSE argument overwrites, non-ESP stores, and partial-register clobbers. No production compiler was used. The initial scanner/result remain separately archived as `initial-scan.py` / `initial-result.json`; they are not the validated final capability.

The initial scan discovered the actor output-throttle `1.0f`/`1.6f` difference. Manual January and supplied-PE evidence independently authenticated that real consumed argument in `../ai-debug-context/primary-length.json`. The fixed conservative scanner gives that call no arguments because intervening x87 stores clear its pending state. It also does not recover the independently known swapped color consumers: both corresponding argument lists are empty. Do not relax the model merely to force these positive controls to appear. Simple modeled immediate pushes remain positive fixtures; manual raw binding supplies the semantic proof for the actual throttle discovery.

The value-identity helper reuses the existing comparator's fail-closed alias rules only on local copies with location zero. It does not change COFF data, aliases or admission rules. Exact controls validate representation consistency in their covered cases; identical code following the same abstract interpreter is not an independent proof of every modeling assumption.

Remaining limits are material:

- A nearby `add esp,N` identifies cleanup words, not a verified ABI signature, argument count, type or branch role. Delayed cleanup, nested calls, alignment and calling conventions require manual binding.
- Equal ordered named calls do not prove corresponding paths. Unresolved/indirect markers cannot establish actual dynamic callee identity. Branch/target cuts deliberately lose facts.
- A direct named-global load denotes a load source, not its numerical contents or a memory-version proof across writes/calls. Distinct names need actual data/type/ownership evidence; matching names cannot prove equal values at different times.
- Floating computations, stack-local pointer propagation, scalar widths and aggregate arguments are outside scope. Missing hits are not evidence that source is correct or a function is exhausted.
- Matching relocation counts or this diagnostic's value identities confer no exact credit. Every proposed source change still needs raw January and independent primary support, the full strict gate, and all ownership/control guards.

No further scanner modification is requested for this bounded run. Its conservative exclusions and manually verified hit provenance are preferable to a wider unsupported coverage claim.
