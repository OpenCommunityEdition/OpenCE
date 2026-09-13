# `particle_systems.obj` Opus5 house-clean lane (2026-09-13)

## 1. Scope and provenance

- Translation unit `source/effects/particle_systems.c`; target
  `build/split/source/effects/particle_systems.obj`
  (SHA-256 `a3a9effbae9943cf78d3dd16ca61cd6ceb1c6f57ebb9610e7bca6083c361b185`).
- Lane worktree `C:\halo-worktrees\opus5-50k-house-clean-20260913`, branch
  `opus/50k-house-clean-20260913`, pinned base `73e97eca5`.
- Compiler: VC7 13.00.9254 `CL.Exe /nologo /c /O2 /Oy- /DDEBUG /Dxbox` (repository
  flags, unchanged). Only this `.c` was edited; no header, config, symbols.json or
  tool change.
- Evidence: January COFF bytes/relocations/.rdata (authority), HCEA
  reconstruction under `research-cache/halocea-full-blobs-20260830/src/blam/effects`
  (semantic corroboration only), all eight prior `particle_systems_*` ledgers.

## 2. Validated baseline

- Fresh pinned gate: `== exact 16  residual 9  unwritten 0  (of 25 listed)`,
  2,048 / 7,680 padded code bytes exact, `_point_from_line3d` guard passed.
- Fake-match scan: 0 review leads. No `config/parked.json` entry for this unit.
- Data: January owns a 32-byte `.rdata` aggregate (`_rdata_0025a6b8`, with public
  `_ground_error` at +0x18) plus string/literal chunks; no data credit claimed.

| residual at baseline | target padded/meaningful/relocs/sha16 | baseline candidate |
|---|---|---|
| `_particle_system_next_particle_state_index` | 144/143/0/`02ca0e6ac1fbf6c2` | 144/0/`76932a2d00699553` |
| `_particle_system_next_type_state_index` | 176/171/0/`c62b4e3b13dc131d` | 176/0/`a7a6d4a01e64f34a` |
| `_randomize_particle_variables` | 256/243/11/`a02cdf74e4fe600a` | 256/11/`d7bdd0a5c8ee30ba` |
| `_particle_system_new_particle_jet` | 368/365/11/`322ad2bf7cdffa79` | 368/11/`ad590a196990edc3` |
| `_particle_system_update_particle_default` | 400/392/12/`f8da0780e495f488` | 400/12/`e3611bbaac37349f` |
| `_particle_system_new_attached` | 416/402/18/`c81dea561857e1b3` | 416/18/`a45c4120b49e9780` |
| `_particle_system_new_particles` | 848/843/35/`42b053f34b78452c` | 848/35/`c29055f1631f8ebe` |
| `_particle_system_render` | 1360/1346/33/`c4d841f9803c7f3e` | 1344/35/`eb89022c439f93d2` |
| `_particle_system_update` | 1664/1651/58/`0a74337d69d2dfdf` | 1616/58/`5147db558c903e3c` |

## 3. Accepted controls (landed, strict EXACT)

Final real-file gate: `== exact 20  residual 5  unwritten 0  (of 25 listed)`,
3,040 / 7,680 padded bytes exact, guard passed, no baseline-exact row lost.
Newly exact: 4 functions, 959 meaningful / 992 padded bytes.

1. **`particle_system_next_particle_state_index` / `particle_system_next_type_state_index`**
   - `short step = states_moving_forward ? 1 : -1;` is initialized before
     `short state_index = ...->state_index;` — January computes `setne dl` before
     `push esi; xor esi,esi; mov si,[eax+..]` in both helpers.
   - The bounce clamp is `PIN(state_index - step, 0, state_count - 1)` (cseries.h).
     January's `jns -> xor ecx,ecx` fall-through is exactly PIN's
     `n<floor ? floor : CEILING(n,ceiling)`; the old hand-written if/else nest had
     inverted test polarity (`js`).
2. **`randomize_particle_variables`**: the byte-shaping `real range[3]` scratch
   array was replaced with the owning real_math.h inline
   `real_local_random_range(lower, upper)` per variable and a named
   `color_fraction` lerp factor. January stores each lower/upper pair into two frame
   temporaries right before the seed/range calls (inline parameter homes); the same
   TU already uses this inline in `particle_system_initialize`. HCEA corroborates
   the order and the shared color fraction. Only stack-slot choices differed before.
3. **`particle_system_new_attached`**
   - single exit: failed initialization deletes the datum and sets
     `system_index = NONE`; one `return system_index;`. January saves ESI before
     `datum_new`, keeps the index in ESI across initialize/delete and returns via
     `mov eax,esi` / `or eax,-1` (single-exit law).
   - the per-tick object velocity is scaled to per-second component-wise
     (`system->velocity.i *= TICKS_PER_SECOND;` ...). January addresses `.i` through
     the `lea ebx,[esi+0x2c]` shared with the `object_get_velocities` argument and
     `.j/.k` as `[esi+0x30]/[esi+0x34]`; the `scale_vector3d` inline addresses all
     three through one pointer register. HCEA also open-codes this scale.
   - Each half alone stayed `[sha]`; together strict EXACT.

Checks for every landing: whole-TU `--source` gate, full `--all` row comparison
(no EXACT row lost), point guard, owner census, fake scan 0. Owner census versus the
gate-compiled baseline: identical except the candidate-only select-any
`_scale_vector3d` copy (absent from January) is no longer emitted — a reduction, no
owner added.

## 4. Experiment matrix (scratch only unless marked landed)

| function | shape | result | disposition |
|---|---|---|---|
| next_particle_state_index | PIN only | [sha] | reverted |
| next_particle_state_index | PIN + `step + state_index` | [sha] | reverted |
| next_particle_state_index | step-before-state decl + PIN | EXACT | **landed** |
| next_type_state_index | same shape | EXACT | **landed** |
| randomize_particle_variables | `real_local_random_range` + `color_fraction` | EXACT | **landed** |
| update_particle_default | `A*t + (1-t)*B` | inert (identical diff) | reverted |
| update_particle_default | `B*(1-t) + A*t` | inert | reverted |
| new_particle_jet | `real const ground_error = 0.05f;` + `static real const seconds_per_tick = 1.0f/TICKS_PER_SECOND;` after the dispatch tables, `seconds_per_tick*velocity` | .rdata becomes byte-identical to January's 32-byte aggregate; `_ground_error+4` relocation normalizes equal; residual = inlined cross_product3d operand-role swaps; [sha] | reverted |
| new_attached | component scale only | [sha] | reverted |
| new_attached | single exit only | [sha] | reverted |
| new_attached | both | EXACT | **landed** |
| render | weights replace t/inverse_t | 1344, relocs 33=33 | scratch (v8) |
| render | + conditional SET_FLAG for sideways rotation | 1360=1360, 33=33, frame 0x118 exact, [sha] | scratch (v9) |
| render | + January shader-compatibility polarity | branch polarity matches; residual ebx/edi swap and sequence_index load order | scratch (`scratch/workers/particle_systems/v10.c`), not landed |

## 5. Do-not-repeat

- Operand-text reorderings of the update_particle_default radius lerp (VC7
  canonicalizes; also exhausted by the Fable w_ps_edits experiments).
- Commutative operand spellings inside `cross_product3d` for jet (memory:
  breakable_surfaces resisted ~15 shapes).
- Do not import HCEA's `primary_weight = remaining` assignment for render; January
  bytes put `t` in the first weight slot.

## 6. Residual classification

- `new_particle_jet`: x87 commutative operand-role tie in two inlined
  `cross_product3d` copies (measured); data owner fact (measured): January's
  1/30 constant is a file-local .rdata object at `_ground_error+4`, not the global
  `__real@3d088889` literal. The name of that static is not recoverable (inference:
  a `seconds_per_tick`-style constant).
- `update_particle_default`: x87 scheduling (2 bytes; `fxch` lerp schedule).
- `render`: register allocation after three measured structural corrections.
  **Measured behavior defect in the retained body**: its shader-compatibility test
  resets the weights when shaders are incompatible; January resets them only when
  both shaders exist and blend function, primary-map flags and sequence index are
  all equal (every mismatch jumps past the reset). This is a reconstruction
  mistake, not an original bug. Not landed because this lane forbids fuzzy rewrites
  of written residuals.
- `new_particles`, `update`: ownership/relocation naming — January references
  `_rdata_0025a6b8+8` / `+0` / `+0x14`; our statics carry real names, so
  `relocation_infos_equal` fails regardless of bytes (verified in-process).
  `update` additionally has two assertion-expression strings that differ from
  January (0x81 / 0x92-byte January strings) and a 48-byte size gap.

## 7. Reopen criteria

- jet: a proven lever for inlined `cross_product3d` operand-role choice; then land
  the ground_error/seconds_per_tick data shape together with it (needs owner
  acceptance of the static's name).
- update_particle_default: a tree-grouping x87 lever for `A*t + B*(1-t)` lerps.
- render: a register-allocation lever for `&definition->types` (January EBX) vs
  `type_definition` (January EDI) plus the 16-bit sequence_index load after the
  rotational compare; land v10's corrections with it, or owner approval to land the
  semantic polarity fix as fuzzy.
- new_particles / update: owner symbols.json naming of the private dispatch
  aggregate(s) at `_rdata_0025a6b8`.

## 8. Disposition

Still active, not Matching: 20 / 25 functions strict exact. Four newly exact
(959 meaningful / 992 padded bytes). No parked sibling exists for this unit; no
data credit is claimed. Final object `scratch/workers/particle_systems/final.obj`
(SHA-256 `2ee2796eac35130a289a508d404356babf5332298f1a80be633afa8b9da10be7`);
final source SHA-256
`4ad25437e1c5e9d417c4776db7c9d57e10f28c99fb5f8d0fbb4dad75140a40ea`.
Per-function worker log: `scratch/workers/particle_systems.md`.
