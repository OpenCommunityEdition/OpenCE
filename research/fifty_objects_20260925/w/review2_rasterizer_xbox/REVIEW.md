# REVIEW: rasterizer_xbox OBJECT_COMPLETE_CANDIDATE (wave 2, base 931ed8dc)

Reviewer slug: scratch/w/review2_rasterizer_xbox/. Worker: scratch/w/rasterizer_xbox_audit/.
All measurements below were re-run with the reviewer's own tooling. The full log is in LEDGER.md.

## Verdict

**approve = false.** The cause is one owner-policy question, not a technical defect.

Every technical claim (a)-(i) reproduces. The whole-object admission, however, depends on adding
**9 unauthenticated (descriptive) .bss static names** to symbols.json. The owner has held that
class twice before:

- the env_fog and models .bss splits (2026-09-15 ruling);
- path_obstacle_avoidance object admission (canonical 2026-09-24: its "BSS relabel lacks
  authenticated private identities").

The only counter-precedent is collision_debug (5671843f). Its descriptive split landed for function
credit only, and that object was not marked Matching. Put the question to the owner with this
evidence. On a yes, the 4 patches can be applied exactly as provided.

The rasterizer_xbox_debug follow-up is independent, storage-only and verified: **approve**.

## Mechanical re-verification (all PASS)

| Check | Result |
|---|---|
| `git apply --check` at 931ed8dc | All 4 patches pass, individually and together, plus the follow-up. The post-images (applied outside the repo) are cmp-identical to cand_L.c, hdr/, config/symbols.json and config/config.json. |
| csplit with the patched config (split_R) | Of 833 objects, only rasterizer_xbox.obj differs. The control csplit of the unmodified config equals build/split, so build/split is current. |
| gate | cand_L plus the patched header, against split_R: **95/95 EXACT**; the _point_from_line3d guard passes. A shadow-header `#error` control proves the patched header was the one used. With the unpatched header the bytes are identical. |
| object_audit (split override) | **PASS**. 216/216 January symbols, all sections ok: .bss 2230 (8-align), .data 6, blend .rdata 108 (4-align). Production scores FAIL(22). |
| Storage truth: cachebeta publics, checked BY ADDRESS | No public in 0x45E028..0x45E8CF, at 0x45E8D4, in .rdata 0x28BFAC..0x28C017, at .data 0x30CEEC, at the filthy function 0x146650, or at any of the 20 wrapper offsets. The only neighbours belong to other objects, plus _global_d3d_device. Note: object_audit storage rows are circular (the split's storage class comes from symbols.json), so this address check is the real storage proof. |
| Surplus | 15 candidate-only externals: 3 code COMDATs, 3 SDK tables, 9 literals. All are IDENTICAL to January's selected definers, and there is no static surplus. provider_link: **PASS 15/15**. |
| objdiff 3.3.1 | Production and candidate measure identically: data 9048/9048, and the same 4 pre-existing code under-credits. audit_object_admission already lists the unit as a candidate (function_gap 0, data_gap 0). |
| Header blast radius (7 includers, both include spellings, control-proven) | All IDENTICAL bytes: xbox_texture_cache, rasterizer, rasterizer_transparent_geometry, active_camouflage, environment, models, xbox transparent_geometry. Only the internal $L ordinals shift by 2. objdiff per-function % is identical for environment, transparent_geometry and models. |
| /W3 /Zs | Warning multiset identical to production (17, all pre-existing). |
| fake_match_scan | 0 leads. |
| Line endings | CRLF, as in the original; `git diff --check` clean. |

## Evidence re-checks

- (b) All 14 bare names appear in January's IDirect3D*() error strings, and no January string
  contains the aggregate name. The aggregate plus `#define` redirects is therefore the
  fake-stringification pattern. **Confirmed.**
- (c)/(d) I re-ran the HCEX compiland dump myself and it matches the worker's dump. It lists
  srcblend/destblend/blendop_table as `Static Local const unsigned long[9]`, 0x24 apart in
  source/dest/op order. /Od strings `srcblend_table[...]==NONE` and `blendop_table[...]==NONE`
  are referenced only by fn 0x7ca560. cand_F (file scope) is also exact, so scope does not
  affect the bytes; it follows the HCEX evidence. **Confirmed.**
- (e) January's __rasterizer_initialize relocations at +2164/+2170 go to
  `_D3DDevice_SetFlickerFilter@4` / `_SetSoftDisplayFilter@4`, and January has no
  IDirect3DDevice8_ copy of either wrapper. **Confirmed.**
- (g) Atlas 4cc87b45 (the January 2002-01-14 build) has 0x546650
  `_rasterizer_filthy_bitmap_defaults_initialize`. Atlas 7eacac85 (2001-09-25 map) has the same
  name, as does HCEX (`static void ...defaults_initialize()`). The singular form appears only in
  January's error message. **Confirmed.**
- (h) Strip test S1, re-run myself: removing the 23 zero initialisers keeps the gate at 95/95 but
  fails object_audit on 23 rows (name-hash order). The attested names alone contradict January's
  order, so the initialisers are layout-attested, not decoration. **Confirmed.**
- (i) I did not re-run it. The file-scope descriptive statics are byte-equivalent in any case.

## Why the one blocker

The split replaces 1 invented aggregate name with 23 file statics:

- 14 names are authenticated by January strings.
- 9 are descriptive, carried over from the invented struct's member names. I re-checked every
  source: none of the 9 appears in January strings, /Od strings (the later build restructures them
  as `global_render_targets[]`), or the HCEX rasterizer_dx9 compiland.

Brief section 2 allows descriptive names. The owner's recorded practice for .bss relabels and
splits in an object admission is stricter, as shown by the two holds above. The owner said not to
re-ask without new evidence. The bare-string proof that the aggregate is invented is new evidence
for 14 of the 23 names, not for the other 9.

Owner question: "Admit rasterizer_xbox as Matching with 14 string-authenticated and 9 descriptive
.bss file statics? The descriptive ones replace the invented `rasterizer_xbox_d3d_globals`
aggregate and its #define redirects. The names do not affect layout, because zero-initialised
statics are placed in declaration order."

## Non-blocking notes

- The header symbol-listing comment still carries stale `_code_XXXXXXXX` entries. They predate
  this patch and no longer exist in symbols.json.
- If the owner declines, the storage-only corrections could land as a zero-credit packet: the 20
  static wrapper rows, the static d3d_palette and rasterizer_state_cache, the HCEX blend tables,
  the direct flicker/soft-display calls and the filthy rename. The .bss split and the Matching flip
  would be left out. No patches for that subset exist yet, and I have not verified it.
