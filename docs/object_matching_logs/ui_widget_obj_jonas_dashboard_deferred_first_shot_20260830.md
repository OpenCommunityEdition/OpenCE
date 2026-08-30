# `ui_widget.obj` deferred-dashboard error first shot (2026-08-30)

## Result

`_display_error_abort_to_dashboard_deferred` matched in the first and only
production candidate compile. The readable typed C89 body is retained:

| Function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_display_error_abort_to_dashboard_deferred` | 51 | 64 | 5 | `4197aaaee10f25b636c1275866abf7bf94bdb034e98c65fbdc71ae5803580a2e` |

The complete exact-set comparison advances `ui_widget.obj` from 18 to 19
strict functions, reports this function as the sole `newly_exact` owner, and
has an empty `lost_exact` list.

## January and source evidence

January's split `source/interface/ui_widget.obj` is 111,969 bytes with
SHA-256
`783de6eb1bb30e272d656316fff549124cf64c47b80f928d34f818aac086af9b`.
It proves the ordinary cdecl `short, boolean` ABI, the pending-code and flag
offsets, complete branch topology, exact diagnostic, error priority, padding,
and ordered relocations:

- DIR32 `+0x06` and `+0x16` to `_bss_00454240+0x848`;
- DIR32 `+0x1C` to `_bss_00454240+0x84A`;
- DIR32 `+0x23` to the exact diagnostic literal;
- REL32 `+0x2A` to `_error`.

The target compares the 16-bit pending-code slot with `NONE`. If it is free,
it copies the caller's 16-bit error code and 8-bit optional flag, then returns
immediately. Otherwise it emits a silent diagnostic and returns. The exact
target diagnostic, including its original misspelling, is:

```text
there is already a deferred dashbaord error queued; ignoring this one!
```

The canonical public header already declares the exact two-argument ABI. The
typed runtime-global owner places the pending code at `0x48` and optional flag
at `0x4A` inside `widget_globals`, whose BSS offset is `0x800`; retained
compile-time proofs freeze the resulting `0x848` and `0x84A` addresses while
preserving the surrounding layout. Claude source trees and all applicable
campaign, ABI/calling-convention, prototype-inference, compiler, verification,
and prior `ui_widget.obj` documentation were audited before mutation. No
recovered Claude body or prior candidate/boundary for this function was found;
January therefore remains the byte authority.

## Retained source and one-shot comparison

The starting source blob was
`e10e50c8464bcd4fbc7ea69ba388182cf5aeb3a7`; the retained source blob is
`34855b58e501b0b6acb0028454e7838b17ead6ea`. The public header is unchanged.

```c
void display_error_abort_to_dashboard_deferred(
	short error_code,
	boolean optional)
{
	if (bss_00454240.widget_globals.deferred_dashboard_error_code == NONE)
	{
		bss_00454240.widget_globals.deferred_dashboard_error_code = error_code;
		bss_00454240.widget_globals.deferred_dashboard_optional = optional;
		return;
	}

	error(
		_error_silent,
		"there is already a deferred dashbaord error queued; ignoring this one!");
	return;
}
```

The 5,662-byte pre-shot object has SHA-256
`cc23c4e989b98c73fec3b41313bcd214ec9cbc3c1d6bce968c910a6dd82a88d6`.
Exactly one ordinary VC7 edge emitted the retained candidate object with
SHA-256
`7378f51f74bf934fb7c95a9b4bd0e0e47af3d333118efc27c915321b46551a6a`.
Hardened comparison reports `all_equal: true` for every padded byte and all
five relocation records. The complete baseline/candidate exact-set comparison
preserves all 18 inherited exact functions and adds only this one.

The retained code contains no assembly, volatile/register/pragma/intrinsic,
attribute or compiler control, raw address or raw offset access,
pointer/integer reconstruction, cast or union pun, undefined behavior,
storage definition, byte forcing, or comparator exception. The BSS owner
remains external and receives no new data credit.

## Validation

The integrating cumulative replay runs the full Halo/libcmt,
semantic/progress, admission, parked-frontier, tooling-test, and protected
Units gates after all concurrent accepted lanes are merged. Direct function
and exact-set comparisons are already strict and lossless at this first
emitted object.
