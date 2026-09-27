# Independent H2 fuzzy-source review

2026-09-26. **ALLOW as a zero-credit canonical fidelity improvement**, subject
to the normal fresh canonical build, stable comparison, parks, admission and
test gates. No source-move hold is waived; no exact or object-completion credit
is authorized. This reviewer made no production edits, builds or Git mutations.

## Actual source and evidence

Read the consolidated `HALO_HOUSE_RULES_20260926.md` in full. Compared the complete
H2 translation unit against canonical `source/interface/ui_widget.c`, not just
the worker's patch. The only changes are:

- `widget_instance_get_child_index_from_parent`: remove the cached parent;
  initialize child from the parent before initializing index; use an ordinary
  while traversal with child advancement before increment. Existing signature,
  result initialization, equality test, break and return remain.
- `ui_widget_launch_widget`: remove parent/parent-tag/child-index temporaries;
  retain the root call and pass the genuine parent-tag conditional and child
  helper call as load arguments. Controller selection, declarations/types that
  remain, diagnostics, casts and return are unchanged. No definition is moved.

The topmost-parent and child-index helpers only traverse/read the existing
non-volatile widget links. Changing argument evaluation order introduces no
new write or competing side effect. There is no new helper, ABI/header change,
forced inline, ownership exception, dummy local, fixed condition, duplicate
branch, raw access cast, synthetic lifetime operation or compiler control.
The existing explicit short conversion is retained, not invented.

Independently read the supplied later binary as data, SHA-256
`740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`:

- Bounded `0x668100..0x66816e` shows the direct parent test at `0x668125`, child
  initialization at `0x668131..0x668134`, index initialization at `0x668137`, and
  child advance before increment at `0x668154..0x668163`.
- Bounded `0x665b10..0x665d05` shows topmost-parent call at `0x665c76`, conditional
  parent-tag value at `0x665c81..0x665c9e`, child-index call at `0x665cb5` followed
  by direct result push at `0x665cbd`, and widget load at `0x665cd5`.

These are independently meaningful coupled source reconstructions, not a
declaration-count sweep. H1 alone changes no non-debug section; the caller-only
H0 is 320 bytes, whereas H2 is 304. The later build is corroboration, not unique
January source proof. The readout's blanket claims about what /Od can never do
are not adopted; temporary stack storage does not uniquely identify C locals.

Rules 23/24 permit evidenced natural/coupled repairs and rule 65 permits useful
compliant fuzzy source. H2 invokes no special admission condition requiring an
exact caller. The known definition-position move remains outside this packet;
its recorded non-admission (`ui_widget_obj_opus5_150k_w1_20260914.md`, section 4)
and the current explicit hold are unaffected. Rules 51/55 still require zero
strict credit for H2.

## Independent saved-object checks

`review.py` recomputes these checks without writing objects or invoking a build:

- Fresh worker baseline and current build/base have identical non-debug sections.
- 372 section slots before/after; all section names and flags unchanged. Only
  non-debug section 130, `_ui_widget_launch_widget`, changes. No added/removed
  named code, data or COMMON owners. Thirteen compiler-local `$L...` labels
  renumber, but their section/type/storage/offset multisets are unchanged.
- January strict function comparison is **98 exact / 4 residual** both ways;
  **zero exact losses**. Child-index helper remains exact. No
  `_point_from_line3d` code owner is present.
- Launch retains 304 padded bytes and 20 relocations, but target normalized SHA
  is `12b6096bb36f7bb1203223061231bde54268f7969f33e38aafb91e2289ac0b25`
  versus H2 `69d1c94357bb154afe55ee65e2c50418d29aed90b9cb5f9e916da462e7531a6e`.
  The tag_get relocation is at `+0x0f` in January versus `+0x12` in H2. The
  normalized section suffix starting at `+0x19` and the other 19 relocations
  match. This is a genuine remaining strict-gate failure, not rounded exactness.

Bound inputs rehashed independently:

- Canonical source: `e4d9ece9d3b6f71d6dbd3224304a5701241010e6e03f7f71ab14d6a29d2a713a`
- H2 source: `f97398d15f34ffafe456501eae42172d65f599f8dc38cca974f19de2b4a061d2`
- Baseline object: `e024f660e47cd527b75dc8fff5d3c7e0103c6eb5d4651dabacd59264798b9d06`
- H2 object: `560fa8468c9a21f19dbc96abe9156dfcacb8df83364e4f1c0f0dd0a505ab04ae`
- January object: `9a3e1518ab0ae613a4e74cd73e515a8fc41840a73ab28272cb460265cf97c75a`

Limits: this read-only reviewer did not independently rerun compilation or
warning collection; those worker claims are not substituted for the canonical
integration gates. Preserve H0/H1 negatives and the precise remaining entry
schedule/reopen criterion. Do not retire a park or modify ownership/Matching
status merely because this fuzzy source is retained.
