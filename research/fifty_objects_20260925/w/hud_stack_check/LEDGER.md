# hud_stack_check - hud_draw.h __inline check_stack_buffer packet (wave 4) - LEDGER

Slug: scratch/w/hud_stack_check/. Tree 16542e46 (git status clean at start). Units: hud_draw, hud_unit,
hud_nav_points, hud_weapon, motion_sensor (+ all hud_draw.h consumers for the declaration-count sweep).

## R0 reading
- WORKER_BRIEF 0-9, house rules, lane log held table (hud_unit row: this packet is the wave-4 reopen path;
  'hud_draw optimizer pragma' is a separate held item - not touched), review3_hud_unit LEDGER/REVIEW,
  hud_unit_audit LEDGER (lab F = lab-local __inline check_stack_buffer in hud_unit: 41/41 sections identical + ANY 48-B COMDAT),
  rejections: hud_draw + hud_unit `_fast_ftol` candidate-only-comdat-owner (hud_unit one to be retired by patch 04).
- hud_draw.h consumers (grep `#include "(interface/)?hud_draw.h"`, no header includes it): 23 TUs:
  hud_draw hud_messaging hud_nav_points hud_unit hud_weapon motion_sensor ui_widget lightning rasterizer_debug
  rasterizer_lights rasterizer_xbox rasterizer_xbox_debug rasterizer_xbox_dynavobgeom rasterizer_xbox_environment
  rasterizer_xbox_environment_fog rasterizer_xbox_models rasterizer_xbox_screen_effect rasterizer_xbox_shadows
  rasterizer_xbox_transparent_geometry rasterizer_xbox_water shader_transparent_generic_preprocessor render_contrails render_sprite.

## F1 COMDAT selection in build/split is a csplit CONVENTION, not January evidence
- comdat.py (slug, read-only COFF reader): build/split hud_draw.obj _get_return_eip sec1 0x10 NODUP, _check_stack_buffer sec2 0x30 NODUP.
- selcensus2.py over build/split/source: EVERY code COMDAT is NODUP (5,299 external + 2,262 static, 0 ANY);
  ANY appears only on data (9,686 string literals). The proven header inline _fast_ftol (actor_combat) is also NODUP there.
  => the split's selection type carries NO January information; it cannot discriminate header-inline vs plain .c.
  (The linked image does not preserve COMDAT selection.) Other evidence needed (map flags, inlining census, assert-line shape).

## F2 January sentinel census (sites2.py, build/split, read-only)
- 26 check sites, ALL identical inlined shape: `mov eax,7Fh; mov ecx,62626262h; L: movsx edx,ax; cmp [ebp+edx*4-X],ecx;
  jne HIT; dec eax; test ax,ax; jge L; or esi,-1; call _get_return_eip; cmp [ebp-Y],eax; ...` and out-of-line
  `HIT: mov esi,eax; jmp back` (= inlined-function return merge); both asserts of every site carry ONE line literal:
  hud_draw x14 (0x64,0xe4,0x38b,0x408,0xb5,0x137,0x3d9,0x55c,0x32e,0x358,0x1ac,0x25a,0x2ad,0x2ec),
  hud_unit update_local_player 0x201, render 0x3c9; hud_nav_points custom_render 0x2a3, update_local 0x1f0;
  hud_weapon grenade 0x3a2, crosshairs 0x4e2, update_local 0x19e, render_weapon 0x308, update_weapon 0xd4;
  motion_sensor motion_sensor_update 0x282, update_motion_sensor 0x2f4, render_motion_sensor 0x33e.
  => ONE shared macro (loop via inlined helper, then return-eip assert, then index assert at the invocation line).
- January _check_stack_buffer (hud_draw sec2, 0x30): `push ebp; mov ebp,esp; mov ecx,[ebp+8]; mov eax,7Fh; ...;
  cmp [ecx+edx*4],62626262h; ...; or ax,0FFFFh; pop ebp; ret` => signature `short check_stack_buffer(long *buffer)`
  (short return in ax, short index, dword compare). 0 relocations to it in any split object. cachebeta publics:
  _check_stack_buffer PUBLIC at 0xC08F0, _get_return_eip PUBLIC at 0xC08E0 (external linkage, consistent with non-static __inline).
- Sept-2001 map atlas 7eacac85: `_get_return_eip` 0xcff20 flag `i hud_draw.obj`; next listed symbol 0xcff60
  (_get_sprite_clip_rect): gap 0x40 = 0x10 get_return_eip + 0x30 check_stack_buffer (unlisted). 556 'i' entries
  in that map are all known header inlines (real_math distance3d/point_from_line3d..., objects.h bounding sphere, fast_ftol_C...).

## B0 production baseline sweep (sweep.py -> base/): 23 consumers
hud_draw 22/0/1unw, hud_messaging 36/0, hud_nav_points 31/1, hud_unit 22/0, hud_weapon 13/3, motion_sensor 16/3,
ui_widget 96/6, lightning 8/1, rasterizer_debug 12/0, rasterizer_lights 10/3, rasterizer_xbox 95/0, rasterizer_xbox_debug 18/0,
dynavobgeom 17/0, environment 42/2, environment_fog 26/1, models 12/2, screen_effect 13/0, shadows 18/0,
transparent_geometry 22/0, water 14/0, shader_transparent_generic_preprocessor 9/0, render_contrails 6/0, render_sprite 7/3.
- Canaries _bitmap_copy (bitmap_drawing.c), _rasterizer_frame_statistics_draw (rasterizer_frame_statistics.c),
  _get_edge_vertex (geometry.c): none of these TUs includes hud_draw.h (not in the 23) -> unaffected by construction.

## C0 harness control (ctrl/interface/hud_draw.h = verbatim copy; gtu_ctrl/ TU copies with ONLY the include line
   changed to "w/hud_stack_check/ctrl/interface/hud_draw.h"; gate copies land in scratch/ so the relative path resolves)
- sweep ctrl_out: identical gate rows to B0 for all 23; objcmp.py (every owned section both directions + anonymous
  sections except .debug$S + external/static symbol sets) base vs ctrl_out: 0 differences x 23. Harness is inert.

## H1 packet v1 (cand/interface/hud_draw.h + mkcand.py TUs; gtu_h1 -> h1_out)
- hypothesis (F2 + lab F of hud_unit_audit): January had `__inline short check_stack_buffer(long *buffer)` visible in the
  5 sentinel TUs and ONE shared macro; spelling the inline in hud_draw.h + `match_assert_stack_frame(file, line)` macro
  (block: corrupt_index = check_stack_buffer(stack_buffer); return-eip vassert; index vassert) reproduces all 26 sites.
- header v1 adds: enum {STACK_BUFFER_LENGTH=0x80, STACK_BUFFER_FILL=0x62626262} (moved from hud_draw.c/hud_unit.c TU-locals;
  hud_weapon's HUD_WEAPON_STACK_BUFFER_* replaced), the macro, section markers, and the __inline in '/* ---------- public code */'.
- result: the 5 sentinel objects: EVERY section byte+reloc identical to production (objcmp 0 diffs for hud_draw; hud_unit,
  hud_nav_points, hud_weapon, motion_sensor differ ONLY by the new 48-B _check_stack_buffer COMDAT (external)). Gate rows
  unchanged (hud_draw 22/0/1, hud_unit 22/0, hud_nav_points 31/1, hud_weapon 13/3, motion_sensor 16/3; residuals byte-identical).
- REGRESSION (declaration-count class): rasterizer_xbox_models __rasterizer_model_transparent_geometry_submit EXACT -> residual
  (12/2 -> 11/3). All other 17 non-user consumers: 0 section differences. -> bisect the added names.

## H1b bisect on rasterizer_xbox_models (hvariants.py; gate --fn row)
- enum only (+2 names): EXACT; macro only: EXACT; inline with literals only: residual; inline+enum: residual;
  full v1: residual; inline placed right after the enum: residual. => the __inline DEFINITION flips it (position inert).
## K-oracle (LAB ONLY, koracle.py: K dummy enum constants appended to hud_draw.h; never lands)
- models __rasterizer_model_transparent_geometry_submit, dummies only: K=0..3 E, 4 r, 5 E, 6..26 r, 27..30 E.
- with the inline + K dummies: K=0..17 r, 18..30 E  => the inline definition costs ~9 name units in this TU
  (18+9=27 = first exact of the upper window). Needed: net added names <=3 (or 5) before the function, or a header the TU does not include.

## L1 models stale include (lab labs/models_noinc.c = production models minus `#include "interface/hud_draw.h"`)
- rasterizer_xbox_models.c references NO hud_draw.h name (grep of all 19 declared names/constants: 0 hits; the include was
  added by 635bd83d when hud_draw.h still declared the ARGB converter, since moved to bitmap_color_conversion.h).
  14 of the 18 non-sentinel consumers use nothing from hud_draw.h (only hud_messaging, ui_widget, environment_fog do).
- result: compiles; 12/2 unchanged; objcmp vs production: 0 section differences (all 62 owned sections + anon + symbols).
  => dropping the dead include is byte-inert for models and removes models from the hud_draw.h blast radius.

## S1 emitted COMDAT identity + selection (h1_out objects)
- hud_draw cand: _check_stack_buffer sel=ANY (production NODUP), chk 0x4957c551 unchanged; hud_unit/hud_nav_points/hud_weapon/
  motion_sensor: one new ANY _check_stack_buffer each. All five section_infos_equal to January build/split hud_draw
  _check_stack_buffer (48 B, 0 relocs, sha c7049c3184d2ed3e). _get_return_eip stays NODUP (plain naked definition, not moved).
## S2 pair links (plink.py = provider_link.py's Link.Exe command with an explicit provider object)
- provider = candidate hud_draw.obj (ANY): hud_unit, hud_nav_points, hud_weapon, motion_sensor each PASS both orders;
  all five together forward and reverse PASS.
- CONTROL provider = production build/base hud_draw.obj (NODUP): hud_unit cand FAILS (LNK2005 both orders) => the hud_draw.c
  change (plain definition -> header inline) is REQUIRED in the same atomic packet; the probe discriminates.

## A1 hud_unit admission packet on the current tree (16542e46)
- 01R, 01Ralt, 02, 03: git apply --check CLEAN on 16542e46. 04 (hud_unit_audit) FAILS on 16542e46 (rejections file moved:
  the hud_unit entry is now lines 36-43, followed by source/models/models) -> regenerated as 04R (see patches/).
- No commits touched hud_unit.c/hud_draw.c/.h/hud_weapon.c/hud_nav_points.c/motion_sensor.c/rasterizer_xbox_models.c since
  954eebd2, so review3 cand_final_amended.c (= prod + 01R) and cand_minimal_amended.c (= prod + 01Ralt) are current.
- config/symbols.json full copy (config/symbols.json in slug): 4 in-place line edits (CRLF kept; `patch` would have LF-converted
  the whole file - not used). config_ctrl = verbatim copies. csplit control -> split_ctrl == build/split 833/833;
  csplit emulated (slug config) -> split_emu differs ONLY in source/interface/hud_unit.obj.
- cand01R/hud_unit.c = 01R + stack packet (mkcand --base-hud-unit), cand01Ralt/hud_unit.c = 01Ralt + stack packet:
  gate vs production split 20 exact + 2 names-only; gate_split vs split_emu 22/22 EXACT (both).
  object_audit_split (SPLIT_ROOT=split_emu) cand01R: every January-owned section ok, 30 January symbols 0 differ, PASS;
  surplus = .drectve, 5 pooled strings, 5 __real, _fast_ftol, _check_stack_buffer (new).
  objcmp vs review3 cand_final_amended.obj / cand_minimal_amended.obj: ONLY the new _check_stack_buffer COMDAT differs.
- surplus identity: _check_stack_buffer IDENTICAL to January hud_draw's; _fast_ftol IDENTICAL to actor_combat's (also for 01Ralt,
  and every surplus of hud_nav_points/hud_weapon/motion_sensor candidates is IDENTICAL to its January provider).
- provider_link.py (repo, build/base providers): all PASS except _check_stack_buffer vs PRODUCTION hud_draw (NODUP) -> LNK2005,
  expected until the packet's hud_draw.c/hud_draw.h change is built. provider_link_override.py (slug copy, provider hud_draw ->
  h1_out/hud_draw.obj): hud_unit 12/12 PASS both orders; hud_nav_points/hud_weapon/motion_sensor (--baseline) PASS.

## H2 FINAL packet v2 = v1 header + TU edits + `long stack_buffer[STACK_BUFFER_LENGTH]` in hud_nav_points(2)/motion_sensor(3)
   + rasterizer_xbox_models.c drops its dead `#include "interface/hud_draw.h"` (gtu_h2 -> h2_out)
- 23-consumer sweep: gate summaries identical to baseline; objcmp vs production: 0 differences in 19 objects (incl. models,
  every residual/parked body byte-identical => no park drift); hud_unit/hud_nav_points/hud_weapon/motion_sensor differ ONLY
  by the new ANY _check_stack_buffer (IDENTICAL to January's selected hud_draw copy).
- /W3 /Zs warning census (warn.py) for the 6 edited TUs: identical warning multisets (hud_draw 5, hud_unit 3, nav 4, weapon 4,
  motion 4, models 4 kinds) - no new warnings.
- canaries _bitmap_copy / _rasterizer_frame_statistics_draw / _get_edge_vertex: their TUs do not include hud_draw.h (not consumers).
## P-A fallback (measured, NOT the proposal): focused header (lab name interface/hud_draw_stack_check.h) with enum+macro+inline,
   included only by the 5 sentinel TUs, hud_draw.h and models untouched: the 5 objects are objcmp-identical to H2 (0 diffs);
   the 18 other consumers are untouched by construction. Viable if the owner rejects the models dead-include removal.

## A2 patches (patches/, LF-normalised like the repo's other patches; core.autocrlf=true)
- P1 hud_draw.h (SHARED HEADER, separate), P2 hud_draw.c, P3 hud_unit.c (standalone), P4 hud_nav_points.c, P5 hud_weapon.c,
  P6 motion_sensor.c, P7 rasterizer_xbox_models.c; 01RS = 01R + P3 in one hud_unit.c patch, 01RaltS = 01Ralt + P3;
  02 (unchanged, clean), 03 (unchanged, clean), 04R (regenerated for 16542e46). 01R/01Ralt copied verbatim.
- git apply --check on 16542e46: every patch CLEAN alone; P1..P7 together CLEAN; FULL set P1,P2,01RS,P4..P7,02,03,04R CLEAN;
  FULL-alt (01RaltS) CLEAN. `patch` replay on LF copies (verify/): every result IDENTICAL to the slug candidate files;
  01R then P3 sequentially also reproduces cand01R/hud_unit.c.
- fake_match_scan: candidates carry exactly the production leads (hud_draw.c get_return_eip naked/asm, pre-existing,
  unchanged); hud_unit 0 leads.
## A3 strict emulation (objdiff 3.3.1 single-unit projects + repo semantic/admission pipeline; strict_emulate.py copy)
- objproj_final (split_emu target, cand01R obj, complete=True, slug config with 04R): strict 22/22 fns, 6176/6176 code,
  152/152 data; rejections [] revoked []; classify candidates [] contradicted [] rejected []. Same for 01Ralt.
- CONTROL with production rejections file (hud_unit _fast_ftol veto kept): 'contradicted' -> 04R is required.

## A4 object audits (repo object_audit.py, production split)
- hud_draw: audit text IDENTICAL for production vs candidate (pre-existing FAIL (2): unwritten _fast_ftol_C / surplus _fast_ftol,
  _square_root - unchanged; the NODUP->ANY selection change of _check_stack_buffer is invisible to audit and scorer).
- hud_nav_points / hud_weapon / motion_sensor: audit text differs ONLY by the added surplus line `+.text _check_stack_buffer 48`.
- hud_unit: see A1 (PASS against split_emu).
## Notes / not done
- get_return_eip stays a plain naked hud_draw.c function with its hud_draw.h prototype (byte-identical everywhere; moving it
  to a header __inline, which the Sept-2001 'i' flag suggests, would only add 4 surplus COMDATs with no credit) - not probed.
- git status of the worktree: clean (no tracked file touched).

## A5 subset measurement (P1 header only; hud_nav_points/hud_weapon/motion_sensor TUs left as production; gtu_sub -> sub_out)
- hud_nav_points, motion_sensor: 0 section differences. hud_weapon: residual _render_weapon_hud BYTES DRIFT (same row class)
  - declaration-count effect of the header's names on an unchanged TU; with P5 (full set, H2) hud_weapon is objcmp-identical.
  => P4-P6 are NOT optional hygiene: apply the full set P1,P2,(P3|01RS),P4,P5,P6,P7 atomically.

## RESULT
- hud_unit: OBJECT_COMPLETE_CANDIDATE with the atomic packet P1+P2+P4+P5+P6+P7 + 01RS (=01R+P3; or 01RaltS) + 02 + 03 + 04R.
  BLOCKER 2 (hand-copied check_stack_buffer with invented gotos) is resolved by the genuine header inline + shared macro;
  BLOCKER 1 is 01R's. gate_split 22/22, object_audit PASS vs emulated split, surplus _fast_ftol + _check_stack_buffer both
  IDENTICAL to January's selected copies, pair links PASS both orders against the packet's hud_draw.obj, strict emulation 22/22.
- hud_draw / hud_nav_points / hud_weapon / motion_sensor: all functions byte-identical (exact rows unchanged, residual/parked
  bodies byte-identical); the goto hand copies and TU-local duplicate macros/constants are gone (future admission hygiene).
- 18 other hud_draw.h consumers: 0 section differences with P7 (models dead include) applied.
- Reopen/owner points: (1) P7 is load-bearing only relative to P1 (the inline's ~9 declared names flip
  __rasterizer_model_transparent_geometry_submit); the include is dead (models uses no hud_draw.h name) and dropping it is
  byte-inert; fallback P-A = focused header included only by the 5 sentinel TUs (measured identical). (2) The macro name
  match_assert_stack_frame is descriptive (Bungie's is lost: /Od build has no sentinel, HCEX none). (3) get_return_eip left as is.
- Integrator: after applying, full sweep of all 23 hud_draw.h consumers + parks; accumulation with other wave-4 header packets
  must be re-measured on the live tree (isolated tests do not compose).
