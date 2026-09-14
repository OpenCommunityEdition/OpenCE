# `effects.obj` Opus5 100K wave D park re-investigation (2026-09-14)

## Scope and provenance

- Translation unit `source/effects/effects.c`, target `build/split/source/effects/effects.obj`.
- Compiler VC7 13.00.9254 (XDK), repository flags `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` plus the project
  include list; no flag change.
- Lane `opus/100k-consolidated-20260914`, worker `waveD:effects`. Wave D is a park re-investigation wave:
  every effects residual is parked and locked, so all experiments ran on scratch copies under
  `scratch/workersD/effects/`; the production file is unchanged.
- Evidence: January target COFF (disassembly, relocations, jump tables, section order); the
  2026-09-04 reconciliation ledger `effects_obj_opus_reconciliation_20260904.md`; the donor worker log
  `C:/halo-worktrees/opus-small-families-30k-20260902/scratch/reports/effects.md` (including its UB
  `effect_allowed_by_environment`); `config/parked.json`; the later HCEA per-function sources
  (`halocea-review/src/blam/effects/*.c`, semantics only); Stian build-2276 `effects.c` (section order).
- Worker notes with per-function blocks: `scratch/workersD/effects.md`.

## Validated baseline

Lane HEAD gate: `== exact 31  residual 10  unwritten 0  (of 41 listed)`; data unchanged from the
reconciliation ledger (all `.rdata`/`.data` exact, BSS topology as recorded there).

| residual (all parked) | target / base padded | relocs | first divergence |
|---|---:|---:|---|
| `_effect_allowed_by_environment` | 128 / 112 | 10 / 10 | January ebp frame + result slot read after the default assert |
| `_effect_evaluate_function_integral` | 176 / 176 | 17 / 16 | `fraction == -1` zero return merged with end-case zero |
| `_effect_generate_part` | 992 / 976 | 27 / 28 | frame `0xb8` vs `0xb4`; malformed January csprintf |
| `_effect_generate_particles` | 1888 / 1616 | 46 / 46 | frame `0xe4` vs `0xcc`; three missing helper inlines |
| `_effect_generate_parts` | 592 / 592 | 15 / 15 | register allocation |
| `_effect_get_node_matrix` | 80 / 80 | 3 / 3 | NONE arm `or eax,-1` vs `push -1`, arm order |
| `_effect_part_allowed_by_disposition` | 32 / 32 | 0 / 0 | CL/AX + compute-and-copy vs AL/CX |
| `_effect_scale` | 48 / 48 | 0 / 0 | missing `fstp st(0); fld [ebp+8]` |
| `_effect_update` | 1328 / 1312 | 48 / 48 | word-load zero extension, branch polarity, private ABI of callee |
| `_effects_object_is_corpse` | 80 / 64 | 2 / 2 | `xor bl,bl` result local and three exits |

## Results (scratch; park-reopen proposals, no production credit)

Candidate files (each gated as a complete TU with `--forbid-emitted-symbol _point_from_line3d`):

| candidate | functions changed | gate | park drift |
|---|---|---|---|
| `scratch/workersD/effects/candA.c` | corpse, scale | exact 33 / residual 8 | none (only the two reopened rows) |
| `scratch/workersD/effects/candB.c` | + disposition (generate_parts follows) | exact 35 / residual 6 | `_effect_generate_particles` base sha changes (still 1616/46) |
| `scratch/workersD/effects/candC.c` | + update, generate_particles loop correction | exact 36 / residual 5 | `_effect_generate_particles` becomes 2048/55 (non-exact) |

All 31 baseline-exact functions remain exact in every candidate; owner census (functions, data, BSS,
COMMON) is identical to `build/base`; the fake-match scan reports 0 leads; the point guard passes.

### Newly strict-exact in scratch (park-reopen)

1. **`_effects_object_is_corpse`** (80/2). January's `xor bl,bl` before the biped accessor proves a
   callee-saved boolean declared and initialised before `unit`; its `mov al,bl` exit is the single
   bottom return; the inner `mov al,1`/`xor al,al` pair without setcc is a boolean `&&` expression value.
   Source: `boolean is_corpse = FALSE; struct biped_datum *unit = biped_try_and_get(object_index); if
   (unit && dead) { is_corpse = unit->unit.time_of_death != NONE && time_of_death + CORPSE_SETTLING_TIME
   < game_time_get(); } return is_corpse;` The donor's four forms all declared the flag after the call or
   used if/returns.
2. **`_effect_scale`** (48/0). `real scaled_value = value;` followed by compound `scaled_value *=
   effect->scale_a/scale_b` reproduces January's pop/reload pair and keeps `effect_real_random_range`
   exact. The donor's fork used `scaled_value = value * effect->scale_a`, which is the spelling that broke
   the caller; the fork is therefore not a genuine same-source conflict.
3. **`_effect_part_allowed_by_disposition`** (32/0) and **`_effect_generate_parts`** (592/15).
   `if (nonviolent) return disposition != _effect_disposition_violent; return disposition !=
   _effect_disposition_nonviolent;` gives January's CL/AX private convention and compute-into-ECX/EDX then
   `mov al` per return arm. The static's register convention is set by its body, and fixing it closes the
   unchanged `effect_generate_parts` caller.
4. **`_effect_update`** (1328/48). Requires two local polarity fixes (`if (TEST_FLAG(flags,
   _effect_loop_bit)) store; else { effect_delete; return; }` in the invisible block and the end-of-events
   block, proven by January's fall-through order and restored `xor eax,eax` word loads) **and** the
   `effect_generate_particles` loop correction below, which gives the callee January's EDI private ABI.

### Reconstruction defect found (not an original bug)

January `effect_generate_particles` emits `count_delta` particles **per location instance**:
`dec dword [ebp-8]; jne 0x1a0` inner loop, counter reloaded at `0x190` from a hoisted `movzx ecx,bx`
copy after each `effect_location_get_next_instance`. Our source followed the later HCEA lift, a single
budget that advanced the instance only when exhausted. This changes behaviour. With the corrected
`while (instance) { for (i = 0; i < count_delta; i++) {...} instance = next(...); }` nest, the particle
block sits at loop depth 3 and VC7 inlines `effect_real_random_range` at all three sites, as January does
(inline frequency law). The park text "natural VC7 keeps the helper calls" is refuted. Two further
evidence-backed shapes bring the body to January's size once the environment helper stays out of line:
a separate `long last_particle_count` statement (January evaluates the previous-fraction integral first;
textual operand swaps do not move VC7's order) and the file's own `short node_index =
instance->node_designator; if (node_index != NONE) node_index &= ...` idiom in the attached arm (January
stores the designator register itself in the NONE arm).

## Experiment matrix (bounded)

| function | shape | result |
|---|---|---|
| corpse | flag declared after accessor + `&&` value | inner branches match, 64 B |
| corpse | flag declared before accessor + `&&` value | **EXACT** |
| disposition | boolean local if/else | ABI CL/AX, generate_parts EXACT, helper lacks xor/copy |
| disposition | boolean local = ?: | same as above |
| disposition | two returns | **EXACT** (+ generate_parts) |
| scale | `scaled_value = value * scale_a` (donor) | helper EXACT, real_random_range regresses |
| scale | `scaled_value *=` compound | **EXACT**, caller exact |
| generate_particles | per-instance loop | 2048/55 (helpers inlined, env + assert inlined) |
| generate_particles | + UB env **probe only** | 1952/46 |
| generate_particles | inner index long / word; do-while copy; if-block instead of continue | frame stays 0xe8 |
| generate_particles | `(short)(long)` casts / negated textual swap | no evaluation-order change |
| generate_particles | `long last_particle_count` statement | January order and `xor ebx,ebx` reproduced |
| generate_particles | + `node_index` idiom (with UB probe) | 1888/46, frame 0xe4, sha only |
| update | + end block polarity | 1 diff |
| update | + invisible block polarity | **EXACT** |
| integral | `?:` / `<` returns / single-exit / if-else returns | 192/18, 176/16, 224, 176/16 |
| node_matrix | nested local, `&&`+?:, cast ?:, inverted ?:, pre-mask, NONE-init, single-exit | 64 or 80 with ECX movsx |
| env (safe) | FALSE init; per-case returns | still inlined into the corrected loop; returns also regress generate_parts |
| (all) | definition order rearranged to January section order | zero codegen change |

## Do-not-repeat

- Rearranging definitions to January's emission order: no effect (VC7 defers callers behind statics).
- Textual reordering of the `count_delta` subtraction operands, `(short)` operand casts, long/word inner
  indices, do-while counter copy, if-block guard.
- effect_scale `value * scale_a` assignment form (breaks effect_real_random_range).
- node_matrix: `&&` + ?: families and nested-local forms listed above.
- integral: the four spellings above.
- The UB `effect_allowed_by_environment` and malformed csprintf remain prohibited, including as a means to
  stop inlining.

## Residual classification

- `_effect_generate_particles`: source/topology corrected; exactness is blocked by the environment
  helper inline decision (policy: only the UB body is out of line), plus register/scheduling residue
  measured only under the UB probe (loop-head count/instance register, inlined node-matrix load form,
  translational-velocity argument schedule).
- `_effect_allowed_by_environment`, `_effect_generate_part`: owner-policy blocked (UB read / malformed call).
- `_effect_evaluate_function_integral`: return-block merge selection (unresolved spelling).
- `_effect_get_node_matrix`: ?: select register form (unresolved spelling).

## Reopen criteria

- generate_particles: an owner decision that a read reachable only if `system_exit` returns is not UB
  (`system_exit` calls `halt_and_catch_fire`), or a safe environment-helper spelling VC7 declines to inline.
- integral: a natural spelling separating the two zero-return blocks.
- node_matrix: evidence for the select spelling that computes in EAX without `movsx`.
- generate_part: unchanged from the reconciliation ledger.

## Disposition

No production change in this wave (`git diff --stat -- source/effects/effects.c` empty); final real-file
gate `== exact 31 residual 10 unwritten 0`, park drift pass. Five parks are proposed for reopening
(corpse, scale, disposition, generate_parts, update) with generate_particles' park base re-measured
under candidate C; credit only after the orchestrator lands them.
