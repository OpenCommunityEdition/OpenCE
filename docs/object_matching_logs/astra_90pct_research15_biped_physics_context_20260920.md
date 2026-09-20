> Research-only packet15: zero production change and zero exact credit. Validated production remains wave13.

# Packet 15: bounded biped physics context

**Closed without a new source premise. Zero source shapes, C compiler calls, source/header/configuration changes, or exact credit. Evidence released.**

Baseline is `4e8d941af217b5ef43bc23a1afc4511881a320cf`. Current atlas `_biped_update_physics` is 5,366 meaningful bytes, written and unaccepted, with the latest recorded similarity 88.408554%. The old baseline atlas fields predate the wave 3 diagnostic restoration and were not used as current measurements.

The fresh read of the current production object confirms:

| Measurement | January | Current |
| --- | --- | --- |
| Padded bytes | 5,376 | 5,376 |
| Relocations | 124 | 124 |
| Decoded instructions | 1,623 | 1,623 |
| Direct relocated calls | 57 | 57 |
| Normalized SHA-256 | `f202fb20dd4d8fba5d969bf881249cedf6de9678d56d207ada9f048ba0f44098` | `bf945c9301d7848f33e0c5a7765f3b3d820380a2310a576d768b2e7fd0968b64` |

The ordered direct-call identities and symbolic relocation multisets agree. Current source SHA-256 is `74b5fe6826d9e8fb94ee10843b0a45096c4cdf845508633d82f18e2d671cdd47`. These fingerprints are unchanged from the already reviewed wave 3 physics restoration. Equal call sequences do not prove that all arguments are equal.

## Narrowed argument and output check

The current source and the selected January/current call windows were inspected at these boundaries:

- `_collision_move_pill`, January `+0x8b6`, current `+0x899`: the sweep still uses distinct input position/velocity and clipped outputs, height/width from the same physics fields, and a consumed collision count. The wave 3 review already authenticates the input initialization, crouch adjustment and diagnostic capture after the branch merge.
- `_distance_squared3d`, January `+0xbdc`, current `+0xbc1`: the x87 result is compared with the saved best distance before the support candidate is updated. No new returned-value type or missing consumption was exposed.
- `_collision_get_features_in_sphere`, January `+0x1342`, current `+0x136f`: both consume the byte boolean result with `test al, al` before the standing-vector test. Source supplies the standing sphere, zero expansion, collision radius, biped index and feature output. The visible differences are scheduling/register/frame choices; no new field or scalar mismatch was identified.
- `_collision_features_test_vector`, January `+0x1389`, current `+0x13b6`: both consume `AL` and conditionally set the same cannot-stand output bit. No new output-local width or absent consumption was identified.

This is a bounded manual check, not a full symbolic proof of every call argument in the function. No fresh PE/Ghidra/DIA extraction was warranted after the current comparison failed to identify a new concrete discrepancy.

## Existing boundary retained

The prior September 14 W2/W3 evidence records January indexing `collisions[best_collision_index]` while the index can still be `NONE`. Current source retains the required `best_collision_index != NONE &&` contact guard. That known guard difference independently blocks complete January equality and was neither removed nor probed.

The September 14 wave A/D and W2/W3 histories, plus the wave 3 producer and independent reports, were read before this comparison. The missing debug block has already been restored and reviewed; its 12 relocations are present. Existing layout, flag-width, x87-order, aggregate-lifetime, helper-emission, and moving-function hypotheses were not repeated. The moving function was not probed or changed.

Existing supplied `/Od` function `0x8c9b80` and its 39 RTC aggregate records can resolve a concrete pointer/aggregate identity when a mismatch is found. They cannot by themselves recover January's scalar lifetime, original lexical declarations, or same source revision. No general compiler law or original-source recovery is claimed.

## Evidence

- `current_audit.py` and `current.json`: current source/object fingerprints, ordered call comparison, all selected call windows and explicit limits.
- `split.asm`, `base.asm`, `jan-current.alndiff`: fresh January/current disassembly and unmodified alignment for this function only.
- Prior reports: `bipeds_obj_opus5_100k_20260914.md`, `bipeds_obj_opus5_100k_waveD_20260914.md`, `bipeds_obj_opus5_150k_w2_20260914.md`, `bipeds_obj_opus5_150k_w3_20260914.md`, `astra_90pct_wave3_bipeds_packet_20260920.md`, and `astra_90pct_wave3_bipeds_independent_review_20260920.md` under `docs/object_matching_logs`.

No new candidate exists, so no new sibling/owner/data/COMMON/point preservation gate is claimed or needed. The lane ends at the existing guard boundary and the absence of a newly authenticated operand/output correction.
