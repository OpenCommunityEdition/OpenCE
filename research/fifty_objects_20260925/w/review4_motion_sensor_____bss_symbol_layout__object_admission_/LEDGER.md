# review4 motion_sensor .bss symbol layout (object admission) - reviewer ledger

Base 16542e46, tree clean at start (git status empty). Scope: FIRST hunk of scratch/w/motion_sensor/production.patch only
(motion_sensor_globals = NULL, sweep_theta = 0.f). The render_motion_sensor hunks are NOT under review here.

## R0 read
- brief 0-9, house rules, lane log held table (models .bss / rasterizer_xbox .bss / hs_scenario data split held items are
  about NAMES or first-party static-local form; none concerns initialisers of already-named file statics).
- motion_sensor ledgers (jonas 20260829, opus5 w1/w3c/n3a/n4) - no recorded .bss negative. parked.json: 3 function parks
  (motion_sensor_update/render/update) unaffected. No object_admission_rejections / semantic_data_matches entry for the unit.
- worker LEDGER.md read fully.

## R1 base vs bss-only candidate (base.c = working-tree copy, cand_bss.c = base + hunk 1 only)
- gate --all: identical output, exact 16 residual 3 (motion_sensor_update/render_motion_sensor/update_motion_sensor residual as before).
- objcmp.py (coff_compare section_infos_equal over all 75 sections): 0 section diffs (only .debug$S path length); symbol
  diffs ONLY _next_vertex_index 8->0 and _sweep_theta 0->8. No code/reloc change anywhere.
- object_audit: base 44 syms / 2 differ (next_vertex_index 0/8, sweep_theta 8/0); cand 44 / 0 differ. Remaining FAIL(3)
  are the three pre-existing residual .text functions. .bss section 13/13 ok both.

## R2 lab: is EACH initialiser load-bearing, and is the declaration order January's? (lab only)
Hypothesis (VC7 .bss law: uninit statics first in name-hash order, zero-initialised in declaration order): January's
next_vertex_index (public, initialised - it is in the object's own .bss, not COMMON) at 0 means NO uninitialised static
precedes it, so both file statics carried initialisers, and 0/4/8/c = declaration order nvi, globals, theta, debug.
- la (only globals = NULL): sweep_theta@0, nvi@4, globals@8 -> 3 differ.
- lb (only sweep_theta = 0.f): globals@0, nvi@4 -> 2 differ.
- lc (both `= {0}` spelling): 0 differ (spelling of the zero initialiser is inert, as the law says).
- ld (both initialised, theta declared before globals): globals@8/theta@4 -> 2 differ => current declaration order is January's.
All four: gate exact 16 residual 3. Law confirmed in this TU; both initialisers necessary and sufficient.

## R3 storage / type / name evidence
- pdb_storage (production): 44 split symbols, 0 disagreements. cachebeta publics: _next_vertex_index (0x453AC8) and
  _debug_motion_sensor_draw_all_units (0x453AD4) public; _motion_sensor_globals / _sweep_theta absent => file statics. The
  hunk keeps `static` (objcmp: storage classes unchanged 3/3).
- HCEX.pdb: `motion_sensor_globals` File Static, `struct motion_sensor_globals_definition *`; `sweep_theta` File Static,
  float (= real). Names/types/storage of the touched declarations match. (HCEX `next_vertex_index` is `short`, ours `long`:
  NOT touched by this hunk, unreferenced in the TU, byte-inert under the 4-byte static padding law - observation only.)
- /Od 2020 build: motion_sensor_globals @0xc83910, sweep_theta @0xc83914 (adjacent, declaration order), but that modern
  compiler interleaves blip_player_index @0xc8390c / debug @0xc838f6, so its data layout is NOT evidence for or against
  VC7 initialisers (inconclusive; not relied on).

## R4 January attribution + relocation identity
- config/contribs.json: .bss contribution 0x453AC8 size 13 (module 267) = this object's own .bss; both public globals
  (nvi @0, debug @0xc) are inside it => they are initialised definitions (not COMMON), so no uninitialised static can
  precede nvi under the law.
- January relocs (tinfo): motion_sensor_initialize stores game_state_malloc result to .bss+4 (pointer), tick reads/writes
  .bss+8 (float) => symbols.json attribution of globals@4 / sweep_theta@8 is consistent with usage; no attribution defect.
- Resolved (section-owner+offset) relocation targets of the 16 exact functions: base differs from January in 6 functions,
  cand_bss in 4; the 2 removed (motion_sensor_initialize, _for_new_map) and every .bss-targeted reloc in tick/others now
  resolve identically (.bss:_next_vertex_index:+4/+8). The 4 remaining are pre-existing folded-literal
  (__real@/??_C@ symbol vs defined .rdata) differences, unrelated to .bss.

## R5 surplus / link / data
- provider_link cand_bss --baseline=base.obj: no new surplus (PASS, nothing to link); full provider_link: 14 surplus, all
  PASS. surplus_identity (production obj == base compile, 0 section diffs): 9 code COMDATs, 0 not identical.
- build/report.json: motion_sensor data 501/501 = 100% already => zero data credit, as the worker said.
- no config/semantic_data_matches.json, object_admission_rejections.json entry for the unit; parks untouched.

## R6 patch application
- bss_only.patch (= first hunk of the worker's production.patch, verbatim, CRLF body): git apply --check OK at 16542e46;
  `patch -o` onto the working-tree copy reproduces cand_bss.c byte-for-byte.
- whole production.patch also applies and equals worker cand_prod.c; gate 16/3 and 44/0 symbols (render hunks outside
  this review's scope).

## R7 verdict
APPROVE hunk 1 (bss_only.patch). Tree clean at end (git status empty). See REVIEW.md.
