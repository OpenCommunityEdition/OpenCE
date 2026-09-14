# `hud_messaging.obj` — opus5 150K house-clean lane, wave w3 (2026-09-14)

## Baseline and result

Real file `source/interface/hud_messaging.c` at HEAD `403108e28`: **35 exact / 1 residual / 0 unwritten**.
The `_point_from_line3d` guard passes. The w1 reopens of `_scripted_hud_get_timer_ticks` and
`_scripted_hud_set_objective` are already landed (`b22cc0a42`); the real file equals w1's `o5.c`.

| function | January padded / relocs / sha16 | real file | outcome |
| --- | --- | --- | --- |
| `_hud_messaging_update` (parked) | 2928 / 137 / `26a1376422e3742f` | 2880 / 136 / `d5d32a3ce4d44e6e` | not landed; strict EXACT unreachable from C |

No source change landed. `git diff --stat -- source/interface/hud_messaging.c` is empty. The final real-file gate is
35 / 1 / 0. The guard passes, park drift is 0, the owner census is 41 = base = target, and the fake-match scan reports 0 leads.

Duplicate check:
- All 17 `hud_messaging_obj*` ledgers were read, plus the w1 worker notes (`scratch/workers/hud_unit_messaging.md`).
- `branch_sweep`: no blob is better than the current file.
- `git log --all`: nothing newer than `b22cc0a42`.

## `_hud_messaging_update`

### Re-verification of the w1 body in the current tree

The candidate is `scratch/workers/w3_hud_messaging_virtual_keyboard/hm1.c`: the current real file with only w1's
`MF2.c` body swapped in. Results:
- The whole-TU gate is 35 / 1 / 0, and only this row changes.
- The row is `2928 [reloc-identity]`: size 2928, 137 relocations, normalized SHA-256 `26a13764…` equal to January.
- `relocdiff` finds 2 differing rows, at +0x4f7 and +0x510. January spells them `_global_hud_anchor_names+2`; VC7 spells
  them `_button_mappings-18`. The source is `static char button_mappings[_icon_custom_1 - _icon_action]`, indexed by
  `element->data - _icon_action`.
- `symbols.json` resolves both spellings to the same address: `_global_hud_anchor_names` is at 3034168, +2 gives
  3034170, and `_button_mappings` at 3034188 − 18 gives 3034170. `_button_mappings` is an owner in January's
  hud_messaging `.data` section.

This is the csplit-relocation-alias class: January's address falls inside the preceding hud-object symbol, and no C
spelling can name it.

### New evidence: the existing alias park class fails only on duplicate TU-static names

`scratch/workers/w3_hud_messaging_virtual_keyboard/resolve_alias.py` runs `coff_compare.section_info_resolved` on the
target and on `hm1.obj`. It binds the three duplicated `"static": true` names to hud_messaging's own image instances:
- `_button_mappings` = 3034188 (the other instance is 3169984);
- `_render_state_text` = 800784 (the other is 869360);
- `_render_state_bitmap` = 800912 (the other is 868944).

With that binding, size, relocation count, normalized bytes and all 137 resolved destinations are equal
(**RESOLVED-EQUAL**). Without it, `image_symbol_addresses` drops the three names as ambiguous and leaves 12 unresolved
relocations. So the `csplit-relocation-alias` validator in `tools/parked_functions.py` cannot admit the body today.

### Law detectors (laws_w2 A1–A22)

The body is already normalized-identical, so no codegen law applies. A7 (missing prototype) applies to the unit, not
to this function; see P2.

### C4013 at HEAD

Line 1334 is `player_get` (players.h macro → `datum_get`) in the already-exact `hud_broadcast_team_message`.

Adding `#include "memory/data.h"` after `#include "items/item_definitions.h"` gave these results:
- `CL /Zs /W3` is clean.
- Every gate row is byte-identical to HEAD (`hm_inc.c`) and to `hm1.c` (`hm1_inc.c`).
- Park drift is 0.
- The owner census has no delta.
- The fake-match scan reports 0 leads.

It is not landed because it is a hygiene-only change with no function status change. See proposal P2.

## Proposals

- **P1 (tooling, then config).** Resolve duplicate `"static": true` symbol names per unit, binding each to the instance
  inside the unit's own image range. Apply this where `tools/parked_functions.py` builds the address map, via
  `tools/coff_compare.image_symbol_addresses`. Then re-class the park `_hud_messaging_update` to
  `csplit-relocation-alias`, with base 2928 / 137 / `26a1376422e3742f…` and the `hm1.c` body.
  This earns zero strict credit either way.
- **P2 (source hygiene, orchestrator-applied).** In `source/interface/hud_messaging.c`, add
  `#include "memory/data.h"` (owner of `void *datum_get(struct data_array *data, long index);`, data.h:59) directly after
  `#include "items/item_definitions.h"`. It is measured inert on all 36 rows.

## Reopen criterion

`_hud_messaging_update` can gain strict credit only through an owner ruling that credits resolved-address-equal csplit
aliases. Otherwise only the park reclass (P1) is available.
