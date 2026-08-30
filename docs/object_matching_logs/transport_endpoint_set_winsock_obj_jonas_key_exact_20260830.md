# `transport_endpoint_set_winsock.obj` typed transport-key leaf (2026-08-30)

## Scope and result

The first and only natural production candidate for
`source/bungie_net/network/transport_endpoint_set_winsock.obj::_transport_get_key`
is strict exact. It contributes 86 meaningful / 96 padded code bytes and nine
ordered relocations. Together with the inherited exact XNADDR and key-ID
accessors, the object advances from 2/26 to 3/26 exact functions and from
112/3,808 to 208/3,808 strict padded bytes. The object remains incomplete and
`NonMatching`.

This lane changes only the owner source and this additive ledger. It does not
change a header, configuration, target artifact, tool, semantic exception,
parked record, completion label, data owner, Units, Vehicles, Matrix Math, AI
Debug, or another agent's lane. Nothing is pushed.

## Authority and provenance

The January authority is `cachebeta.exe`, 6,584,672 bytes, SHA-256
`4CC87B45F721270392A96F1674ED2B5CD4A7BB4355FAEAB4531D1CF1884D9520`.
Its generated owner object has raw SHA-256
`912EB719656C8E64376D86499B22A7505144CA7E4A485B960D472AD1420713C1`.
The unchanged Microsoft VC7 13.00.9254.1 compiler has SHA-256
`483E00C47BB08D699475A642BCFF15B5B2036350B31C540E88A506BAF101DA11`
and uses the ordinary `/O2 /Oy- /DDEBUG /Dxbox` edge. This lane began at
canonical commit `28fcdd96e7bde1d42a85231db1366e81ca2e5e7c`.

The stock XDK 3911 `xbox/include/WinSockX.h` is Git blob
`9fd916ccda1beec852108780bbf467cfc59be08e`, payload SHA-256
`726C8E558CEC1E99352DF9F687004387A60FA15BB252CFC07EECD2391E3BDBD9`.
It defines `XNKEY` as a named structure containing `BYTE ab[16]`. The existing
ordinary `<xtl.h>` include reaches that declaration, so the retained code needs
no surrogate, integer overlay, or invented layout.

The authenticated Stian tree is commit
`fc11d861865735877d5b8b4c408b1a0c06dde3cf`; its corroborating translation
unit is Git blob `eb0e747275325ecca24406f8187ff3585937a028`, payload SHA-256
`5B29F184398C7228FDD95BCC073C30ABD27B6E5DFD87DD95AA3A241E905ECBAC`.
The independent Pastudan tree is commit
`918af885935ec470a31256ecce9a977b12b01f80`; its counterpart is Git blob
`65f6d377bde90704425da6451e7232f1f2746c59`, payload SHA-256
`634BA61D818A1D2D419CF19C4CA8098CD1706E39721225C24DFE1DD37997A9AF`.
Both preserve the `global_key_depth > 0` assertion, exact source path and line
`0xE7`, and a complete four-dword copy from the transport-key global into the
caller's return buffer. Their raw-address, `void *`, and integer spelling is
semantic corroboration only; January's hidden return-buffer ABI and the stock
XDK fix the retained named by-value type.

The acceleration playbook, object-ledger doctrine, COFF/campaign tooling
READMEs, all prior object ledgers, and applicable Claude `AGENTS.md` /
`CLAUDE.md` doctrine were audited before mutation. Reachable Git history and
tracked source contain no earlier function body. A read-only census covered
1,533 registered worktrees and 1,533 owner files: the only definition was the
current sealed candidate, so there was no pre-existing campaign attempt. The
only other definitions found outside the registered campaign worktrees were
the authenticated Stian/Pastudan research copies above. No parked entry or
earlier object ledger freezes this function.

## Frozen January packet

The function is ordinary public cdecl with an explicit no-argument signature.
VC7 lowers the 16-byte structure return through the caller-provided hidden
return buffer, copies all four dwords from `_global_key`, leaves that buffer in
EAX, and returns normally. The packet has normalized SHA-256
`b9a360c1c629f15550c43f2aff8497d795967a219d0dc7bd7a07bda9d8577b5e`
over 96 padded bytes and this exact relocation schedule:

| Offset | Type | Destination |
| ---: | ---: | --- |
| `+0x04` | `DIR32` | `_global_key_depth + 0` |
| `+0x14` | `DIR32` | exact transport source-path literal |
| `+0x19` | `DIR32` | exact `global_key_depth > 0` literal |
| `+0x1E` | `REL32` | `_display_assert` |
| `+0x25` | `REL32` | `_system_exit` |
| `+0x2E` | `DIR32` | `_global_key + 0` |
| `+0x3B` | `DIR32` | `_global_key + 4` |
| `+0x44` | `DIR32` | `_global_key + 8` |
| `+0x4D` | `DIR32` | `_global_key + 12` |

The January object owns `_global_key_depth` inside its BSS. The incomplete
reconstruction deliberately leaves that storage and `_global_key` external.
The strict comparator proves the same symbolic targets and addends; this lane
claims no data bytes and creates no partial or duplicate data owner.

## Retained typed C89 candidate

The source adds only the proven external type and direct named return:

```c
extern XNKEY global_key;

XNKEY transport_get_key(
	void)
{
	match_assert(
		"c:\\halo\\SOURCE\\bungie_net\\network\\transport_endpoint_set_winsock.c",
		0xE7,
		global_key_depth > 0);
	return global_key;
}
```

Retained source is Git blob
`5f8a44249480c5278f1d8841311807569fdde544`, working-tree SHA-256
`8C17E0816A44C5D2B1E0F74957B1872BC5318D3738A447B0D0CB00165DD405D9`.
The pre-lane source was Git blob
`e691e877cc53bf005a1b5e613ecd376f594fdaad`.

VC7 `/Zs` passed before the one sealed code-producing adjudication compile.
That object has raw SHA-256
`7210F1F7CF00E0BAADDF6332F223DD2FC877AFE424DC7AB7F1D57ACCEFD5B43A`.
The required full Ninja gate re-emitted the same frozen source under the same
flags and produced raw object SHA-256
`BEF92B0F1CA9C1F30566F5AC4E6D302D39BBC554F6F8166BD3857EE771EED53`.
Both artifacts have the identical strict function packet above. No alternate
body, type, declaration order, expression, control flow, compiler option,
sweep, or tune was compiled.

The retained code contains no assembly, `volatile`, `register`, pragma,
intrinsic, attribute, barrier, raw address or offset access, pointer/integer
reconstruction, pun, undefined behavior, byte forcing, compiler/tool
exception, or comparator waiver.

## Strict regression and full gates

The hardened comparator proves all 96 padded bytes and every relocation's
address, type, identity, addend, and order. The whole-object exact-set gate
reports inherited `_transport_get_xnaddr` and `_transport_get_key_id`
preserved, `_transport_get_key` newly exact, and no loss.

The complete `halobetacache_build`, `libcmt_build`, `progress`, and
`semantic_progress` graph passes. Semantic output reports 470 units, 4,943
evaluated functions, 4,826 semantic-exact functions, 4,836 accepted-exact
functions, and zero unit errors. Strict progress is 384/833 objects,
4,797/11,060 functions, 609,364/2,198,102 meaningful code bytes, and
1,856,050/4,176,062 data bytes. Halo progress is 282/468 objects,
4,630/7,574 functions, and 596,450/1,770,166 meaningful code bytes.

Protected `_unit_preprocess_node_orientations` remains exact at 1,920 padded
bytes, 87 ordered relocations, and normalized SHA-256
`5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
Admission reports zero candidates, contradictions, and revocations. Parked
validation reports 12 active, zero stale, and zero invalid entries. Pytest
passes all 212 tests. `git diff --check` passes.

`build/report.json` is 1,600,198 bytes with SHA-256
`3FD6DF9DB4D3C769FE571CB584E7D5F29C95987C3C1ADF32BE0320D378143F82`.
`build/semantic_report.json` is 3,284,257 bytes with SHA-256
`46A1D8C98075BCF73A18311F94AF8655528F12CE9796E30452B3FC95149E9D85`.
