# `xbox_sound_cache.obj` Jonas public quintet

## Result

This bounded source-only wave starts from pushed integration commit
`83883cc2e9dc7619b76068d6c86eb01b8ba735d6`. Exactly five previously unwritten,
ordinary-cdecl public owners were frozen together and emitted in one production
compile. All five were independently strict byte-and-relocation exact on that
first object. There was no residual and therefore nothing to tune, retry, or
prune.

`source/cache/xbox_sound_cache.obj` advances from 5/18 to 10/18 strict-exact
functions and from 288/2704 to 1056/2704 padded code bytes. It remains
`NonMatching`.

| Retained function | Meaningful | Padded | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_sound_cache_sound_delete` | 303 | 304 | 28 | `c384b1a2db9ab10a4960bf5151211ae16005782906743f2d821eae2b95d48b5d` |
| `_sound_cache_new` | 175 | 176 | 22 | `2ac1c78359c05c6a5a937cf5ae3f37a0055ababac720b811076a31e4d9252f0f` |
| `_sound_cache_sound_finished` | 106 | 112 | 9 | `e3b449e463e6f83f6443b30ac13d3e9b729965bae516202f238f941a02562fa5` |
| `_sound_cache_flush` | 83 | 96 | 5 | `68e45e0fa5bb92c27a2c5b4625db6b2a098f0dab5039b06f8b42019e2224ceca` |
| `_sound_cache_sound_new` | 69 | 80 | 4 | `fbe723816786643b351e14c428fa63b267126447ad96673881769cdcfe63471b` |
| **Wave gain** | **736** | **768** | **68** | |

The hardened comparator reports identical padded extent, normalized bytes,
relocation address/type, symbolic destination, and addend for every retained
body. The inherited `_sound_cache_delete`, `_sound_cache_open`,
`_sound_cache_idle`, `_sound_cache_sound_hardware_lock`, and
`_sound_cache_close` sections were independently rechecked and remain strict
exact.

## Frozen scope and evidence census

The pre-edit scope was exactly the five rows above. The earlier rejected
`_sound_cache_sound_hardware_unlock` was excluded under the prior ledger's
no-retry boundary. The public request and debug-render bodies were excluded
because they cross private helper/state boundaries. The three private helper
owners and the address-taken LRU callback bodies were not reconstructed.

The sole earlier object ledger,
`xbox_sound_cache_obj_jonas_leaf_wave_20260821.md`, was read before editing.
That wave's five first-shot public hits and one fail-closed rejection supplied
the local hit-rate evidence. The registered
`xbox-sound-cache-baseline-9c260513` and
`xbox-sound-cache-leaves-20260821` worktrees were read-only and clean for the
source and ledger paths. The conservative Claude/Jonas worktree census found
no active overlap on `source/cache/xbox_sound_cache.c`. Units and Vehicles
remained excluded and untouched.

January target COFF/disassembly remained authoritative for every field width,
branch, source line, call order, and relocation. External repositories were
used only as typed semantic/topology donors:

- Clean HCEA pin: `c168af2e747d3095d9a29418ae401f3a39544863`.
  A fresh full-history clone proved the commit is materialized. The five donor
  blobs are `13a98df1175adc85f1cddecb2e577d17b2e9df6c`
  (`sound_cache_sound_delete.c`),
  `a3fc642f86e19a019480124fd7c116e2e1f1c2d8`
  (`sound_cache_new.c`),
  `e07c596a76b31b1415e31420f0948cce0d344910`
  (`sound_cache_sound_finished.c`),
  `082c28edda5b403bbf636b5afcfa0a1f63790db6`
  (`sound_cache_flush.c`), and
  `c12f2c40c6c799684ecc4faded5d1cb6a920350b`
  (`sound_cache_sound_new.c`). They are unchanged at the locally available
  HCEA `c89106c4964f2df3a98ef7b4035d6750372797cd` checkout. Their file SHA-256
  values, in the same order, are
  `448fbc6e7a7f8459f9a5871e5f8e4fc7d6de643adc4d7aac6c8291de18972545`,
  `7de18825d9686b26f474316112285854e5dc441cc89ae74f61f1ef2a5853ed30`,
  `279f93dc9de7f43a32768df48182a58594303e484be517b39442f869fb3610d3`,
  `d636af2d5e6b69c86a875f8b03e7e1a483064367207b3acd453b7df7a316f7ab`,
  and
  `ac4259beadfd9fc1ebf75907a533190a7f420f979d1b8c3dc897101ed299a51a`.
- Stian full-history HEAD:
  `fc11d861865735877d5b8b4c408b1a0c06dde3cf`. Its Xbox sound-cache file
  blob is `4032264437780bfd45aabf2dfed619558b0aac66`, SHA-256
  `9bb1c22d564143cc78b2890368c15ffdee691d26d9275fbb826b90cbd0bbed59`.
  The history records exact-VC71 recovery commits
  `a896c932f69c99ae92e31e7b1750083702b50837` (sound new),
  `528a5cdb9b81bf4481d414f00a79dc3f0e586f40` (sound delete),
  `64ec9faf470ba38ad577d25e8aa68a3a862f3dfc` (cache new), and
  `44bb5c0ba3e9832be7471aef3a8cb5c177bd5cbc` (flush). This donor supplied
  the strongest readable Xbox control-flow topology; its raw-address surface
  was not copied.
- Pastudan full-history HEAD:
  `918af885935ec470a31256ecce9a977b12b01f80`. Its corresponding file blob is
  `1cf535e13176c11df90eec89bd956d336e379f39`, SHA-256
  `d092b54e2265446512789016a4213ec0ee4d384cce12edc9b1bccb1e76d3fa5a`.
  Relevant history includes
  `7e62303afda74a575a9dc3ff6aec5cc2a2392a61` (sound new),
  `a739cb7bfed485225047783526b1f67414600e1e` (cache new), and
  `1119848cf0c1672bad358b34532d3bad14ffc3c1` (sound/texture/cache leaf
  harvest). It corroborates target topology but is weaker than January and
  Stian for the delete body.

## Typed reconstruction and ownership

The existing 0x0C Xbox cache datum gained only the proven byte name at `+0x04`
for `software_reference_count`; compile-time checks retain the `+0x05`
hardware count, `+0x08` sound pointer, and total size. The existing typed
`sound_permutation` members at `+0x2C`, `+0x30`, `+0x34`, and `+0x3C` preserve
the January block-index, base-address, cache-tag, and tag-name dataflow without
raw offset access.

Sound-new validates the null base address, initializes the block index to
`NONE`, clears the base address, and publishes the cache tag. Sound-delete
repeats the typed datum lookup with the same source-expression topology as the
target, preserves both soft and hard reference assertions and their tag/name
formatting, deletes the LRU block, then clears the sound's resident state.
Sound-finished preserves the byte-wide debug flag, silent trace, nonzero
software-reference assertion, and byte decrement. Cache-new creates the
512-entry 0x0C datum array and the 1024-page, 12-bit-page LRU cache, then gets
the physical sound-cache base address. Cache-flush walks the ordinary 0x10-byte
data iterator and deletes only records with both reference bytes clear.

`sound_cache_new` takes the addresses of `_code_001adca0` and
`_code_001adc60` under the existing typed `lruv_delete_block_proc` and
`lruv_locked_block_proc` callback prototypes. It does not call or define a
private ABI. `_bss_004d1088` remains an undefined aggregate reference with
the target `+0x100`, `+0x104`, and `+0x108` addends. The candidate defines no
`.bss`, `.data`, or COMMON owner. Compiler string/debug COMDATs receive no
data credit.

The retained source contains no assembly, volatile scheduling device, raw
address access, cast-based object layout, union pun, undefined behavior,
optimizer pragma/barrier, force-inline annotation, synthetic anchor, or
byte-forcing expression.

## One-shot and validation record

The first `/Zs` shell invocation misspelled the relative include switches as
absolute `/I/source` paths and stopped at the first missing header. It emitted
no object. The corrected production-argument XDK `/Zs` pass then succeeded
with no diagnostics and still emitted no object. A Ninja dry run showed one
candidate edge:

```text
[1/1] CL build\base\source\cache\xbox_sound_cache.obj
```

That edge ran exactly once for the frozen quintet. Its raw COFF SHA-256 is
`4088ee9c4b4625cc024078cb467dc3c99fd2285a929b27b9419705d7d4106ea6`;
the normalized owner hashes above are the reproducible identities. A separate
read-only `gate.py --all` attempt stopped before scratch creation or compiler
invocation because `scratch/` was absent; it wrote nothing and did not emit a
second object. No post-compile source change occurred. The final normal Ninja
replay ran only csplit/report/semantic-report/progress edges and no `CL` edge.

- Source blob: `17ee1011a632c8b04b8590833430813b5598d674`.
- Source SHA-256:
  `b3bc46bb593ef708b22ffb436f2259371c9b8236bafc282db0e340b23ab3eac4`.
- January executable SHA-256:
  `4cc87b45f721270392a96f1674ed2b5cd4a7bb4355faeab4531d1cf1884d9520`.
- January split object SHA-256:
  `d023f0f71d3cc6592096ed895c00f69d137fe5c715b1abb8826ae24c0f5f568f`.
- Microsoft 32-bit C/C++ Optimizing Compiler `13.00.9254.1` SHA-256:
  `483e00c47bb08d699475a642bcff15b5b2036350b31c540e88a506baf101da11`.
  Flags remained `/nologo /c /O2 /Oy- /DDEBUG /Dxbox` with the unchanged
  XDK 3911 and repository include paths.

Validation results:

- Direct hardened comparison: all five new and all five inherited owners are
  strict exact.
- Full 572-action Halo/libcmt build and generated ordinary/semantic reports:
  pass. The semantic report scanned 470 units and 4,699 functions, reported
  4,570 semantic exact, 125 hidden exact / 72,363 hidden code bytes, 4,580
  accepted exact, and zero unit errors.
- Strict campaign board: 273/619 complete objects, 4,551/8,246 exact
  functions, and 612,727/1,922,669 padded code bytes. This wave contributes
  exactly five functions and 768 padded bytes.
- Object-admission audit: zero candidates, contradictions, or revocations.
- Parked validation: 13 active, zero stale, zero invalid.
- Complete tooling suite: 205 tests passed.
- `git diff --check`, changed-path, deleted-path, protected-source, and banned
  construct reviews: pass. Only this source and this new ledger are tracked
  changes.

The pre-edit whole-TU manifest is rooted at `83883cc2` and has SHA-256
`b050de2422b087031f5ebdeb9a38e8cf0411896f1a95076b84944e3d1422b4ee`.
Its comparison correctly reports the five owners as `NEWLY_EXACT` and
`changed_nonexact: []`. It fails closed on the expected new function/string,
symbol, and CodeView inventory and flags two inherited context records after
the new in-TU definitions/type detail appeared. No exception was used to hide
those differences; independent hardened comparison proves every inherited
executable section remains exact.

Nothing is committed or pushed.
