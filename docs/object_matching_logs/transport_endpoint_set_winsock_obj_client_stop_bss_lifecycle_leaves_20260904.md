# Transport client-stop, BSS ownership, and lifecycle leaves (2026-09-04)

## Disposition

This scratch-only packet follows the canonical endpoint-set operations wave at
`416c57f21e19d46c740b7e198bc4c43260bc014e`. It recovers one additional
strict function and the complete January runtime-storage contribution without
changing production source, headers, configuration, build output, or Git refs.

`transport_client_stop` is strict exact on the first natural candidate:

| Owner | Target meaningful / padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `_transport_client_stop` | 73 / 80 | 10 | `9fec770abe1a8d65821707bf96985f7dd58c31eb6c43c9a6d86ba4bc67f0bce3` |

The full scratch gate advances the object from **13 exact / 5 residual / 8
unwritten** to **14 exact / 7 residual / 5 unwritten**. All 13 inherited exact
owners remain strict. The existing nonce park and the four endpoint-operation
parks are byte-, relocation-, and hash-identical. The exact code gain is **73
meaningful / 80 padded bytes**.

The same packet naturally emits January's complete eight-byte BSS contribution
and all three authentic owner boundaries. `section_infos_equal` is true:

| Owner | Offset | COFF storage | Type evidence |
| --- | ---: | ---: | --- |
| `_transport_initialized` | 0 | 2 (external) | `boolean`; existing `transport.h` declaration and byte accesses |
| `_global_client_active` | 1 | 2 (external) | `boolean`; January byte loads/stores |
| implicit alignment | 2 | - | natural alignment before the following `long` |
| `_global_key_depth` | 4 | 2 (external) | `long`; January dword loads/decrements and existing key code |

Target and candidate are both `.bss`, eight bytes, zero relocations, normalized
SHA-256
`af5570f5a1810b7af78caf4bc70a660f0df51e42baf91d4de5b2328de0e83dfc`.
There is no opaque wrapper or explicit padding member. The proposed ordinary
definitions are:

```c
boolean transport_initialized = FALSE;
boolean global_client_active = FALSE;
long global_key_depth = 0;
```

This exact storage result supersedes the August 26 audit's statement that the
BSS was unclaimed. It does not contradict that frozen audit: the earlier
candidate intentionally left the globals undefined, whereas this wave has new
January PDB label evidence, the current real owner header, and a direct strict
section comparison.

## Exact client-stop source

January first tests `global_client_active`. When false it returns. When true it
performs exactly the already-owned `transport_pop_key` operation and clears the
active byte. Natural C2 compilation inlines that ordinary preceding public
function while retaining its standalone owner; no `inline` keyword, compiler
control, source duplication, retention anchor, or private alias is present.

```c
void transport_client_stop(
	void)
{
	if (global_client_active)
	{
		transport_pop_key();
		global_client_active = FALSE;
	}
	return;
}
```

All ten relocation addresses, types, identities, and addends match January:
the two named BSS globals, the existing filename/assertion strings,
`display_assert`, `system_exit`, `global_key_id`, and
`XNetUnregisterKey`. The exact body is public in both objects. Its declaration
belongs in `source/bungie_net/network/transport.h`; there is no reconstructed
production caller that justifies a caller-local prototype.

## Two honest lifecycle residuals

The same authenticated globals make two previously rejected lifecycle leaves
source-complete under the current fuzzy-park policy. They are included only as
natural residual proposals, not exact credit:

| Owner | Target meaningful / padded | Candidate padded | Relocs | Target SHA-256 | Candidate SHA-256 |
| --- | ---: | ---: | ---: | --- | --- |
| `_transport_dispose` | 41 / 48 | 48 | 4 / 4 | `3711768c555070d3283baaee32930eae990a21d6b0f267716d353c99ce483a7f` | `218e24977a5bd9167500aab3b194372c5dba98b5bc6c8d96ccdae430e756e231` |
| `_transport_network_available` | 8 / 16 | 16 | 1 / 1 | `73954fa0da7600bd1a60ddb3387eafabb788779227be715ec886a2fb1bdfe783` | `eef4bf49c6b243bcaae881c7f8494b0d0f6d9e65505d6dbe7d8138d1f2b12b2b` |

The first donor-shaped `void transport_dispose` scratch probe was rejected as
semantically incomplete after inspection: January returns
`_transport_error_none` after cleanup and `_transport_error_not_initialized`
when inactive. The final proposal therefore uses the subsystem's established
`short` result convention, calls `WSACleanup` then `XNetCleanup`, clears the
named boolean, and returns those correct enum constants. This is a semantic and
ABI correction, not a register-lifetime experiment. The target retains zero in
ESI across the cleanup calls while natural C emits direct AX constants; no
source-shape search is justified.

`transport_network_available` returns the boolean state of the stock XDK
`XNET_ETHERNET_LINK_ACTIVE` flag from `XNetGetEthernetLinkStatus`. January
masks AL; natural C masks EAX. The HCEA PDB and the existing owning declaration
both corroborate the `boolean (void)` ABI. The old exact-only rejection already
exhausted a cast variation, so this wave performs no renewed spelling search.

If root admits these two credible residuals, `transport_dispose` must be
declared as `short transport_dispose(void)` in `transport.h` and the stale
caller-local `void` declaration in `source/game/game.c` must be removed.
`transport_network_available` is already declared in `transport.h`; its stale
caller-local declaration in
`source/interface/ui_widget_event_handler_functions.c` should be replaced by
including that owner header. Those header/consumer edits require root's normal
blast-radius gates and are not part of this scratch packet.

## Provenance and ownership

January's PDB2.00 symbol atlas independently names the three functions and all
three globals at the exact image addresses:

- functions: `0x70680`, `0x70940`, and `0x70970`;
- globals: `0x31CE30`, `0x31CE31`, and `0x31CE34`.

The January module itself is stripped to OBJNAME and COMPILE records, so no
procedure type is claimed from that module. January instruction widths and
return paths, the current owning header, stock XDK declarations, and the HCEA
signature evidence establish the retained types. Pastudan is used only as
semantic corroboration; its raw addresses and incorrect `void` dispose ABI are
not imported.

The final candidate has 21 code owners against January's 26. Every candidate
code owner is target-backed; the five remaining target-only owners are
`_code_00070420`, `_transport_client_start`, `_transport_initialize`,
`_transport_server_initialize`, and `_transport_server_terminate`. There is no
COMMON and no candidate-only `.data`. The sole BSS contribution is the exact
eight-byte section above. The existing XDK encode tables and pooled literals
remain inherited ownership debt; this packet makes no whole-object admission
claim. The emitted-symbol audit finds no `_point_from_line3d` definition.

## Frozen reproduction

```text
python tools/campaign/gate.py source/bungie_net/network/transport_endpoint_set_winsock --edits scratch/transport-client-stop-owned-bss-lifecycle-leaves-final-20260904.json --all --forbid-emitted-symbol _point_from_line3d --out scratch/transport-client-stop-owned-bss-lifecycle-leaves-final-20260904.obj
```

- edit manifest SHA-256:
  `EACB78FC22EF007FD709D5063472DF833B3BE6C754CFAFE100253188D11B7A3D`;
- final scratch object SHA-256:
  `EAA473D2330FD2F366E513B7A8E586C15FDF5325CFAC9D356A4891805A29CF21`.

Root still owns production replay, owning-header cleanup, park records, full
Ninja regeneration, stable sweep, admission/data accounting, tests, commit,
and publication. No Fable-lane file was read or modified by this packet.

## Root reconciliation: selective admission, not the entire proposal

The root production replay exposed an unrelated exact-function regression in
`ui_widget_game_data_input_functions::_solo_level_select_list_update_displayed_items`
(693 meaningful / 704 padded bytes) when the new lifecycle declarations were
added to the broad `transport.h`. The common dispose declaration alone still
reproduces the loss. No declaration-order permutations or new header shim are
used to hide it.

The real Xbox client/key lifecycle declaration `transport_client_stop(void)`
belongs in the existing `transport_endpoint_winsock.h`, whose labeled owner is
already `TRANSPORT_ENDPOINT_SET_WINSOCK.C` and which the implementation already
includes. It is placed there without adding XDK dependencies to Game or UI.
`transport_network_available` retains its pre-existing common declaration.

The otherwise credible **short-returning dispose function and its owner/API
repair are withheld together**, not imported behind Game's incorrect inherited
local void declaration. Removing that entire optional subset restores the UI
exact function. Game's previous source is preserved unchanged; no new local
prototype is invented. The retained endpoint-set packet is therefore **14
exact / 6 residual / 6 unwritten**, with the exact client-stop and eight-byte
BSS gains intact. The full rejected proposal remains in the frozen scratch
manifest/object above and is not counted as admitted progress.

The UI event handler now includes the genuine existing `transport.h`, replacing
its duplicate transport-address structures and network-availability prototype.
All 100 target functions and all inherited emitted code owners are unchanged.
This is a narrow API/type ownership cleanup, not whole-source approval of its
unrelated inherited partial views.

Reopen dispose only when its correct common API can be integrated without
collateral loss, backed by a natural owner structure or authoritative evidence.
Do not restore the old void donor or introduce a codegen-only lifecycle header.
