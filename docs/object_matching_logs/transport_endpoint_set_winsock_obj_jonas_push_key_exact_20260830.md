# `transport_endpoint_set_winsock.obj` typed key-push leaf (2026-08-30)

## Scope and result

The first and only natural production candidate for
`source/bungie_net/network/transport_endpoint_set_winsock.obj::_transport_push_key`
is strict exact. It contributes 126 meaningful / 128 padded code bytes and 15
ordered relocations. Together with the inherited exact key-pop, XNADDR,
key-ID, and key accessors, the object advances from 4/26 to 5/26 exact
functions and from 272/3,808 to 400/3,808 strict padded bytes. It remains
incomplete and `NonMatching`.

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
canonical commit `3e919da7`.

The stock XDK 3911 `xbox/include/WinSockX.h` is Git blob
`9fd916ccda1beec852108780bbf467cfc59be08e`, payload SHA-256
`726C8E558CEC1E99352DF9F687004387A60FA15BB252CFC07EECD2391E3BDBD9`.
It declares `INT WSAAPI XNetRegisterKey(const XNKID * pxnkid, const XNKEY *
pxnkey)`. The existing `<xtl.h>` include supplies that declaration and both
named structures, so the retained code uses the natural pointer order and
XDK stdcall decoration without a surrogate prototype or local layout.

The authenticated Stian tree is commit
`fc11d861865735877d5b8b4c408b1a0c06dde3cf`; its corroborating translation
unit is Git blob `eb0e747275325ecca24406f8187ff3585937a028`, payload SHA-256
`5B29F184398C7228FDD95BCC073C30ABD27B6E5DFD87DD95AA3A241E905ECBAC`.
The independent Pastudan tree is commit
`918af885935ec470a31256ecce9a977b12b01f80`; its counterpart is Git blob
`65f6d377bde90704425da6451e7232f1f2746c59`, payload SHA-256
`634BA61D818A1D2D419CF19C4CA8098CD1706E39721225C24DFE1DD37997A9AF`.
Stian's lifted owner independently fixes the complete four-dword key copy,
two-dword ID copy, zero-depth registration, `0 == error` assertion at line
`0x5C`, and unconditional depth increment. Its analyzed callers prove that
argument one is the 16-byte key and argument two the eight-byte ID, while the
XDK registration call receives ID first and key second. Pastudan's callers
independently preserve that owner argument order. Raw addresses and integer
arrays are semantic corroboration only; January and the stock XDK determine
the retained named types.

The acceleration playbook, ledger doctrine, COFF/campaign tooling READMEs,
all prior object ledgers, and applicable Claude `AGENTS.md` / `CLAUDE.md`
doctrine were audited before mutation. Reachable Git history contains no
earlier body. A pre-mutation read-only census covered 1,535 registered
worktrees and 1,535 owner files, finding zero definitions of
`transport_push_key`. No parked entry or prior ledger freezes this function.

## Frozen January packet

The function is ordinary public cdecl with two const-qualified XDK structure
pointers. VC7 lowers each named assignment into its complete dword copy,
tests the signed 32-bit depth, registers the named globals only at zero,
asserts the returned `int` is zero, and then increments depth in memory. The
packet has normalized SHA-256
`5b0841e58bb94b1bf50ffac22b43c8c57aeb8c8fe40a1c092f109d980b415346`
over 128 padded bytes and this exact relocation schedule:

| Offset | Type | Destination |
| ---: | ---: | --- |
| `+0x0A` | `DIR32` | `_global_key + 0` |
| `+0x13` | `DIR32` | `_global_key + 4` |
| `+0x1C` | `DIR32` | `_global_key + 8` |
| `+0x28` | `DIR32` | `_global_key + 12` |
| `+0x30` | `DIR32` | `_global_key_id + 0` |
| `+0x38` | `DIR32` | `_global_key_depth + 0` |
| `+0x40` | `DIR32` | `_global_key_id + 4` |
| `+0x47` | `DIR32` | `_global_key + 0` |
| `+0x4C` | `DIR32` | `_global_key_id + 0` |
| `+0x51` | `REL32` | `_XNetRegisterKey@8` |
| `+0x5E` | `DIR32` | exact transport source-path literal |
| `+0x63` | `DIR32` | exact `0 == error` literal |
| `+0x68` | `REL32` | `_display_assert` |
| `+0x6F` | `REL32` | `_system_exit` |
| `+0x78` | `DIR32` | `_global_key_depth + 0` |

The January object owns `_global_key_depth` in BSS. This incomplete
reconstruction keeps all three globals external. The strict comparator proves
the same symbolic targets and addends; no data bytes or duplicate storage are
claimed.

## Retained typed C89 candidate

```c
void transport_push_key(
	const XNKEY *key,
	const XNKID *key_id)
{
	int error;

	global_key = *key;
	global_key_id = *key_id;
	if (global_key_depth == 0)
	{
		error = XNetRegisterKey(&global_key_id, &global_key);
		match_assert(
			"c:\\halo\\SOURCE\\bungie_net\\network\\transport_endpoint_set_winsock.c",
			0x5C,
			0 == error);
	}
	global_key_depth++;
}
```

Retained source is Git blob
`5eaa505ecf34b00977bd8bd8c08573a83224fdb2`, working-tree SHA-256
`5B75B56AC9EFF85018B94EF05F02EBC0917888B00CAC5AD0415705E8BFE4D617`.
The pre-lane source was Git blob
`eaa3f14b13df5c85a0ce1da2feed1a020cfc3882`.

VC7 `/Zs` passed before the one sealed code-producing adjudication compile.
That object has raw SHA-256
`FA5EEA08A9098DD1CE5221CC708199BAE85235F8F7D6B50B58BF694F298CAEB4`.
The full Ninja gate re-emitted the same source and produced raw object SHA-256
`933DD01DAA0075E76BD05FE4302CB7B4BED927059366331E620EDDFFF327F636`.
Both artifacts contain the identical strict packet. No alternate body, type,
argument order, assignment form, declaration, compiler option, sweep, or tune
was compiled.

The retained code contains no assembly, `volatile`, `register`, pragma,
intrinsic, attribute, barrier, raw address or offset access, pointer/integer
reconstruction, pun, undefined behavior, byte forcing, compiler/tool
exception, or comparator waiver.

## Strict regression and full gates

The comparator proves every padded byte and relocation address, type,
identity, addend, and order. The whole-object exact-set gate preserves the
four inherited transport functions, adds only `_transport_push_key`, and
loses nothing.

The complete `halobetacache_build`, `libcmt_build`, `progress`, and
`semantic_progress` graph passes. Semantic output reports 470 units, 4,947
evaluated functions, 4,830 semantic-exact functions, 4,840 accepted-exact
functions, and zero unit errors. Strict progress is 384/833 objects,
4,801/11,060 functions, 609,754/2,198,102 meaningful code bytes, and
1,856,050/4,176,062 data bytes. Halo progress is 282/468 objects,
4,634/7,574 functions, and 596,840/1,770,166 meaningful code bytes.

Protected `_unit_preprocess_node_orientations` remains exact at 1,920 padded
bytes, 87 ordered relocations, and normalized SHA-256
`5d5c8edc492fb8ab6ea83e1ccaa4cb2798da51ae4a17182fa848878bed05a7ed`.
Admission reports zero candidates, contradictions, and revocations. Parked
validation reports 12 active, zero stale, and zero invalid entries. Pytest
passes all 212 tests. `git diff --check` passes.

`build/report.json` is 1,600,301 bytes with SHA-256
`D29374CB38142A054AFD91C0E1428E4D5DE63C83ACC4630E3D77D3029429A272`.
`build/semantic_report.json` is 3,287,091 bytes with SHA-256
`00A7C4EBB329987AE3314C8C947C9BAB48D2729941ABC65D075ED0A0389DE356`.
