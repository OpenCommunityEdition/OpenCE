# `ai_script.obj` Jonas deselect recovery (2026-08-27)

This ledger records the strict recovery of `_ai_scripting_deselect`.
`ai_script.obj` advances from eight to nine accepted functions and remains
`NonMatching`; no whole-object or data completion is claimed.

## Authority and frozen baseline

- Integration base:
  `a3d6bf9020282bd49dbdbcef6bc9a49be5609dce`.
- Baseline source blob:
  `ef8072f491dd225cd902038e4494b7961073b074`.
- Baseline production object SHA-256:
  `b01d59d942d01c4d9cdaca8f2835c986f55bbd2cfe77e14c600043cbafe08edf`.
- January split `source/ai/ai_script.obj` SHA-256:
  `1a04c31fe9b02c1bdea6d15b2210d4f75493f51ea610cce5742c053bae3eca2e`.
- Pinned XDK VC7 `CL.Exe` SHA-256:
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
- Pre-edit regression manifest:
  `build/audit/ai_script_debug_cluster_baseline_20260827.json`, SHA-256
  `f4a2a8cc5a72adc83baaa17d48d5e14e5ae20653ae1174f43fe932f896bf2ecd`.

January COFF remains the byte, padding, relocation, ABI, linkage, and
ownership authority. The tracked implementation scope is
`source/ai/ai_script.c`; this ledger is the only other tracked file. No
configuration, completion label, semantic allowlist, parked entry, compiler
flag, comparator, admission rule, or tool changes.

The campaign's three distinct Claude guidance documents were already read
before candidate selection:

- `build/audit/refs/stian-halo/CLAUDE.md`;
- `research-cache/stian-halo-cseries-20260820/CLAUDE.md`;
- `work/halo/CLAUDE.md`.

Their applicable requirements agree with the retained wave: machine evidence
is authoritative, source is C89, declarations stay at scope tops, and raw
addresses, speculative ABI tricks, inline assembly, and synthetic byte
anchors are excluded.

## Provenance and platform screening

The PC reconstruction in
`build/audit/refs/stian-halo/src/halo/ai/ai_profile.c`, SHA-256
`ce96e974bdb96c37c32f172d63e5cb562374173bc54a466d3cc7da090e02bcb2`,
identifies the behavior: when AI is initialized for the map, select the
wildcard actor pair through `ai_debug_select_actor(NONE, NONE)`.
Its raw-address spelling is evidence only and was not copied.

The HCEA `ai_scripting_deselect.c` donor, SHA-256
`be4389a80e9077fef48da54c092aceed37f72b3b9f7ecf56f49d1b9d6a44cbe8`,
is an Xbox-build no-op and is explicitly rejected as a platform divergence.
The January PC disassembly independently proves the Stian behavior and exact
control flow.

The retained source uses a typed two-field `ai_script_globals_prefix` view
for the shared global. It performs no raw address arithmetic, pointer/integer
reconstruction, tag access, object access, cast-after-raw-access pattern, or
aliasing trick. The no-argument signature has `void` on its own line and the
void function ends with an explicit `return;`.

## January packet and strict result

`_ai_scripting_deselect` owns 25 meaningful and 32 padded bytes:

```text
0000  mov  eax, dword ptr [_ai_globals]
0005  mov  cl, byte ptr [eax+1]
0008  test cl, cl
000a  je   0018
000c  push -1
000e  push -1
0010  call _ai_debug_select_actor
0015  add  esp, 8
0018  ret
```

The target and retained candidate both have normalized SHA-256
`e9f922363868def52912057fdec2b1a209812617dbecd257d2166e96f736f6a9`.
Both relocation records are identical:

| Offset | Type | Destination |
| ---: | ---: | --- |
| `0x01` | `IMAGE_REL_I386_DIR32` | `_ai_globals + 0` |
| `0x11` | `IMAGE_REL_I386_REL32` | `_ai_debug_select_actor + 0` |

The direct nine-function comparison reports `all_equal: true`. A separate
pre/post exact-set comparison reports all eight inherited owners retained,
`lost_exact: []`, and only `_ai_scripting_deselect` under
`newly_exact`.

## Bounded first-shot pruning

The frozen initial packet also tested the natural PC
`_ai_scripting_select` branch topology. The first production object is
preserved at
`build/audit/ai_script_debug_cluster_first_shot_20260827.obj`, SHA-256
`526b53a790d5ca3bffb93b301a0b7c3256d1e2c8beb10c7471e5a6aa569f3f11`.

`deselect` was exact immediately. `select` had the correct 64-byte padded
size, three relocation destinations, and branch semantics, but normalized
SHA-256
`cef00f50d5e1be7f1d95159389b022be94e6fc9b33a73e275fe00c094ef1d282`
instead of target
`e25848085893c9d62dc6786d9b905c1ec264af5af975c8416700841212e701a2`.
The candidate call relocations were at `0x19` and `0x29`; January owns
`0x1B` and `0x2B` because it retains a redundant `or eax,eax` in the
NONE branch.

No authenticated source explains that no-op instruction. Rather than invent
a source anchor or tune bytes, `select` and its unused declaration were
removed. That first-shot mismatch was the new evidence for one narrowed
rebuild. The first narrowed placement preceded inherited functions and the
fail-closed manifest then showed their COMDAT ownership indices changed even
though direct bytes remained exact. That ownership finding was new evidence
for moving `deselect` behind all inherited implementations and rebuilding
the final source packet. No source-shape search or byte-forcing experiment was
performed.

The final retained source blob is
`d72768f12054880ece5da1724f7027901a507880`, with physical SHA-256
`55b06776fe2b08d142943b85792b7ccfcdf4e55d4582a8014321579a08a4aacd`.
The final production object SHA-256 is
`12bc2cdf8e4d23d64d35c74623105b5011dbdcb03cce59ba34bab9eb3926c613`.

## Regression and full gates

After restoring inherited COMDAT order, the fail-closed pre-edit manifest
reports:

- `_ai_scripting_deselect` as the sole `NEWLY_EXACT` function;
- all eight inherited functions as `still_exact`;
- `changed_nonexact: []`;
- one changed compiler-debug section and the expected symbol-inventory
  expansion for the new external COMDAT.

The generic gate stops on those debug/symbol inventory findings by design.
They are recorded, not adjudicated or waived. Independent section-level
comparison proves all inherited code, padding, relocation, destination,
addend, and ownership evidence unchanged.

The full Halo-plus-libcmt graph and both reports build. Report hashes are:

- `build/report.json`:
  `322179722b41b85177ddfcbf16edd75849066b0d7b0d260b9bc855de1ba10347`;
- `build/semantic_report.json`:
  `10511d1e1049cfd3f47dbd5e3e078f398da67e4c036505bfba14744a3c346bff`.

The direct report records `ai_script.obj` at 9/116 functions and
57/19,281 meaningful bytes. Hardened padded progress is 160/20,112 bytes.
This wave adds one function and 32 padded bytes. The strict campaign board
becomes 260/619 complete objects, 4,282/8,246 functions, and
555,191/1,922,669 padded code bytes. The complete-object count is unchanged.

All remaining gates pass:

- semantic audit: 470 units, 4,442 functions evaluated, 4,291 semantic
  exact, 115 hidden exact / 64,889 hidden code bytes, 4,351 accepted exact,
  and zero unit errors;
- object admission: zero candidates, contradictions, and revocations;
- parked validation: three active, zero stale, zero invalid;
- test suite: 194 passed, with only the known unwritable pytest-cache warning;
- direct exact-set comparison: eight inherited, one added, zero lost;
- `git diff --check`: clean apart from Git's CRLF normalization notice;
- forbidden/raw-access scan: no findings.

No assembly, register/volatile qualifier, barrier, pragma, intrinsic,
attribute, raw address, raw tag/object access, pointer pun, undefined
behavior, synthetic anchor, or comparator exception is present.
