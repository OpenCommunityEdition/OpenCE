# `transport_endpoint_set_winsock.obj` typed XNADDR leaf (2026-08-30)

## Scope and result

This strictly bounded lane recovered one previously unwritten public leaf:

`source/bungie_net/network/transport_endpoint_set_winsock.obj::_transport_get_xnaddr`.

One natural readable C89 candidate was frozen from January code and relocation
evidence, the XDK 3911 `XNADDR` declaration, and two authenticated public-source
donors.  It is strict exact: 36 meaningful / 48 padded bytes, three ordered
relocations, and identical complete normalized bytes.  The object advances
from 0/26 to 1/26 strict functions and from 0/3,808 to 48/3,808 exact padded
bytes.

No sibling function, header, configuration, target artifact, tool, semantic
exception, parked record, protected source, or pre-existing Markdown was
changed.  Nothing is pushed.

## Authority and provenance

The January PC authority is `cachebeta.exe`, 6,584,672 bytes, SHA-256
`4CC87B45F721270392A96F1674ED2B5CD4A7BB4355FAEAB4531D1CF1884D9520`.
Its generated owner object has SHA-256
`912EB719656C8E64376D86499B22A7505144CA7E4A485B960D472AD1420713C1`.
The Microsoft VC7 13.00.9254.1 compiler has SHA-256
`483E00C47BB08D699475A642BCFF15B5B2036350B31C540E88A506BAF101DA11`.
The production edge retains `/O2 /Oy- /DDEBUG /Dxbox` and the repository's
normal include graph.

The stock XDK 3911 `xbox/include/WinSockX.h` declaration is Git blob
`9fd916ccda1beec852108780bbf467cfc59be08e`, file SHA-256
`726C8E558CEC1E99352DF9F687004387A60FA15BB252CFC07EECD2391E3BDBD9`.
It proves that this SDK's `XNADDR` is exactly 12 bytes: two named header bytes,
a six-byte Ethernet address, and a four-byte IP address.

The authenticated Stian tree is commit
`fc11d861865735877d5b8b4c408b1a0c06dde3cf`.  Its corroborating translation
unit is Git blob `eb0e747275325ecca24406f8187ff3585937a028`, file SHA-256
`5B29F184398C7228FDD95BCC073C30ABD27B6E5DFD87DD95AA3A241E905ECBAC`.
The read-only Pastudan tree is commit
`918af885935ec470a31256ecce9a977b12b01f80`; its counterpart is Git blob
`65f6d377bde90704425da6451e7232f1f2746c59`, file SHA-256
`634BA61D818A1D2D419CF19C4CA8098CD1706E39721225C24DFE1DD37997A9AF`.
Both donors independently preserve the three-dword copy and unchanged
destination return.  Their raw-address spelling was used only as semantic
corroboration and was not imported.

The acceleration playbook, relevant object ledger, campaign and COFF tooling
documentation, and applicable donor `AGENTS.md` / `CLAUDE.md` instructions
were read before mutation.  The only prior object wave tested the unrelated
`_transport_dispose` and `_transport_network_available` functions and restored
its source.  `git log --all -S transport_get_xnaddr` reaches only the initial
symbol inventory.  A census of 1,561 copies of the translation unit found no
production body in any campaign worktree; all seven bodies were authenticated
donor/reference copies.  No existing ledger mentions this symbol.  This was a
new candidate.

## Frozen January packet

January loads three consecutive dwords from external `_global_address`, stores
them to the caller's destination, and returns that destination unchanged in
`EAX`.  The complete function has:

- 36 meaningful / 48 padded bytes;
- three `IMAGE_REL_I386_DIR32` relocations at `+0x05`, `+0x12`, and `+0x1B`;
- destinations `_global_address+0`, `_global_address+4`, and
  `_global_address+8`, in that order;
- normalized SHA-256
  `553d449156792f691f23612621989ea47c3317eba7dddd3f225a2b3172545b0f`.

The entry is ordinary public cdecl with one pointer stack argument.  It calls
nothing, crosses no private ABI, and owns no runtime storage.

## Retained typed C89 candidate

The candidate includes the stock Xbox SDK, leaves January's named address
global external, and expresses the operation as the natural whole-structure
copy:

```c
extern XNADDR global_address;

XNADDR *transport_get_xnaddr(
	XNADDR *address)
{
	*address = global_address;
	return address;
}
```

Candidate source is Git blob
`e0d6ae54765f3d120627c8fa5365ffff2b24c491`, working-tree SHA-256
`2591820607664D0AAD4A4647CF1AA16A0E8DB6E52903F1F6E537FCDCB05C4767`.
VC7 `/Zs` passed before code emission.  The first sealed object had raw
SHA-256
`E643612758F87C855F4C0BA90B01FEBEE39D61F059EE35E62A3C4351FC8A86B4`.
The mandatory Ninja full-tree gate re-emitted the identical sealed source
because the direct adjudication compile did not populate Ninja's dependency
database; no source byte, type, flag, expression, or candidate variant changed.
The rebuilt object has raw SHA-256
`50D4A489074529CC38D37DDF7E83CFC5E7CF652E7BCC09DA28F55754A6520F5A`.
Both emissions produce the same strict function packet recorded above.

`tools.coff_compare.section_infos_equal` is true.  Target and candidate
disassembly are instruction-for-instruction identical through all twelve NOP
padding bytes, and every relocation address, type, identity, addend, and order
is identical.

The source contains no assembly, `volatile`, `register`, pragma, intrinsic,
attribute, barrier, raw address or offset access, pointer/integer
reconstruction, pun, undefined behavior, byte forcing, compiler/tool
exception, or comparator waiver.

## Candidate-only SDK COMDAT audit

The normal `<xtl.h>` include emits three unreferenced
`IMAGE_COMDAT_SELECT_ANY` SDK constants.  They have no relocations from this
object, are byte-identical to the January-owned copies already accepted by the
campaign, and are discarded by the ordinary link:

| Symbol | Bytes | Normalized SHA-256 | January owner |
| --- | ---: | --- | --- |
| `_D3DTEXTUREDIRECTENCODE` | 16 | `2df64c1861ed539052d35303c159771b5caeaaed5bfa04b343eb47d48329f807` | `source/cseries/headers.obj` |
| `_D3DSIMPLERENDERSTATEENCODE` | 328 | `e29e444c4976f669d2d4606f5795ee3a6c65d326bbdf8312b31dc735487075f3` | `source/interface/progress_bar.obj` |
| `_D3DPRIMITIVETOVERTEXCOUNT` | 88 | `45dcae937d7b20c7c6c2bacbe90deabbbaa9311f99cd26e5387a6d2a21de1404` | `source/rasterizer/xbox/rasterizer_xbox_decals.obj` |

This is the playbook's reviewed duplicate-SDK-COMDAT representation case, not
newly claimed target data.  `_global_address` remains undefined in this owner.

## Regression and validation gates

The complete `halobetacache_build`, `libcmt_build`, `progress`, and
`semantic_progress` graph passes.  Semantic output advances to 4,804 strict
semantic-exact and 4,814 accepted-exact functions across 4,921 evaluated
functions and 470 units, with zero unit errors.  Strict progress advances to
4,775/11,060 functions and 607,010/2,198,102 meaningful code bytes overall;
Halo advances to 4,608/7,574 functions and 594,096/1,770,166 meaningful code
bytes.  No pre-existing exact function is lost.

`build/report.json` is 1,599,356 bytes with SHA-256
`38F7766AF883FA32BA56A96C4D5CF4D203CBC1EE21D4DF35B92437025DD6F144`.
`build/semantic_report.json` is 3,269,211 bytes with SHA-256
`E1CCC58E10E22F233D926FBF64B599D65E00947A9F09DFCA666A79D8A189EFDF`.

The whole translation unit's strict set is exactly the newly recovered
`_transport_get_xnaddr` packet; the pre-wave owner contained no exact function,
so the exact-set loss gate is empty and preserved.  The protected Units
sentinel remains exact at 1,920 padded bytes, 87 ordered relocations, normalized
SHA-256
`5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
Admission reports zero candidates, contradictions, and revocations.  Parked
audit reports 12 active, zero stale, and zero invalid.  Pytest passes all 212
tests, with only the sandbox's optional cache warning.  `git diff --check`
passes.  Only the typed source and this additive ledger are committed locally.
