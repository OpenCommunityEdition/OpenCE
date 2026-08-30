# `transport_endpoint_set_winsock.obj` typed key-pop leaf (2026-08-30)

## Scope and result

The first and only natural production candidate for
`source/bungie_net/network/transport_endpoint_set_winsock.obj::_transport_pop_key`
is strict exact. It contributes 57 meaningful / 64 padded code bytes and eight
ordered relocations. Together with the inherited exact XNADDR, key-ID, and key
accessors, the object advances from 3/26 to 4/26 exact functions and from
208/3,808 to 272/3,808 strict padded bytes. It remains incomplete and
`NonMatching`.

This lane changes only the owner source and this additive ledger. No header,
configuration, target artifact, tool, semantic exception, parked record,
completion label, data owner, Units, Vehicles, Matrix Math, AI Debug, or other
agent lane changes. Nothing is pushed.

## Authority and provenance

The January authority is `cachebeta.exe`, 6,584,672 bytes, SHA-256
`4CC87B45F721270392A96F1674ED2B5CD4A7BB4355FAEAB4531D1CF1884D9520`.
Its owner object has raw SHA-256
`912EB719656C8E64376D86499B22A7505144CA7E4A485B960D472AD1420713C1`.
The unchanged Microsoft VC7 13.00.9254.1 compiler has SHA-256
`483E00C47BB08D699475A642BCFF15B5B2036350B31C540E88A506BAF101DA11`
and uses the ordinary `/O2 /Oy- /DDEBUG /Dxbox` edge. This lane began at
canonical commit `d370e03f`.

The stock XDK 3911 `xbox/include/WinSockX.h` is Git blob
`9fd916ccda1beec852108780bbf467cfc59be08e`, payload SHA-256
`726C8E558CEC1E99352DF9F687004387A60FA15BB252CFC07EECD2391E3BDBD9`.
It declares `INT WSAAPI XNetUnregisterKey(const XNKID * pxnkid)`, so the
retained call uses the natural const-qualified key-ID pointer and the XDK's
ordinary stdcall decoration. The existing `<xtl.h>` include supplies both the
function and `XNKID`; no surrogate prototype or calling-convention annotation
is introduced.

The authenticated Stian tree is commit
`fc11d861865735877d5b8b4c408b1a0c06dde3cf`; its corroborating translation
unit is Git blob `eb0e747275325ecca24406f8187ff3585937a028`, payload SHA-256
`5B29F184398C7228FDD95BCC073C30ABD27B6E5DFD87DD95AA3A241E905ECBAC`.
The independent Pastudan tree is commit
`918af885935ec470a31256ecce9a977b12b01f80`; its counterpart is Git blob
`65f6d377bde90704425da6451e7232f1f2746c59`, payload SHA-256
`634BA61D818A1D2D419CF19C4CA8098CD1706E39721225C24DFE1DD37997A9AF`.
Their lifted adjacent transport paths independently repeat the same release
block: assert `global_key_depth > 0` at line `0x66`, decrement the signed depth,
and unregister the eight-byte key ID exactly when the depth reaches zero.
Their raw-address spelling is semantic corroboration only; January and the
stock XDK determine the retained globals, signature, and import.

The acceleration playbook, ledger doctrine, COFF/campaign tooling READMEs,
all prior object ledgers, and applicable Claude `AGENTS.md` / `CLAUDE.md`
doctrine were audited before mutation. Reachable Git history contains no
earlier body. A pre-mutation read-only census covered 1,534 registered
worktrees and 1,534 owner files, finding zero definitions of
`transport_pop_key`. No parked entry or prior ledger freezes this function.

## Frozen January packet

The function is ordinary public cdecl `void(void)`. It asserts the signed
32-bit depth is positive, uses a direct memory `DEC`, and branches over the
stdcall XDK call unless that decrement reaches zero. The packet has normalized
SHA-256
`38902332dbf4881b10f0a7800e55aba0c5dad035591d32aada0192b4711c096f`
over 64 padded bytes and this exact relocation schedule:

| Offset | Type | Destination |
| ---: | ---: | --- |
| `+0x01` | `DIR32` | `_global_key_depth + 0` |
| `+0x0E` | `DIR32` | exact transport source-path literal |
| `+0x13` | `DIR32` | exact `global_key_depth > 0` literal |
| `+0x18` | `REL32` | `_display_assert` |
| `+0x1F` | `REL32` | `_system_exit` |
| `+0x28` | `DIR32` | `_global_key_depth + 0` |
| `+0x2F` | `DIR32` | `_global_key_id + 0` |
| `+0x34` | `REL32` | `_XNetUnregisterKey@4` |

The January object owns `_global_key_depth` in its BSS. This incomplete
reconstruction keeps that storage and `_global_key_id` external. The strict
comparator proves identical symbolic targets and addends; no data bytes or
duplicate storage are claimed.

## Retained typed C89 candidate

```c
void transport_pop_key(
	void)
{
	match_assert(
		"c:\\halo\\SOURCE\\bungie_net\\network\\transport_endpoint_set_winsock.c",
		0x66,
		global_key_depth > 0);
	global_key_depth--;
	if (global_key_depth == 0)
	{
		XNetUnregisterKey(&global_key_id);
	}
}
```

Retained source is Git blob
`eaa3f14b13df5c85a0ce1da2feed1a020cfc3882`, working-tree SHA-256
`ED464F8282C21D78F5F74942543CEA7CBBEFE2196FAF23915A7FE774534A0DCB`.
The pre-lane source was Git blob
`5f8a44249480c5278f1d8841311807569fdde544`.

VC7 `/Zs` passed before the one sealed code-producing adjudication compile.
That object has raw SHA-256
`E017056E743EC990D55717B8B7DD6D46A7640D16C77506CC9D9477D608E7F045`.
The full Ninja gate re-emitted the same frozen source and produced raw object
SHA-256
`989830B286E580A9455C14EBEE95C4C7827CBA1EAED2C5A16098286BCA42225B`.
Both artifacts contain the identical strict packet. No alternate body,
decrement form, condition, declaration, compiler option, sweep, or tune was
compiled.

The retained code contains no assembly, `volatile`, `register`, pragma,
intrinsic, attribute, barrier, raw address or offset access, pointer/integer
reconstruction, pun, undefined behavior, byte forcing, compiler/tool
exception, or comparator waiver.

## Strict regression and full gates

The comparator proves every padded byte and relocation address, type,
identity, addend, and order. The whole-object exact-set gate preserves
`_transport_get_xnaddr`, `_transport_get_key_id`, and `_transport_get_key`,
adds only `_transport_pop_key`, and loses nothing.

The complete `halobetacache_build`, `libcmt_build`, `progress`, and
`semantic_progress` graph passes. Semantic output reports 470 units, 4,945
evaluated functions, 4,828 semantic-exact functions, 4,838 accepted-exact
functions, and zero unit errors. Strict progress is 384/833 objects,
4,799/11,060 functions, 609,516/2,198,102 meaningful code bytes, and
1,856,050/4,176,062 data bytes. Halo progress is 282/468 objects,
4,632/7,574 functions, and 596,602/1,770,166 meaningful code bytes.

Protected `_unit_preprocess_node_orientations` remains exact at 1,920 padded
bytes, 87 ordered relocations, and normalized SHA-256
`5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
Admission reports zero candidates, contradictions, and revocations. Parked
validation reports 12 active, zero stale, and zero invalid entries. Pytest
passes all 212 tests. `git diff --check` passes.

`build/report.json` is 1,600,250 bytes with SHA-256
`2B8B6119132F4AC9101EC50A1E8420860D9F70D5B13D06EB8C58F20C9F997838`.
`build/semantic_report.json` is 3,285,663 bytes with SHA-256
`6004131218B029BFD21522B1111638375A89C7339EDBC2F3AFDF2820C95975DF`.
