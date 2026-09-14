# `render_particles.obj` — Opus5 150K house-clean lane, wave w3 (2026-09-14)

Nothing landed, and `source/render/render_particles.c` is unchanged. The real-file gate reads `exact 2 residual 1 unwritten 0` before
and after. The guard passes, park `_render_particles` has drift 0, and the fake scan reports 0 leads.

| | padded / relocs / sha16 |
| --- | --- |
| January `_render_particles` | 1168 / 39 / `97e541b16e87e1e0` |
| retained park body | 1168 / 39 / `8a7510241d251d09` (391/391 instructions) |

## New evidence

- **HCEX.pdb SHIP locals** for `render_particles`: `rendered_particles[0x400]`, `sprite_groups short[0x200]`,
  `current_sprite_group short*`, `data` (build_sprite_data), `world_position`, `world_direction`. The retained source has exactly this
  set (names differ). That confirms the park's one-pointer topology and rules out a missing or extra local; the A6 frame recipes do
  not apply, and the 0x24E8 frame already matches.
- **Slot census** (`slots.py`): January cell `-4` holds rendered_particle_count and fade (11 refs), and cell `-8` holds the byte
  `previous_attached_to_first_person_weapon` at `[ebp-5]` (3 refs). Ours swaps the two cells. Emitted reference counts are identical
  object for object, and January's own deeper cells are not monotone in emitted refs. The allocator's ranking input therefore differs
  without leaving any trace in the instruction stream (frame-density law).

## Law detectors and shapes

- A5 (hand strength-reduction removal) detects `if (group_count>0){ long groups_remaining=(word)group_count; do ... while(--groups_remaining); }`.
  Shape rp1 (a natural `short group_index` for loop) still has 391 instructions, but the `lea [ebp-0x4e8]` moves before the `jle`, so
  the stream moves away from January; the census is unchanged. Reverted (1 shape).
- A1 does not apply (no TU `.bss`); A3/A10/A20 have no site.

## Reopen criterion

Unchanged: January local records, or a lifetime topology that swaps the boolean and count cells while preserving the 391-instruction
stream. Do not repeat rp1 or the earlier do-not-repeat families.
