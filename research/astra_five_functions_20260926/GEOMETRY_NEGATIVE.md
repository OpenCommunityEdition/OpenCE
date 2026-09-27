# Geometry safe-source investigation — 2026-09-26

## Outcome

**No admissible new strict function or code bytes. No production edit.**
Canonical base is `fdf76bd0caf859aa46e5a6ca41b33164635350d0`.
Three bounded safe-source cells were tested; the explicit UB hold was never
instantiated. The clip2D held helper/cast packet was not replayed.

All four fresh whole-TU gates (baseline plus three cells) are **27 exact / 3
residual / 0 unwritten**. The 27 inherited exact rows stay byte/relocation
identical. Only `_convex_hull3d_expand` changes. No added or removed code,
data, or COMMON definitions; all non-debug, non-hull sections remain equal.
The diagnostic compile emits zero warnings in each case under the frozen
production flags. Its complete function sections equal the gate's output.

| Cell | Source factor | Hull padded bytes / relocations | Strict result |
| --- | --- | --- | --- |
| baseline | unchanged canonical | 1808 / 67 | residual |
| H1 | meaningful long surface-index snapshots; pointer formation stays after validation | 1824 / 67 | residual |
| H2 | genuine distance helpers plus /Od closing pointer locals, all validations preserved | 1808 / 67 | residual |
| H3 | H1 + H2 | 1824 / 67 | residual |

January hull normalized SHA-256:
`bad17e2158737f40d52656451e9f2ce826a8b27d395f84032536dceab223fbb0`.

Candidate hashes:

- baseline: `3b42a1bae5ecb4b878280d95cedf580f83abd8c332c193aae825d59a7ae3c5f1`
- H1: `4971eb0475a57d30f28b54bbb2a13c66f3ff5fb1fead5f84e812af4adb8a793d`
- H2: `23e468b9de4d8f1c9d1621815c3dfc779a92ed0b4b1db30c7c82b8316e776e21`
- H3: `8291cb56db6fbef1e9164907a828768d95bf7875ff73bb7f0c79ff77a6d78ccf`

## Prediction and negative evidence

`PLAN.md` was recorded before the probes. The index snapshots are meaningful
reused values, not filler. The hypothesis was that they could let the compiler
hoist scales while expressing only validation-first pointer arithmetic. It
failed: H1 checks begin at `+0x195`, and scales/address formation follow them
(`+0x1bc` onward). January forms both surface pointers at `+0x18c..+0x194`
before its first check at `+0x196`. H3 does not repair this. H2 is the genuine
source-topology control for a coupled test, not a claimed new discovery.

The later /Od build does NOT attest the H1 local names or snapshot shape;
it reloads the surface-index fields. Exactness, had it occurred, would still
have required source review. No new source hypothesis survives this bounded
test. No integer-address escape, fake local, invented macro, cast, assembly,
compiler flag, declaration count, or unrelated dependency was used.

## Current owner boundaries

Inventory: read-only donor
`C:/halo-worktrees/claude-remaining-frontier-20260926/research/remaining_frontier_20260926/workers/RF-T/VERIFY_GROUP_13.md`,
rows 10 and 11. Its cited records were read, not treated as permission.

### `_convex_hull3d_expand`

`docs/object_matching_logs/geometry_obj_convex_hull3d_begin_expand_reconciliation_20260908.md`
explicitly rejects forming indexed pointers before validation. Lane D's
`h3e_c1` is exact only with that rejected form at three sites. The structural
owner packet section 4 re-offers it with /Od evidence; that re-offer remains
unanswered. Existing aim-grenade original-bug admission does not lift this
site's explicit rejection. The source here retains all three checks before
pointer formation. No exception has been requested or inferred.

Fresh independently extracted `/Od` evidence agrees with the known packet:

- `0x6bbe25..0x6bbe4e`: surface pointers formed and homed before bounds tests
  beginning `0x6bbe51`;
- `0x6bbfc6`: vertex-fan edge pointer before the `0x6bbfc9` assertion path;
- `0x6bc1d2`: horizon pointer formation before the `0x6bc1df` assertion;
- `0x6bc76b..0x6bc7f5`: closing vertex and first/last edge locals.

This is corroborating source-topology evidence, **not a new owner ruling**.

### `_convex_polygon2d_clip_to_plane`

The accepted realcmp ownership repair did not land the point-to-vector cast.
Structural owner packet section 3(b), “admit the geometry cast”, is still
unanswered. The newer precedence audit (`research/fifty_objects_r2_20260924/results/r2w2/PRECEDENT_AUDIT.md:370-378`)
calls this `NEW_RULING_NEEDED`: the cast now belongs in the shared
`real_math.h` helper after provider repair, every consumer instantiates that
body, and its current whole-board sweep is unmeasured. It is not an explicit
site rejection, but neither the unanswered packet nor accepted similar casts
establishes current admission. No automatic extension of the admitted
view-cast class is made here.

Fresh `/Od` helper `0x6bf170` directly pushes the plane prefix and point
pointer to `dot_product2d` at `0x6bf18c` (callee `0x42dd30`); there is no
copied local vector. Thus a new field-copy temporary would be an alternative
implementation, not the attested source. The known exact packet also needs
the recorded SET_FLAG postincrement/PIN topology and the genuine
`realcmp_epsilon` ownership repair. A new private helper or invented macro
would evade these dependencies, so none was tested.

The earlier canonical scratch audits from 2026-09-25 already independently
tested the legitimate structural-only caller and genuine-helper leads.
Their negative measurements were read and preserved rather than relabeled
new hypotheses. No new January-applicable type/API evidence emerged.

## Reproduction and artifacts

Run `python -B scratch/astra_five_functions_20260926/geometry/audit.py` from
canonical. It invokes the reviewed `tools/campaign/gate.py --source --out`,
then repeats the exact frozen compile for warnings and verifies identical
function sections. It modifies only generated artifacts in this scratch
directory plus gate.py's existing PID-isolated scratch outputs. It never
invokes ninja/configure or alters production, headers, config or git state.

`results.json` binds HEAD, production source, compiler, gate, comparator,
target and each candidate/object to hashes; it contains all 30 strict rows,
complete selected function relocation information, diagnostic commands,
definition deltas and section deltas. Per-cell `.gate.txt`, `.compile.txt`
and `.hull.diff.txt` retain the raw receipts. `h1_named_indices.c`,
`h2_od_safe.c`, `h3_combined.c` are preserved research-only full-TU candidates.

The inspected donor `scratch/orch/odbuild.py` was run read-only against
`C:/Users/isabe/Documents/Codex/2026-07-13/i-w/research/symbol-build-h1-tags-20260906/halo_cache_symbols.exe`
for all three later functions. Its SHA-256 is
`740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`.
Fresh outputs: `later_od_expand.txt`, `later_od_clip2d.txt`,
`later_od_plane2d.txt`. The executable was analyzed as data, never executed.

Reopen only on independently supported safe source/API facts or an explicit
owner ruling on the exact held site. This is a bounded negative result, not
a claim that either function is impossible to reconstruct.
