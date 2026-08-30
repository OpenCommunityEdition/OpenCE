# `ui_widget.obj` main-menu error first shot (2026-08-30)

## Result

`_display_error_when_main_menu_loaded` matched in the first and only
production candidate compile.  The readable typed C89 body is retained:

| Function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_display_error_when_main_menu_loaded` | 42 | 48 | 4 | `fd899ac6b230853ea71afeb0d2e76185e7447375d1633b20960ee67d66fefb36` |

The complete exact-set comparison advances `ui_widget.obj` from 17 to 18
strict functions, reports this function as the sole `newly_exact` owner, and
has an empty `lost_exact` list.

## January and source evidence

January's split `source/interface/ui_widget.obj` is 111,969 bytes with
SHA-256
`783de6eb1bb30e272d656316fff549124cf64c47b80f928d34f818aac086af9b`.
It proves the ordinary cdecl `short` argument, the pending-slot offset
`_bss_00454240+0x828`, complete branch topology, literal, error priority,
padding, and ordered relocations:

- DIR32 `+0x06` and `+0x13` to `_bss_00454240+0x828`;
- DIR32 `+0x1A` to the exact diagnostic literal;
- REL32 `+0x21` to `_error`.

The target compares the 16-bit slot with `NONE`.  If it is free, it copies the
caller's 16-bit code and returns immediately.  Otherwise it calls
`error(_error_silent, ...)` and returns.  The exact target diagnostic is:

```text
there is already an error message queued for display at the main menu; ignoring this one
```

Claude's independently recovered later source at
`C:/halo-worktrees/claude-ai-conversation-2/src/halo/interface/ui_widget.c`
provides the same short-width slot, sentinel check, early return, diagnostic,
and error priority.  It is semantic corroboration; January remains the byte
authority.  The canonical public header already declares the exact one-short
ABI.  The existing typed runtime-global owner places
`main_menu_deferred_error_code` at `0x28` inside `widget_globals`, whose BSS
offset is `0x800`; the retained compile-time proof freezes the resulting
`0x828` address.

All applicable Claude campaign, ABI/calling-convention,
prototype-inference, compiler, and verification documents, all prior
`ui_widget.obj` ledgers, canonical history, and the current translation-unit
layout were audited before mutation.  No prior candidate, boundary, or body
for this function was found.

## Retained source and one-shot comparison

The starting source blob was
`4acac4dd46b0b417d3b173bafbd8cec7dea6f6fa`; the retained source blob is
`e10e50c8464bcd4fbc7ea69ba388182cf5aeb3a7`.  The public header remains
unchanged at blob `f95a7faea563957e1cc3e08e8987758a25df1c53`.

```c
void display_error_when_main_menu_loaded(
	short error_code)
{
	if (bss_00454240.widget_globals.main_menu_deferred_error_code == NONE)
	{
		bss_00454240.widget_globals.main_menu_deferred_error_code = error_code;
		return;
	}

	error(
		_error_silent,
		"there is already an error message queued for display at the main menu; ignoring this one");
	return;
}
```

The 5,202-byte pre-shot base object has SHA-256
`b3bc1395761c30d81df5ee9172503c341120df8ddeb660cd8f24a4d41a7a9893`.
Exactly one ordinary VC7 edge emitted the 5,662-byte candidate object with
SHA-256
`cc23c4e989b98c73fec3b41313bcd214ec9cbc3c1d6bce968c910a6dd82a88d6`.
Hardened comparison reports `all_equal: true` for every padded byte and all
four relocation records.  The complete baseline/candidate exact-set
comparison preserves all 17 inherited exact functions and adds only this one.

The retained code contains no assembly, volatile/register/pragma/intrinsic,
attribute or compiler control, raw address or raw offset access,
pointer/integer reconstruction, cast or union pun, undefined behavior,
storage definition, byte forcing, or comparator exception.  The BSS owner
remains external and receives no new data credit.

## Validation

The integrating cumulative replay runs the full Halo/libcmt,
semantic/progress, admission, parked-frontier, tooling-test, and protected
Units gates after all concurrent accepted lanes are merged.  Direct
function and exact-set comparisons are already strict and lossless at this
first emitted object.
