# source/physics/collision_debug

## verdict
OBJECT_COMPLETE_CANDIDATE

## newly_exact_functions
[]

## candidate_files
[
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\collision_debug\\cand_final.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\collision_debug\\collision_debug.c.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\collision_debug\\collision_debug.helpers_only.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\collision_debug\\candB_mag_acos.c",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\collision_debug\\symbols.json.patch",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\collision_debug\\config\\symbols.json",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\collision_debug\\LEDGER.md",
 "C:\\halo-worktrees\\claude-fifty-objects-20260925\\scratch\\w\\collision_debug\\gate_rows.txt"
]

## production_changes
All paths are under C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\collision_debug\. Every patch passes `git apply --check` against 931ed8dc. Applied outside the repo, each post-image is byte-equal to its candidate file, with CRLF kept.

(1) source/physics/collision_debug.c: apply collision_debug.c.patch. The full post-image is cand_final.c. The patch has three hunks, all inside _collision_debug_render:
  a. Features block: `radius = magnitude3d(&collision_debug_vector) * 0.5f + collision_debug_height * 0.5f + collision_debug_width;` replaces the hand-expanded `(real)sqrt(v.i*v.i + v.j*v.j + v.k*v.k)`.
  b. Textstring: `arccosine(collision.plane.n.k) * 360.f / (_pi*2.f)` replaces `(real)acos(collision.plane.n.k) * 57.295776f`.
  c. Phantom cube angles: `yaw/pitch/roll = (real)fmod(game_time_get() * {1.0f | -0.7f | 1.4f} * _pi / 30.f, _pi*2.f);` replaces the read-back literals 0.104719758f / -0.073303826f / 0.146607652f and the raw double 6.2831854820251465.
  Fallback, if the integrator wants only the helper fixes: collision_debug.helpers_only.patch (hunks a and b; the post-image is candB_mag_acos.c; also exact and link-clean).

(2) config/symbols.json: apply symbols.json.patch. The full in-place-edited copy is config/symbols.json. It is exactly one line op: line 19564 becomes `{ "file_offset": 2667176, "flags": 0, "name": "_collision_debug_cube_vertices", "static": true },`. Nothing is re-serialised and CRLF is kept.

(3) Integrator: after the split regen and a full ninja, flip config/config.json line 515 (`source/physics/collision_debug.c` index 140) from "NonMatching" to "Matching". No header, parked.json or semantic entry is needed.

## evidence
Evidence is in scratch/w/collision_debug/: LEDGER.md, gate_rows.txt, od_linear.txt and od_proj/report.json.

/Od call census (od_linear.py 0x7af800..0x7b1ee4): every callee maps to production at equal counts, except two sites.
- Features radius, /Od 0x7b1dc6: calls 0x432950 = magnitude3d. The chain is 0x432950 -> square_root 0x42e320 -> sqrt(float) 0x42e2f0 -> sqrtf 0x42e450 -> CRT.
- Degrees, /Od 0x7b1086: calls 0x46d330 -> 0x4625b0 -> 0x46d3e0 -> CRT. That is three levels, the same depth as square_root. Bare fabs (0x42dec0 -> 0x42e400) and fmod (0x57f040 -> 0x581cd0) are two levels. The wave-1 review missed this site and called it a <cmath> overload artifact.
- The three-level classification holds on other January owners. 0x4625b0 is called directly from "units\bipeds.c", where production spells bare acos (bipeds.c:3158). 0x46d330 is called from "ai\actor_moving.c" and "units\units.c", where production spells arccosine. So 0x46d330 is arccosine.

Constants: /Od computes t*{1.0,-0.7,1.4}*pi_f/30.0 and fmod(x, 6.2831855f), and arccosine(k)*360.0/6.2831855f. In float arithmetic, 360/2pi_f = 180/pi_f = 0x42652ee0, which is January's literal and 1 ULP below round(180/pi). pi_f/30, -0.7f*pi_f/30 and 1.4f*pi_f/30 give 3dd67750 / bd962051 / 3e162051, all equal to January's constants.

Gates on cand_final.c:
- gate.py: EXACT 4192 _collision_debug_render, exact 1/1. Same result against the regenerated split.
- objeq: identical to production (build/base) except 4 new COMDATs: _arccosine 16, _magnitude3d 48, _magnitude_squared3d 48, _square_root 16.
- Probes: every helper and constant variant was EXACT and object-IDENTICAL, except RADIANS_TO_DEGREES(), which was a sha residual and was rejected. There is no decoration: the read-back form and the attested form give identical bytes.

Storage fix: csplit regen with the edited symbols.json changes ONLY collision_debug.obj (833 objects compared byte for byte). object_audit against the regen split: 17/17 sections ok, 29 January symbols, 0 differ, OBJECT AUDIT: PASS.

Surplus: surplus_all found 22 candidate-only external definitions, problems 0, each IDENTICAL (raw bytes and flags) to January's selected copy. _arccosine matches actor_moving hash bbe13982. provider_link: 22/22 PASS in both orders. nodup_census: NODUP [] for the new helpers.

Scorer: objdiff 3.3.1 one-unit report gives code 4182/4182, data 29278/29278, and all four sections at 100.

Clean-up checks: fake_match_scan 0 leads. /W3: 0 warnings in the TU (14 header warnings, identical to the base).

Blast radius: the edit is .c-only, and the symbols.json row regenerates only this object.

## blockers
None for admission. The integrator still has to run the usual chain: split regen, full ninja, progress, stable sweep, parks, admission audit, fake scan, pytest.

## reopen_criteria
Reopen only if the integrator's full sweep shows a regression or a provider-link failure, or if the owner rejects hunk c (the /Od-attested constant expressions). If hunk c is rejected, fall back to collision_debug.helpers_only.patch, which is also EXACT and link-clean.

## task notes
1. The wave-1 review missed a second hand-expanded helper. `(real)acos(...)` is a hand-expanded real_math.h `arccosine`: the /Od call chain is three levels deep, and bipeds.c, actor_moving.c and units.c confirm that depth means arccosine. Both helper sites are now genuine calls and the function stays EXACT.
2. Hunk c is optional. It is included because the raw double and the read-back floats cannot have been typed by hand (house rule 20), and the /Od-attested expressions compile to identical bytes. It keeps the /Od `1.0f` rate factor, which parallels the -0.7f and 1.4f rates. It drops the redundant `(real)` cast on game_time_get(), since the plain spelling gives identical bytes.
3. The cube-vertex view cast and the add_vectors3d casts are unchanged: the add_vectors3d call is /Od-attested, and wave 1 strip-tested both casts as byte-inert. The collision-user push/pop is inline in the /Od build too, and collision_usage.h has no macro for it, so it is not a helper.
4. No tracked file was edited, and no ninja, configure or git-mutating command was run.
ledger: C:\halo-worktrees\claude-fifty-objects-20260925\scratch\w\collision_debug\LEDGER.md
