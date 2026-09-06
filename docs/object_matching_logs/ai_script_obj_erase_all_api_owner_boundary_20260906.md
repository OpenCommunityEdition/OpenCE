# AI Script erase-all API owner boundary, 2026-09-06

## Result

The prerequisite API-owner trial was fully restored. No
`ai_scripting_erase_all` body was added or compiled, and no new code or object
completion is credited. Canonical production remains the published
`ced790c736ea9e7c50b9328269b5e96ad66053d0` payload: 917,456 meaningful exact
code bytes, 6,215 credited functions, 6,261 strict functions, and 391/833
Matching objects. The current documentation checkpoint is `f2bd028bd`.

The apparently header-independent 47-meaningful/48-padded-byte wrapper was
not actually owner-closed: its own void prototype exists in `ai_script.h`,
but its callee `ai_erase` has no declaration in any canonical header. The
callee is already implemented in `ai.c`. Adding a caller-local prototype
would violate the ownership rules.

## Authenticated proposal

January's complete wrapper disassembly reads `ai_debug.print_scripting`,
conditionally calls `hs_runtime_get_executing_thread_name` and
`error(_error_silent, "%s: ai_erase_all", name)`, then calls
`ai_erase(NONE, NONE, NONE, FALSE)`. The wrapper is an ordinary no-argument
public function, not the later HCEA evaluator's three-argument wrapper.

The existing `ai_obj_jonas_erase_leaf_20260821.md` ledger, current callee,
and three independent January callers authenticate:

```c
void ai_erase(
	long encounter_index,
	long platoon_index,
	long squad_index,
	boolean immediate);
```

Historical donor commit `c5c20b99146ab8d6c47f3c6af4e84823be72fa18` reports
an exact wrapper, but uses a caller-local declaration with misleading
parameter names and raw severity `2`. Root inspected its complete diff;
its report is not canonical admission. The prototype trial does not depend
on resolving that donor's full implementation chronology.

The sole owner proposal added this declaration to `source/ai/ai.h` and
explicitly included that owner in `ai.c` and `ai_script.c`. It changed no
function body, field layout, compiler flag, configuration, or comparator.
The actual Ninja graph contains 64 built consumers including both source
owners; all were frozen before any change.

## Rejection and verified restoration

Full Ninja succeeded, but the full stable-section sweep fell from 6,261 to
6,259 strict functions. Both regressions were in unchanged Race source:

- `_race_touch_flag`, target section 33, 336 padded bytes;
- `_race_engine_player_update`, target section 34, 160 padded bytes.

The all-section comparison classified 5,457 records unchanged, 73 as only
compiler-local label spelling changes, and the two Race records as requiring
runtime/owner review. No new `point_from_line3d` appeared. This is enough to
reject the packet under inherited-exact preservation; no claim of semantic
equivalence for the changed Race bodies is made here.

All three source/header changes were inverse-restored. Full Ninja succeeded
again, the 6,261-function exact census has zero gains or losses, and the
frozen 64-consumer comparison proves all 5,532 runtime records unchanged.
`git diff --exit-code HEAD` for the three source/header paths is clean.
No published progress was lost. No alternate declaration order, include
position, local facade, source recipe, or comparator waiver was tried.

Evidence is under `scratch/ai-script-erase-all-{before,owner,restored}-20260906.json`
and `scratch/ai-script-erase-all-{owner,restored}-runtime-20260906/summary.json`.
The capture is `scratch/actor_looking_validity_capture_20260906.py`, invoked
with its explicit `erase-all` mode. Before/restored manifest SHA-256 values:

- `d0eb168db219e678b8a492c8f40efd7dae51ff5aa2dbd760a7510b71644428bc`;
- `ea9088b338e2cd71565bc9b17658aead755e0ae6e8820db9edef3dd05369dcee`.

Reopen only with genuinely new owner/compiler context or explicit permission
to consider independently verified semantic-only inherited regressions for
a substantial net gain. Until such permission, exact preservation remains
the rule. Both Opus implementation reservations remain untouched.
