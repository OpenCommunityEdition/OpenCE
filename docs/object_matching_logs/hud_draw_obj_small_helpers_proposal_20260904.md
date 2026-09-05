# HUD Draw: two small exact leaves, two withheld conversions (2026-09-04)

## Scope and result

This worker reconstructed a distinct scratch full translation unit; production
source/header/configuration edits and the full build belong to the root
integrator. Final proposed source is
`scratch/hud-draw-small-helpers-proposal-20260904.c`. Root independently applied
the same two leaves to `source/interface/hud_draw.c` and reported its own scratch
gate before starting the full build. This document does not claim that build or
publication has completed.

The final worker gate improves **2 exact / 0 residual / 21 unwritten** to
**4 exact / 0 residual / 19 unwritten**, preserving both inherited exact owners.
The new strict gain is **204 meaningful / 208 padded bytes**, two functions:

| Owner | Meaningful / padded | Relocations | Target and candidate normalized SHA256 |
| --- | --- | --- | --- |
| `_real_alpha_intensity_to_pixel32` | 172 / 176 | 13 | `82b3d86d1b33555fbecbbe6faec45a094ba2dca6e3b36c110daebe38251842a6` |
| `_get_flash_duration` | 32 / 32 | 1 | `dcf81ee7e83bf429645379d2ecd9b0992d7d77fa53b25a791a6c04de16d0e973` |

Neither the rejected RGB conversion nor the withheld `fast_ftol_C` body is
present in the final proposal. Neither receives exact, fuzzy, or parked credit.
The object remains incomplete, including 19 unwritten functions, the missing
four-byte January BSS owner, and the shared-helper ownership issue below.

## Reproduction and frozen artifacts

Run from the canonical worktree:

```powershell
python tools/campaign/gate.py source/interface/hud_draw --source scratch/hud-draw-small-helpers-proposal-20260904.c --out scratch/hud-draw-small-helpers-two-exact-20260904.obj --all --forbid-emitted-symbol _point_from_line3d
```

The correctly spelled full-candidate emitted-symbol guard passed. No compiler
flags, ordering probes, alias mappings, production headers, build rules,
comparator rules, or configuration were changed by this worker.

| Artifact | Physical SHA256 |
| --- | --- |
| `scratch/hud-draw-small-helpers-proposal-20260904.c` | `2e78971ee380afa6c6365d4a8177b0dad5fa20dc1e2ae53dd25ce8c355242d0e` |
| `scratch/hud-draw-small-helpers-two-exact-20260904.obj` | `c1f81e84e6af0d60ecc310c35f86ebb50d678cdb1001626445a1d9758d3103a7` |
| `scratch/hud-draw-small-helpers-baseline-20260904.obj` | `149b93b7b89033cd95100c34aeaa4251fd34e6e1b58af8b4d47ee5cc16aeb4cb` |
| `build/split/source/interface/hud_draw.obj` | `27d9adb322c237ea9114cd9a8e4d8b5457de517a7feef20ea6bd7158263b20db` |

The preserved baseline exact owners remain byte/relocation-identical:
`_check_stack_buffer`, 48 padded / 0 relocations, hash
`c7049c3184d2ed3e342f95abfbe849a3bd037456bf88a70fc93c713b48c376a6`;
`_hud_globals_get_scale`, 16 / 1, hash
`410b62fd58558df56fc858a393c0a69c2aac6142be8edf5b4a366b99ce01a691`.

## Source and ABI evidence

January `hud_draw.obj` is the sole byte, call-ABI, diagnostic, and arithmetic
authority. The public APIs already have proper declarations in
`source/interface/hud_draw.h`; no caller-local public declarations were added.
The sole new include is `interface/unit_hud_interface_definition.h`, the existing
complete owner of `struct hud_color_definition`, including its `flash_period`
member at offset `0x08`. No local opaque type or padding facade was introduced.

Alpha/intensity uses an ordinary `real_argb_color`, validates each input with the
January assertions at `..\bitmaps\bitmaps_inlines.h` lines `0x13F` and `0x140`,
sets alpha and the three identical intensity channels, and calls the genuine
public `real_argb_color_to_pixel32` API. That callee remains unwritten here;
its real external reference is not a fabricated emission anchor. The source
uses project types and the existing assert mechanism, with one parameter per
line and explicit return statements.

The pinned later donor
`../../research-cache/halocea-full-blobs-20260830/src/blam/effects/real_alpha_intensity_to_pixel32.c`
corroborates the channel semantics but directly packs bytes and does not supply
the January assertion/call schedule. It was not copied as byte authority.

Flash duration is the natural expression
`fast_ftol(hud_color->flash_period * 30.0f)`. This uses the existing authenticated
`source/cseries/cseries.h` math helper; this packet adds no assembly or forced
inline directive. January explicitly performs `fmul` at `+0x0A`, `fstp dword`
at `+0x10`, `fld dword` at `+0x13`, and `fistp dword` at `+0x16`. Thus the helper's
32-bit argument staging and current-rounding-mode conversion are authentic at
this call site, not an approximation of an unstaged extended product.

The earlier rejection remains intact at
`docs/object_matching_logs/hud_draw_obj_jonas_flash_duration_rejection_20260829.md`.
It tried a C cast, which emitted `__ftol2` and has different rounding semantics.
Root expressly authorized this one reopening because the established helper
identity and real owning type are new semantic evidence, not a compiler-mode
lottery. The authentic helper provenance is recorded in
`docs/object_matching_logs/actor_looking_obj_jonas_secondary_look_research_20260828.md`;
January's selected `source/ai/actor_combat::_fast_ftol` independently matches it.

## Actual relocation comparison

Both new owners pass `coff_compare.section_infos_equal`, including the ordered
relocation comparison, not just size/count or normalized byte hashes.

For alpha/intensity, corresponding target/candidate relocations have identical
addresses, types, addends, and symbolic identities:

| Offsets | Type | Referent |
| --- | --- | --- |
| `0x0B`, `0x4B` | DIR32 | `__real@00000000` |
| `0x1B`, `0x5B` | DIR32 | `__real@3f800000` |
| `0x2E`, `0x6E` | DIR32 | original `..\bitmaps\bitmaps_inlines.h` literal |
| `0x33` | DIR32 | original alpha-range assertion literal |
| `0x73` | DIR32 | original intensity-range assertion literal |
| `0x38`, `0x78` | REL32 | `_display_assert` |
| `0x3F`, `0x7F` | REL32 | `_system_exit` |
| `0xA1` | REL32 | `_real_argb_color_to_pixel32` |

Flash has one DIR32 relocation at `0x0C` to `__real@41f00000` (`30.0f`).
The split target represents several pooled literals as undefined symbolic
references whereas the candidate defines their normal select-any copies.
These are not literally identical raw COFF relocation dictionaries; the
comparator's supported symbolic identity comparison accepts them. The actual
literal payloads and selections were separately checked below.

## Complete candidate owner census and disposition

The full COFF symbol table contains **five** defined code owners, not merely the
four target-listed exact functions: the four functions above plus `_fast_ftol`,
32 padded / 17 meaningful bytes, zero relocations, normalized hash
`8d76b4470cb80ce310b0c4d5255470b182cfa0a78d93b9df3cdcb6f56678f248`.
There is no defined or referenced `_point_from_line3d` and no COMMON allocation.
There are no new writable DATA/BSS allocations.

The complete named non-code census is six ordinary read-only owners. Each has
zero relocations, flags `0x40301040`, select-any COMDAT selection 2, and payload
strict-equal to its January selected owner:

| Candidate owner / payload | Bytes | January selected owner |
| --- | --- | --- |
| `__real@3f800000` (`1.0f`, inherited) | 4 | `source/ai/action_charge.obj` |
| intensity-range assertion | 35 | `source/interface/hud_draw.obj` |
| alpha-range assertion | 27 | `source/effects/decals.obj` |
| `..\bitmaps\bitmaps_inlines.h` | 29 | `source/effects/decals.obj` |
| `__real@00000000` (`0.0f`) | 4 | `source/ai/action_charge.obj` |
| `__real@41f00000` (`30.0f`) | 4 | `source/ai/action_alert.obj` |

The intensity literal is real newly reconstructed HUD-owned read-only data;
other new literals have compatible selected ownership elsewhere. This packet's
204-byte figure is code-only and does not inflate it with pooled literals.

The extra `_fast_ftol` copy has flags `0x60501020` and selection 2. Its bytes
strictly equal January `source/ai/actor_combat.obj`, but that selected owner has
selection 1 (no duplicates). Identical code is **not** proof of compatible
coalescing. The pre-existing XDK link experiments documented in
`docs/object_matching_logs/player_rumble_obj_jonas_symbol_backed_fast_ftol_closeout_20260831.md`
and `docs/object_matching_logs/bitmap_utilities_obj_jonas_vector_map_quartet_20260831.md`
reject this selection-2/selection-1 pairing with duplicate-symbol errors.
This extra helper must remain an explicit whole-object admission blocker;
do not count it as fresh HUD-owned code or waive it because the four listed
functions are exact.

## Rejected RGB: real product-staging defect

The discarded RGB proposal used three ordinary channel expressions of the form
`(byte)fast_ftol(color->red * (real)UNSIGNED_CHAR_MAX)` after authentic validation.
It produced 176 padded bytes / 11 relocations versus January 192 / 8, but this
was not merely a scheduling residual. January multiplies each `real` channel by
255 and directly converts the x87 product. The candidate stores the product to
32-bit `real` before reloading it for FISTP, adding an observable rounding step.
At the first channel, the candidate sequence includes `fmul` at `+0x62`,
`fstp dword [ebp+8]` at `+0x68`, `fld dword` at `+0x6B`, and `fistp` at `+0x6E`;
January's `+0x72..+0x99` channel block has no corresponding intermediate spill.

January `real_math_reset_precision` establishes `_control87(0x9001F, 0xFFFFF)`;
the owning `xbox/include/float.h` constants identify PC53, nearest rounding,
and masked exceptions. For the valid real channel `0.0019607844296842813`,
the product by 255 is `0.5000000295694917`: direct nearest-even conversion
gives integer **1**, while narrowing first to real gives exactly `0.5` and then
integer **0**. This is a reproducible IEEE arithmetic counterexample, not a
claim of a live engine execution test. The body was therefore removed rather
than parked, and there was no code-generation search for an artificial remedy.

The rejected RGB-plus-cast artifact is retained as
`scratch/hud-draw-small-helpers-frozen-20260904.obj`, physical SHA256
`4cda67ca7eb6d89ff095e397fb21c0b624ba8e90f3fab50a4e08b35da1160dbd`.
Despite its historical filename it is **not** the accepted frozen artifact.
Its RGB normalized hash is
`4f15e2c5bc7c313a71a112b3c78e1281df922eefbf888d59af3889f4de0e2239`;
January's is
`01d6a96ad9a03f25ea303e03c37799d696a3738a9268a820870ed11ceccbb1b8`.

The same concern prompted an independent recheck of all twelve published
HUD Unit shield/auxiliary conversions. There January itself stages each product
to real, matching the candidate. That separate follow-up is documented by the
other reviewer in
`docs/object_matching_logs/hud_unit_obj_renderer_precision_followup_20260904.md`;
the old review ledger was left unchanged.

## Withheld ANSI `fast_ftol_C`: domain not authenticated

Historical cseries evidence and the pinned later donor
`../../research-cache/halocea-full-blobs-20260830/src/blam/math/fast_ftol_C.c`
suggest a normal C cast. The attempted `return (long)value;` emits a 16-byte,
one-relocation `__ftol2` caller, unlike January's 56-byte zero-relocation routine
using FIST, FISUB, sign tests and bit-pattern corrections. For representable
finite inputs both implement truncation, but that alone is insufficient:
the C conversion has undefined behavior for NaN/unrepresentable values, while
the target has concrete masked-exception machine behavior. No authenticated
finite/in-range contract for this public January helper was established.

Root therefore required withholding this body as semantics-unproven. It is
absent from the final proposal and receives no residual or parked credit.
The intermediate no-RGB-but-cast artifact is preserved as
`scratch/hud-draw-small-helpers-valid-subset-20260904.obj`, physical SHA256
`f33159e277a829f7c8bd0b3540523b49c1300460b43342e64c781c8a09f179a3`.
The attempted helper normalized hash is
`bbe139825c1f5012bc08ffc81ed8a634feef80fe809ac3c6959eee118ef7edf5`;
January's is
`6d480d71829c2f3a315f260e2cbf5b219dd02b8337b08606ea4b250fdd47ce14`.
Reopening requires real January-domain evidence or an independently justified
implementation preserving its full arithmetic contract, not an assumption
that a later source cast authenticates all January behavior.
