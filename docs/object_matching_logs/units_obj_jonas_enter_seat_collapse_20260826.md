# Units enter-seat base-collapse recovery (Jonas, 2026-08-26)

## Result and bounded scope

This one-function wave starts from clean cumulative commit
`86977d76d4b775a9882c55ce4022966a6030720e` (the shared-interface fold) on
isolated branch `jonas/units-enter-seat-collapse-20260826`. It recovers the
last divergence in `unit_enter_seat` with a three-line, units.c-only source
change and no header, configuration, storage, or protected-path edit:

| January owner | Padded | Relocs | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_unit_enter_seat` | 592 | 34 | `12dc030d6e58e40e4b7e48afdbb7b37c2403fcbb46d1a15330cdcaad9648ff53` |

The direct hardened census moves from 175 exact only in prospect to fact:
**175 exact / 10 emitted nonexact / 4 absent** code owners and 128/0/1
runtime-data owners. The remaining ten emitted nonexact owners are
`_code_00198fd0`, `_code_001a0cf0`, `_unit_animation_set_state`,
`_unit_died`, `_unit_euler_aiming_update`,
`_unit_preprocess_node_orientations`,
`_unit_set_or_test_seat_and_weapon_label`, `_unit_throw_grenade_release`,
`_unit_update`, and `_unit_update_animation`; the four absent owners are
`_unit_animation_start_action`, `_unit_cause_player_melee_damage`,
`_unit_export_function_values` (policy-blocked), and `_unit_place`.
`units.obj` remains `NonMatching`.

## The residual, its recorded mechanism, and the recovered source shape

The standing adjudication (Claude `units_obj.md` and
`research/integer_coalesce_probe{,2}.md`) classified this owner as a
single-event base-collapse coalescing tie: January's
`mov ebx, [ebx + 0x44]` collapses the `unit_definition ->
object.animation_graph.index` load into its base register because the
base's register-resident live range ends at that load; our build kept the
base live and loaded into `eax`. The lab-measured rule — *base-collapse
occurs iff the load is the last use of the base value's register-resident
range* — was proven, but thirteen source spellings plus a full c2
interference decode never produced a lever, and the entry was parked
"reachable in principle, lever not found."

The lever is the function's own idiom. The lower animation block already
spells the retrieval through a named long
(`animation_graph_index = unit_definition->object.animation_graph.index;`),
twice. Naming the *first* retrieval the same way moves the loaded value
into the named long's tuple, ends the base's register residency exactly at
the load, and VC7 collapses the load precisely as January does:

```c
	long animation_graph_index;                 /* hoisted to the outer block */
	...
	unit_definition = unit_definition_get(unit->definition_index);
	animation_graph_index = unit_definition->object.animation_graph.index;
	animation_graph = animation_graph_definition_get(
		animation_graph_index);
```

with the inner block's shadowing `long animation_graph_index;` declaration
removed (one shared local; the January-proven double-reload topology in the
animation block is untouched). This is the sound_class_get /
reference_list_copy live-range family applied at the collapse site, and it
is a source-shape recovery, not a compiler control: the original code almost
certainly named the retrieval consistently.

Scratch probes measured before the production emission (full-TU scratch
compiles against the split target; no tree or object edits):

- moving the `unit_definition` assignment across the inner-block boundary:
  byte-identical to the old candidate (inert);
- naming the first retrieval through a distinct long: strict exact;
- naming it through the one shared `animation_graph_index` local (retained
  form): strict exact, zero bijection events, and a whole-probe census of
  175/10/4 with every other owner byte-identical to the baseline object.

## One-shot record

Pre-edit one-unit snapshot
`build/audit/units_enter_seat_pre_20260826.json` (pinned to `86977d76`,
SHA-256 prefix `41df940e269ac603...`) was captured from the clean rebuilt
baseline (split regenerated after the fold so the thread-lane
`config/symbols.json` rename is reflected; split `units.obj` unchanged at
`e6a24ca5...`). The three-line edit was applied by a count-asserted script;
`git diff --check` and the added-line policy scan pass (3 insertions,
3 deletions, `source/units/units.c` only). The generated object was
containment-deleted and rebuilt by exactly one ordinary
`[1/1] CL build\base\source\units\units.obj` edge with no warning.
Ordering note, recorded for honesty: the `/Zs` helper invocation ahead of
that emission failed on a shell quoting defect and was not re-run before the
build; the emitted text is byte-identical to the already probe-compiled
form, and the sole production compile itself succeeded cleanly on the first
and only attempt. No retry, tuning, or post-emission edit occurred.

The immediate check against the pre-edit manifest is fully strict — no
structural renumbering at all this time:

```text
ok: true
failures: 0
warnings: 1 (NEWLY_EXACT _unit_enter_seat)
still_exact: 174   changed_nonexact: []
```

Independent adjudication: the only runtime-evidence delta in the whole
translation unit is `_unit_enter_seat` itself, and the symbol-set delta
contains no non-`$L` name.

## Gates

- `halobetacache_build` / `libcmt_build`: no work; report, semantic report,
  and progress regenerate cleanly.
- Semantic audit: 4,133 semantic exact, 4,194 accepted exact, zero unit
  errors (baseline at the fold measured 4,193 accepted; the +1 is this
  owner).
- Object admission: zero candidates, zero revocations; only the inherited
  `shell_xbox` completion-label contradiction.
- Parked validation: three active, zero stale, zero invalid. The
  parked-catalog narrative for this owner (registered as a coalescing tie in
  the Claude research corpus, not in `config/parked.json`) is superseded by
  this close; the reachability criterion's prediction — that register ties
  fall to live-range levers — is confirmed by a fourth close.
- Complete tooling suite: 179/179.

## Commit/replay boundary

After the implementation commit, a fresh one-unit snapshot pins that commit;
the generated object is containment-deleted, rebuilt by its ordinary edge,
and re-checked (expecting 175 still-exact identities and empty deltas), with
a final no-work dry run, appended below in a ledger-only commit, followed by
the fold into `jonas/units-integration-20260824` and a cumulative replay
there. No push, amend, rebase, or history rewrite.

## Actual committed-state forced replay

(appended after the implementation commit)
