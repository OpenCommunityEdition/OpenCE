# Adversarial review: source/saved games/player_profile (worker scratch/w/player_profile, base 954eebd2)

Verdict: **APPROVE packet A (production.patch)** at zero code credit. `production_defined_full.patch` is an
acceptable optional alternative. Packet B (owner-gated) is **not landable**: its evidence is verified and it
waits on owner rulings R1-R3.

## Independently re-run

| Check | Result |
|---|---|
| Production gate (954eebd2) | 19 exact / 3 residual: create_default [size 336!=288, sha], new [relocs 18!=19, sha], write_thread [size 384!=352, sha] |
| `git apply --check` on the current tree | production, production_defined_full and production_owner_gated all OK. Each patch, applied to the HEAD blobs, reproduces final_min.c, final_def.c or final_og.c exactly. The two incremental owner-gated patches stack to final_og.c. |
| symbols.json copy | 1-line in-place diff at line 22790 (`_profile_color_table`, `"static": true`). |
| Own csplit simulation (my slug) | Control regen with the unedited symbols.json equals build/split byte-for-byte. Full regen with the edit (833 objects): only player_profile.obj differs, and only in `_player_profile_primary_colors` (storage 2) becoming `_profile_color_table` (storage 3), plus the get_rgb_color relocation name. |
| Gate against the current split | final_min 18/4, final_def 18/4, final_og 21/1. The extra residual is get_rgb_color [reloc-identity]; the split has not been regenerated. |
| Gate against the simulated split (own simgate.py, gate.py logic) | final_min 19/3: create_default and new shas unchanged (3ce0cc6f, 4df7452a); write_thread 384/27 bbb652e0. final_def 19/3 (320/14 c4f4ba33, 464/18 4f42ddba, 384/27 2d428375). **final_og 22/22 EXACT.** |
| object_audit against the simulated split | final_og PASS (51 symbols, 0 differ, all sections ok). min/def FAIL(3): only the 3 residual .text sections; 0 symbol differences. |
| Surplus | Identical in base, min, def and og: .drectve, 4 pooled literals, 3 D3D tables, `__real@3b808081`. No new surplus. provider_link PASS for og, min and def. surplus_identity: 0 code COMDATs. |
| objdiff 3.3.1 against the simulated split | base 95.341644; min 95.34539 (wt 88.451324); def 95.71412 (wt 87.56637, new 81.26, cd 82.65909); og 100 (code 4119/4119, data 1644/1644). |
| /W3 /Zs | Same 12 header-only warnings in base, min, def and og; 0 TU-local. |
| fake_match_scan | 0 leads. |
| pdb_storage (production) | 1 disagreement: `_player_profile_primary_colors` is external but absent from cachebeta publics. Packet A fixes it. |

## Evidence verified

- **HCEX (DIA2Dump, my own queries).**
  - `profile_color_table`: DataKind 5 (File Static), `long[0x12]`, in the player_profile compiland.
  - No `player_profile_primary_colors` symbol exists.
  - `union player_profile_block { unsigned char data[0x200]; struct player_profile_internal profile_block; }`.
  - `player_profile_new` locals: `profile_file` and `block` (the union).
  - `_player_profile_globals` members are default_profile, thread_input, thread and initialized. There is no pad member.
- **cachebeta publics.** There are no publics between 0x31620C and 0x316AA8, so 0x316880 is file-static. `_player_profile_globals` is public.
- **/Od build (odbuild re-run).** The listings of 0x8596d0, 0x85d0c0 and 0x85acf0 are identical to the worker's.
  - In 0x8596d0, `if (failed) delete` is at 0x8597f2, after the open if/else.
  - R1-R3 are visible in the /Od listings exactly as the worker described.
- **January bytes.**
  - Uninitialised block with production's in-arm `if (failed)`: [sha], one raw byte at +0x10b (09 vs 21). This is the byte from the 20260909 rejection.
  - Uninitialised block with the /Od placement: EXACT.
  - Result: January's bytes and the /Od build both attest the placement.
  - In `new`, January's failure path is delete, then `or esi,-1`, then close(&file, esi), then return esi. R3 is genuine.

## House-rule review of packet A

- **Rename and `static`.** The name and File Static storage come from HCEX, and cachebeta confirms the storage. The table is non-const, as HCEX types it, and it lives in .data. No other TU, header or split object references it.
- **`byte pad[3]` removal.** The member is invented and HCEX has none. Removing it is byte-inert: the size stays 0x6C and the verify typedefs still compile.
- **`if (failed)` move.**
  - The source stays defined: `failed` can only become TRUE inside the open arm.
  - Two first-party sources attest the placement.
  - It adds no block scope or declaration.
  - It is not a decoration, so the strip test does not apply.
- No header, prototype, `float`, helper or config changes.

## Integrator notes for A

1. Apply production.patch. It covers the .c file and symbols.json line 22790, edited in place.
2. Regenerate the split with csplit. This is **mandatory**: without it `_player_profile_get_rgb_color` drops to [reloc-identity].
3. Run the full ninja build and the stable sweep.
4. Re-baseline only the `_player_profile_write_thread_proc@4` park: base 384 / 27 relocs / sha bbb652e083aadc92e51e4445f93d49d41941e7c53f50cc1883658c355aa579cb / objdiff 88.451324.

The other two parks stay valid (shas unchanged). Code credit is 0 and data is unchanged at 1644/1644.

## Advisory (non-blocking)

- **Globals type names.** The globals type tag and members are descriptive, not HCEX's names:
  - HCEX spells them `struct _player_profile_globals` and `struct player_profile_write_internal_input { long index; ... }`, with the member `thread_input`.
  - Adopting them is byte-inert. I measured min_hcexnames: 19/3 with identical shas, and every section identical except .debug$S.
  - Optional patch: hcex_names_over_production.patch (applies over production.patch).
  - If it lands before B, owner_gated_over_production.patch must be regenerated, because its context lines change.
  - Recommend adopting these names before any whole-object admission.
- **Union adoption in final_def is partial.** player_profile_read and get_from_path keep `byte block[512]`, but HCEX says read uses the union. This is a possible follow-up.
- **B's R3 BUG comment is incomplete.** It should also say that the file is deleted while still open.
- **B needs owner rulings R1-R3.** It belongs to the same authentic-bug class as king, glow, fpw-B and dead_camera, and it reverses the 20260909 rejection. The new /Od evidence is real, but the owner decides. R1's /Od attestation covers only the missing initialiser: in the later format the block is fully overwritten. January's 444 serialized stack bytes follow from its 0x200 format size, which the worker disclosed.
