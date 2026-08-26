# `rasterizer_geometry.obj` Jonas stripify first shot (2026-08-26)

This Jonas-owned ledger records one frozen, source-only production experiment
for `_rasterizer_geometry_stripify`. The readable two-parameter constant-return
stub is strict on its first production compile and is retained. The object
advances from 8/19 to 9/19 exact functions and from 639 to 642 exact meaningful
code bytes. It remains `NonMatching`; no data or whole-object completion claim
is made.

## Scope and immutable authority

- Authoritative cumulative base:
  `ea21f77bab8fbc719115dfc80a3a35f80e1ece02`.
- Baseline source blob:
  `d12046aee23f4c5c9e7418ccecb095cafa154036`.
- Baseline source physical SHA-256:
  `44a141571cb56ce9b34818ccd1c0c8b293100fcbf636a1684ceb5b6c06339459`.
- January executable: 6,584,672 bytes, SHA-256
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split `rasterizer_geometry.obj` SHA-256:
  `d6357486072be1c8fb98fed92bc36eb0c1d6c7e68386df5c9ce164f0627ebb49`.
- Compiler: Microsoft 32-bit C/C++ Optimizing Compiler 13.00.9254.1,
  81,920 bytes, SHA-256
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.

January COFF is the byte, ABI, linkage, extent, padding, relocation, and
ownership authority. The unchanged production edge uses `/O2 /Oy- /DDEBUG
/Dxbox` and the generated include graph. No alternate compiler, flag, or
code-generation control is used.

Only `source/rasterizer/rasterizer_geometry.c` and this new ledger are in
tracked scope. No header, configuration, existing ledger, semantic allowlist,
park record, completion label, Units source, or protected/shared interface is
changed.

## Claude, HCEA, history, and tooling evidence

Claude's three byte-identical residual inventories at
`claude-near-complete-objects-20260816`,
`claude-finish-hs-20260816`, and `claude-untried-objects` are 562,334 bytes,
SHA-256
`db87717092d2e7c71160619e9df25f0b940f1762725c81f8bd2d6ece77fbde0d`.
They classify all 19 target functions, including stripify, as target-only and
provide no prior body, residual recipe, error, or owner-name candidate. Their
byte-identical 539,013-byte `unwritten_string_synopsis.json` files have
SHA-256
`9786012cf2daa5dda7b228ae0ecef0ee80e3e37760f21d4d9bd4e538beb5f1d1`
and contain no stripify record.

The HCEA/January correspondence file is 3,749,602 bytes, SHA-256
`c6ea66d530438fba92d27790001c631d973d41e7fb6bacb21a4525763ec6cfbc`.
It records an `exact_unique` cdecl mapping, two parameters, eight HCEA bytes,
three January meaningful bytes, and the normalized name
`rasterizer_geometry_stripify`. The 572,341-byte exact-tier ranking, SHA-256
`f3fdc2e18c2f173f0bc9c6bec5aa6c8250ed92c99137f79a8d4a5c0d295e2476`,
ranks the same three-byte function with complete name coverage.

The clean HCEA source body is 314 bytes, SHA-256
`34da06ce1d5b9e4ea85823cde909fb82beca809739755b07fb0912481c6f505b`.
It independently supplies `uint8_t(triangle_buffer *, vertex_buffer *)` and
an unconditional return of one. HCEA is semantic/source-shape provenance,
not a January byte oracle. Its `static` scope is not imported because January
COFF directly proves external linkage.

An exhaustive registered-worktree census inspected 1,391 worktrees and found
no real stripify definition. Reachable file history and the geometry,
hardware-geometry, Claude, exact-triangle-strip, and prior Jonas rasterizer
branches likewise contain no body. The exact-triangle-strip lane concerns
triangle iteration and is unrelated. The two earlier Jonas waves cover only
the exact byte-swap stub, scalar decoders, and five compressed-vertex
accessors. This is therefore a genuine first shot rather than a retry.

The applicable 47,280-byte Claude instruction copies are byte-identical,
SHA-256
`c62ae42b467ee4954519af61d711575b110299da33115140448cf260308e699a`.
Their binary-first ABI verification, C89 compatibility, small-change, and
no-inline-assembly guidance is followed. Their Clang functional-decomp,
knowledge-base patching, runtime, and `rtk` mechanisms concern a different
later-XBE project and are not admission or mutation tools here.

Claude's 11,329-byte matching methodology is byte-identical to the current
repository copy, SHA-256
`3eb52f6b2df7b94b4d732161c0f86e2becebbcf0b546824deb957882118b1b5c`.
Its padded-byte, normalized-relocation, ABI, ownership, first-shot, and
whole-TU regression rules are the direct gates below. The hardened comparator
and regression gate are respectively SHA-256
`411e41698a9491a7b69430307d03f844ee3144633e5d3038d3ffde81a8eccfb7`
and
`d09b46dd2331ff7ca677cad78a66c780c4effa41a5389f0c614c9760d7632a5f`.

## Exact January packet and closure

January owns one external storage-class-2, type-`0x20` cdecl function. Its
complete 16-byte COMDAT is:

```text
B0 01 C3 90 90 90 90 90 90 90 90 90 90 90 90 90
```

The first three bytes are `mov al, 1; ret`; the remaining 13 bytes are NOP
padding. There are zero relocations. The padded normalized SHA-256 is
`1191f37eec740e495eda01ae2ac0969bf44c3c661ce4ae90583b5fdc05c0f707`.

The two pointer parameters are independently established by the HCEA PDB/body
and corroborated by a later decorated `@rasterizer_geometry_stripify@8`
symbol. January's leading underscore and COFF metadata decide cdecl external
linkage. No repository caller or address-taken edge exists, and the current
`triangle_buffer` and `vertex_buffer` definitions already supply the exact
typed parameter vocabulary. A header declaration would add unneeded shared
surface area and is intentionally omitted.

## Frozen readable body

The sole retained definition is inserted after the already exact byte-swap
stub, preserving the relative target order of every emitted function section:

```c
boolean rasterizer_geometry_stripify(
	struct triangle_buffer *triangle_buffer,
	struct vertex_buffer *vertex_buffer)
{
	return TRUE;
}
```

The source is C89-compatible, gives each parameter its own line, and returns
explicitly. It contains no assembly, `volatile`, `register`, pragma,
intrinsic, attribute, barrier, raw address or offset, pointer/integer
reconstruction, cast or union pun, undefined behavior, synthetic caller or
anchor, byte patch, comparator exception, or alternate compiler control.

## First-shot discipline and strict result

Before source mutation, the isolated baseline object was rebuilt from the
clean cumulative commit. It is 4,107 bytes with SHA-256
`e34720b345347fe74e338aea5ab5172988eed9c0800435dd59a057208e190f41`.
The pre-wave whole-TU manifest is 457,098 bytes with SHA-256
`3dd5169909b8d564b94ce75fdc0f1b535aa352f3c3c99fda79599a1e9732fdcd`.

The frozen source first passed XDK `/Zs` with the exact production arguments.
The baseline object's length, timestamp, and SHA-256 remained unchanged. The
resolved absolute production object path was then proved inside the isolated
worktree, exactly that literal file was deleted, absence was verified, and a
selected-object dry run named one edge. The unchanged ordinary edge ran once:

```text
[1/1] CL build\base\source\rasterizer\rasterizer_geometry.obj
rasterizer_geometry.c
```

The preserved first artifact is 4,349 bytes, SHA-256
`284eb6f22c694e8003302890eb7b25c4fc011bab44b64f0709976bcfe3ad673d`.
Direct hardened comparison proves the new function strict on that artifact:
16 padded bytes, zero relocations, and the January normalized hash above.
No second candidate compile or source-shape tune exists.

Direct hardened comparison also proves all eight inherited exact functions
remain strict. The complete accepted runtime table is:

| Function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_uncompress_int8_to_real` | 21 | 32 | 1 | `c8d445f22bb94140d08cc78f9d312c2d55474343075bc7ddcf3caa8d620da8ca` |
| `_uncompress_int16_to_real` | 29 | 32 | 2 | `55efeed54cdddbcddd62789b26e398aa30207db3dd5e87fa475a2787fc7b9886` |
| `_rasterizer_geometry_byte_swap_vertices` | 1 | 16 | 0 | `499f1f307c1cb989f968a6b7fcaec591e1828877223d0b0e7e8e8b76cde8c9ca` |
| `_rasterizer_geometry_stripify` | 3 | 16 | 0 | `1191f37eec740e495eda01ae2ac0969bf44c3c661ce4ae90583b5fdc05c0f707` |
| `_environment_vertex_compressed_get_point` | 103 | 112 | 8 | `422ed56a44b2b6eea91e984a52987163bac6e59c74f3b50126fc6c9155a8665d` |
| `_environment_vertex_compressed_get_normal` | 124 | 128 | 9 | `ea7cc0a60709c0310e4f59c2324c5b7937bf2a1426c9d0c546fb9e8948dceb60` |
| `_environment_vertex_compressed_get_texcoord` | 98 | 112 | 8 | `aa14c02f57fdc23f55183c2804d5b2c16824dde32c59f3741cc92e3bb2331272` |
| `_environment_lightmap_vertex_compressed_get_incident_radiosity` | 123 | 128 | 9 | `2bf9486ce2c1a183e94bc214e54b4017a26cd66bd3cb7b752101df2f31a673e7` |
| `_environment_lightmap_vertex_compressed_get_texcoord` | 140 | 144 | 12 | `5fb82963b6c9e014802b61529ce70a855fb730120cfb23e2d28974657e8559ab` |

The pre-wave regression check reports stripify as `NEWLY_EXACT`, has an empty
`changed_nonexact` list, and flags the expected downstream COMDAT/symbol/debug
renumbering caused by inserting a new external function before the five
accessors. It also flags compiler-owned string sections whose COFF indices
move. These generic findings are reviewed rather than waived: direct strict
comparison proves every complete padded accessor section, normalized byte,
ordered relocation address/type/destination/addend, and external owner still
exact. No adjudication or semantic exception is added.

The first artifact defines exactly the nine functions in the table. Its only
undefined externals are `__fltused`, `_display_assert`, `_system_exit`, and
`_uncompress_int32_to_real_vector3d`. It introduces no source-defined runtime
data, BSS, COMMON owner, or string. The target's 864 data bytes remain
uncredited.

## First-shot validation

The retained source blob is
`4b8701acfc443808777a59cb27fb0ce1570e2f86`, with 6,450 physical bytes,
226 CRLF endings, zero bare LF endings, and SHA-256
`de6617a6b9b34281892b3bb160b267252a22088cab8826e1896e6eb523692688`.

The complete Halo-plus-libcmt object graph and report generation pass. The
canonical outputs are:

- `build/report.json`: SHA-256
  `ea4fbffd31ffc708bdc691b3a1e35c5bab5ef0148af61321ca361d71d2280cc1`;
- `build/semantic_report.json`: SHA-256
  `76386592b31ee8aeb78081c731281f74c9df5d61b01f16b3b7185c647484704f`.

The direct report records `rasterizer_geometry.obj` at 9/19 exact functions,
642/4,054 meaningful code bytes, and 0/864 credited data bytes. Campaign
progress becomes:

- all categories: 375/833 matched objects, 4,144/11,060 exact functions,
  502,350/2,198,102 meaningful code bytes, and 1,835,208/4,176,062 data
  bytes;
- Halo: 273/468 matched objects, 3,977/7,574 exact functions,
  489,436/1,770,166 meaningful code bytes, and 1,830,016/3,923,451 data
  bytes;
- libcmt: 102/212 matched objects, 167/476 functions, 12,914/55,015 code
  bytes, and 5,192/8,637 data bytes.

The strict semantic audit reports 470 units, 4,251 functions evaluated,
3,478 missing, 4,109 semantic exact, 114 hidden exact / 64,705 hidden code
bytes, 36 ordinary-only, 35 structural, one rejected, zero unit errors, 12
local skips, and 4,170 accepted exact functions.

Object admission remains zero candidates and zero revocations, with only the
inherited unrelated `source/shell/shell_xbox` completion-label contradiction.
Park validation remains three active, zero stale, and zero invalid entries.
All 179 tooling tests pass. All selected configuration, canonical-report, and
audit JSON files parse.

The exact eight-path protected guard and the complete `source/units/**` tree
have zero intersection with this wave. No push, amend, rebase, history rewrite,
or worktree removal is performed.

## Clean committed-state replay

The implementation-and-ledger commit is
`b5ab0e9bc0a1870bec5f0cabe7e147d4f59888d4`, authored and committed by
Jonas Volman. The committed source remains blob
`4b8701acfc443808777a59cb27fb0ce1570e2f86`, with the physical size, line
endings, and SHA-256 recorded above. The initial ledger blob is
`f5ca2f1681a965b04c0654ae7c500125473d4935`.

The tracked worktree was clean at that commit. The 4,349-byte first artifact
was preserved again as `pre-committed-replay.obj`. The resolved production
object path was proved inside the isolated worktree, exactly that literal
artifact was deleted, absence was verified, and the selected-object dry run
again named one edge. The unchanged ordinary edge then reported:

```text
[1/1] CL build\base\source\rasterizer\rasterizer_geometry.obj
rasterizer_geometry.c
```

This is a clean committed reproducibility replay, not a source-shape or tuning
retry. The 4,349-byte result is preserved as `committed-replay.obj`, SHA-256
`34d5f78f96304d9bb6ea876a2d95107b54b8888aa88ea5df5698ee241bac5405`.
Its raw hash differs from the first artifact because the COFF producer embeds
phase-specific compiler/debug metadata. Direct hardened comparison proves
identical runtime evidence: all nine complete padded function sections, every
normalized hash in the table, and all ordered relocations remain strict.

The replay's external defined-function inventory is exactly the nine
functions in the table. Its undefined inventory remains exactly `__fltused`,
`_display_assert`, `_system_exit`, and
`_uncompress_int32_to_real_vector3d`. No source-defined runtime data, BSS,
COMMON owner, or string is introduced.

The clean accepted-state manifest is 460,754 bytes, SHA-256
`81f53f62845e0e142de5ff2175d889c6be3d404d08efb0f6786686eb96133960`.
Its immediate no-build self-check has no failure or warning, empty
`newly_exact` and `changed_nonexact` arrays, and exactly the nine functions in
the table under `still_exact`.

The replayed canonical outputs are 1,574,280-byte `build/report.json`,
SHA-256
`ea4fbffd31ffc708bdc691b3a1e35c5bab5ef0148af61321ca361d71d2280cc1`,
and 2,976,116-byte `build/semantic_report.json`, SHA-256
`76386592b31ee8aeb78081c731281f74c9df5d61b01f16b3b7185c647484704f`.
They reproduce every first-shot object, semantic, and campaign count above.
The complete source-object graph returns `ninja: no work to do` after replay.

Admission remains zero candidates / zero revocations plus the inherited
unrelated `shell_xbox` contradiction. Parks remain three active / zero stale /
zero invalid. All 179 tooling tests pass, and all selected JSON files parse.
The tracked worktree was otherwise clean before this additive replay record.

## Corrected cumulative-HEAD closure

The isolated replay record is commit
`2b25f7cd3ca10e64922c85487a9fa70288650dd3`. The authoritative
`jonas/units-integration-20260824` branch was cleanly fast-forwarded from
`ea21f77bab8fbc719115dfc80a3a35f80e1ece02` through both isolated commits.
The committed source remains blob
`4b8701acfc443808777a59cb27fb0ce1570e2f86`, with 6,450 physical bytes,
226 CRLF endings, zero bare LF endings, and SHA-256
`de6617a6b9b34281892b3bb160b267252a22088cab8826e1896e6eb523692688`.

Before corrected replay, the cumulative worktree still held its 4,096-byte
stripify-free `rasterizer_geometry.obj`, SHA-256
`c9af67d09178b8f5a18f025ad7c2c3c8bd4d8cd314bbb6abcfd6b31aaae6158c`.
That artifact was preserved. Its resolved path was proved inside the
cumulative worktree, exactly that literal file was deleted, absence was
verified, and the selected-object dry run named one edge. The unchanged
ordinary edge then reported:

```text
[1/1] CL build\base\source\rasterizer\rasterizer_geometry.obj
rasterizer_geometry.c
```

The corrected replay is preserved as
`build/audit/rasterizer_geometry_stripify_corrected_head_20260826/corrected-replay.obj`,
4,338 bytes, SHA-256
`6e1ec94eb90bc3daf4cd030f4c179a7b07642d98aba8de29b97b6aa146850d52`.
Its raw size and hash differ from the isolated replay because the compiler
debug records encode a different absolute worktree/output path. Direct
hardened comparison proves identical runtime evidence: all nine padded
function sections, every normalized hash in the table, and every ordered
relocation identity are strict.

The corrected clean manifest is 460,754 bytes, SHA-256
`39af33774b5cce9cd53d54a2a40a916a76851d6173f859f3770977de3e35870f`.
Its immediate no-build check is fully clean: exactly nine `still_exact`
functions, empty `newly_exact` and `changed_nonexact` arrays, and no failure or
warning.

The cumulative canonical outputs reproduce the isolated results byte for
byte:

- `build/report.json`: 1,574,280 bytes, SHA-256
  `ea4fbffd31ffc708bdc691b3a1e35c5bab5ef0148af61321ca361d71d2280cc1`;
- `build/semantic_report.json`: 2,976,116 bytes, SHA-256
  `76386592b31ee8aeb78081c731281f74c9df5d61b01f16b3b7185c647484704f`.

They reproduce 9/19 exact functions and 642/4,054 meaningful code bytes for
the object, 4,170 accepted semantic functions, 4,144/11,060 campaign
functions, and 502,350/2,198,102 meaningful campaign code bytes. Halo remains
at 3,977/7,574 functions and 489,436/1,770,166 meaningful code bytes.

The complete source-object graph returns `ninja: no work to do`. Admission is
zero candidates / zero revocations plus the inherited unrelated `shell_xbox`
contradiction. Parks are three active / zero stale / zero invalid. All 179
tooling tests pass, and all 59 recursively selected configuration, canonical,
and audit JSON files parse.

Final tracked scope from the pre-wave base is exactly
`source/rasterizer/rasterizer_geometry.c` plus this ledger, with zero tracked
deletion. No header, configuration, existing ledger, semantic record, parked
record, Units source, or protected/shared interface changed. The cumulative
worktree was otherwise clean before this additive closure record. No push,
amend, rebase, history rewrite, or worktree removal is performed.
