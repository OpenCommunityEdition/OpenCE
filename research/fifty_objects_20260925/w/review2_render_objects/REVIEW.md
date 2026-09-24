# Adversarial review: source/render/render_objects (worker verdict OBJECT_COMPLETE_CANDIDATE, packet bounding_sphere)

Reviewer lane claude/fifty-objects-20260925, tree 931ed8dc (clean). Every number below comes from my own runs.
The full log is in LEDGER.md in this directory.

## Verdict: APPROVE, but only as one packet

render_objects is a whole-object admission candidate only when all five patches land together and csplit is re-run.

Without the patches:
- Without action_vehicle.patch, render_objects gets LNK2005 on `_object_get_bounding_sphere` in both link orders
  (I reproduced this against the production action_vehicle).
- Without objects_h.patch, render_objects' surplus copy is not identical to January's selected copy (Rule 6 fails).
- Without object_lights.patch (objects.h alone), object_lights loses a 33-byte `.rdata` literal that January owns:
  object_audit reports it MISSING.

## Checks I re-ran myself

### Packaging
- All 5 patches pass `git apply --check` against HEAD, one at a time and as a set.
- Each file in final/ equals HEAD plus exactly the patch hunks, and keeps its CRLF line endings.
- The symbols.json copy differs from HEAD by one line, 23251, edited in place.

### Split
I ran my own csplit with the HEAD config plus the edited symbols.json over all 833 objects.
- Only render_objects.obj changes.
- A control run with the unmodified HEAD config gives 833/833 identical to build/split.
- In the new split, the `.bss` symbol at offset 0 becomes `?lighting_storage@?1??object_get_cached_render_lighting@@9@9`, with storage 3.

### Compile
I used my own shadow-tree harness (rvcomp.py). It was validated first: control objects are identical to build/base,
ignoring only `.debug$S`, which holds the /Fo path.
- render_objects scores **22/22 EXACT** against the new split.
- Against the production split it scores 21/22. The one residual is `_object_get_cached_render_lighting`, whose
  relocation targets differ by name only, which is what the symbols.json rename fixes.
- `gate.py --source final/render_objects.c` with the production headers also gives 21/22, with the same residual.

### Whole-object admission
I ran unmodified copies of the tools from a mirror root. In it, build/split is my new split and build/base is
build/base with the candidate objects swapped in.
- object_audit: **PASS**, 41/41 January symbols. The 1150-byte `.bss` is identical, with the function static at offset 0.
- surplus_identity: 9 candidate-only code COMDATs, **0 not identical**. `_object_get_bounding_sphere` is IDENTICAL to
  action_vehicle's copy.
- provider_link: **PASS**, 26 symbols in both orders (candidate action_vehicle as provider).
- pdb_storage: **0 disagreements**. Production has 1: `_lighting_storage` external, but it is absent from the publics.
- Mini objdiff 3.3.1: code 5437/5437, data 4678/4678, every section at 100%. **Production already scores exactly the
  same**, so the packet moves no objdiff bytes. The only gain is the strict object admission.

### Board sweep
I recompiled all 381 source/ cl units from the candidate tree. The 211 libs/ edges are not affected.
- 66 objects differ raw. rowcmp.py compared every January function row and every owner-keyed section with
  section_infos_equal.
- **Zero row changes** anywhere, apart from render_objects `_object_get_cached_render_lighting` going from residual
  to EXACT.
- Section content changes only in:
  - the 7 users of `_object_get_bounding_sphere`, where the literal changes from c:\ to ..\;
  - physics `_physics_update_old` and collisions `_collision_move_point`, both residual before and after.
- In the other ~55 objects the only difference is internal `$L` label renumbering. Control compiles confirm the
  packet causes it: the tree0 builds are identical to base.
- objdiff 3.3.1 over those 66 units: matched code, data and functions are unchanged everywhere. Fuzzy scores drop
  slightly in two already-residual objects:
  - collisions: 99.5478 to 99.5418;
  - physics: 87.9567 to 87.9547.
  This is not an exactness loss, but it should be recorded.

### Interaction probe
I rebuilt the held collisions "broad route" on the current real_math.h: the valid_real_plane3d inline, the prototype
removed, and production collisions.c.
- It gives 20/20 both with and without the packet.
- So the literal-count shift does not block the January-like collisions closure.

### Warnings and scans
- Warnings are unchanged: 0/0 in render_objects and action_vehicle, and the pre-existing C4133 in object_lights.
- fake_match_scan: 0 leads on the 4 files.

## Evidence checks

### lighting_storage
- **HCEX.pdb** (my DIA2Dump run, saved as hcex_ogcrl.txt) lists `Static Local, Type: struct render_lighting,
  lighting_storage` inside object_get_cached_render_lighting.
- `-sym lighting_storage` finds no global of that name. In the same PDB, render_object_globals is a `File Static`.
- cachebeta publics do not list it. January relocations to it come only from `_object_get_cached_render_lighting`
  (3 DIR32), and no January or base object references it from outside the unit.
- **Strip test:** the plainer, uninitialised spelling is the one that matches.
  - The worker's `= { 0 }` function-static form (L3) moves the variable to `.bss` offset 1032, so the missing
    initialiser is supported by the January `.bss` layout.
  - The file-scope static (L1) is byte-equal too, so the scope comes from HCEX, not from the bytes.
  - HCEX is a later build. Its names and scope count as evidence, not as a layout.
- The symbols.json rename has 6 exact precedents in the same file (`?x@?1??fn@@9@9` with "static": true).

### OBJECTS_H_FILE
- The January census, which I reproduced with litcensus.py:
  - "..\objects\objects.h" is defined in action_vehicle and referenced twice by its `_object_get_bounding_sphere`.
  - render_objects has an UNDEF reference to it from `_object_get_render_bounding_sphere`.
  - "c:\halo\source\objects\objects.h" is defined only in object_lights, and no relocation references it.
- The macro only picks the string value of January's assert file literal per unit. It is the same device as the
  project's match_assert file strings. Under NON_MATCHING it is ignored, because MATCH_FILE(file) becomes __FILE__.
- It is not in any section-7 owner-gated class, and it does not change any declaration count: no rows change
  board-wide.

### action_vehicle
- The NODUP hand copy and its `#define ... _inline` rename are removed, so the genuine header inline emits the same
  bytes and relocations.
- The only raw change is COMDAT selection 1 (NODUP) becoming 2 (ANY).
- object_audit and provider_link PASS. The other 7 users pair-link PASS with it.

## Non-blocking notes for the integrator

1. The packet does not include the object status flip (config.json `NonMatching` to `Matching` for render_objects).
   That is the admission step.
2. Record the two small fuzzy drops (collisions and physics) and the `$L` renumbering in ~55 units. Both are
   non-exactness effects.
3. The objects.h comment explains the mechanism as "found it beside themselves". That is a hypothesis: January's .c
   literals use upper-case `SOURCE`, while this header literal uses lower-case `source`. The value itself is proven by
   the literal census. Optionally reword the comment to cite the census.
4. object_lights stays function- and data-exact, but it is still NOT admissible. Its own `_object_get_bounding_sphere`
   copy references the c:\ literal, which January's own object also did. That needs an owner ruling (worker's
   question), and this packet does not claim it.
5. The optional evaluate_seat patches were not reviewed as part of this verdict.
