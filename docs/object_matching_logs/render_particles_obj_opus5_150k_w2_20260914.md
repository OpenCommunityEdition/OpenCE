# `render_particles.obj`: Opus5 150K lane, wave w2 (2026-09-14)

## Result

Nothing landed, and `source/render/render_particles.c` is unchanged. The unit gate is still exact 2 / residual 1 / unwritten 0. The guard passes, the park has not drifted, and the fake-match scan reports 0 leads.

| | padded / relocs / sha16 |
| --- | --- |
| January `_render_particles` | 1168 / 39 / `97e541b16e87e1e0` |
| retained (park base) | 1168 / 39 / `8a7510241d251d09` |

## Re-investigation

- **Duplicate check.** All four ledgers for this object were read, together with the do-not-repeat list in the dead_camera/render_particles freshness audit. That list covers declaration-order, lexical-scope, flags, cursor/index, clean independent-array and pointer-reuse variants.
- **Static-global lever does not apply.** The lever that closed both decals parks this wave needs a non-public TU global. This object defines no `.bss`. Every data reference is either a PDB public (`_render`, `_particle_data`, `_render_particles_enabled`) or the owned profile section.
- **Alignment diff.** The target and the retained build both have 391 instructions. Every difference is a stack-slot permutation, apart from one eax/edx swap at +0x1B4. The frame size, instructions and relocations all match.
- **Slot census.** January colours three values into `[ebp-14]`:
  - the widened local_player_index (+0x7F);
  - the uninitialised first-pass `current_sprite_group` load (+0x140);
  - `groups_remaining`.

  It colours the second-pass `current_sprite_group` together with `previous_definition_index` into `[ebp-10]`.

  VC7 instead colours the uninitialised load together with the second-pass pointer. This is interference-graph colouring of an uninitialised value. It is not a source-shape fact.
- **Probes.** No new shapes were compiled. The only shape family that separates the two pointer uses (two pointer variables) is already rejected by the park's anti-fake ruling.

## Classification and reopen criterion

The residual remains a register-allocation (colouring) tie.

Reopen only with January local records, or with a lifetime topology that gives the uninitialised first-pass `current_sprite_group` its own slot while keeping the HCEA one-pointer record.
