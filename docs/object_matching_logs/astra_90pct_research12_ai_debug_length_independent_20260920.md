> Research-only packet12: zero retained production changes and zero admitted exact credit. Latest validated production remains wave9.

# AI debug output-vector length: independent appendix

**PASS for primary authenticity and preservation; HOLD nonexact and unlanded.** This is a separate review of the second supported shape. The earlier two-color review remains frozen.

January's `_ai_debug_render_actor+0x49e0` pushes `0x3f800000` (1.0f) as the fourth argument of `_render_debug_vector+0x4a24`. The preceding named purple pointer is the fifth argument; the remaining pushes supply the initialized throttle vector, point, and TRUE, followed by 20-byte cleanup. Current code pushes `0x3fcccccd` (1.6f) at `+0x49e4` for the corresponding call at `+0x4a28`. No intervening branch or write can replace this immediate.

The branch is specifically the output-throttle rendering block for a biped without a parent: it obtains facing, chooses flying-axis or planar throttle construction, then offsets the actor body point by `global_up3d * 0.1f`. The next block tests charge-decision debugging. The earlier pink movement-throttle vector also uses 1.6f and is untouched. This distinguishes the intended consumer from neighboring calls.

Independent direct PE reads confirm the later block's `MOVSS` of the 1.0f constant at `0x4a2352`, its outgoing-stack store at `0x4a235a`, and matching five arguments at `0x4a236f`. RTC identifies the passed 12-byte `throttle_vector` at -4344 and `p0` at -4264. Dereferencing the later purple pointer yields exactly the bytes of January's named purple color-table entry. January establishes the desired value independently; identical cross-build source revision is not claimed.

The same bounded history corpus used by the earlier independent review contains 15 reachable git versions and 27 archived/direct scratch probes. Thirteen git versions and all 27 probes contain the named purple-throttle call with 1.6f. Two earlier versions lack the named pattern and are uncovered, rather than evidence of absence. The focused `ai_debug_obj*` histories discuss allocator, scope, switch and helper probes; no previous correction of this specific scalar consumer was located. This is a distinct observed source mistake, not a new generic compiler law.

The second candidate differs from the reviewed first candidate by exactly this one literal. Its SHA-256 is `664d27c941d72efd6a0908266acede32e0ef4da7c2d209e924d778ff5efe9d84`. Direct object review confirms all 57 inherited exact controls, 78 code owners, 491 named runtime owners, 485 other runtime sections, 408 nondebug noncode sections, COMMON, section flags/COMDAT selection and the inherited 48-byte point helper are preserved. Only `_ai_debug_render_actor` changes. Production source and base object remain unchanged; the fake scan reports no findings.

The result remains **25,040 bytes / 1,905 relocations**, normalized SHA-256 `41cbd64afe98de003ff4c497725f4b29c1c8f78ec893359b11f3934cdbd1a45b`, versus January's 24,976-byte owner. Both worker shapes remain unlanded, with zero exact credit. I ran no C compiler. `review.py` and `review.json` reproduce and retain the independent evidence. Evidence is **RELEASED**; no third shape is proposed.
