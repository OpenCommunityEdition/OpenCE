# zlib `deflate.obj` / `gzio.obj` upstream-name closeout (2026-08-31)

This ledger records the bounded integration of the authentic zlib 1.1.3
private names and data owners in `source/memory/zlib/deflate.obj` and
`source/memory/zlib/gzio.obj`. Both translation units remain completely strict
exact: `deflate.obj` is 18/18 functions and `gzio.obj` is 24/24 functions, with
no residual, missing, or candidate-only function. This is a provenance and
ownership hardening change, not new byte credit; both objects were already
complete under their January address-derived aliases.

## Scope and source authority

- Integration commit at audit time:
  `ee62f36f1f440ad9d17b925bdfe4449c84afbc72`.
- Fable donor commit:
  `21f4fc5505754272da1ba64b7f0d51b08c6517d7`.
- Donor parent:
  `0eb0d09aa51c95622afba046b81aeb40e76db79e`.
- Parent source blobs:
  - `deflate.c`: `99b9fcbc79cde499d54293cf1dcebf2d36bc9310`.
  - `gzio.c`: `fd20daee815b8eaf48a23e0f85bb44f555f2a6d2`.
- Donor source blobs:
  - `deflate.c`: `add1e7b8002fb2273ac15e56e476f80f17cb2073`.
  - `gzio.c`: `adcd3748a380c715cd7374558e99d7efeea57978`.

The naming authority is the official
[zlib v1.1.3 source](https://github.com/madler/zlib/tree/v1.1.3):

- Tag object: `d95de550db334fb4c649a8b9b4048bc4aaf7a332`.
- Commit: `14763ac7c6c03bca62c39e35c03cf5bfc7728802`.
- Tree: `f1055d11ef7b282b698ce7c40e1a9c061413cbdf`.
- `deflate.c` blob / SHA-256:
  `25d5818e2ece934bfa178d6cf9de7c951876f2da` /
  `c06fd141a6cea8cf4c5cf1ee5580096ed83a662f690b61ba9371ecbcbadce76e`.
- `gzio.c` blob / SHA-256:
  `f7c336a55a0911bdd45fcd9caa10c8570b3c5d6c` /
  `46c17eab7da3d6c71d669fa4e35a3eea68c9259977461b95e154053f7a744e45`.
- `zutil.h` blob / SHA-256:
  `6f2cb97ca11ff23c92c0e7a84079d6494ce81669` /
  `90463dc0b6613745904e430fd127e8af942f81ba55d325ce266ca65ca7738e82`.

The official files independently establish all fifteen private function
names, `gz_magic`, and the linkage spelling: zlib's `local` macro is `static`.
No inferred Halo name is used in this tranche.

## Strict code identity

The following January functions are exact after replacing their address
placeholders with the official zlib names. Meaningful size excludes compiler
padding; padded size is the complete target function section.

| Function | Meaningful bytes | Padded bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| `_putShortMSB` | 39 | 48 | 0 | `b87db1f4d0b00a605c04bbdcc6e0850309c882e9bfe8c67a843c4db08d0a531d` |
| `_flush_pending` | 95 | 96 | 1 | `044875a69c40c06ec198616eb061e0d2c74bf4e853c33a2461587f7f06ea61e3` |
| `_read_buf` | 93 | 96 | 2 | `9b6f0f03e8384f91e5faacd99935846810d99a167d355b0a5cbe5cc6df400bb9` |
| `_lm_init` | 126 | 128 | 5 | `b860fa72e74d4ce750eddd5e5ff68e976cf0331beb099c87268b5cc4a04e9e14` |
| `_longest_match` | 521 | 528 | 10 | `8a1d6344cfb68529c5fd6f53b56d5c197d29c9bb90f3eb7aa3839282e2687b5e` |
| `_check_match` | 182 | 192 | 15 | `c451d25eae06a4b6038fc085aa0b33aadffe11702c5bc2cfb319c8593ecea9c6` |
| `_fill_window` | 408 | 416 | 5 | `00cb4427bb4942f97710e07d02c90f1213a976944241c537628241847f229ff2` |
| `_deflate_stored` | 652 | 656 | 23 | `d7eda463eba9161ecef6783c0ed07998d6f57cdada916b8a565cce6ee3f14f1e` |
| `_deflate_fast` | 764 | 768 | 21 | `dbcf220787581bda88365da8521f655b48d50124ea232bd418c50a659a805ee6` |
| `_deflate_slow` | 957 | 960 | 34 | `89848a5e64d7da3f20d3db1c70e3958530e4b2a2af4c4ca689ea82d18a8f1686` |
| `_get_byte` | 103 | 112 | 2 | `f0f334ce081302b46f83cb629cdee79f879d8bd53e19d65b5eb8ade98a25de22` |
| `_check_header` | 1,035 | 1,040 | 19 | `8d6a57fe8ff82a95c7ee5e3a5eb4a29227118fe10a6b71fc37201ed25a50ee66` |
| `_destroy` | 220 | 224 | 14 | `53c204d7182b9e708d1c0e1778d304b06f80b1b3e4f2035fced309417e4d55e1` |
| `_getLong` | 56 | 64 | 4 | `e311fcf89ee2a32738d3613bf7cea2437e5427b24b5b796ac8a346f0c72d04fc` |
| `_gz_open` | 581 | 592 | 25 | `ea7db4c73fa915ab47efad602c1dc07a885657a26ea1469d52e9f5bd6b1b90b8` |

These renamed private functions cover 5,832 meaningful and 5,920 padded
bytes. All fifteen have COFF storage class 3 in both target and candidate.
The already named `do_flush` and `putLong` also have target/candidate storage
class 3; their January symbol records are therefore now explicitly marked
`static`. The remaining eight `deflate` and seventeen `gzio` public functions
retain storage class 2. No function body, scheduling construct, compiler flag,
or matching exception was introduced.

## Authentic local-static data ownership

Upstream zlib 1.1.3 declares this object inside `deflateInit2_`:

```c
static const char *my_version = ZLIB_VERSION;
```

The Fable donor moved it to file scope as `my_version`. That deviation was not
integrated. VC7 gives the authentic function-local static the decorated owner
`?my_version@?1??deflateInit2_@@9@9`; the file-scope deviation instead emits
`_my_version`. The January symbol and semantic-data manifests now name the
decorated owner directly, preserving upstream source placement and January
relocation identity at the same time.

| Owner | Bytes | Relocations | Normalized SHA-256 |
| --- | ---: | ---: | --- |
| `?my_version@?1??deflateInit2_@@9@9` | 4 | 1 | `df3f619804a92fdb4057192dc43dd748ea778adc52bc498ce80524c014b81119` |
| `_gz_magic` | 8 | 0 | `726e93918cc0c08e90b8acf61fa02f457571446c5429c3fd58c231d769c0a559` |

The `my_version` relocation points to the exact zlib version-string COMDAT
`??_C@_05HBMFIKCP@1?41?43?$AA@`. Both data owners have storage class 3 in
target and candidate. `gz_magic` is spelled as upstream `local int`, rather
than an external address-derived `data_...` variable.

An isolated replay through `apply_semantic_data_matches` credited the
decorated owner as `+4 data bytes` and advanced both unit and aggregate test
measures from 0/4 to 4/4. Thus the authentic spelling passes both the strict
COFF comparator and the repository's semantic-data gate; no data-placement
exception or file-scope compromise is needed.

## Blast radius and source credibility

No header changes are required. All renamed helpers and both renamed data
owners are translation-unit private. The only source-tree occurrence outside
`deflate.c` is zlib's inactive `ASMV` implementation of `longest_match` in
`contrib/asm386/gvmat32c.c`; the Halo edge neither defines nor builds `ASMV`.
The semantic-data manifest is the only non-symbol-map consumer of the renamed
data owner.

The retained source is the readable upstream algorithm. It contains no added
inline assembly, `volatile` lifetime steering, forced-inline/noinline
annotation, optimizer pragma or barrier, raw-byte emission, address-derived
identifier, type pun, self-assignment, contradictory branch, or inert fake
match. `tools/fake_match_scan.py --include-vendored` reports zero review leads
for both changed source files.

## Reproducibility and focused gates

The executable was split to a PID-independent temporary directory with the
edited symbol manifest, and each canonical source was compiled directly with
the unchanged production VC7 flags. No global configure or Ninja invocation
ran.

| Object | Target functions | Candidate functions | Strict exact | Residual | Missing | Candidate-only |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| `deflate.obj` | 18 | 18 | 18 | 0 | 0 | 0 |
| `gzio.obj` | 24 | 24 | 24 | 0 | 0 | 0 |

- Split `deflate.obj` SHA-256:
  `78aafce10156cb2d59b9f738c8b085c6b086da95fc2ef5b335a6d4651fb40ace`.
- Candidate `deflate.obj` SHA-256:
  `3b185c4629553f4f5ff5d20351430abc2eec215c8ddf8306d78eb4ea8bd8b424`.
- Split `gzio.obj` SHA-256:
  `19bee670bcba4f28485e956119bdb3f2039b327d72b86f82b8a8a98b502135fc`.
- Candidate `gzio.obj` SHA-256:
  `5d39291265ccf9516ee815cdf23af694a8c126cfed15308d8c666dba56ba8522`.
- Retained `deflate.c` SHA-256:
  `0bb2b4b021a5cb14e34355ed3e9eaf46fbdd34cf05d11276bc06a2072a627fce`.
- Retained `gzio.c` SHA-256:
  `faefe356cee9c48d478d2f41b07c855215bab06fd5b14c082968451277419b11`.

The focused storage census found no mismatch, the complete function census
found no candidate-only COMDAT owner, JSON parsing passes for both edited
manifests, and `git diff --check` is clean apart from Git's line-ending notice.
The orchestrator still owns the consolidated global build, regression sweep,
commit, and publication gates.
