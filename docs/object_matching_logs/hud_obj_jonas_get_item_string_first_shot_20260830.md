# `hud.obj::_hud_get_item_string` first-shot boundary (2026-08-30)

This additive ledger freezes the measured boundary for the natural typed-C89
reconstruction of `_hud_get_item_string`. The sole production candidate was
not strict exact and has been completely removed. `hud.obj` remains at its
inherited ten accepted functions; no production source, header, configuration,
status, compiler, comparator, or admission change is retained.

## Authority and provenance

- Integration base: `51fbd5de17b0c2f2c438d5ef2f19ad9c1192c6ce`.
- Restored `source/interface/hud.c` blob:
  `44ad43b20390aaecda40685ec611a64447832782`.
- Restored `source/interface/hud.h` blob:
  `4feb134fc6ee315b6198e68013dcdc933dcf32af`.
- January split `hud.obj` SHA-256:
  `4fec7af778c957b49525d56739902f8b4023534e2ea3fbd1af7686c9840f0e31`.
- Pre-shot production `hud.obj` SHA-256:
  `dabeb3cf95aadbbe601c4edce0d022aac91743c0c9587bbe72b55366e1da8603`.
- Sole candidate `hud.obj` SHA-256:
  `2363c41b16b04e043ffc5d6d63295c22963ea73075a99dc04dc9714ec6e15d84`.
- Compiler: the pinned Microsoft VC7/XDK `CL.Exe` through the ordinary Ninja
  production edge.

January COFF is the byte, padding, relocation, ABI, linkage, and ownership
authority. Repository-wide symbol/history and object-ledger searches found no
earlier implementation or measured attempt for this symbol.

The authenticated HCEA donor is
`src/hud_get_item_string.c` at clean commit
`aa635a70b2c3285ade3f53c2f3f94de759ce7cd4`; its Git blob is
`14716050e4834f78dcfcacc1c546e3238f136d02` and physical SHA-256 is
`1bb5bb722d0093a2e3f2f104e85d341823eb230824b6d533eaa1d218a6294da5`.
Stian, Pastudan, and the Claude PDB/raw-lift trees independently corroborate
the `wchar_t *(long)` signature and behavior. The candidate used only typed
named access to the HCEA-backed HUD message-list tag reference at offset
`0x94` (its index at `0xA0`) and the public typed `tag_get` and Unicode string
list APIs. The lane was therefore link-closed and suitable for one natural
shot.

## January packet

The target owns 76 meaningful / 80 padded bytes with normalized SHA-256
`88279655b02010490e2b9306363a215576b892ae68d6f74deba1c6aadea198fa`.
Its ordered relocations are:

| Offset | Type | Destination |
| ---: | --- | --- |
| `0x04` | `IMAGE_REL_I386_DIR32` | `_hud_globals` |
| `0x15` | `IMAGE_REL_I386_DIR32` | `_data_002e4c34` |
| `0x22` | `IMAGE_REL_I386_REL32` | `_tag_get` |
| `0x3B` | `IMAGE_REL_I386_REL32` | `_unicode_string_list_get_string` |

The target loads the current HUD item-message tag index, materializes the
default string pointer in callee-saved `EDI` before branching, returns that
default for an absent list, failed tag lookup, or out-of-range string index,
and otherwise returns the public Unicode-list accessor result.

## Sole candidate and fixed boundary

The frozen candidate followed the donor semantics directly: fetch the tag
index, conditionally resolve and range-check the string list, return the
resolved string on success, and return the typed default-string owner on the
fallback path. Exactly one code-producing edge ran:

```text
[1/1] CL build\base\source\interface\hud.obj
```

The candidate also occupied 80 padded bytes and owned four relocations, but it
was not identical. Its normalized SHA-256 was
`78920f46354b9b640ae466ffb990e80990541ffeb6aa9f31dd5c6f58cd495755`;
its ordered relocation offsets were `0x04`, `0x1B`, `0x34`, and `0x3F` to
`_hud_globals`, `_tag_get`, `_unicode_string_list_get_string`, and
`_data_002e4c34`, respectively.

The fixed mismatch is scheduling and register lifetime, not missing semantic
evidence: January hoists the fallback pointer before the branch and preserves
it in `EDI`, whereas VC7 compiled the natural source with no `EDI` lifetime
and deferred the default pointer load until the fallback block. This moved
both calls seven bytes earlier and the default-data relocation from `0x15` to
`0x3F`. Reproducing the target shape would require source-shape tuning or
byte-directed register scheduling, which this campaign forbids. No second
candidate was compiled.

## Restoration and regression evidence

The candidate source and header changes were inverse-reverted. Git object
identity confirms both files are byte-for-byte their canonical blobs above.
The preserved baseline object was restored without recompilation. A direct
hardened comparison reports `all_equal: true` for all ten inherited HUD exact
functions:

- `_hud_initialize`, `_hud_dispose`, `_hud_dispose_from_old_map`;
- `_scripted_show_hud`, `_scripted_show_hud_help_text`, `_hud_update`;
- `_hud_picked_up_grenade`, `_hud_picked_up_ammunition`;
- `_hud_picked_up_weapon`, `_hud_picked_up_powerup`.

Therefore this ledger records no exact gain and does not unblock `_hud_load`.
Future work must treat this natural HCEA topology as exhausted unless new
same-compiler source provenance—not source variation—fixes the early fallback
materialization and `EDI` ownership.
