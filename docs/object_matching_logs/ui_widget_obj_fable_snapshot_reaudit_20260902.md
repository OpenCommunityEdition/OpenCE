# `ui_widget.obj`: Fable snapshot re-audit and house reconciliation (2026-09-02)

## Outcome

Fable snapshot `a0f2e4615284b9de6d9f92967d1e18e4d28ac16c` was audited in
a fresh worktree based on canonical
`251fc455f51648204d0c72547957798898b43c5d`.  The raw import is preserved
separately as `8f0ba3060`.

The snapshot contains no UI Widget function progress not already present in
canonical.  Its function bodies are the same January shapes, but it rolls
back the later house-rule reconciliation by replacing owner-header includes
with cross-module declarations, duplicating `game_input_preferences`, using a
`long` where both profile owners declare `word`, and naming the main UI BSS
owner `bss_00454240` instead of `ui_widget_globals_storage`.

The final reconciliation restores canonical's stronger source exactly and
formats the existing `ui_widget.h` declarations to the current multiline
house style without changing any non-whitespace header token.

| State | Ordinary gate | Address-normalized gate | Exact padded bytes |
| --- | ---: | ---: | ---: |
| Canonical baseline | 53 exact / 3 residual / 46 unwritten | same | 3,872 |
| Raw Fable import | 27 exact / 29 residual / 46 unwritten | 53 / 3 / 46 | 1,808 ordinary; 3,872 normalized |
| Reconciled | 53 exact / 3 residual / 46 unwritten | same | 3,872 |

Net strict gain over current canonical is therefore **zero functions, zero
padded code bytes, zero meaningful code bytes, and zero data bytes**.  The
snapshot is preserved for provenance, but no already-superseded source is
credited again.

## January-address identity

The raw import's apparent loss of 26 exact functions / 2,064 padded bytes is
not an instruction-body loss.  Every one is reported by `gate.py` as a
relocation-identity residual after the source changes the external spelling
from `_ui_widget_globals_storage` back to `_bss_00454240`.  An analysis-only
gate alias,

```text
bss_00454240=ui_widget_globals_storage
```

restores the raw snapshot to 53 exact / 3 residual / 46 unwritten.  This proves
the inherited set by January address and relocation destination rather than
by a report label.  The alias is not retained in source or configuration; the
final source uses the semantic name and passes the ordinary gate directly.

The source inventory and `config/symbols.json` together fix semantic names to
January addresses.  Examples that retain their exact owners include:

- `_pool_alloc` at `0xD32C0` and `_pool_free` at `0xD32E0`;
- `_pop_widget` at `0xD3EF0` and `_dispose_widget_stack` at `0xD3F50`;
- `_should_flip_sticks_for_local_player` at `0xD44C0`;
- `_widget_instance_reload_recursive` at `0xD4680` and
  `_ui_widget_reload_by_tag` at `0xD4690`;
- `_play_sound_tag` at `0xD48E0`;
- `_filesystem_initialization_thread_proc@4` at `0xD4D10`;
- `_perform_filesystem_initialization` at `0xD6100`.

The decorated private worker name remains intact.  No implemented function or
global uses a `code_<address>` or `bss_<address>` name; unresolved inventory
placeholders remain only in the historical symbol-list comment.

## House-rule reconciliation

The raw source removed eleven natural owner headers and copied seventeen
declarations into `ui_widget.c`.  The final source restores the owners:

- `input/input_abstraction.h` owns `game_input_preferences` and its input API;
- `interface/ui_widget_event_handler_functions.h` owns the profile-reset API;
- `main/main_runtime.h`, `memory/stack_memory_pool.h`,
  `networking/network_server_manager.h`, the three saved-game headers,
  `shell/shell_xbox.h`, `sound/game_sound.h`, and `sound/sound_manager.h` own
  their respective declarations.

The profile enumeration declarations in both `player_profile.h` and
`playlist_profile.h` take `word *number_of_profiles`; the final worker uses
that real owner-defined type.  The recovered UI runtime storage remains
`ui_widget_globals_storage`, consistent with subsystem naming and the symbol
map.  Private functions retain semantic names, enum-based switch cases, shared
`real`/`word`/`pixel32` types, and existing cseries flag macros.

The final source contains no raw `tag_get` or `object_get` call.  Existing
typed subsystem access and all explicit function returns are preserved.  No
inline function, assembly, `volatile`, fake branch/call, undefined behavior,
raw offset, codegen directive, or forced-inlining construct was added.  The
January math-inline schedule is unchanged and no `point_from_line3d` source
reference was introduced.

`ui_widget.h` had pre-existing one-line declarations despite the current
house rule.  They are now formatted with `void` on its own line for
no-argument functions and one parameter per line.  A whitespace-stripped
comparison against canonical reports 4,015 / 4,015 identical characters;
there are no declaration, type, order, or ownership changes.

## Header blast evidence

`ui_widget.h` has ten direct C includers.  Because its only change is
token-preserving whitespace, the compiler input token stream is unchanged.
Each direct includer was nevertheless compiled and checked in isolation with
`gate.py`:

| Unit | Final gate |
| --- | --- |
| `source/cutscene/cinematics` | 16 exact / 1 residual / 0 unwritten |
| `source/game/game_engine` | 169 / 11 / 0 |
| `source/game/players` | 54 / 15 / 1 |
| `source/interface/attract_mode` | 5 / 0 / 0 |
| `source/interface/marketing_and_strategic_business_development` | 5 / 0 / 0 |
| `source/interface/player_ui` | 38 / 0 / 4 |
| `source/interface/ui_widget` | 53 / 3 / 46 |
| `source/interface/virtual_keyboard` | 18 / 1 / 1 |
| `source/main/main` | 73 / 2 / 20 |
| `source/render/render` | 13 / 0 / 0 |

No owner header was otherwise modified, so the declaration-ownership repair
has no additional header blast radius.

## Honest fuzzy frontier

The snapshot adds no fuzzy owner.  The reconciled object retains canonical's
three previously written residuals, covering 688 target-padded bytes:

| January address | Function | Padded bytes | Gate reason |
| ---: | --- | ---: | --- |
| `0xD4200` | `_get_icon_type` | 80 | normalized SHA differs |
| `0xD4520` | `_remap_sticks_for_local_player` | 304 | normalized SHA differs |
| `0xD7E80` | `_main_screen_shell_load` | 304 | relocation identity |

These are inherited source-plausible bodies, not new exact credit.  No new
park entry is added: the existing Fable closeout explicitly leaves them as
honest residuals and forbids anchors, dead branches, fake calls, or relocation
aliases without independent source evidence.

## Evidence reviewed

The complete prior UI Widget ledger set was checked, including the lifecycle,
child-count, controller-compatibility, public-leaf, error, color, visibility,
reallocation, and Fable closeout records.  The decisive current ledger is
`ui_widget_obj_fable_salvage_exact_wave_20260902.md`: it records the same
53 / 3 / 46 frontier, the semantic BSS rename, owner-header migration,
owner-defined `word` count, and the three residual boundaries.

The January object remains the byte and address authority.  HCEA/PDB and
Stian source records are used for semantic names, types, and control-flow
corroboration only.  No donor evidence supports reverting the canonical owner
headers or semantic global name.

## Verification

- baseline ordinary gate: 53 exact / 3 residual / 46 unwritten;
- raw ordinary gate: 27 / 29 / 46;
- raw address-normalized alias gate: 53 / 3 / 46;
- final ordinary gate: 53 / 3 / 46;
- inherited exact padded set: 3,872 bytes before and after;
- source blob after reconciliation equals pinned canonical blob
  `d404beb3cb18bf95dd1991fe02504dce367bd221`;
- fake-match scan of the final source/header pair: zero review leads;
- targeted comparator, park, fake-scan, regression, and admission tests:
  103 passed;
- header non-whitespace identity: true, 4,015 characters each;
- ten direct-includer isolated gates: pass;
- `git diff --check`: clean apart from Git's local LF/CRLF advisory.

Per lane instructions, no shared or canonical Ninja target was run, canonical
was not modified, and nothing was pushed.
