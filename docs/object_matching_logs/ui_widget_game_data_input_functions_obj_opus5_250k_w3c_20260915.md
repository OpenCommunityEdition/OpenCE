# `ui_widget_game_data_input_functions.obj` — opus5 250K house-clean lane, wave w3c (2026-09-15)

Worktree `C:\halo-worktrees\opus5-150k-house-clean-20260914`, branch `opus/250k-house-clean-20260915`.
Worker scope: `source/interface/ui_widget_game_data_input_functions.c` only. No header, config, build or git
operation was performed. Worker totals are not credit; only the orchestrator's full build counts.

**Outcome: nothing landed — honest early exit. The real file is byte-for-byte unchanged
(`git diff --stat` empty).** The unit's only strict-reachable non-exact row is a measured block-layout tie; the
other five rows are either accepted-semantic, csplit-alias-bound (strict exact unreachable), or a locked park whose
recorded blocker no w2/w3 law explains.

## Baseline and final

`gate.py source/interface/ui_widget_game_data_input_functions --all --forbid-emitted-symbol _point_from_line3d`

| | exact | residual | unwritten | guard |
|---|---:|---:|---:|---|
| baseline (HEAD) | 40 | 6 | 0 | passed |
| final (unchanged) | 40 | 6 | 0 | passed |

Parks: `_solo_level_select_list_update_displayed_items` — `scratch/parkcheck.py`: **parks 1, drift 0**.
`tools/fake_match_scan.py`: 0 review leads. Owner census: unchanged (no edit).

| function | padded | tag | w3c verdict |
|---|---:|---|---|
| `_multiplayer_game_directions` | 320 | sha | NOT-LANDED — block-layout tie (A43 family), 1 new shape + 8 lab spellings |
| `_player_profile_1wide_list_update` | 1,152 | sha | OWNER-BLOCKED — duplicate-store ruling, and reloc-identity even then (zero strict credit) |
| `_player_profile_3wide_list_update` | 1,232 | reloc-identity | SKIPPED — accepted_ledger (semantic tier) |
| `_player_profile_update_cache_for_nwide_list` | 272 | reloc-identity | SKIPPED-EXHAUSTED — bytes already equal; csplit alias (A45) |
| `_solo_level_select_list_update_displayed_items` | 704 | sha | SKIPPED-EXHAUSTED — parked two-reload transposition; no law fires |
| `_splitscreen_pregame_status_screen_update` | 1,216 | reloc-identity | SKIPPED-EXHAUSTED — bytes already equal; csplit alias (A45) |

There are no UNWRITTEN rows in this object, so the w3 work order started at the structural/residual rows.

Prior ledgers read in full: `*_obj_opus5_150k_w1_20260914.md`, `*_obj_fable_salvage_20260902.md`,
`ui_widget_game_data_profile_fuzzy_reconciliation_20260909.md`, the park text in `config/parked.json`, plus
`scratch/w2/laws_w2.md` and `scratch/w3/laws_w3.md` (applied as detectors to every row; see below).

## `_multiplayer_game_directions` (320) — the only strict-reachable row

w1 had already driven this to 122/122 instructions with equal size and relocations using
`scratch/workers/ui_widget_game_data_input_functions/mgd_a.c` (the real body plus the teams `if/else` join, i.e. one
`widget->visible = TRUE;` after the teams arms — itself the laws_w3 **A25** join-store form, confirmed by January's
tail-duplicated per-arm stores). The single remaining hunk is block placement:

```
January                                   ours (mgd_a)
 55 cmp word [esi+0x112],di                55 cmp word [esi+0x112],di
 5c jl 0x89              <- machine arm    5c jge 0x6f          <- skip over inline arm
 5e call is_splitscreen                    5e .. machine arm (0x22) + ret
 ..                                        6f call is_splitscreen
 78 player arm (0x23) + ret                89 player arm (0x23) + ret
 89 machine arm (0x22) + ret
```

Flow graph, register allocation (widget in EBX, the shared `mov edi,2` constant), relocations and every other byte
are identical. January keeps the two test chains contiguous and emits both cold arms afterwards, player arm first.

### New work this wave

1. **f1 (A25 join-store applied to the server block)** — `scratch/workers/w3c_ui_widget_game_data_input_functions/f1.c`:
   `short string_list_index = NONE;` assigned in the two arms, then one guarded store plus `visible = TRUE`.
   Result **336 != 320 [size]**: VC7 does tail-duplicate the stores into both arms (A25 holds) but materialises the
   value in a register (`mov eax,0x22 … mov word [ebx+0x40],ax`), and the arm order does not move. Reverted.
2. **Lab census** (`scratch/workers/w3c_ui_widget_game_data_input_functions/lab1.c`, `lab2.c`, runner `lab.py`,
   compiled with this unit's ninja cflags). `h1` reproduces our real layout exactly, so the lab is faithful.
   All of these leave the machine arm inline: else-if ladder (f2, h2 control), `while` instead of the for-increment
   teams loop (f3 — laws_w3 **A23** frequency probe), the whole teams tail removed (f4), a `visible = FALSE` exit
   inside the `if (server)` block (f5), and an extra nesting level on the splitscreen test (h3).
   `h2` (the two ifs swapped in source order) swaps the **test** order in the emitted bytes, which positively proves
   January's source order is machine-test-first — i.e. our source order is right and the difference is layout only.

Together with w1's five shapes (local join store, else-if, flat single-exit chain, global visible join, condition
boolean local) that is **14 measured spellings**. Classification: block-layout tie, laws_w3 **A43** family (no source
lever has ever moved late block/epilogue placement). laws_w3 A39's switch device does not apply (no switch; both arms
already end in an explicit `return`), and A25/A23/A16/A26 were each tested above.

**Reopen criterion:** a measured law that predicts which of two cold early-return arms C2 emits first (a real
block-frequency/placement decode), or January-side evidence that the machine arm is not a plain `if`-then in source.
Do not re-spend shapes on if/else-if, condition booleans, join stores, nesting or loop respellings.

## `_player_profile_1wide_list_update` (1,152) — owner-blocked and strict-unreachable

Re-measured w1's shape C (`scratch/workers/ui_widget_game_data_input_functions/p1_final.c`) at this HEAD:
**1,152/70, normalized bytes equal, tag [reloc-identity]** (the extra residual row in that gate is only because the
file predates the landed `_network_pregame_status_screen_update` unpark).

Two independent reasons it cannot land in this wave:

1. The body depends on January's byte-proven duplicated `text[0xFF] = 0` store (default-profile valid path stores at
   +0x370 and again at +0x408). Brief section 5 prohibits duplicated stores; the w1 owner-ruling request is still open
   (laws_w3 C3 "byte-proven redundant stores/branches", the A42 family) and the 2026-09-15 rulings do not cover it.
2. **Even with that ruling the row stays [reloc-identity]**: the remaining relocation differences are the csplit
   one-past-the-end alias at +0x1a9 and the `L""` / `"\r\n"` literal COMDATs January references undefined
   (laws_w3 **A45**). So it is worth **zero strict credit**, and brief section 6 forbids landing an already-written
   residual that does not become strict EXACT.

For the orchestrator: `_player_profile_3wide_list_update`, `_player_profile_update_cache_for_nwide_list` and
`_splitscreen_pregame_status_screen_update` are all already in `build/semantic_report.json` accepted_ledger, so w1's
semantic-credit request for the latter two is satisfied; `_player_profile_1wide_list_update` is the only alias-class
row in this object that is uncredited, and it needs the duplicate-store ruling first.

## `_solo_level_select_list_update_displayed_items` (704, PARKED) — no law fires

Park class `instruction-scheduling`; target and parked base are both 704/45 with objdiff 99.89 %. Measured today:
214/214 instructions with **one** transposition — the loop back-edge block at +0x9a reloads
`mov eax,[ebp-0xc]` (item_index) then `mov edi,[ebp+8]` (list_widget) in January and the reverse in ours. The loop
bottom is `mov eax,[ebp-0xc]; inc eax; cmp eax,3; mov [ebp-0xc],eax; jl 0x9a`, so both reloads are allocator-redundant;
the frame (profile −0x54, index array −0x24, item_index −0xc, marker homes −0x10/−0x14/−0x18) and all 45 relocations
already match.

Detectors run with no hit: A6/A33 (frame layout, block scoping — the frame is already exact), A12 (preheader invariant
order — this is a back-edge reload block, not a preheader), A24 (parameter-home offer — no home is packed differently),
A5 (alias removal — there is no alias), A23/A25/A39 (layout/frequency — no block moves), A35 (x87 only).
The park's recorded cause is the shared `player_profile.h` ownership: the older exact body used a TU-local shadow
struct, which brief section 5 prohibits, and the park text forbids source-pressure tuning. No scratch candidate was
produced, because any edit here would be a pure two-load steering change.

**Reopen criterion:** a measured law predicting the order of two independent spill reloads in a loop back-edge block,
or January-side evidence that `struct player_profile` / the `player_ui_get_active_player_profile` signature differs
from the shared header in a way that changes allocation.

## Other rows

- `_player_profile_update_cache_for_nwide_list` (272) and `_splitscreen_pregame_status_screen_update` (1,216): landed
  byte-identical in w1; the residual tag is purely the csplit one-past-the-end alias (+0x40/+0x79 and +0x4a1
  respectively). laws_w3 A45 makes strict exact unreachable; no source work is possible or useful.
- `_player_profile_3wide_list_update` (1,232): accepted_ledger, semantic tier, untouched.

## C4013 note (no action)

`scratch/gate4/c4013.txt` reports one implicit declaration for this unit,
`ui_widget_game_data_input_functions.c(648) : warning C4013: 'error' undefined`. The site is inside
`_ui_widget_game_data_function_invoke`, which is **already strict EXACT**; laws_w3 A32 records that A7 includes whose
C4013 result is discarded or already exact are byte-inert, and E18 forbids landing byte-inert hygiene includes in a
unit where nothing else lands. No include was added.

## Proposals

None. No header, config or owner packet is needed for any row of this object beyond the two rulings already on the
orchestrator's list (the `_player_profile_1wide_list_update` duplicate-store ruling, which yields zero strict credit,
and the standing park criterion for `_solo_level_select_list_update_displayed_items`).
