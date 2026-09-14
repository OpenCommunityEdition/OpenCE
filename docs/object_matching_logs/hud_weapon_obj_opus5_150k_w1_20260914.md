# `hud_weapon.obj` opus5 150K house-clean lane, wave w1 (2026-09-14)

## Result

`_hud_render_weapon_interface` is strict **EXACT** in the whole-TU real-file gate
(496 padded / 494 meaningful bytes, 24 relocations, normalized SHA-256 prefix
`dc32e965f3a6f7f3`). HUD weapon advances from **10 to 11 exact / 16**. No
baseline-exact row was lost. Only `source/interface/hud_weapon.c` changed.

```text
baseline: python -B tools/campaign/gate.py source/interface/hud_weapon --all --forbid-emitted-symbol _point_from_line3d
== exact 10  residual 6  unwritten 0  (of 16 listed)
final:    python -B tools/campaign/gate.py source/interface/hud_weapon --all --forbid-emitted-symbol _point_from_line3d --out scratch/workers/hud_weapon/real_L1.obj
== exact 11  residual 5  unwritten 0  (of 16 listed)
emitted-symbol guard passed
```

No header, config, symbols.json, parked.json, build or tool file was edited. No
configure, ninja, pytest or git write was run.

## Provenance and duplicate prevention

- Base `12f7375d4` (100K consolidated lane). The unit has no parks and no
  functions inherited from the 100K lane.
- Ledgers read: `hud_weapon_obj_private_graph_fuzzy_20260912`,
  `interface_obj_weapon_hud_best_fuzzy_20260912`,
  `weapons_owner_packet_source_park_20260913`, `weapons_obj_opus5_100k_20260914`,
  the three Fable crosshair ledgers, the two Jonas ledgers, the safe-subset and
  100K handoffs.
- Donor: branch `root/opus5-hud-reconcile-20260913`, commits `2a766f42f`
  ("Match hud_render_weapon_interface") and `c6723f7c8` (Codex house-rule
  reconciliation, adversarially reviewed). That branch is not an ancestor of
  this lane. `branch_sweep` reports its blob `772abc65` as the best history
  (11 exact). The donor worker notes and scratch bodies were read from
  `C:\halo-worktrees\opus5-50k-house-clean-20260913-evidence\scratch\workers\hud_weapon`.
- The donor file was re-gated on this lane's headers before reuse:
  `scratch/workers/hud_weapon/codex_c6723.c` gives exact 11.

## Landed change

The landed file is the donor file plus two codegen-neutral cleanups.

1. The donor's one-member TU-local `enum bitmap_group_type` is replaced by the
   complete five-value copy. A disclosure comment names the existing copies:
   `bitmap_extract.c` (complete), `bitmap_group.c` and `hud_draw.c`. There is no
   shared header owner.
2. A `/* fall through */` comment is added where the flash-gated crosshair
   states fall into the animated states.

Evidence for `_hud_render_weapon_interface`, from the January COFF:

- **Zero fill.** The default weapon state is filled by `mov [ebp-0x24],0; mov ecx,7; rep stosd`. That is the block-scoped aggregate `struct weapon_interface_state weapon_state = { 0 };`. The old `csmemset` call was the surplus relocation.
- **Parent unit re-fetch.** The parent's current weapon index re-calls `object_get_and_verify_type`, so the source re-reads `unit->object.parent_object_index`. The `parent` local is kept for the seat lookup.
- **Assert literal.** `??_C@_0DG@HKMKFIHC@` is 0x36 bytes, which is the no-space text `player->local_player_index==render.local_player_index`.
- **Callee convention.** January calls `crosshairs_draw` with `ecx` = player, `eax` = hud index, and the weapon index and weapon state on the stack. Only the donor's January-listing `crosshairs_draw` body produces this private convention. That body lands with **zero credit** as the caller's dependency.

Admission checks on the final real-file object:

- All 10 baseline EXACT rows are still EXACT, compared row by row.
- The `_point_from_line3d` guard passes.
- `scratch/parkcheck.py` reports no parks for the unit.
- **Owner census against `build/base`.** Only literal owners change:
  - the spaced assert literal becomes January's no-space literal;
  - `__real@3fa00000` disappears;
  - `__real@3ff0000000000000` is added and `__real@3ff4000000000000` is now defined; both are January-referenced.
  
  The `_power` and `_square_root` COMDATs already exist in base. No code, data, BSS or COMMON owner is new.
- `tools/fake_match_scan.py` finds 0 leads before and after.
- `git diff --check` passes.

## Per-function outcomes

| function | target padded / relocs | final | outcome |
| --- | --- | --- | --- |
| `_hud_render_weapon_interface` | 496 / 24 | 496 / 24 EXACT | landed |
| `_crosshairs_draw` | 2256 / 76 | 2256 / 77 | donor dependency body, zero credit; policy-blocked |
| `_render_grenade_hud` | 736 / 33 | 736 / 33 | owner-blocked plus register tie |
| `_hud_update_weapon_local_player` | 1472 / 68 | 1456 / 69 (scratch 1472 / 68) | register/spill tie; not landed |
| `_hud_update_weapon` | 624 / 34 | 608 / 35 (scratch 640 / 35) | cross-jump tie; not landed |
| `_render_weapon_hud` | 2656 / 84 | 2624 / 85 (scratch 2656–2672 / 84) | owner ruling (raw-bit sentinel) plus register tie |

### `_crosshairs_draw`

January never writes `firing_active` (`[ebp-1]`) before the type-14 path reads
it. The target writes it only at `+0x406` and `+0x48f` and reads it at `+0x495`,
so a strict-exact body needs an uninitialized read. That is undefined behaviour,
which is prohibited even with a BUG comment. Two further differences remain: the
frame is 0x318 against the body's 0x314 (one 4-byte local is missing), and the
item and frame index are homed differently.

**Reopen:** an owner ruling on the quirk, then recover the missing local.

### `_render_grenade_hud`

The target tests the result with `test al,al`, the boolean ABI. The current TU
has only the implicit declaration of `weapon_prevents_grenade_throwing` (C4013,
the only implicit declaration at /W4).

With a scratch-only prototype, 26 register-rotation blocks remain. New
measurements:

- **Inert or worse:**
  - a nested single-exit body (G1);
  - removing the `grenade` pointer local;
  - assignment in the condition;
  - hud_state fetched before hud_index (worse: 32 blocks);
  - 1–16 dummy declarations;
  - `long draw_flags`.
- **Diagnostic only:** `long grenade_type` removes the entire rotation (6 blocks left). The rotation is therefore seeded by the sign-extension of the short `grenade_type`: January coalesces it into EAX and VC7 here picks ECX. No natural short spelling reproduces January's allocation.

**Reopen:** the owner prototype plus such a spelling.

### `_hud_update_weapon_local_player` / `_hud_update_weapon`

The donor bodies were re-verified here:

- `v2.c`: uwlp at 1472 / 68, 453 / 453 instructions, 16 aligned blocks;
- `v3.c`: `hud_update_weapon` at 640 / 35, 11 blocks.

The residuals are spill-timing and register ties:

- **uwlp spill.** January spills `player` immediately after `datum_get` and reuses EAX; VC7 here defers the spill.
- **Call-tail cross-jump.** January's two `hud_update_weapon_local_player` call tails are cross-jumped. Here the weapon path caches the local player index in EDI and the default path loads EAX between pushes.

One new inner-block declaration shape (D3) is inert. Nothing was landed: a fuzzy
rewrite of an already-written residual is not admissible.

### `_render_weapon_hud`

A new structural reconstruction from the January listing brings the body to
equal size and relocation count, sha only (R1: 2656 / 84).

Evidence for the reconstruction:

- The four arrays are `= { 0 }` aggregates, initialized after `tag_get` and `get_hud_state`.
- The flag words are updated in place with direct `SET_FLAG` on array elements. `state_flags[0]` stays in DX.
- The `PIN(fast_ftol_C(...))` expression is evaluated three times.
- There are two `hud_draw_numbers` statements with cross-jumped tails. The decimal value goes through the `cseries.h` `fast_ftol` inline.
- `state_type` is re-read from the element after calls.

Remaining differences:

- **Policy (raw-bit sentinel).** January writes and compares the "no target" sentinel as raw bits: `mov eax,0xffc00000` and `cmp eax,0xffc00000`. HCEA agrees. A cast or a union element type (U1, identical code) reproduces this, but either one needs an owner ruling on representation access.
- **Register/slot tie.**
  - January keeps `definition` in EDI with `element_index` in memory in the statics loop, and the flash-loop induction pointers are in ESI/EDI order.
  - This permutes three frame slots. A `flags` copy flips the flash loop to January's order but breaks the 16-bit operations.
- **x87 sum order.** VC7 canonicalizes every flat spelling of the distance sum to z, y, x. Only a named horizontal intermediate reproduces January's y, x then z order, and that would be steering without evidence.

**Candidates:** `scratch/workers/hud_weapon/rwh_R3.txt` and `U1.txt`.

## Proposals

1. **weapons.h prototype (repeat of the donor proposal).** Add `boolean weapon_prevents_grenade_throwing(long weapon_index);` to `source/items/weapons.h`. Needed but not sufficient. `units.c` carries its own consumer prototype. Consumers: `grep -rl 'items/weapons.h' source`.
2. **Owner ruling on the raw-bit sentinel.** Rule on the raw-bit "no target" real sentinel in `render_weapon_hud` (cast or union element type). This is evidence-backed, not steering, but no sanctioned idiom exists.

## Scratch artifacts

All under `scratch/workers/hud_weapon/`:

- **Target listings:** `T_*.txt`.
- **Tools:**
  - `fdump.py`: disassembly dump;
  - `aln2.py`: jump-table tolerant aligned diff;
  - `slots.py`: frame-slot census;
  - `fnsplice.py` and `variant.py`: candidate builders;
  - `warn.py`: /W4 compile;
  - `census.py`: owner census.
- **Landed candidate:** `v1.c`.
- **Donor re-verifications:** `v2.c`, `v3.c`.
- **Grenade:** `G1.c` and the `d*.c` diagnostics.
- **render_weapon_hud:** `R1`–`R3` and `U1`, with diagnostics `C1`, `A1`, `M1`, `M2`, `N1`, `Q1`, `S1`, `Hx`, `Jx`, `Kx` and `Lx`.
- **Worker notes:** `scratch/workers/hud_weapon.md`.
