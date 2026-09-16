# `game_sound.obj` Opus 5 next-150K lane, wave n1 (2026-09-15)

## Scope and provenance

- Translation unit `source/sound/game_sound.c`; target `build/split/source/sound/game_sound.obj`
  (January 2002 Xbox debug build, VC7 13.00.9254, `/O2 /Oy- /DDEBUG /Dxbox`).
- Lane `opus/next-150k-house-clean-20260915`, frozen baseline `ae10935da`, worker n1
  (files `source/sound/game_sound.c`, `source/sound/ima_adpcm.c`). No header, config, park or build edit.
- Assigned target: `_update_potentially_audible_looping_sound` (PARKED, scratch-only re-investigation).
  Out of scope by orchestrator ruling: `_compute_sound_obstruction` (blocked on the
  `_vector_from_points3d` surplus-COMDAT census ruling, w2 ledger) and `_game_sound_update`
  (do-not-repeat, `scratch/w2/laws_w2.md` section D).
- Ledgers read first: `game_sound_obj_opus5_150k_w2_20260914`,
  `game_sound_obj_obstruction_current_owner_boundary_20260905`,
  `devices_game_sound_small_fuzzy_harvest_20260908`,
  `game_sound_obj_pas_current_context_integration_20260904`,
  `game_sound_obj_pas_independent_review_20260904`, `game_sound_obj_jonas_public_leaf_pair_20260828`.
  **Provenance correction:** the three `sound_manager_obj_*` ledgers named in the wave brief
  (`idle_channel_spine_20260908`, `opus5_100k_waveC_20260914`, `small_looping_graph_20260908`) describe the
  *homonymous* `static long update_potentially_audible_looping_sound` in `source/sound/sound_manager.c`
  (`sound_manager.c:642`/`:1516`) — a different unit, a different signature and a different function. They
  carry no evidence about this row. The only ledger that owns this function is the w2 one.
- Worker notes: `scratch/workers/n1_game_sound_ima_adpcm.md`; candidates in
  `scratch/workers/n1_game_sound_ima_adpcm/`.

## Baseline and final

| | exact | residual | unwritten |
| --- | ---: | ---: | ---: |
| baseline (point guard passed) | 28 | 3 | 0 |
| final real file (unchanged, guard passed) | 28 | 3 | 0 |

`git diff --stat -- source/sound/game_sound.c` is empty. Both parks drift 0 (`scratch/parkcheck.py`:
`_update_potentially_audible_looping_sound` 640/25, `_game_sound_update` 464/31). Fake-match scan 0 leads.
Owner census unchanged (nothing landed). Duplicate prevention: `branch_sweep.py source/sound/game_sound`
sees 13 unique blobs and the current tree is the best of them (28/3/0; next best 27/4/0 and 27/3/1).

## `_update_potentially_audible_looping_sound` — NOT LANDED (tie confirmed, park stays)

Target `624 padded / 619 meaningful / 25 relocs / sha16 8fe3a4a813d3a2a8`.
Real (parked) body `640 / 25 / 6763e595e2871c3e`.
w2 scratch body `scratch/workers/bink_playback_game_sound_etc/pas_v2.c`: **`624 / 25 /
f3008036d008e7f4`** — size-exact and relocation-exact, 623 meaningful.

### Census (brief section 5), run before any shape

**Frame: there is no frame gap.** January `sub esp,0x54`, ours `sub esp,0x54`, and every `[ebp-N]` slot
agrees: `struct sound_source source` at `[ebp-0x54]` (0x40 bytes — spatialization_mode +0, scale +4,
gain +8, location.position +0xc, forward +0x18, translational_velocity +0x24, game_location +0x30),
`real_vector3d angular_velocity` at `[ebp-0x14]`, `definition` at `[ebp-8]` (both builds reuse that slot
for `fade_time` on the not-audible path), `serviced_last_frame` at `[ebp-1]`. Ours neither lacks nor adds
a slot. The 640 -> 624 structural gap that the park records was already closed by w2's single-exit
restructure; what the park's base measurement shows is the *pre-restructure* body.

**Relocations: 25 vs 25, all targets and addends equal** (`relocdiff --count-by-target`). One ownership
note: January's `game_sound.obj` references `??_C@_08CBBFJFMD@location?$AA@` (the `match_assert`
expression string for line 619) as an *undefined external*, because csplit assigned that select-any
`.rdata` COMDAT to another object; our TU defines it locally. `relocdiff` resolves the pair equal
(`symbolic=` row) and the gate tags the row `[sha]` only, so it is a COMDAT-ownership artifact, not an
independent blocker. The eleven "differing" address rows are the uniform +4 shift of everything after
0x140 — a consequence of the residual below, not a separate gap.

### The residual is one constant-materialisation decision

January pins a zero in ECX across the `if (sound->object_index != NONE)` join and spends it four times;
we re-materialise immediates. Sites (offsets are January's):

| site | January | ours (`pas_v2`) |
| --- | --- | --- |
| 0xaf, in the dominator between `cmp eax,-1` and `je 0x13c` | `xor ecx,ecx` | absent (emitted at 0xb8, inside the then-arm) |
| 0xba | `mov cx,word[esi+0x1a]` — the `node_index` zero-extension that seeds the pin | same, but locally seeded |
| 0x138, then-arm edge copy into the join | `xor ecx,ecx` | absent |
| 0x13c `source.spatialization_mode = _sound_spatialization_mode_none` | `mov word[ebp-0x54],cx` (4 B) | `mov word[ebp-0x54],0` (7 B) |
| 0x14f `sound->state == _game_looping_sound_active` | `cmp word[esi+2],cx` (4 B) | `cmp word[esi+2],0` (5 B) |
| 0x165 `sound_refresh_looping(..., 0.f)` | `push ecx` (1 B) | `push 0` (2 B) |
| 0x166 `sound->state = _game_looping_sound_active` | `mov word[esi+2],cx` (4 B) | `mov word[esi+2],0` (7 B) |

Values read straight off January's bytes: `_game_looping_sound_active` = 0, `_deactivating` = 1,
`_inactive` = 2 (`mov word[esi+2],2` @0x94, `mov word[esi+2],1` @0x22e);
`_sound_spatialization_mode_none` = 0; `_looping_sound_refresh_start` = 0, `_loop` = 1
(`xor eax,eax` @0x15c vs `mov eax,1` @0x160). The pin's live range ends at `je 0x1ee`: on the not-audible
path January itself writes the 0.f fade with an immediate (`mov dword[ebp-8],0` @0x203). So the region is
exactly [0xaf .. 0x166], and in that region the two builds have the same five zero uses, the same
register availability (ECX is dead after `mov cl,byte[ebp-1]` @0x87) and the same IR-level constants. Only
the promote-constant-to-register decision differs.

### Shapes

| shape | change | result |
| --- | --- | --- |
| `pas_v2.c` (w2, re-measured at this baseline) | single-exit restructure + statement order | 624 / 25 / `f3008036d008e7f4`, 16 aligned diff blocks |
| `g1.c` | the audible `sound_refresh_looping` fade argument `0.f` -> `0`, testing whether a float-typed constant is value-numbered apart from the integer zeros of the state store and compare | **byte-identical to `pas_v2`** (same 624 / 25 / `f3008036d008e7f4`). Measured negative; it agrees with `laws_w3` section D (`_custom_render_nav_point`: `0`, `0.0` and `0.f` identical). |

The tie budget of brief section 5 (two shapes once only allocation remains) is spent; no third shape was
attempted.

### Why no admissible source form was proposed

The zero-register pinning family is documented as *not* a compiler flag and *not* reachable by
ternary-versus-if/else rewriting or declaration reordering (both provably inert in the prime_numbers and
lra_cache measurements). The single measured source handle in that family is aggregate-initialiser
completeness (`real_vector3d v = {0};` fills from a zeroed register, `= {0,0,0}` stores per member), and
**this function has no zeroed aggregate**: January's bytes contain neither a memset nor a register fill,
and both `struct sound_source source;` and `real_vector3d angular_velocity;` are uninitialised in the
target as in our source. Every remaining device that would pin a zero here — a named zero local reused for
the enum constants, a redundant pre-store of `spatialization_mode` before the if, or an arm-duplicated
statement — is a prohibited steering alias (R16), a redundant store (R12) or R2.

## Reopen criterion (supersedes the w2 wording)

Reopen when a **second measured instance of the zero-register-pinning lever family** exists, i.e. a source
handle that makes VC7 promote the constant 0 to a register across an if/else join in a function with no
zeroed aggregate. Start from `scratch/workers/bink_playback_game_sound_etc/pas_v2.c`, which is already
size-exact and relocation-exact at 624/25 and is 4 meaningful bytes from the target.

Do **not** re-try: `0` versus `0.f` for the fade argument (this wave), ternary/if-else rewriting,
declaration reordering, or a named zero local.

## Orchestrator proposals

None. No header, config, symbols.json or park change is required or requested by this wave.
