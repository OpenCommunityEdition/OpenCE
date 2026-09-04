# `ui_widget.obj`: Opus pool-owner reconciliation (2026-09-04)

## Outcome

The unfinished Opus UI-widget packet was audited from its frozen, read-only
worktree at `C:\halo-worktrees\opus-small-families-30k-20260902`, commit
`eeedd72e5ba0fc1761519f68cad241fe606320e7`, including its dirty source and
preserved `scratch/w6_ui_widget.obj`.  The audit and selective reconstruction
were performed in an isolated worktree based on canonical
`069982cce1c82e514600aa78c3c762e56d06f4c3`.

The canonical translation unit advances from **54 exact / 4 residual / 44
unwritten** to **56 exact / 4 residual / 42 unwritten**.  Its strict exact
coverage moves from 3,904 to **4,272 target-padded bytes**.

| Strict-exact addition | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_main_screen_shell_load` | 304 | 33 | `178a32293b92ffc3dcaabde0ab63208d175045b4f3e75b3c2428cc3729f068aa` |
| `_transition_to_game_in_progress` | 64 | 5 | `9f8734a7a759957744c35fa2af3ae44163e57c4d9266880b8f666194b0ffac0e` |
| **Total** | **368** |  |  |

The rename-stable sweep across all 8,245 tracked functions reports exactly
these two additions and **zero regressions**.  The project-level semantic
report increases by one accepted function and 53 meaningful bytes because
`main_screen_shell_load` was already credited as semantic-exact before its
relocation identity became strictly exact; this packet does not double-count
that earlier semantic credit.

## Why the earlier residual is now admissible

Canonical already had the natural `main_screen_shell_load` body.  Its sole
strict blocker was the relocation identity and position of the private
`main_screen_shell_first_load` flag.  The Opus packet supplied materially new
ownership evidence rather than a relocation alias: January's target `.data`
owner begins with static `___medium_widget_memory_pool` at offset `0x0000`,
followed by external `_widget_memory_pool` at `0x4034`, static
`_main_screen_shell_first_load` at `0x4038`, and external
`_dashboard_abort_error` at `0x403C`.

The reconstructed source uses the corresponding semantic private identifier
`__medium_widget_memory_pool`, a TU-private `stack_memory_pool_medium` layout,
and the authenticated maximum of 4,096 blocks.  VC7 emits all four symbols at
the January offsets and naturally closes `main_screen_shell_load`; no alias,
custom section, anchor, or address-derived owner is involved.

Independent HCEA material at
`570c83fd9c365dad6f2a3e7041705d5b84c7847c` corroborates the 16,436-byte
medium pool, its `"widget_memory_pool"` name, its 4,096-entry block table, and
the public pointer to that storage in:

- `src/data/_medium_widget_memory_pool.c`;
- `src/data/widget_memory_pool.c`;
- `src/headers/stack_memory_pool.h`.

The structure remains private to `ui_widget.c`.  No broad header was changed,
so the C2 definition-position hazard described by the campaign methodology
has no transitive header blast radius.

## Honest fuzzy caller

`transition_to_game_in_progress` is a genuine private helper.  C2 omits it
when its real caller is unwritten, so forcing an otherwise unreferenced static
function to emit would have been an inadmissible matching construct.  The
packet therefore retains the complete ordinary-C `display_error` caller.  Its
control flow, diagnostics, UI paths, typed widget fields, pause bookkeeping,
and controller-error enum cases are supported by January's instruction,
string, relocation, and field-access topology and independently corroborated
at the behavioral level by HCEA's `src/blam/interface/display_error.c`.

A natural common failure tail improves the preserved Opus draft from 912
bytes / 72 relocations to January's **896-byte padded envelope and 70
relocations**.  It remains an honest normalized-SHA residual at **80.05102%**:

| Function | Target SHA-256 | Candidate SHA-256 |
| --- | --- | --- |
| `_display_error` | `10e4892eb711e06841f935458d6d4c0162924d80dae2512da2afc2b9843b0438` | `a5f8fc9769eb0885dc202ce54bcebb0559df54b311552fe88a42138bc56c2c93` |

It is retained as source progress, but no exact credit or evidence-exhaustion
park classification is claimed.  The remaining differences include natural
switch layout and register allocation, and should be revisited only with
additional authoritative source/compiler evidence.

## Excluded donor material and data boundary

The dirty donor's `display_scenario_help` body remains a 608-byte residual and
its `get_icon_type` rewrite remains an 80-byte residual; neither is required
for the two proven gains, so neither was imported.  This avoids reopening the
previously adjudicated widget remnants without new evidence.

The candidate composite `.data` section is 16,608 bytes with 42 relocations,
while January's complete owner is 16,700 bytes with 52 relocations.  The
missing suffix is associated with the still-unwritten scenario-help table.
The proven pool prefix and symbol positions are used for source ownership,
but **no matched-data credit** is claimed for the incomplete composite owner.

## House-rule review

- Public `display_error` keeps its existing owner-header declaration; private
  `transition_to_game_in_progress` keeps its prototype and definition in this
  translation unit.
- The reconstructed storage and every implemented function use semantic names,
  cseries types, typed fields, and the existing enum constants.
- No raw `tag_get`, raw `object_get`, address-derived name, raw structure
  offset, register or volatile forcing, barrier, codegen pragma, forced inline,
  representation pun, fake dependency, dead call, or nonsensical branch was
  added.
- The January inline schedule is unchanged, and the rebuilt object contains no
  `point_from_line3d` symbol or COMDAT.
- No protected bitmap source, shared header, generated `--help` artifact, or
  scratch evidence is part of the admitted packet.

## Verification

- full `ninja`: pass;
- `tools/campaign/gate.py source/interface/ui_widget --all`: **56 exact / 4
  residual / 42 unwritten**;
- rename-stable before/after comparison: **2 gains / 368 padded bytes / 0
  regressions**;
- `tools/campaign/gate.py source/units/units --all`: **189 exact / 0 residual /
  0 unwritten**;
- semantic audit: 473 units scanned, 6,440 functions evaluated, zero unit
  errors, 6,098 accepted exact;
- parked-policy audit: 212 active, 0 stale, 0 invalid;
- object-admission audit: 0 candidates, 0 contradictions, and only the two
  unrelated pre-existing explicit rejections;
- fake-match scan of `source/interface/ui_widget.c`: 0 review leads;
- `python -m pytest -q -p no:cacheprovider`: **261 passed**;
- `git diff --check`: pass.
