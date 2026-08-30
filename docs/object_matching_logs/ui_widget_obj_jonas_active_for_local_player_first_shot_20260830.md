# `ui_widget.obj` local-player activity first-shot boundary (2026-08-30)

## Result

`_ui_widgets_active_for_local_player` remains unwritten. The first and only
natural code-producing candidate reproduced the complete 112-byte padded
extent, all seven relocation destinations, the four-slot January widget
range, and every authenticated aggregate offset. It did not reproduce the
target register lifetime, signed end comparison, return materialization, or
relocation addresses. The candidate and its typed aggregate expansion were
removed wholesale without a source-shape retry.

| Artifact | Meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| January target | `102 / 112` | 7 | `18c8c640a9ddd0ce1d1bec2c789087aa29ae3cc093989b8cd6d0f0e928b8a8b8` |
| first-shot candidate | `97 / 112` | 7 | `5ea59cb95fe09334ca71255b411c1f397bd828e0c4610194683cfbed9a02e91a` |

This is a measured source-topology and local-lifetime boundary, not a semantic
or layout ambiguity. Reopen only with new January-concordant source or debug
evidence that fixes the saved `EBX` false-value lifetime and the signed pointer
comparison. Do not replay this donor-shaped array walk or tune equivalent loop,
return, declaration, or condition spellings.

## Freshness, authority, and provenance

The isolated branch was at local commit
`bf8a8c0988f46be0f75bab6472440013c24620f3`, containing the canonical
`1c6f96dc088dbd19d61a3cf2fe2bbba5bde9c981` UI frontier plus the exact nth-child
and recursive-visibility recoveries. The January target owner
`build/split/source/interface/ui_widget.obj` has raw SHA-256
`5650f62a4bc7039276d4a7cb978755e9fd3802dadf272faac0249ecd957ac865`.
The target packet and its seven relocations remain the byte, ABI, layout, and
ownership authority.

Exact-name and descriptive searches across every UI Widget ledger, all
reachable refs and committed `ui_widget.c` blobs, registered worktrees,
Claude/Fable documentation trees, and repository history found no prior
candidate for this function. The earlier public-donor leaf-pair ledger compiled
only the distinct no-argument `_ui_widgets_active`; that frozen miss was not
retried. History reaches only the original inventory and public declaration for
the local-player function.

The authenticated HCEA donor is
`C:/Users/isabe/Documents/Codex/reference/halocea/src/ui_widgets_active_for_local_player.c`,
payload SHA-256
`1a729af0ad5152c02a150468a309316d085b3435b4ffd58aefb834fc5556147e`.
Its later build uses two slots, while its decompiler note independently fixes a
raw pointer walk from `active_widgets` to the immediately following
`widget_stack`. Pastudan's independent January lift at
`C:/Users/isabe/Documents/Codex/reference/pastudan-halo/src/halo/interface/ui_widget.c`
(payload SHA-256
`27fa5fec523d655b0a7877fa3e1da39d24c70bd55dffaca63ff491a0f01e5ea8`)
fixes the January four-slot range, valid-index assertion at original line 1110,
initialized gate, and signed-short widget field at `+0x08`.

The function is an ordinary public cdecl leaf and is link-closed. Its only
external calls are the implemented `_display_assert` and `_system_exit`
owners emitted by `match_vassert`. Existing exact fields independently place
the runtime aggregate at `_bss_00454240+0x800` and `initialized` at aggregate
offset `+0x62`; January places `active_widgets[0]` at aggregate offset `+0x00`
and its exclusive end at `widget_stack[0]`, offset `+0x10`.

## Frozen candidate and one-shot discipline

The sole candidate replaced only the anonymous leading `0x24` bytes of the
existing runtime prefix with typed four-element `active_widgets` and
`widget_stack` arrays plus the following timestamp field, guarded by
compile-time offset checks. Its ordinary C89 body was:

```c
boolean ui_widgets_active_for_local_player(
	short local_player_index)
{
	struct widget_instance **active_widget;

	match_vassert(
		"c:\\halo\\SOURCE\\interface\\ui_widget.c",
		1110,
		local_player_index >= 0 &&
			local_player_index < MAXIMUM_UI_WIDGET_LOCAL_PLAYERS,
		"expected a valid local_player_index");
	if (!bss_00454240.widget_globals.initialized)
		return FALSE;

	for (active_widget = bss_00454240.widget_globals.active_widgets;
		active_widget < bss_00454240.widget_globals.active_widgets +
			MAXIMUM_UI_WIDGET_LOCAL_PLAYERS;
		active_widget++)
	{
		if (*active_widget &&
			(*active_widget)->local_player_index == local_player_index)
			return TRUE;
	}

	return FALSE;
}
```

The production Ninja edge invoked VC7 exactly once:

```text
[1/1] CL build\base\source\interface\ui_widget.obj
```

No assertion, declaration, pointer type, loop, comparison, branch, return,
local lifetime, compiler option, or source variant followed the strict miss.
The candidate used no assembly, `volatile`, `register`, pragma, intrinsic,
attribute, optimizer barrier, raw address/offset access, representation pun,
undefined behavior, byte forcing, private ABI, object patch, or tool exception.

## Exact mismatch

Both packets have 112 padded bytes and the same ordered symbolic destination
sequence. Every candidate relocation occurs exactly three bytes earlier:

| Destination | January offset | Candidate offset |
| --- | ---: | ---: |
| source-path COMDAT | `+0x1E` | `+0x1B` |
| custom assertion COMDAT | `+0x23` | `+0x20` |
| `_display_assert` | `+0x28` | `+0x25` |
| `_system_exit` | `+0x2F` | `+0x2C` |
| `_bss_00454240 + 0x862` (`initialized`) | `+0x37` | `+0x34` |
| `_bss_00454240 + 0x800` (`active_widgets`) | `+0x40` | `+0x3D` |
| `_bss_00454240 + 0x810` (`widget_stack`) | `+0x54` | `+0x51` |

January saves both `EBX` and `ESI`, initializes `BL` to false before the
assertion, uses signed `jl` for the typed-range end packet, and materializes
the false return with `mov al,bl`. The natural candidate saves only `ESI`, uses
unsigned `jb` for its conforming pointer comparison, and materializes false
with `xor al,al` at the return tail. Hardened `tools.coff_compare` therefore
reports `all_equal: false`; equal padded extent and relocation identities do
not constitute strict equality.

## Restoration and validation boundary

The body, enum, forward declaration, typed aggregate fields, and offset checks
were inverse-restored. `source/interface/ui_widget.c` again hashes to its exact
starting Git blob `dc04195b7d22ae3e564cc8638f33e3112f450e8f`; no source, header,
configuration, storage, symbol map, semantic exception, parked record, or
completion label is retained. The rebuilt restored candidate owner has raw
SHA-256
`9f30e89dc32b017115ba53af2e8a2eded54a2394a0340157235cebe931d0d463`.

A direct three-way exact-set comparison against the pre-shot object preserves
all 23 inherited exact UI Widget owners, with `lost_exact: []` and
`newly_exact: []`. Full Halo/libcmt build, report, semantic report, and progress
targets pass; semantic progress scans 470 units and 4,952 functions with 4,835
semantic exact, 4,845 accepted exact, 131 hidden exact, and zero unit errors.
The focused UI Widget gate, object admission audit, parked audit, all 212 tooling
tests, and `git diff --check` pass.

Protected `_unit_preprocess_node_orientations` remains strict exact at 1,920
padded bytes, 87 ordered relocations, and normalized SHA-256
`5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
The only intended tracked change is this additive boundary ledger. Nothing is
pushed.
