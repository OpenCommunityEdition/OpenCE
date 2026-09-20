> Research-only packet16: zero production change and zero exact credit. Validated production remains wave13.

# HUD relocation aliases: January binding proved; no current candidate

Status: **CLOSED — conditional existing admission path only, zero new credit**.
Baseline: `29ff9e6f934c3fb4308e712268727a1d10b75b9f`. Scope is only
`source/interface/hud_messaging::_hud_messaging_update`, 2,922 meaningful
bytes. No source shapes, C compiler calls, source/header/config/tool edits,
target rewrites or comparator changes were made.

## January evidence

`audit.py` reproduces the narrow check; `audit.json` preserves input hashes,
owner records and all raw operands. The January public-symbol extraction
names `_hud_messaging_update` at RVA 804,560 and
`_global_hud_anchor_names` at RVA 3,034,168. January contribution records
place the update function and its following three private owners in module
273, identified as `source/interface/hud_messaging.c` by `config/config.json`:

| Private owner | HUD RVA | Other same-name RVA | Independent target/image check |
| --- | ---: | ---: | --- |
| `_button_mappings` | 3,034,188 | 3,169,984 | Complete 28-byte containing data section equals January image |
| `_render_state_text` | 800,784 | 869,360 | Complete 128-byte code section and all relocated destinations equal January image |
| `_render_state_bitmap` | 800,912 | 868,944 | Complete 400-byte code section and all relocated destinations equal January image |

The other instances belong to module 251. These are bindings to actual
defined HUD owners and their original image contents, not an arbitrary
selection of the first duplicate name. The current rebuilt three owners
also compare strictly equal to the target. The 28-byte data section includes
the button array, alignment, `time_code_time`, `time_code_stop_time` and
`split_screen_hud_message_offset`; **28 is not the button-array length**.

At update offsets `+0x4F7` and `+0x510`, both original image operands encode
VA `0x6E4C3A`, proving:

```
_global_hud_anchor_names + 2 = 3,034,170
_button_mappings - 18        = 3,034,170
```

Using the existing `section_info_resolved` API with this independently bound
scratch map resolves every one of the target's 137 destinations; all agree
with the original January image operands. The complete normalized target
code also equals the image at its proved address. The unmodified global
map leaves five target and seven current-base destinations unresolved.
The scratch map leaves none unresolved. This is **not** a candidate equality
claim: the current base remains 2,880 bytes / 136 relocations, versus January
2,928 / 137, and compares unequal even after resolution.

## Existing admission path and limits

`tools/audit_semantic_matches.py::audit` already accepts ordinary **100%
objdiff plus equal COFF size, normalized bytes and ordered relocation
address/type geometry**, recording `objdiff-coff-shape`. The currently
accepted `_player_profile_1wide_list_update` is a live control for that route.
A future admissible HUD candidate satisfying those conditions can use the
same existing path, supplemented by independent complete image-destination
and source/ownership proof. No new exception or comparator change is needed.

This is distinct from two unavailable shortcuts. Explicit
`semantic_matches.json` function entries still require strict symbolic
equality. The parked `csplit-relocation-alias` validator uses the global
address map, drops duplicate static names, and consumes no per-unit binding
field; that class validates a park and does not itself grant exact credit.
If a future candidate is below 100% objdiff and remains symbolically unequal,
the current automatic admission has no address-resolved function-manifest
route. This packet does not propose changing it.

## Prior disposition and reopening boundary

The Sept. 14 w1 and w3 HUD ledgers report MF2/hm1 at 2,928 / 137 with January's
normalized SHA and these two symbolic differences, plus a manual full
resolved comparison. Those candidate files are unavailable in the current
evidence set; their historical measurements were not replayed. Current
production remains 66.71034% objdiff and has no accepted-ledger entry for
this function.

The prior candidate also incorporated a measured 24-order rectangle-store
sweep. Its existence and reported normalized equality do not establish
independent source provenance. A new attempt requires authenticated source
facts for the complete retained shape, a current reproducible candidate,
100% objdiff with the existing COFF-shape check, all 137 final destinations
verified, and preservation of every inherited exact control and runtime
owner/data/COMMON/point constraint. No old swept body is reconstructed here.

History read: `hud_messaging_obj_opus5_150k_w1_20260914.md`,
`hud_messaging_obj_opus5_150k_w3_20260914.md`,
`hud_messaging_obj_jonas_claude_exact_trio_20260831.md`,
`hud_messaging_obj_renderer_caller_closure_20260908.md`, and the campaign's
one-wide independent review. No unrelated alias census was rerun.

Evidence ownership: **RELEASED**.
