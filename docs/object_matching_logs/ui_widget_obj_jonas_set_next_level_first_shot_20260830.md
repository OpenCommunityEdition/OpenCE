# `ui_widget.obj` next-level first-shot boundary (2026-08-30)

## Result

`_ui_set_next_level` remains unwritten.  The first and only natural
code-producing candidate has the exact 71 meaningful / 80 padded-byte extent
and the same seven relocation identities as January, but not the same bytes,
relocation order, or relocation offsets.  The complete candidate body and its
sole new include were removed without a source-shape retry.

| Artifact | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| January target | `71 / 80` | 7 | `ac9aadd3432be86c98b503dec837d2ed16c80506c18f24043b3a60da4082e137` |
| first-shot candidate | `71 / 80` | 7 | `3ae341d2dcc64ead6b9c84c40b74c91ab8f14f8aca14fe372443d790443c4c02` |

This is a measured source-topology boundary, not a semantic ambiguity or a
fuzzy near-match.  Reopen only with new January-concordant source provenance;
do not replay the HCEA-shaped candidate or tune equivalent condition and
return spellings.

## Immutable evidence and provenance

The isolated lane was fast-forwarded before mutation to canonical commit
`ef51cab6de891c65c4a5fac7b249edbf8043ffe7` on branch
`agent/ui-set-next-level-20260830`.  The January split
`source/interface/ui_widget.obj` is 111,969 bytes with SHA-256
`783de6eb1bb30e272d656316fff549124cf64c47b80f928d34f818aac086af9b`.

The untouched production `ui_widget.c` is Git blob
`34855b58e501b0b6acb0028454e7838b17ead6ea`; after inverse restoration its
worktree payload SHA-256 is
`4ddd299d53c1b2410be556772f30a0c61521207e47d25c54dd2be113c5914d0b`.
The public declaration owners remained unchanged:

- `source/interface/ui_widget.h`: Git blob
  `f95a7faea563957e1cc3e08e8987758a25df1c53`, declaring
  `void ui_set_next_level(short level)`;
- `source/main/main.h`: Git blob
  `dea858acf3dc87bd4e89a8e9614d8659699954d1`, declaring all five typed Main
  callees; and
- `source/cseries/errors.h`: Git blob
  `81f2b8e018d21cb5853e55d181992c3700e49a30`, declaring the variadic typed
  error function and `_error_silent == 2`.

The authenticated HCEA body at
`C:/Users/isabe/Documents/Codex/reference/halocea/src/ui_set_next_level.c`
has payload SHA-256
`35fd904c429c2278c8be6e27c7acc9b8debe08ec29faac9adeba11de93b84707`.
It proves the public signed-short ABI and the three-way semantics: `NONE`
rolls credits, values outside `[0, 9]` return to the main menu, and valid
values set the solo-level map name then disallow persistent storage.  Its
`main_won_map_private.c` and `main_loop_update.c` callers independently pass a
signed 16-bit next-level value.  January adds the exact silent
`"unknown level"` diagnostic before the invalid-level menu transition.

All applicable campaign methodology, acceleration, UI Widget, Main, Claude,
AGENTS, and tooling documents were read before mutation.  `git log --all -S`
finds no earlier committed body or direct boundary.  A census of 1,533
registered worktrees found no `ui_set_next_level` definition in
`source/interface/ui_widget.c`.  The HCEA file above is therefore semantic
source evidence, while January remains the byte and layout authority.

## Frozen candidate and one-shot discipline

The candidate used the existing public headers with no local prototype
shadows.  It preserved the HCEA branch order and local result, adding only the
January-proven diagnostic:

```c
void ui_set_next_level(
	short level)
{
	if (level == NONE)
	{
		main_roll_credits();
	}
	else if (level < 0 || level > 9)
	{
		error(_error_silent, "unknown level");
		main_goto_main_menu();
	}
	else
	{
		char const *solo_level_name;

		solo_level_name = main_get_solo_level_name(level);
		main_set_map_name(solo_level_name);
		main_disallow_persistent_storage();
	}

	return;
}
```

A `/Zs /W3` prerequisite pass succeeded with only the translation unit's
inherited `cseries.h` and `real_math.h` warnings and emitted no object.  Ninja
then exposed exactly one pending production edge and invoked it once:

```text
[1/1] CL build\base\source\interface\ui_widget.obj
```

The compiler was Microsoft 32-bit C/C++ Optimizing Compiler
`13.00.9254.1`; `CL.Exe` SHA-256 is
`483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
The candidate object is 6,740 bytes with raw SHA-256
`34bece3bbcde9a636d7158ea8c88afc742f4b6d591eb2a2d923a8792e314a7e1`.

No second compile, branch inversion, return rewrite, condition rewrite,
declaration change, local-lifetime change, or compiler/flag variant was tried.

## Exact mismatch

January sign-extends the low word of the argument into a separate 32-bit
comparison value, branches over the valid and invalid bodies to a final
credits tail, and orders its relocations as follows:

| Offset | January destination |
| ---: | --- |
| `+0x19` | `_main_get_solo_level_name` |
| `+0x1F` | `_main_set_map_name` |
| `+0x28` | `_main_disallow_persistent_storage` |
| `+0x2D` | exact `"unknown level"` COMDAT |
| `+0x34` | `_error` |
| `+0x3D` | `_main_goto_main_menu` |
| `+0x43` | `_main_roll_credits` |

The HCEA-shaped candidate compares the argument directly through `AX` and
places the credits tail first.  Its relocation order and offsets are:

| Offset | Candidate destination |
| ---: | --- |
| `+0x0E` | `_main_roll_credits` |
| `+0x1F` | `_main_get_solo_level_name` |
| `+0x25` | `_main_set_map_name` |
| `+0x2E` | `_main_disallow_persistent_storage` |
| `+0x33` | exact `"unknown level"` COMDAT |
| `+0x3A` | `_error` |
| `+0x43` | `_main_goto_main_menu` |

Thus equal extent, equal relocation count, and equal destination set do not
constitute identity.  Hardened comparison reports `all_equal: false` and the
candidate is rejected.

## Restoration and validation boundary

The candidate body and `main.h` include were removed wholesale, and Git then
restored `source/interface/ui_widget.c` to the exact starting blob shown
above.  No source, header, configuration, storage, semantic exception,
parked record, or completion label is retained.  The only tracked change is
this additive boundary ledger.

The pre-shot protected Units sentinel was strict at 1,920 padded bytes,
87 ordered relocations, and normalized SHA-256
`5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
The one-shot regression check reported no newly exact owner and identified
`_ui_set_next_level` as the sole changed non-exact function; the additional
section/symbol diagnostics were the expected physical consequences of the
rejected function and literal COMDAT.  Because production source is restored
and the final payload is documentation-only, no exact-set or protected source
change is retained.  No push is performed.
