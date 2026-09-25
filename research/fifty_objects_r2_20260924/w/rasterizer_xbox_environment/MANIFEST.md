# MANIFEST: source/rasterizer/xbox/rasterizer_xbox_environment (round 2, r2w1 ATTACK)

Base: f6d00a8c (worktree HEAD b7c1b7d9 = scratch-only commit on top). Tip: no commits (worker; integrator applies).
Outcome: FUZZY_IMPROVED (zero exact credit). Object NOT admissible (spot residual + lightmap_draw owner hold).

## Function table (target vs candidate, gate.py strict)

| function | January | production f6d00a8c | candidate p1 (cand_spot_odorder.c) |
|---|---|---|---|
| _rasterizer_environment_specular_spot_light_begin | 960 / 64 relocs / e0331f6b94a74f9c | 944 / 64 / d103843e (frame 0x74) | 960 / 64 / a0b9f9f4383b30bc (frame 0x84, 248 = 248 insns) |
| __rasterizer_environment_lightmap_draw | 4016 / 248 | 3984 / 246 | unchanged (owner hold, see below) |
| other 42 functions | - | EXACT | EXACT (identical exact-name set, md5 d9c1bafe of sorted names; no swaps) |

Gate rows: cand/cand_spot_odorder.obj -> `== exact 42 residual 2`; point_from_line3d guard pass.
Real alndiff hunks for spot: production parked body 28 (n3a s1 shape) ; p1 = 11 (0xf3..0x113 x87 j/k load order
= M8 count; 0x12d..0x154 + 0x1b5/0x1bd register rotation).

## Source evidence per changed construct (p1)
- `real_matrix4x3 light_matrix` : /Od fn 0x7e4590 RTC descriptor 0x7e4968 ([ebp-0x40], 52, "light_matrix");
  HCEX.pdb local `struct real_matrix4x3 light_matrix`; January frame sub esp,0x84 with unreferenced scale (-0x34)
  and position (-0xc..-0x4) slots.
- separate `cross_product3d(&light->forward, &light->up, &light_matrix.left); normalize3d(&light_matrix.left);`
  : /Od 0x7e46b1..0x7e46d4 (two calls, return of cross discarded, fresh lea for normalize).
- `inner_radius = radius * 0.5f; cone_scale = 1.0f / (radius - inner_radius); ... .l = -cone_scale * inner_radius`
  : /Od [ebp-0x50] named local (0x7e46f5..0x7e4718, 0x7e4907..0x7e4913). Scalar NAME is descriptive (HCEX lists
  only light_index and light_matrix; enregistered scalars are absent).
- declaration order light, light_matrix, gel_bitmap_index, radius, inner_radius, cone_scale = /Od frame order
  (-8, -0x40, -0x48, -0x4c, -0x50, -0x54); vertex_constants is Xbox-only (DX9 writes a global) and placed last.
  Declaration order is byte-inert (measured).

## Proof of the residual (oracle only, never landing)
- p1 + 2 initialised dead longs (M8 instrument) + ONE forced local-allocator cursor store under dbg32c (C2
  0x1075680d, 7th local TAKE = the normalize3d argument lea, cursor kept at eax) => spot STRICT EXACT
  (960/64, full normalized sha256 e0331f6b...4ab7af == January). lab/oracle/forcecur.py, forced_7_10894708.obj.
- C2 local allocator decoded (LEDGER R15): block-local temps are taken round-robin from the entry after the last
  TAKEN register (cursor [0x1088b96c] over {eax,ecx,edx}); pre-assigned (global/fixed) operands are pinned without
  moving the cursor. January's `&light_matrix.left` argument was pinned, ours is a TAKE.

## Data / symbols / surplus
- Data: no data gap in the hints packet (data_gap 0); not re-audited beyond object_audit (sections ok).
- object_audit (cand_full = p1+p2): FAIL(10): 2 code size/sha rows (spot, lightmap_draw) + 8 storage rows
  (6 XDK wrappers, spot, permutation index: split external / ours static; cachebeta publics say file-static for
  all 8 -> symbols.json static:true needed; lines 5722-5727, 5755, 22727 of config/symbols.json).
- pdb_storage (production): same 8 disagreements.
- Surplus identity: production emits _normalize3d DIFFERENT from January's selected action_charge copy (Rule 6);
  with p2 (REAL_MATH_EXTERNAL_NORMALIZE3D) the remaining surplus COMDATs (_cross_product3d,
  _real_rgb_color_to_pixel32, _real_alpha_to_pixel32) are all section_infos_equal to January's selected copies.
  provider_link not run (object not admissible).
- TU-view debt (triage P15: 17 TU-local struct views, 4 consumer-local externs) untouched.

## House-rule review (p1)
real types, one declaration per line, explicit return kept, no casts, no fillers, no pragmas/volatile/asm,
no new helper emission (guard passes), names: light_matrix (first-party), inner_radius (descriptive).

## Credit
Meaningful/padded code: 0 (no new exact function). Data: 0. Admission: none.

## Held / reopen
- spot: needs a first-party-attested construct that (1) adds +2 (mod 8) IL symbols (lab/m8count.py measures c)
  and (2) makes the normalize3d argument a pinned operand (or adds two vanishing local TAKEs between it and the
  definition reload). Verify with lab/oracle/forcecur.py semantics (unforced build must reproduce sha e0331f6b).
- lightmap_draw: astra 0920 intensity.c strict exact but HELD (per-arm stage-0 filter blocks lack a witness;
  Oct-2001 betaP identical merged tail) - OWNER.
- p2 + symbols.json storage edits: prerequisites only; apply together, and only with an emulated-split audit.

## Tests run
gate.py --all on production, every lab variant (306 harness compiles + ~35 dbg32c-traced compiles), cand_spot/cand_spot_odorder/cand_full/
cand_prereq_only; alndiff/real-hunk harness; object_audit; pdb_storage; surplus identity (local re-implementation
for a candidate obj); git apply --check for p1, p2 and p1+p2 at the worktree (source identical to f6d00a8c).
