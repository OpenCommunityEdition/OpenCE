# MANIFEST review_critic3_players (source/game/players, critic_3 patch 12)

- Base/tip: 5d3ca708 (clean). Patch reviewed: scratch/w/critic_3/patches/12_players.patch (unchanged; no amendment).
- Change: static `unit_create_starting_weapon` -> `create_weapon` (definition, 2 calls in player_add_equipment, symbol-listing
  comment) + config/symbols.json row 2595 `_create_weapon` ("static": true kept, in-place line edit).
- Evidence: Aug-2001 + Sept-2001 beta maps `_create_weapon`, Sept-2001 retail map `@create_weapon@8` (static section, same
  neighbours, size 0x70); Sept xbe body instruction-identical (septcmp.txt); sole callers player_add_equipment x2
  (sept_callers.txt); HCEX.pdb static create_weapon in game/players.c (hcex_create_weapon.txt); /Od parameter order + RTC
  `placement_data` (od_create_weapon.txt). Atlas `unit_create_starting_weapon` is exact-tier C++ propagation with reversed
  parameter order, contradicted by the Aug map at the same address.
- Target vs candidate: _create_weapon 112 B, 3 relocs, EXACT (was EXACT as _unit_create_starting_weapon). Unit 68/2 both.
- Data: all sections identical (objcmp split + compiled); objdiff data 5349/5349 both.
- Symbols/storage: object_audit 118 January symbols 0 differ both; pdb_storage unchanged (static = absent from publics).
- Surplus: surplus_identity 13/0 both; provider_link PASS both orders both.
- House rules: authentic first-party name; no new construct; no prototype needed (static defined before use).
- Credit: ZERO (byte-inert rename). Held items: none. Tests: git apply --check, csplit emulation, gate, objcmp, battery,
  objdiff 3.3.1. Not run: ninja, pytest, board-wide sweep (integrator).
