# `cache_files_windows.obj` register cluster ownership boundary (2026-08-30)

## Boundary and fail-closed outcome

This bounded lane started from canonical commit
`f93c899f4aeaa86544e86d8ede7d08a45a971b90` in isolated worktree
`C:\Users\isabe\Documents\Codex\2026-07-13\i-w\work\cache-files-windows-register-cluster-20260830`
on branch `jonas/cache-files-windows-register-cluster-20260830`. The frozen
candidate set contained only these four ordinary-cdecl public functions:

- `tags_header_register_vertex_and_index_buffers`;
- `tags_header_deregister_vertex_and_index_buffers`;
- `structure_bsp_header_register_vertex_buffers`;
- `structure_bsp_header_deregister_vertex_buffers`.

The four code COMDATs were all strict exact on the sole frozen VC7 emission.
The candidate nevertheless failed whole-translation-unit ownership because
the broad public headers instantiated four target-absent non-code owners. The
lane therefore failed closed: every production source and header edit was
inverse-reverted immediately, no alternate dependency spelling was compiled,
and this evidence ledger is the only retained change. Nothing is pushed,
cherry-picked, amended, or integrated into canonical.

## Immutable authority and provenance

The January PC authority is `cachebeta.exe`, 6,584,672 bytes, raw SHA-256
`4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
Its generated target `build/split/source/cache/cache_files_windows.obj` is
38,027 bytes with raw SHA-256
`668d9703ed73e60f0f8e261b31c817a9546052e1528d5b18db7a972d876f313e`.
Strict admission used each function's padded COMDAT, normalized bytes, and
ordered symbolic relocation destinations/addends. The candidate used the
repository VC7 `/O2 /Oy- /DDEBUG /Dxbox` edge;
`xbox/bin/vc7/CL.Exe` has SHA-256
`483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.

Stian and Pastudan provide identical source-topology provenance. Stian commits
`891c19c20` and `691fd6995` introduced the tag-header and BSP-header pairs;
`cbdaba363` applied the PDB names. Stian's VC71 mnemonic/operand scores are
100/97.4, 100/96.9, 100/100, and 95.2/92.9 percent in target order below.
Retail meaningful bounds are exactly 92, 172, 102, and 108 bytes, equal to
January. Their raw byte-offset/address forms were evidence only and were not
copied. HCEA authenticates the resource layouts and
`_rasterizer_lock_bsp_switch = 17`, but its later HCEX registration topology
is not a byte-shape donor.

## Frozen typed topology

Before the code-producing compile, the candidate moved the two cache header
layouts into the closest cache header, with incomplete typed
`D3DVertexBuffer *` and `D3DIndexBuffer *` members. The authenticated layouts
were:

- tag header: vertex count/base at `+0x10/+0x14`, index count/base at
  `+0x18/+0x1C`, total size `0x24`;
- BSP header: vertex count/base at `+0x04/+0x08`, index count/base at
  `+0x0C/+0x10`, total size `0x18`;
- Xbox vertex and index resources: `Common`, `Data`, and `Lock`, total size
  `0x0C`.

Each body used one `short` counter, a positive-count guard, and a `do/while`
loop. The tag register body wrote vertex `Common = 1`, called the public XDK
register wrapper with a null base, and wrote index `Common = 0x10001` without
registering it. The BSP register body wrote `Common = 1` and registered both
arrays. The tag deregister body blocked each resource, queried `IsBusy`, and
used the exact public wrapper expressions in January assertions at lines 523
and 536. The BSP deregister body wrote the typed short
`rasterizer_globals.current_lock_operation` to
`_rasterizer_lock_bsp_switch`, blocked both arrays, and restored
`_rasterizer_lock_unlocked`. Every function ended with an explicit return.

The source was VC7 `/Zs` syntax-checked before emission. Adding `<xtl.h>` made
the inherited local `SleepEx` declaration redundant and type-inconsistent, so
that declaration was removed before the freeze; the subsequent syntax check
was clean. This was a non-code-producing prerequisite correction.

## Sole code-producing emission

The candidate object path was proved absent. The explicit Ninja dry run showed
exactly one action, and the only candidate-shape compile was:

```text
ninja build\base\source\cache\cache_files_windows.obj
[1/1] CL build\base\source\cache\cache_files_windows.obj
```

The resulting object is 7,803 bytes with phase-specific raw SHA-256
`4a8d1d1581bf32bd01a1868e0dd1f785bfd7dd6edfd8c1d64835b6fcd37af8da`.
No function body, declaration, statement order, spelling, type, flag, or
scheduling shape was retried.

## Strict code result

| Function | January RVA | Meaningful / padded | Relocations | Normalized SHA-256 | Result |
| --- | ---: | ---: | ---: | --- | --- |
| `_tags_header_register_vertex_and_index_buffers` | `0x001AC6A0` | `92 / 96` | 1 | `b03999929dea98c01645c7fc13d3801d8e54c58f22e5a0f97464f4967927c426` | strict exact |
| `_tags_header_deregister_vertex_and_index_buffers` | `0x001AC700` | `172 / 176` | 12 | `bf9e335e14af3539637c08e200a95656a8d0029f0adeed4a72d32d7f2f551ec8` | strict exact |
| `_structure_bsp_header_register_vertex_buffers` | `0x001AC7B0` | `102 / 112` | 2 | `d0a059fb5b47ac676b5af496f5cab6eb5e79bd692859483ca699acf6306f2286` | strict exact |
| `_structure_bsp_header_deregister_vertex_buffers` | `0x001AC820` | `108 / 112` | 4 | `ed01dd86068b125d9ad14eb00b102bf6ca3ca4514b793dbb14d4d0b18e21ac1e` | strict exact |

The strict code gain would have been four functions, 474 meaningful bytes,
496 padded bytes, and 19 relocations. Every ordered relocation matched:

- tag register: one `_D3DResource_Register@8` call at `+0x26`;
- tag deregister: both `_D3DResource_BlockUntilNotBusy@4` and
  `_D3DResource_IsBusy@4` calls, exact source/expression literals,
  `display_assert`, and `system_exit` for both loops;
- BSP register: `_D3DResource_Register@8` at `+0x26` and `+0x53`;
- BSP deregister: `_rasterizer_globals + 2` at `+0x0A` and `+0x63`, with
  block calls at `+0x2B` and `+0x50`.

Direct hardened comparison also reconfirmed all seven inherited exact code
owners. The frozen object therefore reached 11/50 strict functions and
880/6,432 padded code bytes, with no residual emitted function and no private
helper owner.

## Whole-TU ownership failure

The exact code result is not admissible. `<xtl.h>` defines three external
`__declspec(selectany)` XDK tables even when this TU does not reference them,
and the current `rasterizer/rasterizer.h` contains a non-`extern` declaration
of `global_frame_parameters`. The candidate consequently added these owners:

| Candidate owner | COFF form | Size | January target |
| --- | --- | ---: | --- |
| `_D3DTEXTUREDIRECTENCODE` | external `.rdata` COMDAT | 16 | absent |
| `_D3DSIMPLERENDERSTATEENCODE` | external `.rdata` COMDAT | 328 | absent |
| `_D3DPRIMITIVETOVERTEXCOUNT` | external `.rdata` COMDAT | 88 | absent |
| `_global_frame_parameters` | external COMMON | 8 | absent |

An independent target-symbol census confirmed that January defines none of
these four names in `cache_files_windows.obj`. This is not a comparator
identity ambiguity: it is additional ownership. The intended
`_rasterizer_globals`, three XDK resource APIs, assertions, and inherited
cache globals remained undefined external references as required, but those
correct references do not excuse the extra definitions.

Under the frozen-batch rule, this ownership miss prohibited a second compile.
The obvious dependency-only correction was recorded, not attempted.

## Validation completed before fail-closed reversion

The unchanged exact candidate passed the proportional gates that had already
started before the non-code ownership census exposed the blocker:

- complete `halobetacache_build` and `libcmt_build`: pass; the cached candidate
  object was not recompiled;
- shared-header preservation: `source/cache/cache_files` remained 28/28 strict
  and `source/rasterizer/rasterizer` remained 131/131 strict;
- semantic report: 470 units, 4,990 functions evaluated, 4,870 semantic exact,
  131 hidden exact / 78,940 hidden code bytes, 4,880 accepted exact, zero unit
  errors;
- campaign progress: 385/833 matched objects, 4,841/11,060 functions,
  616,948/2,198,102 code bytes, and 1,859,379/4,176,062 data bytes;
- object admission: zero candidates, contradictions, or revocations;
- parked validation: 13 active, zero stale, zero invalid;
- complete Python tooling suite: 212/212 tests pass.

These passes establish that the code and shared type move were technically
sound. They do not override the whole-TU ownership failure and grant no credit.

## Reversion proof and next lawful lane

All four production paths were restored to the exact starting Git blobs:

| Path | Restored Git blob |
| --- | --- |
| `source/cache/cache_files.c` | `510bf8c15bf79c01b8438d3ac037266825725301` |
| `source/cache/cache_files.h` | `1cc2d7085e0ef6a0aab780aee0dc3ca5efdef3e2` |
| `source/cache/cache_files_windows.c` | `5663572f16534801c467e9a0ea0bfcd270e3fb42` |
| `source/rasterizer/rasterizer.h` | `1d5066f22e6ed63bf05c202698f7b7a05c6595b0` |

`git hash-object --path` equals the index blob for every path, and the staged
production diff is empty. Only this ledger is retained.

A future fresh one-shot should preserve the proven bodies verbatim and change
only the dependency surface:

1. provide a narrow cache/Xbox resource interface that declares the two
   12-byte resource structs and the three stdcall `D3DResource_*` APIs without
   instantiating the unrelated XDK tables;
2. preserve the public `IDirect3DVertexBuffer8_IsBusy(...)` and index-buffer
   assertion text, either through storage-free wrapper macros or an exact
   explicit assertion string;
3. separate the rasterizer-lock enum into a storage-free associated header,
   and access `_rasterizer_globals + 2` through a typed TU-local prefix unless
   the broader `global_frame_parameters` header-definition defect is repaired
   in its own audited change;
4. census external text, `.rdata`, writable data, BSS, and COMMON owners before
   accepting the first object.

No `_code_<addr>` or `_bss_<addr>` name, raw address, raw offset dereference,
assembly, pragma, barrier, volatile scheduling device, pointer/integer
reconstruction, pun, private helper, or synthetic owner was introduced or
retained.
