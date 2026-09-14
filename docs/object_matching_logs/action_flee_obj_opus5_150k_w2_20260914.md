# `action_flee.obj` opus5 150K wave 2: park re-investigation (2026-09-14)

## Scope and provenance

- Translation unit `source/ai/action_flee.c`; target `build/split/source/ai/action_flee.obj`
  (January 2002 Xbox debug build, XDK 3911 / VC7 13.00.9254, repository flags unchanged).
- Lane worktree `C:\halo-worktrees\opus5-150k-house-clean-20260914`, wave w2. Worker notes:
  `scratch/workers/actions_action_flee_etc.md`; scratch candidates in `scratch/workers/actions_action_flee_etc/`.
- Ledgers read first: `action_flee_obj_jonas_two_leaf_20260821.md`,
  `action_flee_obj_update_semantic_owner_boundary_20260905.md`,
  `action_flee_obj_threshold_leaf_reconciliation_20260908.md`,
  `action_flee_obj_position_helpers_exact_20260909.md`, the three `config/parked.json` entries.
- `branch_sweep.py source/ai/action_flee`: 10 blobs, none better than the current file.

## Baseline (real file, unchanged by this wave)

`== exact 10  residual 3  unwritten 0`, `_point_from_line3d` guard passes. All three residuals are parked:

| Function | Target | Base | Relocs | Gate tag |
| --- | ---: | ---: | ---: | --- |
| `_action_flee_setup` | 272 | 256 | 12 | size, sha |
| `_action_flee_current_position_exposed` | 352 | 352 | 13 | sha |
| `_action_flee_perform` | 864 | 848 | 30 | size, sha |

## Result

Production file **unchanged** (every target is parked). A scratch candidate reopens all three parks:

| Candidate | Gate | Notes |
| --- | --- | --- |
| `scratch/workers/actions_action_flee_etc/flee_c1.c` | 12 exact / 1 residual / 0 unwritten | setup and exposed strict EXACT; perform 864/30 sha-only |
| `scratch/workers/actions_action_flee_etc/flee_c1diag.c` | **13 / 0 / 0** | c1 plus a diagnostic local `unit_is_speaking` prototype (NOT landable; proves the header proposal) |

Diff: `scratch/workers/actions_action_flee_etc/flee_park_reopen_c1.diff`. Every baseline EXACT row stays EXACT,
the guard passes, the owner census against `build/base` shows no new or dropped owners (the three `real_math`
select-any fallbacks are pre-existing), and `tools/fake_match_scan.py` reports 0 leads (HEAD also 0).

## Evidence per function

### Shared: `action_flee_find_flee_position` has a third parameter

HCEX.pdb (SHIP), `DIA2Dump -sym`, gives
`static void action_flee_find_flee_position(long actor_index, unsigned char respect_direction, struct flee_state_data *flee_state_data)`.
January's call sites prove the stack argument:

- setup +0xe1: `push 0; call; add esp,4`;
- perform +0x1a6: `mov ebx,[ebp+8]; push 1; call; add esp,4`.

The callee never reads it, so its body stays EXACT (640/23) with the unused parameter. The candidate declares
`boolean respect_direction` with a one-line disclosure comment. Setup passes `FALSE` and perform passes `TRUE`.
The old two-parameter prototype was an ABI error, not a tie. It cost setup its frame spill: EBX has to carry
`actor_index` into the call, so January spills the actor pointer to `[ebp-8]`.

### `_action_flee_setup`: strict EXACT (272/12, sha 1a415a0adece0dd5)

- The 3-parameter call fixes the size and the frame.
- January loads `force_state_entry` first and emits the stores after `csmemset` as +5, +8, +0xc, +4, +0x1c, +0.
  The source order that reproduces this is: forced_flee_ticks, flee_firing_position_index = NONE, panic_type,
  flee_from_last_visible_location, allow_occluded_points, flee_prop_index.
- This is store-order recovery for independent field initialisation, per the methodology case `lra_new`.
- Shapes: v1 (prototype only), v2/v3/v4/v5b/c/e (other orders) were sha-only; v5d was EXACT.

### `_action_flee_current_position_exposed`: strict EXACT (352/13, sha 498dc2c046073108)

- **Wrong field in base.** At +0x130, January copies prop_datum +0xbc into `approach_point`. In our `props.h`
  layout, +0xbc is `body_position`: `definitely_located` is 0xb8, `abandoned_search` 0xbb, and `head_position` is
  +0x104, which is the pointer January passes to `ai_test_line_of_sight`.
- Base copied `head_position`, the same error as the HCEA lift. The second use of `&prop->head_position` created
  the extra `[ebp-8]` pointer home, the 0x14 frame (January: 0x10) and the EBX/EDI swap.
- The park's premise, "coherent EDI/EBX allocation, not missing behaviour", is refuted.
- +0x16..+0x22: `exposed = FALSE` is stored after `datum_get`. By the store/call-order law, `actor` is declared
  before `exposed`.

### `_action_flee_perform`: strict EXACT only with the proposed prototype (864/30, sha d62f05304574f88e)

January facts that each correct the base:

| January bytes | Source fact |
| --- | --- |
| +0x40 `cmp [esi+2],0; jle; mov [esi+8],-1` | `if (flee_stationary_ticks > 0) { index = NONE; } else { ... }` (base had the inverted polarity) |
| +0x109 `mov byte ptr [eax+0x74], bl` | prop +0x74 is `currently_damaging_me`; base wrote `unopposable_enemy` (+0xa4, HCEA's misnamed lift) |
| +0x101/+0x106 `movsx ecx,cx` / `movsx ecx,dx` | `prop->perception = MAX(prop->audibility, prop->ineffability);` direct, int-promoted |
| visibility, perception, damaging, LOS stores | statement order visibility, perception, currently_damaging_me, line_of_sight |
| +0x255 `or eax,-1; cmp ecx,-1; je` | `cause_unit_index = NONE; if (flee_prop_index != NONE) cause_unit_index = prop_get(...)->unit_index;` |
| +0x27a fall-through | the not-yet-communicated flee event is the then-arm; the idle event is the else-arm |
| +0x2d3 `mov al,[edi+6]; test; jne 0x324` | the line-302 assertion is guarded by `if (!actor->meta.swarm)` |
| +0x1a9 `push 1` | `respect_direction = TRUE` |
| +0x1ea `test al,al; jne; mov [esi+0x10],al` | `unit_is_speaking` returns `boolean` |

The last row is an owner gap. No header declares `unit_is_speaking` (definition: `unit_dialogue.c:389`,
`boolean unit_is_speaking(`), so `action_flee.c` compiles it as a C4013 implicit `int` and emits `test eax,eax`.
With every other fix, the flee_c1 perform differs from January by that single instruction. The diagnostic
candidate, which adds a local prototype, is strict EXACT.

Shapes: v6 (polarity, field, cause, arm order, swarm guard) gave 848. v6p (+ diagnostic prototype) gave 848. v7
(perception after damaging) gave 848. v7b gave EXACT. v7n (v7b without the prototype) gave 864, sha-only.

## Proposals

1. **ORCHESTRATOR-PROPOSAL (header), `source/units/units.h`**. Insert after the `unit_notify_impulse_sound`
   prototype, before `unit_make_damage_sound`:

   ```c
   boolean unit_is_speaking(
    long unit_index);
   ```

   Evidence:
   - January +0x1ea `test al,al`;
   - the definition's return type;
   - punpckhdq `units.h:557` declares the same prototype at the same neighbourhood.

   Consumers: 66 direct includers (`scratch/workers/actions_action_flee_etc/units_h_consumers.txt`). The
   declaration-count law applies, so a full build is required.

2. **park-reopen `_action_flee_setup` and `_action_flee_current_position_exposed`** from `flee_c1.c`. Both are strict
   EXACT without any header change.
3. **park-reopen `_action_flee_perform`** from `flee_c1.c` once proposal 1 lands. If proposal 1 does not land, the
   reopen of (2) still changes perform's call site (`push 1`), so perform's park must be re-baselined to
   864/30 sha cefb8e42bcdf2d49.

## Do-not-repeat

- The two-parameter `find_flee_position` prototype.
- `head_position` as the exposure approach point.
- `unopposable_enemy` in perform's arrival block.
- The `<= 0` stationary polarity.
- A short perception local.

## Reopen criteria

- setup and exposed: met (January-proven ABI and field facts).
- perform: land the `unit_is_speaking` prototype in its owner header.
