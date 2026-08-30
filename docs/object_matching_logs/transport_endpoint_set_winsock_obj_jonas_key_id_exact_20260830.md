# `transport_endpoint_set_winsock.obj` typed key-ID leaf (2026-08-30)

## Scope and result

The first and only natural production candidate for
`source/bungie_net/network/transport_endpoint_set_winsock.obj::_transport_get_key_id`
is strict exact.  It contributes 53 meaningful / 64 padded code bytes and
seven ordered relocations.  Together with the inherited exact
`_transport_get_xnaddr`, the object advances from 1/26 to 2/26 exact functions
and from 48/3,808 to 112/3,808 strict padded bytes.  It remains incomplete and
`NonMatching`.

This lane changes only the owner source and this additive ledger.  No header,
configuration, target artifact, tool, semantic exception, parked record,
completion label, Units, Vehicles, Matrix Math, AI Debug, or active agent lane
is changed.  Nothing is pushed.

## Authority and provenance

The January authority is `cachebeta.exe`, 6,584,672 bytes, SHA-256
`4CC87B45F721270392A96F1674ED2B5CD4A7BB4355FAEAB4531D1CF1884D9520`.
Its generated owner object has raw SHA-256
`912EB719656C8E64376D86499B22A7505144CA7E4A485B960D472AD1420713C1`.
The unchanged Microsoft VC7 13.00.9254.1 compiler has SHA-256
`483E00C47BB08D699475A642BCFF15B5B2036350B31C540E88A506BAF101DA11`
and uses the normal `/O2 /Oy- /DDEBUG /Dxbox` edge.  This lane began at
canonical commit `62ef6b955e2eec827b0dbb398c8218c99a209ceb`.

The stock XDK 3911 `xbox/include/WinSockX.h` declaration is Git blob
`9fd916ccda1beec852108780bbf467cfc59be08e`, payload SHA-256
`726C8E558CEC1E99352DF9F687004387A60FA15BB252CFC07EECD2391E3BDBD9`.
It defines `XNKID` as one named eight-byte `BYTE ab[8]` member.  The retained
`<xtl.h>` include reaches that header through its ordinary `winsockx.h`
include, so no local surrogate, integer overlay, or invented layout is needed.

The authenticated Stian tree is commit
`fc11d861865735877d5b8b4c408b1a0c06dde3cf`; its corroborating translation
unit is Git blob `eb0e747275325ecca24406f8187ff3585937a028`, payload SHA-256
`5B29F184398C7228FDD95BCC073C30ABD27B6E5DFD87DD95AA3A241E905ECBAC`.
The independent Pastudan tree is commit
`918af885935ec470a31256ecce9a977b12b01f80`; its counterpart is Git blob
`65f6d377bde90704425da6451e7232f1f2746c59`, payload SHA-256
`634BA61D818A1D2D419CF19C4CA8098CD1706E39721225C24DFE1DD37997A9AF`.
Both preserve the `global_key_depth > 0` assertion, exact source path and line
`0xE0`, and the two-dword return in EDX:EAX.  Stian's network-server caller
also stores the complete eight-byte result into its message body.  Their raw
address and `int64_t` spelling is semantic corroboration only; January plus
the stock XDK fix the retained named type.

The acceleration playbook, object-ledger doctrine, COFF/campaign tooling
READMEs, both prior object ledgers, and applicable Claude `AGENTS.md` /
`CLAUDE.md` doctrine were audited before mutation.  `git log --all -S
transport_get_key_id -- source/bungie_net/network/transport_endpoint_set_winsock.c`
reaches only initial inventory commit `e830efbd`.  A read-only census of all
1,529 registered worktrees found 1,529 owner files, four unique payloads, and
zero actual key-ID function bodies.  No parked entry or earlier object ledger
mentions this function.

## Frozen January packet

The function is ordinary public cdecl with an explicit no-argument signature.
It asserts the target-owned signed 32-bit key depth is positive, then returns
the external eight-byte key ID in EDX:EAX.  It has normalized SHA-256
`ad7e97ac5d79c623de68dd37e1717903af5470956bbd20da1a46942b6ea40128`
over 64 padded bytes and this exact relocation schedule:

| Offset | Type | Destination |
| ---: | ---: | --- |
| `+0x01` | `DIR32` | `_global_key_depth + 0` |
| `+0x11` | `DIR32` | exact transport source-path literal |
| `+0x16` | `DIR32` | exact `global_key_depth > 0` literal |
| `+0x1B` | `REL32` | `_display_assert` |
| `+0x22` | `REL32` | `_system_exit` |
| `+0x2A` | `DIR32` | `_global_key_id + 0` |
| `+0x30` | `DIR32` | `_global_key_id + 4` |

The January owner defines `_global_key_depth` inside its eight-byte BSS record.
The current incomplete reconstruction deliberately leaves that storage and
the independently owned `_global_key_id` external.  The strict comparator
proves the same symbolic relocation target and addend; this lane claims no
data bytes and does not create a partial or duplicate BSS owner.

## Retained typed C89 candidate

The source adds the ordinary assertion declaration surface, names both globals
with their proven types, and expresses the full return directly:

```c
extern long global_key_depth;
extern XNKID global_key_id;

XNKID transport_get_key_id(
	void)
{
	match_assert(
		"c:\\halo\\SOURCE\\bungie_net\\network\\transport_endpoint_set_winsock.c",
		0xE0,
		global_key_depth > 0);
	return global_key_id;
}
```

Retained source is Git blob
`e691e877cc53bf005a1b5e613ecd376f594fdaad`, working-tree SHA-256
`5BA247906253B0FD655376A9B2EDDF3DFC8B3DF095106AB756F05CF217C1177E`.
The pre-lane exact-XNADDR source was Git blob
`e0d6ae54765f3d120627c8fa5365ffff2b24c491`.

VC7 `/Zs` passed before the one sealed code-producing adjudication compile.
That object has raw SHA-256
`A4C78D0A82FBDE697D77909E8BBE2C4205656975563EB1BB1564DD15D42E49F9`.
The required full Ninja gate re-emitted the same frozen source under the same
flags and produced raw object SHA-256
`F85A24BF3207F3D2B38D58352B9D825197C166116FCCA1EF957D395F7757FECA`.
Both artifacts have the identical strict function packet above.  No alternate
body, type, declaration order, expression, control flow, compiler option,
sweep, or tune was compiled.

The retained code contains no assembly, `volatile`, `register`, pragma,
intrinsic, attribute, barrier, raw address or offset access, pointer/integer
reconstruction, pun, undefined behavior, byte forcing, compiler/tool
exception, or comparator waiver.  The normal `<xtl.h>` duplicate SDK COMDATs
remain unreferenced and linker-discardable exactly as audited by the prior
XNADDR ledger; no new data ownership is claimed.

## Strict regression and full gates

The hardened comparator proves all 64 padded bytes and every relocation's
address, type, identity, addend, and order.  The whole-object exact-set gate
reports inherited `_transport_get_xnaddr` preserved, `_transport_get_key_id`
newly exact, and no loss.

The complete `halobetacache_build`, `libcmt_build`, `progress`, and
`semantic_progress` graph passes.  Semantic output reports 470 units, 4,941
evaluated functions, 4,824 semantic-exact functions, 4,834 accepted-exact
functions, and zero unit errors.  Strict progress is 384/833 objects,
4,795/11,060 functions, 609,222/2,198,102 meaningful code bytes, and
1,856,050/4,176,062 data bytes.  Halo progress is 282/468 objects,
4,628/7,574 functions, and 596,308/1,770,166 meaningful code bytes.

Protected `_unit_preprocess_node_orientations` remains exact at 1,920 padded
bytes, 87 ordered relocations, and normalized SHA-256
`5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
Admission reports zero candidates, contradictions, and revocations.  Parked
validation reports 12 active, zero stale, and zero invalid entries.  Pytest
passes all 212 tests.  `git diff --check` passes.

`build/report.json` is 1,600,113 bytes with SHA-256
`E9B13B931FE2ED4B4584B54DA3BEFD4218940530BDF4E845DAA0ED281D1FEF29`.
`build/semantic_report.json` is 3,282,863 bytes with SHA-256
`5F63A5F83195CB20389AD6EE6D9131030BCC92AB6EF818F2AE2AB6070941D057`.
