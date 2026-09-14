# `first_person_weapons.obj` park re-investigation (Opus5 150K lane, wave w2, 2026-09-14)

## Result

Four of the five `first_person_weapons.obj` parks become strict EXACT in a scratch candidate. The candidate uses house-rule source and changes no
exact sibling. The real file is **unchanged**, because all four functions are parked (`git diff --stat` is empty). The orchestrator must verify and
unpark them in a dedicated commit.

| function | January padded / meaningful / relocs | base candidate | scratch candidate |
|---|---:|---|---|
| `_first_person_weapon_set_visibility` | 160 / 146 / 12 | 176 / 12 | **EXACT** `d4692680178268ea` |
| `_first_person_weapon_render_update` | 176 / 173 / 14 | 192 / 14 | **EXACT** `1eae8155ba3c823b` |
| `_first_person_weapon_switch_weapons` | 608 / 604 / 26 | 544 / 20 | **EXACT** `7d83d24b330d5e74` |
| `_first_person_weapon_message` | 544 / ~542 / 20 | 544 / 20 | **EXACT** `cd9c8aedea8b749e` |
| `_first_person_weapon_update` | 1,536 / 1,533 / 59 | 1,520 / 60 | not landed (owner-blocked) |

The gates, both with the `_point_from_line3d` guard passing:

- baseline real-file gate: 29 exact, 5 residual, 0 unwritten;
- candidate `scratch/workers/first_person_weapons/m1.c` (SHA-256 `e946f98c…3848`): **33 exact, 1 residual, 0 unwritten**.

Candidate checks:

- A row-by-row diff against the baseline changes only the four reopened rows.
- `scratch/parkcheck.py` shows `_first_person_weapon_update` unchanged. The four reopened parks drift to exactly the January hashes.
- The owner census against `build/base` and `build/split` finds no new code, data, BSS or COMMON owner.
- `tools/fake_match_scan.py` reports 0 leads on both HEAD and the candidate.

## Candidate diff (relative to the current real file)

1. Add `#include "effects/particles.h"` after `#include "effects/effects.h"`. This is the genuine owner of
   `void particles_stop_on_first_person_weapon(short local_player_index);`, declared at `source/effects/particles.h:171`.
2. In `first_person_weapon_render_update`, compute the visibility into a named local first, then pass the local to the call:
   `boolean visible= director_get_perspective(...)==_director_perspective_first_person && player_control_get_zoom_level(...)==NONE;`
   is followed by `first_person_weapon_set_visibility(render.local_player_index, visible);`.
3. In `first_person_weapon_message`, three comparisons become switch statements (behaviour is unchanged):
   - `switch (weapon_definition->weapon.weapon_type) { case _weapon_type_shotgun: ... }`
   - `switch (first_person_weapon->shotgun_reload_type) { case _shotgun_reload_type_first_round: case _shotgun_reload_type_first_and_last_round: enter; case NONE: reload_while_empty; }`
   - a closing `switch (message_type) { case _first_person_weapon_message_ready: ... }`

## Evidence

### The implicit declaration behind the "private two-register ABI"

`CL /Zs /W3` on the real file reports the only C4013 in the TU:

```
first_person_weapons.c(840) : warning C4013: 'particles_stop_on_first_person_weapon' undefined; assuming extern returning int
```

The implicit `int` declaration changes the code in three steps:

1. It promotes the short argument. The base candidate pushes a sign-extended EBX copy to `particles_stop` only, while January pushes the raw EDI to both
   stop calls.
2. That extra EBX copy evicts the boolean `visible` parameter from BL onto the stack. The helper grows from 160 to 176 bytes.
3. At 176 bytes the helper falls outside VC7's inline threshold, so `switch_weapons` loses January's inline of the FALSE call.

January inlines the nesting-1 FALSE call and keeps the deeply nested TRUE call out of line, which is the inline frequency law.

The parks' recorded experiments with a "false long local-player parameter" were therefore masking the implicit promotion. With the genuine prototype
visible, the honest `short, boolean` signature reproduces January's DI/BL boundary.

### `render_update` byte-materialised boolean

With the include in place, the only remaining difference was `mov ebx,1` / `xor ebx,ebx` (ours) against January's `mov bl,1` / `xor bl,bl`. A named
`boolean` local gives the byte form. HCEA's `first_person_weapon_render_update.c` holds the same `uint8_t visible` local.

### Switch-width law (new, reusable)

VC7 13.00.9254 lowers `switch (short_expr)` in a specific way:

- It sign-extends the expression to `int` and compares at full width, using a `dec`/`sub` chain or a `cmp` chain.
- It CSEs that sign-extended value across later switches on the same variable, homing it to a stack slot when it is live across calls.

`if (short_expr==K)` instead compiles to a 16-bit `cmp word`. January's message dispatcher shows all three switch forms:

- `movsx eax,[ebp+0xc]; cmp eax,0xd; mov [ebp-4],eax` … and later `cmp dword ptr [ebp-4],0xc`. This is the park's unexplained "four-byte message local".
- `movsx ecx,word [eax+0x4e2]; dec ecx; jne`, which is a single-case switch on the weapon type.
- `movsx eax,word [esi+0x1e94]; cmp eax,-1; je; test eax,eax; je; cmp eax,2; jne`, with the enter-reload body laid out before the
  reload-while-empty body. This fixes the case order.

Necessity probes:

- M2 turns the closing switch back into an `if`: residual, because the `[ebp-4]` slot disappears.
- M3 turns the weapon-type switch back into an `if`: residual, because the compare becomes `cmp word [eax+0x4e2],1`.

Style corroboration: January `weapons.c` (`source/items/weapons.c:1247`) switches on `shotgun_reload_type` with the same first-round and
first-and-last cases. `weapons.c:1327` and the exact `first_person_weapon_set_state` use small switches.

### Definition order is inert

`v1.c` reorders all 34 definitions into January's section order. Its object section order then equals January's, and every function's bytes are
identical to the baseline.

## Park criteria

- **`set_visibility`** — "authoritative January signature/local records or a natural same-compiler private-ABI donor". The premise is refuted: the
  boundary comes from a missing genuine prototype, not from an unknown signature.
- **`render_update`** — the same criterion. It is met by the include plus the HCEA-corroborated boolean local.
- **`switch_weapons`** — "when the visibility private-ABI blocker is solved". It is met, with no body change.
- **`message`** — "local records or a donor explaining both width/lifetime choices". Both choices are explained by the switch-width law above,
  measured with necessity probes.

## `_first_person_weapon_update`: not landed

This function stays parked. Two owner-level issues block it, and a set of scheduling ties would remain after them.

**1. Inline schedule of `magnitude3d`.** January inlines `magnitude3d(&unit->unit.throttle)`, but the TU's `#define REAL_MATH_EXTERNAL_MAGNITUDE3D`
forces a call and adds one relocation. Removing the three math defines (`u1.c`) gives 1,552 bytes and 59 relocations. However, it emits the
unreferenced SELECT_ANY COMDATs `_magnitude3d`, `_magnitude_squared3d` and `_square_root`. Those collide with January's NODUPLICATES owners, as the LNK2005
proof in `first_person_weapons_obj_update_private_cluster_boundary_20260904.md` shows. The fix needs an owner-level `real_math.h` schedule.

**2. Unguarded NULL dereference.** January dereferences `count ? TAG_BLOCK_GET_ELEMENT(...) : NULL` without a null test, in both the moving block and the
overcharged-jitter block. Removing our `first_person_weapon_animations &&` guards (`u2.c`) reaches 1,536 bytes, but that reproduces a NULL-dereference
original bug. Brief section 5 prohibits this without an owner ruling.

**Frame and remaining ties.** Two local-scope probes left the frame at `0x10`:

- `t1.c` declares `turning_i`/`turning_j` at the top of the rendered block;
- `t2.c` declares them in the outer block.

`t3.c` uses a `real_vector2d turning` aggregate local (fields `.i`/`.j`) instead. On the inadmissible `u2` base this reaches frame `0x14`, and
every stack slot equals January's. What remains are dependency-free register and push-order ties:

- the `director_get_perspective` argument;
- the `object_impulse_sound_new` load/push order;
- the `unit_flying_through_air` argument;
- two element-pointer temporaries;
- the `fmod` element push.

**Reopen criterion.** Two things are needed first:

- an owner ruling admitting the target-proven unguarded dereference, which switch_weapons' commit invariant makes unreachable in practice;
- a `real_math.h` schedule that inlines `magnitude3d` here without emitting helper COMDATs.

Then start from the `t3.c` aggregate.

## Proposals

- **Park reopen:** the four functions above, from candidate `scratch/workers/first_person_weapons/m1.c`.
- **Law for brief section 9:** the switch-width law, plus "run `CL /Zs /W3` before classifying a private-register-convention park".
- **Follow-up lead:** a `/Zs /W3` sweep of all 138 parked units found **53 C4013 implicit declarations in 28 parked units**
  (`scratch/workers/first_person_weapons/c4013_parked_units.txt`). The list includes:
  - bipeds: `unit_get_local_player_index`, `weapon_prevents_melee_attack`, `unit_animation_start_action`, `weapon_stop_reload`, …;
  - `actor_firing_position`, `hs_runtime`, `main`, `render_debug`, `game_engine_race`;
  - light_volumes and rasterizer_xbox_models.

  Each is a candidate for the same short/boolean/real promotion mechanism. Adding a genuine owner include in the consumer `.c` is not a header
  edit, but it must be measured per unit.

Scratch evidence is in `scratch/workers/first_person_weapons/`:

- `v1.c`, `v2.c` and `v3.c`;
- `m1.c`, `m2.c` and `m3.c`;
- `u1.c`, `u2.c`, `t1.c`, `t2.c`, `t3.c` and `t4.c`;
- the objects and the notes file `scratch/workers/first_person_weapons.md`.
