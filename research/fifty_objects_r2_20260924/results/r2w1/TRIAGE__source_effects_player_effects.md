# source/effects/player_effects

## decision
RETIRE

## est_probability
0.02

## route
None for functions. Zero-credit hygiene is available: symbols.json `"static": true` on 8 rows (_effect_scale_factor, _effect_scale_value, _get_shake_matrix, _player_effect_globals, _player_effect_update_camera_impulse, _player_effect_update_camera_shake, _player_effect_update_screen_flash, _render_screen_flash_type_map), then a csplit-only regen. pdb_storage shows split 2 / ours 3 / PDB-public False, so our static storage is already right. Optionally, round-1 production.patch (camera-matrix /Od body, park re-baseline) still applies.

## why_new
Nothing new for the three residuals. The storage finding is first-party (cachebeta publics), but it is admission hygiene only, since three functions stay residual.

## prior_negatives_checked
Round-1 wave4 player_effects. add_continuous_effect is EXACT only with the Marathon PIN spelling in cseries.h, which loses 5 strict functions board-wide; s1/s2 variants also lose; ac2-ac9, D=0..16 and N=1..32 were inert. camera_effect_matrix: cm2/cm2b/cm2e, M8 at 4 scopes, pointer locals and /Od order all remain; cross_product3d leaf order is coupled to set_real_point3d. update_camera_impulse: i,k,j vs k,j,i dot order; u1/u2/u12, M8 D=0..8, argument, chain and triple_product3d were all inert. Ledgers opus5_next150_n3a, 250k_w3c and private_helpers_reconciliation were read.

## notes
The PIN parenthesisation key is a header-text/paren-count family. A TU-local paren respelling would be decoration-only parentheses (fake), so no admissible lever exists.