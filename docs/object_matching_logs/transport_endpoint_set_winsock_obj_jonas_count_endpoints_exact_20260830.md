# `transport_endpoint_set_winsock.obj` typed endpoint-count leaf (2026-08-30)

## Scope and result

The first and only natural production candidate for
`source/bungie_net/network/transport_endpoint_set_winsock.obj::_count_endpoints_in_set`
is strict exact. It contributes 94 meaningful / 96 padded code bytes and nine
ordered relocations. Together with the five inherited exact XNADDR and key
functions, the object advances from 5/26 to 6/26 exact functions and from
400/3,808 to 496/3,808 strict padded bytes. It remains incomplete and
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
canonical commit `bf31eaabdcbe0a0081bf5b77538e679b3f9d22d0`.

The stock XDK 3911 `xbox/include/WinSockX.h` is Git blob
`9fd916ccda1beec852108780bbf467cfc59be08e`, payload SHA-256
`726C8E558CEC1E99352DF9F687004387A60FA15BB252CFC07EECD2391E3BDBD9`.
It fixes `FD_SETSIZE` at 64 and defines `SOCKET` as the 32-bit `UINT_PTR`
handle. The named `fd_set` is therefore a 32-bit count followed by 64 handles,
for an exact natural size of `0x104` bytes.

The authenticated Stian tree is commit
`fc11d861865735877d5b8b4c408b1a0c06dde3cf`; its corroborating translation
unit is Git blob `eb0e747275325ecca24406f8187ff3585937a028`, payload SHA-256
`5B29F184398C7228FDD95BCC073C30ABD27B6E5DFD87DD95AA3A241E905ECBAC`.
The independent Pastudan tree is commit
`918af885935ec470a31256ecce9a977b12b01f80`; its counterpart is Git blob
`65f6d377bde90704425da6451e7232f1f2746c59`, payload SHA-256
`634BA61D818A1D2D419CF19C4CA8098CD1706E39721225C24DFE1DD37997A9AF`.
Both preserve the owner function at January address `0x82A30`; Stian labels it
`FUN_00082a30`, while Pastudan's later friendly-name lift at `0x82DF0` is a
different assert wrapper and was rejected as provenance for this body.

The adjacent authenticated create, delete, add, remove, rewind, get-next, and
poll functions jointly fix the complete endpoint-set layout rather than merely
the one accessed offset:

| Offset | Typed field | Evidence |
| ---: | --- | --- |
| `+0x000` | `fd_set sockets` | XDK size and socket-set operations |
| `+0x104` | `struct transport_endpoint **ep_array` | allocation, free, indexing, and the exact `set && set->ep_array` assertion |
| `+0x108` | `long max_endpoints` | positive create argument and bounds checks |
| `+0x10C` | `long last_endpoint_index` | initialized to -1 and maintained as the inclusive last occupied index |
| `+0x110` | `long current_endpoint_index` | rewind and get-next cursor |
| `+0x114` | `long needs_compaction` | remove/poll compaction state |

The create owner allocates exactly `0x118` bytes. Compile-time C89 layout
checks cover the XDK `fd_set`, every named field offset, and the final
structure size. The retained body accesses only the named
`last_endpoint_index` field; it contains no opaque offset expression.

The acceleration playbook, matching methodology, campaign and COFF tooling
documentation, all six prior object ledgers, applicable Claude
`AGENTS.md`/`CLAUDE.md` doctrine, reachable Git history, and registered local
donor/reference trees were audited before mutation. HCEA contains no endpoint
set owner or named layout. History reaches only the initial symbol inventory
and contains no prior implementation or measured boundary for this function.
No parked entry freezes it.

## Frozen January packet

January asserts `set` at exact source line `0x289`, asserts the external
`transport_initialized` boolean at `0x28A`, loads the signed 32-bit named field
at `+0x10C`, adds one, and returns. The result is the count because the adjacent
owners maintain that field as an inclusive last index starting at -1.

The packet has normalized SHA-256
`c8ab8d6383c830e5ebcbe4ea686dbca9eb73432cba2d2a3e0219523198b9154b`
over 96 padded bytes and this exact relocation schedule:

| Offset | Type | Destination |
| ---: | ---: | --- |
| `+0x13` | `DIR32` | exact transport source-path literal |
| `+0x18` | `DIR32` | exact `set` literal |
| `+0x1D` | `REL32` | `_display_assert` |
| `+0x24` | `REL32` | `_system_exit` |
| `+0x2C` | `DIR32` | `_transport_initialized + 0` |
| `+0x3C` | `DIR32` | exact transport source-path literal |
| `+0x41` | `DIR32` | exact `transport_initialized` literal |
| `+0x46` | `REL32` | `_display_assert` |
| `+0x4D` | `REL32` | `_system_exit` |

The January object owns `_transport_initialized` in BSS; this incomplete
translation unit keeps it external. The strict comparator proves the same
symbolic target and addend, so no data bytes or duplicate storage are claimed.

## Retained typed C89 candidate

```c
long count_endpoints_in_set(
	struct transport_endpoint_set *set)
{
	match_assert(
		"c:\\halo\\SOURCE\\bungie_net\\network\\transport_endpoint_set_winsock.c",
		0x289,
		set);
	match_assert(
		"c:\\halo\\SOURCE\\bungie_net\\network\\transport_endpoint_set_winsock.c",
		0x28A,
		transport_initialized);
	return set->last_endpoint_index + 1;
}
```

Retained source is Git blob
`23528aa6fc5557f1b75ec6cc621da3b8bbf93cfe`, working-tree SHA-256
`D1C8E3C15888F2AC5BA0621B41243900788F7B3AEBE88768BC5E5B3AFA93F9C8`.
The pre-lane source was Git blob
`5eaa505ecf34b00977bd8bd8c08573a83224fdb2`.

VC7 `/Zs` passed before the one sealed code-producing adjudication compile.
That object has raw SHA-256
`8BB951ECA57D894D80A55ADF444B96A9931509A5BA989DBDAB97CFDC0A32190F`.
The mandatory full Ninja gate re-emitted the same frozen source and produced
raw object SHA-256
`0EA0D8053B2E800F0E438BF092F5FC156C4D353A278E8A5379F0216DCBC9A682`.
Both artifacts contain the identical strict function packet. No alternate
body, layout, type, declaration, compiler option, sweep, or tune was compiled.

The retained code contains no assembly, `volatile`, `register`, pragma,
intrinsic, attribute, barrier, raw address or offset access, pointer/integer
reconstruction, pun, undefined behavior, byte forcing, compiler/tool
exception, or comparator waiver.

## Strict regression and full gates

The comparator proves every padded byte and relocation address, type,
identity, addend, and order. The whole-object exact-set gate preserves all five
inherited exact functions, adds only `_count_endpoints_in_set`, and loses
nothing. The campaign whole-TU gate reports six exact, zero residual, and 20
unwritten functions. It used an isolated temporary harness so the repository's
`scratch` tree remained untouched.

The complete `halobetacache_build`, `libcmt_build`, `progress`, and
`semantic_progress` graph passes. Semantic output reports 470 units, 4,950
evaluated functions, 4,833 semantic-exact functions, 4,843 accepted-exact
functions, and zero unit errors. Strict progress is 384/833 objects,
4,804/11,060 functions, 610,003/2,198,102 meaningful code bytes, and
1,856,050/4,176,062 data bytes. Halo progress is 282/468 objects,
4,637/7,574 functions, and 597,089/1,770,166 meaningful code bytes.

Protected `_unit_preprocess_node_orientations` remains exact at 1,920 padded
bytes, 87 ordered relocations, and normalized SHA-256
`5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
Admission reports zero candidates, contradictions, and revocations. Parked
validation reports 12 active, zero stale, and zero invalid entries. Pytest
passes all 212 tests. `git diff --check` and the changed-line prohibited-form
scan pass.

`build/report.json` is 1,600,394 bytes with SHA-256
`9CFE467183E948F544E55856DF1B628234795649934FAC615315B22B3F7655E3`.
`build/semantic_report.json` is 3,289,131 bytes with SHA-256
`3DCC1D26D354529513D56FED0EC98474781968FC654DEAFC3C23058C9F62CC7E`.
