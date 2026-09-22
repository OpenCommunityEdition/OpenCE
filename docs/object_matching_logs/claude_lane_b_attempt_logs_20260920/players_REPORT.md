# players.c Lane B worker report

Resumed run: candidate.c existed and was byte-identical to source/game/players.c (cmp). No prior REPORT.md.

## Baseline (python tools/campaign/gate.py source/game/players --all)

```
EXACT        80  _any_player_is_dead
EXACT       240  _any_player_is_in_the_air
EXACT       144  _debug_player_teleport
EXACT       208  _find_best_starting_location_index
EXACT        80  _find_unused_local_player_index
EXACT        64  _is_player_in_trigger
EXACT        16  _local_player_count
EXACT        96  _local_player_exists
EXACT        64  _local_player_get_next
EXACT        80  _local_player_get_player_index
EXACT       160  _local_player_set_player_index
EXACT        80  _machine_add_player
EXACT        32  _machine_get_player_list
EXACT       112  _placement_data_set_change_color
EXACT       176  _player_active_camo_screen_effect
EXACT       336  _player_add_equipment
EXACT       272  _player_aiming_vector_from_facing
EXACT       288  _player_control_fix_for_loaded_game_state
EXACT        32  _player_delete
EXACT       144  _player_died
residual    176  _player_examine_nearby_device  [size 160!=176, sha]
EXACT       800  _player_examine_nearby_item
EXACT       240  _player_examine_nearby_objects
EXACT        16  _player_examine_nearby_unit
EXACT       448  _player_examine_nearby_vehicle
EXACT       192  _player_get_starting_location
EXACT        80  _player_get_starting_location_count
EXACT       592  _player_handle_action
EXACT       240  _player_handle_powerup
EXACT       336  _player_handle_powerup_equipment
EXACT       160  _player_handle_powerup_minor
EXACT       192  _player_handle_weapon_swap
EXACT       144  _player_health_pack_screen_effect
EXACT        80  _player_index_from_unit_index
EXACT        32  _player_input_enable
EXACT        32  _player_input_enabled
EXACT       288  _player_new
EXACT       176  _player_over_shield_screen_effect
EXACT        48  _player_powerup_additional
EXACT        48  _player_powerup_off
EXACT        64  _player_powerup_on
EXACT       224  _player_pseudo_kill
EXACT        32  _player_reset_action_result
residual    240  _player_set_action_result  [sha]
EXACT       816  _player_spawn
EXACT        96  _player_teleport
residual   1296  _player_teleport_internal  [size 1312!=1296, relocs 62!=61, sha]
EXACT       320  _player_teleport_on_bsp_switch
EXACT       112  _player_update_powerups
EXACT        16  _players_are_all_dead
EXACT       368  _players_compute_combined_pvs
EXACT        32  _players_compute_local_player_count
EXACT       416  _players_debug_render
EXACT        48  _players_dispose
EXACT        32  _players_dispose_from_old_map
EXACT        16  _players_get_combined_pvs
EXACT        16  _players_get_combined_pvs_local
EXACT        16  _players_get_respawn_failure
EXACT       112  _players_handle_deleted_object
EXACT       112  _players_initialize
EXACT       176  _players_initialize_for_new_map
EXACT       736  _players_reconnect_to_structure_bsp
EXACT       464  _players_respawn_coop
EXACT       192  _players_set_local_player_unit
EXACT       816  _players_update_after_game
EXACT      1760  _players_update_before_game
EXACT        32  _random_direction3d
EXACT       112  _unit_create_starting_weapon
EXACT       144  _unit_should_autopick_weapon
EXACT        64  _valid_real_vector2d
== exact 67  residual 3  unwritten 0  (of 70 listed)
```

## _player_examine_nearby_device (baseline: residual 176 [size 160!=176, sha])

January bytes (aln_device_full.txt): after `call _unit_get_camera_position` January has
`fld dword [edi+0x5c]; add esp,0x1c; fstp dword [esp]` (radius staged through x87, folded into the
pending stack adjust, then `add esp,0x10` after the sphere call). Ours: `mov eax,[edi+0x5c]; push eax`
and `add esp,0x30`. Frame equal (0xc), ret count equal, relocation multiset equal. Byte ledger:
+3 (fld vs mov same length 3; `add esp,0x1c`=3 new) +3 (fstp [esp]) -1 (push eax) = +5 real bytes
(162 vs 157), everything else is scratch-register renumbering downstream of the freed eax/ecx.

Attempts (gate.py --source <variant> --fn _player_examine_nearby_device), variants in dev/:
| variant | spelling | gate row |
|---|---|---|
| baseline | `device->object.bounding_sphere_radius` | residual 176 [size 160!=176, sha] |
| const | `struct device_datum const *device` | residual 176 [size 160!=176, sha] (inert) |
| nested | `if (sphere) { if (frontfacing && can_change) ...` | residual 176 [size 160!=176, sha] (inert) |
| cast | `(real)device->object.bounding_sphere_radius` | residual 176 [size 160!=176, sha] (cast folds away) |
| local | named `real radius` assigned before the camera call | residual 176 [sha]; frame 0x10 != 0xc, radius homed at [ebp-4] - structural fail |
| local2 | named `real radius` assigned after the camera call | residual 176 [sha]; frame 0x10 != 0xc, `mov eax,[edi+0x5c]; mov ecx,eax; push ecx; mov [ebp-4],eax` - structural fail |
| macro_strip | `#define DEVICE_BOUNDING_SPHERE_RADIUS(device) (device)->object.bounding_sphere_radius` (no outer parens) | residual 176 [size 160!=176, sha] |
| paren | bare `(device->object.bounding_sphere_radius)` | EXACT 176 |
| paren2 | `((device->object.bounding_sphere_radius))` | EXACT 176 (idempotent) |
| macro | `#define DEVICE_BOUNDING_SPHERE_RADIUS(device) ((device)->object.bounding_sphere_radius)` | EXACT 176 |
| macro_idem | macro + extra call-site parens | EXACT 176 (idempotent) |

STRIP TEST: FAILS as a plain strip (macro_strip / baseline are 160). The close therefore relies on the
owner's narrow admission (Lane A `LINE_OF_FIRE_PILL_SPHERE_RADIUS`, collisions.c:1572 kept bare form):
conventional accessor-macro outer parenthesisation; measured idempotent (paren==paren2==macro==macro_idem, all EXACT);
every undecorated alternative measured and failing (baseline, const, nested, local, local2; cast too); name attested by
objects.h:250 field `bounding_sphere_radius` / objects.h `object_get_bounding_sphere` and real_math.h:409 parameter `radius`.
ACCEPTED into candidate.c (macro form). unitcheck: gains=1 regressions=0 other=0 (exact 67 -> 68).

## _player_set_action_result (baseline: residual 240 [sha])

January bytes (aln_sar_full.txt): 94==94 instructions, frame 0xc==0xc, 1 ret each, relocs equal. ONLY difference is the x87 order of the first (x87-resident) distance3d sum: January k*k, i*i, add, j*j, add (all three via fld-copy; result fstp st(3) + 2 pops); ours i*i in place (fld st(2); fmulp st(3)), k*k, j*j, fxch. Second (memory-resident) distance identical.

- M8 diagnostic (m8probe.py, D=0..8 dead initialised locals, instrument only): every D -> residual 240 [sha]. Verdict M8-INSENSITIVE/non-closing: IL-local count is not the lever.

- Shapes measured (sar/, fnsha.py):
  - v1 explicit `real_vector3d` locals + vector_from_points3d + magnitude3d compare == v2 `set_action = distance3d(..) > distance3d(..)` (no distance locals): residual 240 [sha] sha 017a0d67; first sum becomes k,j,i copies (January k,i,j). Closer in form (top-accumulate) but j/i swapped.
  - v3 `if (distance3d > distance3d) set_action = TRUE`: residual [sha] 583373e3, same k,j,i plus branch layout drift.
  - v4 named distances + if form: residual [sha] 9d65ab11 (P1 in-place i, branch drift).
  - h1 current named/new inline: byte-identical to baseline (bc9bed2c). h2 new named/current inline == h3 swapped statement order: frame becomes push ecx (4) - structurally worse (99 insns). h4 `new_distance < current_distance`: frame 4, 100 insns - worse.
  - o1/o2 object_datum locals with &obj->object.position args: offsets fold into [reg+0xc] - 96 insns, worse. January's `add esi,0xc / add edi,0xc / add eax,0xc` proves position-pointer values are bound.

- More set_action_result evidence/shapes:
  - Board scan (patscan.py) of January split objects: January's k,i,j top-accumulate pattern ("P0", bytes D9C0D8C9D9C3D8CCDEC1D9C2D8CBDEC1) occurs 35x, ours' i-in-place ("P1") 7x; both are natural VC7 output of the SAME real_math.h inline (exact exemplars: P0 object_lights `real distance = distance3d(light_position, center)`, units_debug_get_closest_unit; P1 collision_edge_length, closest_available_point_on_surface). So the header spelling is NOT the blocker by itself; the site context is.
  - n-ary sum order table measured here: baseline (named distances, bare position-pointer locals) x87 sum = i(in place),k,j + memory sum k,j,i; January x87 = k,i,j (copies, `fsqrt; fstp st(3)`), memory = k,j,i; v1/v2/s6/o1 x87 = k,j,i; s7/s8 (ONE reused function-level vector) x87 = k,i,j (January's!) but sum1 is scheduled BEFORE the v2 stores and memory sum becomes k,i,j (s8: 94==94 insns, still [sha]; s5/s7: frame 4, worse).
  - January's two sums use DIFFERENT orders (k,i,j vs k,j,i) => two distinct vector objects, not one reused local.
  - M8 instrument re-run with fillers at the TOP of the declarations (before the pointer locals) on baseline and on v2: all D=1..8 byte-identical sha => truly M8-insensitive.
  - q1 (pointer decl order reversed), q2 (block-scoped declare-and-initialise), q5 (distances declared first): all byte-identical to baseline (bc9bed2c).
  - Verdict so far: residual-unchanged. Remaining ledger: 9 instruction rows inside the first inlined magnitude (0x92..0xa7): January `fld st0; fmul st1; fld st3; fmul st4; faddp; fld st2; fmul st3; faddp; fsqrt; fstp st3; fstp st0` vs ours `fld st2; fmulp st3; fld st0; fmul st1; faddp st3; fld st1; fmul st2; faddp st3; fxch st2; fsqrt; fstp st2`. The named decision: whether the x87-resident result (current_distance) is coalesced with v.i's stack slot (ours) or accumulated at top-of-stack with the i-term second (January).

## _player_teleport_internal (baseline: residual 1296 [size 1312!=1296, relocs 62!=61, sha])

January facts read from jan_tele.txt (428 insns, frame 0x7c, 2 epilogues = one `return result` tail-duplicated):
- ONE else arm at 0x48d reached from both `source_unit_index==NONE` and `parent==source` (jump-threaded through the single `source_unit_index==NONE || local_player_count()>1` assert) => single `&&` condition; the baseline's duplicated else (extra `_biped_fix_position` reloc, 62 vs 61) is a source defect.
- Slot map: -4 collision_height, -8 player, -0xc root pointer then outer adjustment index, -0x10 biped then scenario, -0x14 player_unit_index, -0x18 scale then trigger index, -0x24 best vector, -0x30 random vector then forward, -0x3c adjusted, -0x48 random adjusted, -0x7c matrix (6 scalar cells; t1/v2 have 5 => frame 0x78).
- Register winners: ebx=result, edi=player_unit_index (global; evicted by the random index in the inner loop and by player/scenario/source in the tail), esi=biped (global; evicted by the root INDEX for the whole search block, reload at 0x2d9, else-path jumps past the reload to 0x2dc). source_unit_index is never enregistered before the tail (reloaded from [ebp+0xc] at 0x2c, 0x6d, 0x76, 0x8a), root POINTER lives in memory [ebp-0xc], outer index lives in memory.
- Tail copies `unit_get(source_unit_index)->object.forward` (push 3) into the random-vector cell (block-scoped `forward`), then `biped_try_and_get` (push 1). The baseline's `biped_get(source_unit_index)` spelling is wrong on that push.
- x87: `magnitude_squared3d(&best)` with k==0 is evaluated j*j, i*i (ours i*i, j*j); the 0x525 assert magnitude is j,i,k (ours i,j,k).

Measured (score.py: gate row | insns | frame | rowdist = multiset distance of differing aligned rows):
| variant | gate | insns | frame | rowdist |
|---|---|---|---|---|
| baseline candidate.c | [size 1312!=1296, relocs 62!=61, sha] | 431 | 0x7c | 229 |
| tele/t1.c (prior lane t1 natural form re-measured on HEAD) | [size 1280!=1296, sha] | 428 | 0x78 | 226 |
| tele/v2.c (prior lane v2) | [size 1280!=1296, sha] | 428 | 0x78 | 234 |
| m8/t1_D1..D8 (dead-local instrument, diagnostic) | identical to t1 | 428 | 0x78 | 226 |

- set_action_result, late diagnostics: TU-level count key (a dummy function with D=1..8 live locals defined just before the target, m8/tu_D*.c) -> all byte-identical sha bc9bed2c; non-const position pointers (sar/c1.c) -> byte-identical. Both count keys (in-function M8, TU-level) and const-ness are measured INERT here; the order is a function of local IL structure only.

- teleport_internal, further shapes on t1 (tele/): x3 (`result = FALSE` first statement) -> identical to t1 (rowdist 226); x4 (`biped_get(player->unit_index)` before the index local) -> [size 1312!=1296, sha], 434 insns, rowdist 260 (worse). f1 (minimal fix on the BASELINE body: tail `unit_get(source_unit_index)->object.forward` instead of `biped_get`, January pushes type mask 3 at 0x39c) -> same gate tags [size 1312!=1296, relocs 62!=61, sha], rowdist 229 -> 227.
- unitcheck on tele/t1.c (device macro + t1 teleport): gains=1 regressions=0 other=1 (teleport tags [size 1312!=1296, relocs 62!=61, sha] -> [size 1280!=1296, sha]).
- DECISION: candidate.c keeps the baseline teleport body (function still fuzzy either way; two earlier orchestrators declined fuzzy teleport landings). tele/t1.c and tele/f1.c are preserved as optional variants. Two genuine source defects in the production body are byte-proven and reported: (1) duplicated else arm => extra `_biped_fix_position` relocation (62 vs 61); (2) tail uses `biped_get` (push 1) where January has `unit_get` (push 3 at 0x39c) - the production spelling would assert when the source root is a vehicle.
- Teleport blocker (named decision): global integer colouring of the entry/search region. January: ebx=result, edi=player_unit_index, esi=biped then root INDEX; source_unit_index and root POINTER memory-resident (6 scalar cells => frame 0x7c). Ours (t1): esi=source_unit_index then root POINTER then outer index, edi=player then root index; player_unit_index memory-resident (5 cells => frame 0x78). Everything else (61/61 relocs, 428/428 insns, block layout, both epilogues) already agrees. Secondary: x87 order at `magnitude_squared3d(&best)` j,i(,k) in January vs i,j(,k) ours.

## Final verification (candidate.c = production players.c + commented DEVICE_BOUNDING_SPHERE_RADIUS macro + its one call site)

- gate.py --all --forbid-emitted-symbol _point_from_line3d: `== exact 68  residual 2  unwritten 0  (of 70 listed)`; `== emitted-symbol guard passed (1 forbidden names checked)`.
- remaining rows: `residual    240  _player_set_action_result  [sha]`, `residual   1296  _player_teleport_internal  [size 1312!=1296, relocs 62!=61, sha]` (both byte-identical to baseline).
- unitcheck: gains=1 regressions=0 other=0. newsyms: `candidate newly DEFINES 0 symbol(s), no longer defines 0` / `OWNERSHIP PASS`. fake_match_scan: 0 review leads.
- CL /Zs /W3 (w3check.py): 18 diagnostics, identical set to the production source; the single C4013 (`error`, first use ~line 944) is pre-existing and documented byte-neutral in players_obj_opus5_150k_w3.
