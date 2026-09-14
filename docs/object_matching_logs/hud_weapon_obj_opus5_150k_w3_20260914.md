# `hud_weapon.obj` opus5 150K house-clean lane, wave w3 (2026-09-14)

## Result

No function landed. `source/interface/hud_weapon.c` is unchanged
(`git diff --stat` is empty), so the unit stays at **11 exact / 5 residual / 0
unwritten**. This was a second pass with the w2 laws (A1–A22) run as detectors
over all five residuals.

- Four residuals are backend allocation ties that no source spelling moves:
  `render_grenade_hud`, `hud_update_weapon_local_player`, `hud_update_weapon`
  and `render_weapon_hud`. `render_grenade_hud` also needs the owner header
  prototype.
- One residual, `crosshairs_draw`, is blocked by policy: January reads an
  uninitialised local.

```text
baseline/final: python -B tools/campaign/gate.py source/interface/hud_weapon --all --forbid-emitted-symbol _point_from_line3d
== exact 11  residual 5  unwritten 0  (of 16 listed)   emitted-symbol guard passed
parkcheck: no parks for source/interface/hud_weapon ; fake_match_scan: 0 leads (file unchanged)
```

No header, config, build or tool file was edited. No configure, ninja, pytest or
git write was run.

## Provenance

- **Ledgers read:**
  - `hud_weapon_obj_opus5_150k_w1_20260914`
  - `hud_weapon_obj_private_graph_fuzzy_20260912`
  - `interface_obj_weapon_hud_best_fuzzy_20260912`
  - the three Fable crosshair ledgers and the two Jonas ledgers
  - `scratch/workers/hud_weapon.md` (w1 notes)
  - the donor notes in the `opus5-50k-house-clean-20260913-evidence` worktree
  - `scratch/w2/laws_w2.md`
- **Duplicate checks.** No w2 ledger exists for this object. None of the five
  rows appears in the w2 do-not-repeat table (section D) or in the 100K
  admission manifest. The best branch history is still blob 772abc65, the
  w1-landed file.
- **Earlier w3 run.** A previous w3 run was cut off. It left only a header block
  and some remeasured objects; there was nothing to salvage.

## Cross-cutting measurements (new this wave)

| probe | change | result |
| --- | --- | --- |
| D-order, `o1.c` and `o1p.c` | Function definitions reordered to January section order: grenade, `strip_path_name`, `crosshairs_draw`, `hud_fix_weapon_data`, uwlp, `render_weapon_hud`. The real file has uwlp before `crosshairs_draw`, `hud_fix` late and `strip_path_name` last. | Every function is byte-identical with and without the shadow header. Definition order is inert for this TU; there are no static call cycles. |
| Shadow owner header, `p0.c` | A scratch copy of the genuine `items/weapons.h` plus the `weapon_prevents_grenade_throwing` prototype, included by relative path. No consumer-local prototype. | `test eax,eax` becomes January's `test al,al`. Only `render_grenade_hud` changes: still sha-only, 24 blocks. |
| Stack-check macro, `mA.c` | A3(a) found-flag `break` loop instead of the goto form. | Worse on all five functions: `or esi,-1` moves before the loop. January's goto form is confirmed. |

Harness files in `scratch/workers/w3_hud_weapon/`:

- `reorder.py`: reorders the function definitions;
- `splice.py`: splices a replacement text into a copy;
- `shas.py`: per-function size, relocations and hash;
- `scanfn.py`: byte-pattern census over January objects, with base exactness;
- `shadow/weapons.h`: the shadow owner header.

## Per-function outcomes

| function | target padded/relocs | real file | best scratch | outcome |
| --- | --- | --- | --- | --- |
| `_render_grenade_hud` | 736 / 33 | 736 / 33 sha | `p0.c` 736 / 33 sha (24 blocks) | owner-blocked (A7) + coalescing tie |
| `_crosshairs_draw` | 2256 / 76 | 2256 / 77 | — | policy: uninitialised read (R5/R12) |
| `_hud_update_weapon_local_player` | 1472 / 68 | 1456 / 69 | `vA.c` 1472 / 68 sha (16 blocks) | allocator tie |
| `_hud_update_weapon` | 624 / 34 | 608 / 35 | `vA.c` 640 / 35 | cross-jump blocked by register priority + uwlp tie |
| `_render_weapon_hud` | 2656 / 84 | 2624 / 85 | w1 `R3` 2672 / 84 (80 blocks) | allocator + x87-context tie |

### `_render_grenade_hud`

**Detectors that hit:**

- **A7.** The implicit declaration causes C4013, and January tests the result
  with `test al,al`. The genuine owner is `weapons.c:693`. `weapons.h` has no
  prototype; `units.c` (protected) carries its own consumer copy.

**Detectors that were already satisfied:**

- A16: the `&&` order of the parent-seat test.
- A17: `unit_definition_get` as a bare validation call.
- A18: the `char` grenade index (`cmp byte,0xff`) and the 16-bit
  `movsx dx,al; cmp dx,word` compare.

**Detectors already measured inert in w1:**

- A3: the nested single-exit body (G1).
- A5: removing the one-use `grenade` alias (dC).

**Residual with the shadow prototype.** The first divergence is at `+0xbc`.
January sign-extends the `short grenade_type` in place (`movsx eax,ax`); our
build uses `movsx ecx,ax`. The rest of the function follows as a rotation of
the caller-saved registers.

**Corpus census.** I scanned January objects for
`cmp ax,0xffff; je; movsx r,ax; push size; push r; call; add eax,off`:

- There are 30 EAX forms and 39 ECX/EDX forms in total.
- The same source shape compiles both ways in functions that are exact in our
  build:
  - `hs_evaluate_by_name` uses ECX;
  - `hs_rebuild_source_file` and `players_reconnect_to_structure_bsp` use EAX.

This is a coalescing tie, not a spelling difference.

**Proposal:** the header packet below (a repeat of w1).

**Reopen:** the prototype lands, and a source fact is found that makes VC7
coalesce this sign-extension into EAX. Retest with `p0.c`.

### `_crosshairs_draw`

January reads `firing_active` (`[ebp-1]`, at `+0x495`) on the state-14 path
(`fired_secondary_with_no_ammo`). No write reaches that read. The only writes
are at `+0x406` and `+0x48f`, and the if-chain covers states 18, 8 and 9 only.
Exact bytes therefore require an uninitialised read, which is undefined
behaviour and prohibited (R5/R12, brief section 5).

For the record, two structural differences also exist:

- **Frame.** January's frame is 0x318 against our 0x314. January shares slots:
  `sequence` at `[ebp-8]` doubles as a fild temporary, `[ebp-0x10]` holds the
  item index and a fild temporary, and `render_flags` is homed at `[ebp-0x74]`.
- **Relocations.** January cross-jumps the zoom-state and animated-state
  `get_flash_color` calls at `+0x393`, giving 76 relocations against our 77.

**Reopen:** an owner ruling on the uninitialised read.

### `_hud_update_weapon_local_player`

**Donor v6 body.** Carried here in `v3.c`, the body measures 1472 / 68 with 453
of 453 instructions. All 16 differing blocks lie in `+0x3b`–`+0xee`, starting
with January's `mov eax,[eax+0x34]` against our `mov edx,[eax+0x34]`.

**House-rule fix, A17.** v6 spelled the discarded weapon-definition fetch as an
unused `?:` local. `vA.c` replaces it with the admitted bare validation
statement
`if (weapon_index != NONE) { weapon_definition_get(weapon_get(weapon_index)->definition_index); }`
inside a scope block. The result is **byte-identical** to v6, so the compliant
spelling costs nothing.

**Other detectors:**

- A21 (the `definitions = { root }` aggregate) and A9 (switch width) are already
  satisfied.
- The zero-register pattern is a consequence, not a cause. January stores
  `render_flags = 0` as an immediate because EAX already holds `definitions[0]`.

**Why this is a tie.** No least-recently-used or coalescing model explains both
directions:

- at `+0x3e` January reuses the dying EAX and our build does not;
- at `+0x57` our build reuses EAX and January does not.

**Reopen:** a lever for the first caller-saved register choice after the spill
of `player`. Then land `vA.c`'s uwlp together with `hud_update_weapon`.

### `_hud_update_weapon`

**Detectors that hit.** Both are already in `huw_v3`/`v3.c` and unchanged in
`vA.c`:

- **A21.** The default `weapon_state` is `= { 0 }`
  (`mov [ebp-0x28],0; rep stosd`). The real file's `csmemset` is the extra
  relocation.
- **Accessor re-fetch.** January re-calls `object_get_and_verify_type` for the
  parent unit's `current_weapon_index`, like the exact sister function
  `hud_render_weapon_interface`.

**A8.** The function needs uwlp's EAX-plus-three-stack ABI, which only the v6
body provides.

**Residual.** January cross-jumps its two
`mov eax,[ebp-4]; call; add esp,0xc` tails. In our build the weapon path gives
EDI to `local_player_index` and ESI to the weapon definition, which costs +16
bytes and +1 relocation.

**Reopen:** uwlp closes, and a lever gives EDI to the weapon definition in the
weapon path.

### `_render_weapon_hud`

**Raw-bit sentinel.** The real file already uses a local union for the "no
target" value. Under w2's admitted "local union bit transfer" precedent this is
no longer an owner-ruling item.

**x87 distance-sum order (A4/A22).** January computes the squares as j², i², add,
k², add.

- **Census.** The same byte pattern occurs in the exact function
  `_ctf_engine_initialize_for_new_map`, produced by the plain `real_math.h`
  `distance_squared3d` inline. The identical source line three instructions
  later in the same function yields the other order (k², j², add, i², add). The
  order therefore depends on context.
- **Probes.** None changed the object:

  | probe | spelling | result |
  | --- | --- | --- |
  | `R3d1` | `square_root(distance_squared3d(...))` | identical to R3 |
  | `R3d2` | named delta vector + `magnitude3d` | identical to R3 |
  | `R3d3` | named `distance_squared` | identical to R3 |

**Frame-density census (A6).** The frame size is equal, but three slots are
permuted.

| slot | January | our build |
| --- | --- | --- |
| temp | -0x14, 14 refs | -0x18, 14 refs |
| element_index | -0x18, 12 refs | -0x2c, 9 refs |
| definition | -0x1c, 11 refs | -0x14, 14 refs |

The cause is register allocation in the statics loop:

- **January** keeps `definition` in EDI and caches `&definition->statics` in ESI,
  using non-destructive `lea esi,[edi+0x60]` and `lea edx,[edi+0x3c]`.
  `element_index` stays in memory.
- **Our build** enregisters `element_index` in ESI and reloads `definition`.

This is allocation, not a missing local.

**Reopen:** a lever for the statics-loop register priority. The x87 order would
then need re-measuring in that context.

## Orchestrator proposal

**Header packet (repeat of w1).**

- **Text.** In `source/items/weapons.h`, directly after
  `void weapon_set_integrated_light_power(long weapon_index, real light_power);`,
  add:

  ```c
  boolean weapon_prevents_grenade_throwing(
    long weapon_index);
  ```

- **Position.** This follows the definition order in `weapons.c`.
- **Evidence.**
  - the C4013 census (`scratch/w3/c4013.txt`, line 38);
  - January's `test al,al` in `render_grenade_hud`;
  - the boolean definition at `weapons.c:693`.
- **Direct consumers.**
  - `source/ai/`: `actors.c`, `actor_combat.c`
  - `source/game/`: `aim_assist.c`, `game_engine.c`, `game_engine_ctf.c`, `game_engine_oddball.c`, `players.c`, `player_control.c`
  - `source/interface/`: `first_person_weapons.c`, `hud.c`, `hud_draw.c`, `hud_weapon.c`, `interface.c`
  - `source/items/`: `weapons.c`
  - `source/objects/`: `objects.c`, `object_types.c`
  - `source/units/`: `bipeds.c`, `units.c` (protected; it keeps a duplicate consumer prototype), `unit_scripting_commands.c`

The prototype is needed for correctness but does not close `render_grenade_hud`
by itself. Because of the declaration-count law, re-gate the Units sentinel
after applying it.

## Scratch artifacts

All paths are under `scratch/workers/w3_hud_weapon/`.

- **Candidates:** `p0.c`, `o1.c`, `o1p.c`, `mA.c`, `vA.c` and `R3d1`–`R3d3`.
- **Objects and alignment diffs:** the `.obj` and `.aln` files next to each
  candidate.
- **Worker notes:** `scratch/workers/w3_hud_weapon.md`.
