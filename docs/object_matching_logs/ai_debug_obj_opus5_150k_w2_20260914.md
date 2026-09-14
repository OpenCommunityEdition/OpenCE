# ai_debug.obj — opus5 150K house-clean lane, wave w2 (2026-09-14)

Unit `source/ai/ai_debug`. Worker scope: `source/ai/ai_debug.c` only. **Nothing landed; the real file is
unchanged** (`git diff --stat -- source/ai/ai_debug.c` empty). Worker notes: `scratch/workers/ai_debug.md`;
probes: `scratch/workers/ai_debug/w2/` (and the cut-off run's `scratch/workers/ai_debug/p?.c`).

## Baseline and final gate

`python -B tools/campaign/gate.py source/ai/ai_debug --all` (the `--forbid-emitted-symbol _point_from_line3d`
guard fails at baseline on the pre-existing out-of-line `_point_from_line3d`, as recorded by the 100K handoff):

| row | January padded / relocs | candidate | tag |
| --- | ---: | --- | --- |
| 56 functions | — | — | EXACT |
| `_ai_debug_render_actor` | 24,976 / 1,905 | 24,848 / 1,903 | excluded by the orchestrator, not touched |
| `_code_00039990` | 848 / 42 | 848 / 42, sha `77336ed2` | sha |
| `_code_0003a910` | 704 / 23 | 704 / 23, sha `1f99ed48` | sha |
| `_code_00041220` | 2,336 / 169 | 2,448 / 185, sha `46fab4ca` | size, relocs, sha |

Final real-file gate: identical (== exact 56, residual 4, unwritten 0). Owner census of the real-file candidate
equals `build/base` (candidate-only code owners vs target: the pre-existing real_math/header set
`_cosine _cross_product3d _distance_squared3d _encounter_get_squad _magnitude2d _magnitude3d
_magnitude_squared2d _magnitude_squared3d _normalize2d _normalize3d _object_get_bounding_sphere
_point_from_line3d _scale_vector2d _scale_vector3d _set_real_vector3d _sine _square_root
_vector_from_points3d`; no additional math owner). No parks in this unit. Fake scan: 0 leads.
Branch sweep: no branch or history blob above 56 exact.

## Authentic names (atlas) — measured codegen-inert

January map atlas (`scratch/atlas/ai_debug.obj.txt`, the halo-symbol-atlas file keyed by cachebeta.exe's hash),
re-anchored on `_ai_debug_dispose_from_old_map` (0x375B0) and `_ai_debug_drawstack` (0x39920), both already
strict-exact at those offsets:

| offset | current | atlas name | tier | corroboration |
| --- | --- | --- | --- | --- |
| 0x39990 | `_code_00039990` | `_ai_debug_render_path_node` | exact | called per node only from 0x3a910 |
| 0x3A910 | `_code_0003a910` | `_ai_debug_render_path_nodes` | fuzzy | the tree already declares an uncalled stub `ai_debug_render_path_nodes` with a910's exact six-parameter list (also in punpckhdq ai_debug.c:63) |
| 0x41220 | `_code_00041220` | `_ai_debug_render_encounter` | fuzzy | `ai_debug_render` calls it with `selected_squad_index` (an encounter index) |

Measured: renaming the three (and deleting the dead stub plus the duplicate prototype) gates 56/4/0 under
`--alias` with all four residual hashes identical. Renaming the other 17 atlas-named `_code_` functions as well
(`scratch/workers/ai_debug/w2/n1.c`) is also byte-identical everywhere. No function became exact, so there is
**no exact-pending-rename**; the names are offered as a names-only batch (below).

## Per-function outcomes

### `_code_00041220` (`ai_debug_render_encounter`) — NOT LANDED

The prefix is instruction-identical to January through 0x5D2; the whole residual is one inline decision:
`ai_debug_drawstack` at site 1 (the `%3.2f` evaluation render, January 0x646 `call`) is inlined in our build.
Drawstack census, January vs ours: 39e80 1 call + 3 inline, 3a2e0 1 + 3, render_actor 55 calls,
render_path_storage 2, 41120 1 — all equal; only 41220 differs (3 calls vs 2).

New observations:
- A float vararg is not the discriminator. In render_actor every site after 0x4121 is inline (including
  `"analog %.2f"`) and early `%d` sites are calls. In 3a2e0 the first site is a call and the later ones inline.
  Late sites inlining and early sites calling fits a frequency-weighted allowance consumed from the end of the
  function.
- The inline-frequency law does not transfer to this auto-inlined static. Splitting site 1 into
  `if (valid) render(..., white) else render(..., red)` (s1) inlines BOTH arms (2,480 / 191).

Shapes (this run plus the unlogged cut-off run of the same wave):

| shape | result |
| --- | --- |
| pA: `||` for the `goto default_firing_position_colors` pair | 2,432 / 185; the colour block is laid out wrong and the inline stays |
| pB: merge the site 3/4 if/else | 2,544 / 201 |
| pC: A+B | 2,544 / 201 |
| pD: positive `&&` with the squad block first, `else if` default | 2,432 / 185 |
| pF/pG: an invented `actor_get` wrapper (probe only) | inert |
| s1: if/else colour arms | 2,480 / 191 |
| q1/q2: PROBE ONLY, a redundant folded `&& pursuit` guard | inert (and inadmissible) |
| s2: the evaluation-context origin through `point_from_line3d(..., 1.f, ...)` | byte-identical, decision-identical |
| r1: whole-file definition order = January's section order | all 60 hashes identical |

Reopen criterion: January-side evidence of a different context for site 1 (authentic local or statement
records for the pursuit block) that lowers its inline allowance without moving site 2. Do not run spelling
sweeps or guards, and do not add wrapper, alias or forced-inline constructs.

### `_code_00039990` (`ai_debug_render_path_node`) — SKIPPED (exhausted)

290/290 instructions. The only change is `push edx` (the attractor colour argument of the following
`render_debug_string_at_point`), scheduled one slot before the inlined `point_from_line3d`'s `fmul [eax]` at
0x229. Ten prior forms are recorded in `ai_debug_obj.md`. No brief section 9 law addresses call-argument push
interleave across an inlined helper. Only the inert name and order passes were run (sha unchanged). Reopen on a
demonstrated push-scheduling lever.

### `_code_0003a910` (`ai_debug_render_path_nodes`) — NOT LANDED

The residual is still only the z-component fold at 0x211. Ours is `fmul [ebp-0x30]` consuming the reciprocal;
January is `fld; fmul st(1); fstp` followed by `fstp st(0)`. The frame (0x38) holds exactly one `real_point3d`.

New evidence: the tree's dead stub (and punpckhdq) list this function's stack locals as
`attractor_distance_color, closest_color, temp_point2, polygon_color, cost_color, attractor_weight_color,
temp_point, static short current_traverse_index`. The list's provenance is unknown. It is not HCEX: neither
HCEX.pdb nor HCEX_Release.pdb has ai_debug symbols.

| probe | result |
| --- | --- |
| p1a: function-scope `static short current_traverse_index;` replacing the file-scope `global_ai_debug_path_render_id` | a910 sha unchanged; `.bss` grows to 76 bytes (wrong layout) |
| p1b: `static short current_traverse_index = 0;` | a910 sha unchanged; `.bss` stays 74 bytes and the counter still resolves to January's `_actor_path_debug_array+4` |

p1b is a names-only improvement and leaves codegen unchanged. Reopen criterion: unchanged from the main ledger
(a chase-exit keep worked example or an IR lever).

## Reusable finding: VC7 function emission order (measured)

Our object's section order is reproduced exactly by this rule:
1. Emit definitions in source order.
2. Defer any function that calls a *static* defined later, or calls a deferred static.
3. At the end of the TU, sweep the deferred list in source order repeatedly, emitting each function once all
   its static callees have been emitted.

A public callee defined later does not defer its caller (`select_encounter` → `select_actor`). Inverted,
January's section order is consistent with a source order equal to its address order. Rewriting `ai_debug.c` to
that order (r1) changes no byte of any of the 60 functions. Definition order is not a lever in this TU.
January emits `set_real_point3d` after `ai_debug_render_surface`; we emit it first. That is a header-inline
placement detail with no effect on bytes.

## ORCHESTRATOR-PROPOSAL: names-only batch (no credit)

- **Kind:** config and source rename. It must land in one commit.
- **Evidence:** the atlas records above, anchored as described. The measured inert candidate is
  `scratch/workers/ai_debug/w2/n1.c`, which gates 56/4/0 with identical hashes under the 20 aliases in
  `n1.aliases`.
- **Consumers:**
  - No other source file names any of these identifiers. The only mention is a comment in
    `source/ai/ai_scenario_definitions.h:313` that refers to `code_00041220`.
  - `tools/emission_shape_oracle.py` and `tools/context_diff_oracle.py` mention `_code_00039990` and
    `_code_0003a910` in text.
  - No parked.json or semantic_matches.json entries reference these names.

Edit `config/symbols.json` in place: change only the `"name"` value on these lines.

```
604  _code_00037890 -> _ai_debug_render_path_line            (exact tier)
610  _code_00037af0 -> _ai_debug_render_lineoffire           (exact)
611  _code_00037bc0 -> _ai_debug_render_ballistic_lineoffire (exact)
614  _code_00037dd0 -> _ai_debug_lineofsight_findpoint       (fuzzy)
615  _code_00037ee0 -> _ai_debug_lineofsight_storeray        (exact)
616  _code_00037fa0 -> _ai_debug_render_lineofsight          (fuzzy)
619  _code_000383d0 -> _ai_debug_render_aiming_validity      (fuzzy)
621  _code_000386a0 -> _ai_debug_speech_update               (exact)
624  _code_00038ad0 -> _ai_debug_communication_toggle_bits   (exact)
630  _code_00038de0 -> _ai_debug_render_idle_look            (fuzzy)
632  _code_00039060 -> _ai_debug_path_storage_update         (exact)
639  _code_00039990 -> _ai_debug_render_path_node            (exact)
642  _code_00039e10 -> _ai_debug_select_this_actor           (exact)
643  _code_00039e80 -> _ai_debug_render_speech               (fuzzy)
644  _code_0003a2e0 -> _ai_debug_render_vehicles_enterable   (exact)
647  _code_0003a910 -> _ai_debug_render_path_nodes           (fuzzy)
649  _code_0003af00 -> _ai_debug_render_path                 (exact)
652  _code_00041120 -> _ai_debug_render_paths_failed         (exact)
653  _code_000411d0 -> _ai_debug_render_all_actors           (exact)
654  _code_00041220 -> _ai_debug_render_encounter            (fuzzy)
```

- **Matching source edit in `ai_debug.c`:** apply the same identifier renames, word-bounded. Replace the dead
  `ai_debug_render_path_nodes` stub (unused locals) with the renamed a910 body, and delete the now-duplicate
  a910 prototype. That is exactly `v1.c` + `n1.c`.
- **Optional:** replace the file static `global_ai_debug_path_render_id` with the function-scope
  `static short current_traverse_index = 0;` (probe p1b). It is byte- and `.bss`-identical, but its name
  provenance is weaker.
- **Left unnamed:** `_code_00038280` and `_code_00038f30`, which have no atlas record.

## Rejected and inert (do not repeat)

- **Site 1 of 41220:** if/else arms (s1), redundant folded guards (q1/q2), and the helper respelling of the
  origin block (s2) did not work.
- **Colour-block goto respellings (pA/pD):** both are worse. The goto CFG is the byte-proven layout; its
  natural spelling is still open.
- **Invented wrappers (pF/pG):** not admissible.
- **Definition order (r1)** is inert.
- **Atlas renames** are inert.
- **A function-scope counter without an initialiser (p1a)** puts the counter at the wrong `.bss` layout.
