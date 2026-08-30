# `xbox_sound_cache.obj` Jonas private cluster

## Result

This bounded ordinary-C wave starts from local canonical commit
`0b86790bfa8827e7f53b19861d6ddcfec27decdd`. Four previously unwritten
private owners are independently strict byte-and-relocation exact:

| New exact owner | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_code_001adc60` | 54 | 64 | 2 | `72a06dde5b807b2c9c8448e0549e6ae8fa3949a06177699f2911cd4611a634f0` |
| `_code_001adca0` | 182 | 192 | 15 | `f59bc02b43e15a9df3c89eafc5735d5483dcf0d2f40d5e76a723c501f7745e76` |
| `_code_001add60` | 64 | 64 | 7 | `7d30146060ee33d1c9e3862d69a08a9897b0fb94fa1ec5b18e6c5a5c128aacab` |
| `_code_001adda0` | 290 | 304 | 31 | `ece86e96952044d361925858d8cd29c98313c1b33c7f48061902a232558a9941` |
| **Wave gain** | **590** | **624** | **55** | |

`source/cache/xbox_sound_cache.obj` advances from 10/18 to 14/18 strict
exact functions and from 1056/2704 to 1680/2704 padded code bytes. It remains
`NonMatching`.

The ten inherited named exact owners were independently rechecked and remain
strict exact: `_sound_cache_close`, `_sound_cache_delete`,
`_sound_cache_flush`, `_sound_cache_idle`, `_sound_cache_new`,
`_sound_cache_open`, `_sound_cache_sound_delete`,
`_sound_cache_sound_finished`, `_sound_cache_sound_hardware_lock`, and
`_sound_cache_sound_new`. A section-level comparison also reported the COFF
pseudo-symbol `.text` as changed; that is an inventory artifact, not a lost
named function. No named exact owner regressed.

## Typed recovery

The January object proves the complete Xbox cache datum and global layouts:

```c
struct xbox_cache_sound_datum
{
    short identifier;                    /* +0x00 */
    boolean loaded;                      /* +0x02 */
    boolean initialized;                 /* +0x03 */
    byte software_reference_count;       /* +0x04 */
    byte hardware_reference_count;       /* +0x05 */
    byte reserved006[2];                  /* +0x06 */
    struct sound_permutation *sound;      /* +0x08 */
};                                       /* 0x0C */

struct xbox_sound_cache_globals
{
    char debug_block_name[0x100];         /* +0x000 */
    struct data_array *cache_sounds;      /* +0x100 */
    byte *base_address;                   /* +0x104 */
    struct lruv_cache *cache;             /* +0x108 */
    unsigned long last_allocation_failure_time; /* +0x10C */
};                                       /* 0x110 */
```

Compile-time offset and size checks cover the newly named fields. The existing
`sound_permutation` unknown members receive source-local semantic aliases for
the target-proven cache block index, cache base address, and cache tag index;
the underlying shared structure and ABI are unchanged.

`_code_001adc60` is the LRUV locked-block callback. It resolves the datum and
reports unlocked only when the asynchronous read is complete and both
reference counts are zero.

`_code_001adca0` is the LRUV delete callback. It validates both reference
counts, validates the sound's block index, clears the sound's cache block and
base address, and deletes the datum. Its assertion strings, source lines, call
order, relocation destinations, and compiler shape all match January.

`_code_001add60` is the LRUV debug-name callback. It formats `"%s (%s)"`
into the proven 0x100-byte global buffer and returns that buffer.

`_code_001adda0` is the private start-loading helper emitted with the
caller's sound pointer in ESI. It allocates an LRUV block, derives the backing
address, creates and resolves the matching datum, publishes the typed sound
state, and starts `cache_file_read`. On allocation failure it rate-limits the
purple cache-blown diagnostic, writes `d:\\stabbed.txt` through the proven
scenario and block-name callbacks, and records the timestamp. The private ABI
is compiler-produced from ordinary C; it is not declared or forced.

## Retained residual boundary

Four owners remain nonexact and receive zero credit:

| Residual owner | Target padded / relocs | Candidate padded / relocs | Boundary |
| --- | ---: | ---: | --- |
| `__sound_cache_sound_request` | 352 / 25 | 368 / 27 | The readable caller is retained because it naturally emits the exact private ESI loader. Stian's byte-exact source shape requires prohibited `volatile`; no scheduling device was admitted. |
| `_code_001ae1a0` | 256 / 22 | 256 / 22 | Readable screen-to-world ray helper has target topology but an ECX/EDX register mirror. |
| `_sound_cache_debug_render` | 368 / 14 | 368 / 14 | Semantic 640-by-4 graph and ray projection has the target extent and relocation count but residual register/local allocation differences. |
| `_sound_cache_sound_hardware_unlock` | 48 / 3 | absent | The earlier ordinary-C premise matched extent/relocations but not normalized bytes; no new source-shape evidence justified a retry. |

Normalized target/candidate evidence:

- Request target
  `92c886403f8cde61d3d896b9ed86e8834df085c15e28f3db224e2e28628e968a`;
  candidate
  `8949af89c5592c066f29a1ea7dd9514f23d783eb55548ce806a9c5fe91ba055f`.
- Ray helper target
  `c39690660070bc740fc89bc34ed735ce9c30406d8f26cfe5f853c9d2e5261bee`;
  candidate
  `d434a17b8d90be4fa31d5651b27f45be31c5a6c6e5d0c21b366fc2d174e23acc`.
- Debug renderer target
  `4c8eb53f6c7feaadf4c4b83d4b2587f923802d0cfa271d08fb642c35e0cb225e`;
  candidate
  `2baa9dd5fe0db8bda42be8161b51d2cea9e50e266a152c01778c11301ed167cd`.

The retained residual bodies are readable, typed semantic recovery only.
Their presence does not imply exactness, admission, or partial byte credit.

## Provenance and tooling reconciliation

January target COFF, disassembly, and relocation records remain authoritative
for exactness, layout, control flow, call order, and private ABI. The preceding
`xbox_sound_cache_obj_jonas_leaf_wave_20260821.md` and
`xbox_sound_cache_obj_jonas_public_quintet_20260828.md` ledgers were read
before editing.

External trees were used only as provenance-ranked semantic and source-shape
evidence:

- HCEA checkout
  `C:\Users\isabe\Documents\Codex\2026-07-13\i-w\_research_cache\halocea`
  at `c89106c4964f2df3a98ef7b4035d6750372797cd`: locked, delete,
  debug-name/dump, start-loading, and debug-render topology. Its evolved PC
  layouts, PC-only statistics branch, and deliberate null write were not
  copied.
- Pastudan full history
  `C:\Users\isabe\Documents\Codex\2026-07-13\i-w\research-cache\pastudan-halo-full-history-20260828`:
  readable ray donor and register declaration at
  `4b231941cbe264d1914696f7e246ea0582c34b82`, independently corroborated
  by the texture-cache twin at
  `c1ffd11510569bfd0c5c1ef0fa5a2190ba9eb93e`. The naked-assembly CFG at
  `f40eefb1` was treated as evidence only, and the autogenerated body at
  `318164ca` was rejected.
- Stian full history: exact-VC71 request source at
  `11e1c1ba3247953a6aa1069d2457347a1c35da48`. Its use of
  `volatile result` establishes why the request can match there, but that
  optimizer-scheduling construct is outside this campaign's source policy.
- Pastudan callback/loader history at `78bb0531`, `e7b02bd6`,
  `a2bea24a`, and `ff17731f` corroborates the four admitted bodies.

The repository `CLAUDE.md` and both donor-history `CLAUDE.md` files were
read in full and reconciled. Applicable requirements were preserved: C89
declaration placement, binary authority over decompiler output, typed ABI and
layout recovery, small reviewable scope, explicit unknowns, and strongest
available function plus relocation comparison. Donor-specific clang/RTK,
`kb.json`, and path conventions were used as research metadata rather than
substituted for this repository's production VC7.1 and hardened COFF gates.

The accepted source contains no inline or naked assembly, `volatile`,
`register`, optimizer pragma or barrier, forced-inline annotation, raw
absolute address, cast-based object layout, union pun, synthetic anchor,
undefined-behavior scheduling trick, or byte-forcing expression.

## Validation record

The final production source compiled through the normal Ninja graph. The full
`halobetacache_build`, `libcmt_build`, `progress`, and
`semantic_progress` replay completed 573 actions successfully.

- Direct hardened comparison: all four new and all ten inherited named owners
  strict exact.
- Semantic report: 470 units scanned; 4,981 functions evaluated; 4,861
  semantic exact; 131 hidden exact / 78,940 hidden code bytes; 4,871 accepted
  exact; zero ordinary rejected; zero unit errors.
- Strict campaign board: 385/833 files, 4,832/11,060 functions, and
  615,719/2,198,102 padded code bytes.
- Halo board: 283/468 files, 4,665/7,574 functions, and
  602,805/1,770,166 padded code bytes.
- Object-admission audit: zero candidates, contradictions, or revocations.
- Parked-function audit: 13 active, zero stale, zero invalid.
- Tooling suite: 212 tests passed.
- Scoped prohibited-token and raw-cast scans: no matches.
- `git diff --check`: pass.

Reproducibility identities:

- Baseline source blob:
  `17ee1011a632c8b04b8590833430813b5598d674`.
- Final source blob:
  `a66fd35d2452eb0f01ac1d917491d5f3de975fe1`.
- Final source SHA-256:
  `a3b989c310c40f6df3b5e67a81cdd6908165af61f66b7e4a9b43253507857952`.
- January split object SHA-256:
  `d023f0f71d3cc6592096ed895c00f69d137fe5c715b1abb8826ae24c0f5f568f`.
- Candidate raw object SHA-256 at the final source:
  `9e923383e10cd1d18de2d28d154b77c6c4934fc7e0832503edc84f69491b5645`.
  Normalized owner hashes above, not timestamp-bearing raw COFF bytes, are
  the admission identities.
- `build/report.json` SHA-256:
  `c091fb4a5911bce2db66c71b7c7da5113edcbb814b3dddd8f18290f53f0f77ce`.
- `build/semantic_report.json` SHA-256:
  `07de36f26091a8d5a3569776aada71b4ddfa23751ca6ba8281c710817f367e8f`.

Only `source/cache/xbox_sound_cache.c` and this ledger belong to the wave.
