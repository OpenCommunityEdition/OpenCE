# xbox_texture_cache.obj Jonas delete-callback pair

## Result

This bounded ordinary-C wave starts from canonical commit
08cc4a5398e0f2174cba1839a17e021603dca9f6. It recovers the LRUV delete
callback and, through the callback's natural XDK call, gives the previously
anonymous BaseTexture busy helper its official XDK 3911 identity.

| January owner | Evidence-backed owner | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | --- | ---: | ---: | ---: | --- |
| _code_001ae880 | _code_001ae880 | 148 | 160 | 11 | 685b1662cd3f0ca7451403bdebf6fef5287fcf5d31ce95dc3de0b88abc19c068 |
| _code_001ae800 | _IDirect3DBaseTexture8_IsBusy@4 | 7 | 16 | 1 | 43d508dca4211f96527b769632b4c2daf03f76d0dd594c76496b6c2c32c5c27a |
| **Wave gain** | | **155** | **176** | **12** | |

source/cache/xbox_texture_cache.obj advances from 11/25 to 13/25 strict
exact owners and from 688/3,648 to 864/3,648 padded code bytes. It remains
NonMatching.

All eleven inherited exact owners were independently rechecked and remain
strict exact: _texture_cache_delete, _texture_cache_open,
_texture_cache_idle, _texture_cache_bitmap_new,
_texture_cache_bitmap_delete, _texture_cache_return_memory,
_texture_cache_flush, _texture_cache_new, _texture_cache_close,
_IDirect3DDevice8_IsBusy@4, and
_IDirect3DDevice8_KickPushBuffer@4.

## Typed recovery and callback behavior

January code, assertion text, and field accesses prove this 0x20-byte cache
datum prefix:

~~~c
struct xbox_texture_cache_texture
{
    short identifier;                    /* +0x00 */
    short read_request_handle;           /* +0x02 */
    boolean loaded;                      /* +0x04 */
    byte reserved005[3];                 /* +0x05 */
    struct bitmap_data *bitmap;          /* +0x08 */
    D3DBaseTexture hardware_format;      /* +0x0C */
};                                       /* 0x20 */
~~~

The production source has compile-time checks for loaded at +0x04, bitmap at
+0x08, hardware_format at +0x0C, and total size 0x20. No shared layout or
header ABI is changed.

The delete callback:

1. Resolves and preserves the texture datum for block_index.
2. Re-resolves the datum at the loop head until its asynchronous load flag is
   set and IDirect3DBaseTexture8_IsBusy reports that the resource is idle.
3. Enforces the January line-0x187 assertion
   texture->bitmap->cache_block_index==block_index.
4. Clears the bitmap cache block to NONE and its base address to NULL.
5. Deletes the cache datum.

The callback's exact relocation schedule is:

- +0x04 DIR32 textures global; +0x10 REL32 _datum_get;
- +0x22 DIR32 textures global; +0x29 REL32 _datum_get;
- +0x3C REL32 _D3DResource_IsBusy@4;
- +0x54/+0x59 DIR32 source-path and assertion-expression literals;
- +0x5E/+0x65 REL32 _display_assert and _system_exit;
- +0x82 DIR32 textures global; +0x89 REL32 _datum_delete.

This duplicates the January pre-loop datum lookup and the loop-head lookup
instead of collapsing them, preserving the target's aliasing and register
lifetimes in readable typed C.

## XDK identity and retained no-retry boundary

The official XDK 3911 definition of IDirect3DBaseTexture8_IsBusy forwards a
D3DBaseTexture pointer to D3DResource_IsBusy. The ordinary callback call
naturally emits the January 16-byte helper with one REL32 relocation at +0x02
to _D3DResource_IsBusy@4. The campaign alias scanner found that helper as the
unique candidate counterpart of January _code_001ae800, so
config/symbols.json now records its official decorated identity. No wrapper
body, synthetic emission anchor, or raw resource cast is hand-authored.

The earlier code_001ae840 locked-block callback experiment remains a
fail-closed one-shot rejection. It was not defined, edited, compiled, renamed,
or retried in this wave. Its prior 64-byte full-width-return mismatch remains
documented in
xbox_texture_cache_obj_jonas_xdk_device_wrapper_pair_20260830.md.

A Ninja dry run exposed the single production CL edge. The frozen typed
callback premise was compiled once; both owners were immediately strict
exact, so no source-shape retry or byte tuning occurred. The subsequent full
validation graph found the object current.

## Provenance and tooling reconciliation

January split COFF, disassembly, relocations, and assertion metadata remain
the authority for exactness, layout, call order, and symbol ownership.
Before editing, every prior xbox_texture_cache ledger, the repository
campaign and matching doctrine, the relevant Claude tooling documentation,
the texture-cache corruption note, and the applicable repository and donor
CLAUDE.md files were read in full.

External evidence was used only for provenance-ranked semantics and source
shape:

- Stian full history at
  fc11d861865735877d5b8b4c408b1a0c06dde3cf, source blob
  fae53c8ce7fff7671e61e81d7291d3a3a2edbf5d and SHA-256
  007911c6aa0e6b8434f965d779baab752e650af4dbd468853e9cdc302fb7ef30,
  provides disassembly-backed FUN_001bed90 with the exact double datum_get,
  load/busy loop, assertion, stores, and delete topology.
- HCEA at 570c83fd9c365dad6f2a3e7041705d5b84c7847c,
  texture_cache_delete_block_proc.c blob
  4d54d0fdaee2372bced02405067d90e0271be67e and SHA-256
  d916f7a8611de1b0c084767ce8421e67faf7b1a51f91b0b9406b1b3771ad79f4,
  independently corroborates the eviction callback's wait, unlink, and datum
  deletion semantics. Its PC allocator and rasterizer teardown are not copied.
- XDK 3911 xbox/include/D3D8.h, SHA-256
  7f7f603e1b2fa13ef36a05923eaa36d0d7094302522edbac9855b28f0909f1a1,
  is authoritative for D3DBaseTexture and the compatibility wrapper identity.

The accepted source contains no inline or naked assembly, volatile,
register, pragma or optimizer barrier, forced-inline annotation, raw absolute
address, cast-based object layout, representation pun, undefined behavior,
dummy owner, or byte-forcing expression.

## Validation and reproducibility

The normal halobetacache_build, libcmt_build, progress, and semantic_progress
graph completed 574 actions successfully.

- Direct hardened comparison: both new and all eleven inherited named owners
  pass section_infos_equal.
- Semantic report: 470 units scanned; 4,983 functions evaluated; 4,863
  semantic exact; 131 hidden exact / 78,940 hidden code bytes; one
  ordinary-only and one structural match; 4,873 accepted exact; zero ordinary
  rejected and zero unit errors.
- Strict Halo campaign board: 278/619 exact objects, 4,845/8,246 exact
  functions, and 673,287/1,922,669 padded code bytes.
- Full progress board: 385/833 files, 4,834/11,060 functions, and
  615,874/2,198,102 code bytes.
- Halo progress board: 283/468 files, 4,667/7,574 functions, and
  602,960/1,770,166 code bytes.
- Object-admission audit: zero candidates, contradictions, or revocations.
- Parked-function audit: 13 active, zero stale, zero invalid.
- Complete tooling suite: 212 tests passed.
- Scoped prohibited-construct/raw-cast scan and git diff --check: pass.

Reproducibility identities:

- Baseline source blob:
  8836e5efb424acb7ec76738c63a02a31fa119434.
- Final source blob:
  6bfe6f1357ca9cfea9ace7bac5e54fe6882a5a84.
- Final source SHA-256:
  19576d8160cb14c5fadcbdb490100c5a33270fd103507f95cc5af043253938da.
- Final symbols configuration blob:
  27c5879c13f9165fa2fd40e55142b5676f22c074.
- Final symbols configuration SHA-256:
  50eab3a38429610ef6b9d1e5060eb6383f4c080a6c3aa5f88aa85dd81e245fa3.
- Candidate raw object SHA-256:
  7295a5417e7158027bdc8a7356202e94b83c1e11d9fea7654b3100d5fd164f6f.
- Re-split January target object SHA-256 after the official symbol identity:
  c8260ab4e676d6f6abdf8d41d4c7c9d2fd49bf65affa0b12c8a17ed635b7d2ea.
  The immutable pre-alias January split snapshot remains
  457d6eb9f4e79e816f6539c4bdbe2555c957267e75fefef156fa48076f03dc0f;
  only emitted symbol-table identity changes.
- build/report.json SHA-256:
  f6c5c808f336553dc6ea2a9770997ace8ad646f8bb09d2b19f8ce1f8d1a9a447.
- build/semantic_report.json SHA-256:
  bde47cfccab5ce2d8f42aaf9eff6db685434c7557b8043b305923c4eb2cc75b0.

Tracked scope is exactly source/cache/xbox_texture_cache.c,
config/symbols.json, and this ledger. No protected source, completion label,
semantic exception, parked record, shared header, or unrelated object changes.
