# `libs/libcmt/osfinfo.obj`

## Status

Evidence-backed park. The production source is absent and the object remains
`MISSING`; this log grants no matching credit.

## Plain-English result

The original Microsoft C source was recovered and reproduced the January
machine code. Six of the seven functions match strictly. The seventh,
`__alloc_osfhnd`, also has every byte, size, and relocation site/type correct,
but the January split object calls one address `___piob`, while Microsoft's
original compiler object calls that same address `___pioinfo + 256`.

Those expressions resolve to the same linked address, but this project
correctly requires the symbolic relocation owner to match as well. Changing
the C merely to force the adjacent symbol name either changed the generated
control flow or required describing unrelated storage as the loop boundary.
Neither is an acceptable exact-match reconstruction, so the object is parked
rather than falsely admitted.

## Provenance

- XDK 3911 `libcmt.lib`, member `obj\\i386\\osfinfo.obj`.
- CodeView identifies Microsoft C source `crtw32\\lowio\\osfinfo.c` and
  compiler 13.00.9254.1.
- Authenticated Microsoft Xbox CRT source was read from
  `xerohour/xbox_leak_may_2020` commit
  `fd00b4b3b2abb1ea6ef9ac64b755419741a3af00`.
- Required generated flags remained `/O1 /Gy`; no flag, comparator, or symbol
  exception was introduced.

## Best strict measurements

The faithful readable candidate made these functions strict exact:

- `__set_osfhnd`: 70 bytes, 4 relocations, SHA-256
  `634a7f6c114d9f67283e56aca48fd8ea13b933b4cec0ee1c4c5b006452d877e6`
- `__free_osfhnd`: 73 bytes, 4 relocations, SHA-256
  `c0cdb8a41798e0d6505d8430888dcab76215d9496604d70b2bdcaca23aba9cf5`
- `__get_osfhandle`: 65 bytes, 4 relocations, SHA-256
  `3a61a4db311bee3ee9d423fb622f581b062f01826d85c3e6bd506191d3b1506d`
- `__open_osfhandle`: 120 bytes, 6 relocations, SHA-256
  `8c9136431a3ea319607025f39b70f90a5dad4eab4860c0959799f5c38ddd9317`
- `__lock_fhandle`: 115 bytes, 6 relocations, SHA-256
  `0041ca9f2b58b2a479c54cae701d529b3e779dd7310bb2ff401a9746f20af46b`
- `__unlock_fhandle`: 34 bytes, 2 relocations, SHA-256
  `dc1f79b5500aebe0540dbbcf5ea13de980f24b646b34c8916a0e0f64f5b11674`

`__alloc_osfhnd` is 333/333 bytes and 15/15 relocation records, with identical
normalized SHA-256
`3bf5c4032f0e4f3597a3294b00a1bb7bf52bb37ad2225bf55aee37db9138c878`.
The sole strict difference is relocation +0xCD:

| Object | Symbolic destination |
|---|---|
| January csplit target | `___piob + 0` |
| authentic XDK object / faithful candidate | `___pioinfo + 256` |

Campaign symbols place `___pioinfo` at file offset 6,400,736 and `___piob` at
6,400,992, proving the effective destination is identical without treating
that fact as permission to weaken the gate.

## Measured experiments

| ID | One changed factor | Result |
|---|---|---|
| E00 | Faithful source with provisional eight-entry table and direct critical-section declarations | Four accessors exact; lock/unlock and allocator had direct-call and loop-limit drift |
| E01 | Restore `__declspec(dllimport)` and the proven 64-entry table | Allocator became byte/hash/site exact; only the `___pioinfo+256` owner remained; unlock exact |
| E02 | Preserve the authentic second `_pioinfo(fh)` expansion in `__lock_fhandle` | Lock function became 115/115, 6/6, strict exact |
| E03 | Name `___piob` as a typed one-past-table boundary in the `for` condition | 352 bytes/16 relocs; loop was prechecked and reordered, rejected |
| E04 | Put that boundary in a `do/while` condition | 339 bytes/15 relocs with broad block reordering, rejected |
| E05 | Use an explicit post-body signed induction and boundary break | 359 bytes/16 relocs with extra reload/control flow, rejected |

## House and Berth audit

The retained repository change is documentation only. All experiments used
readable typed C: no assembly, `volatile`, `__forceinline`, unsequenced UB,
byte forcing, compiler-flag changes, relocation equivalence, or comparator
exceptions. The faithful source and rejected shapes were removed, and config
was restored to `MISSING` under the atomic-object rule.

## Reopen criteria

Reopen only if one of these becomes available:

1. January-local source/header provenance that naturally names `___piob` as
   this loop boundary;
2. target extraction that preserves the authentic `___pioinfo + 256`
   relocation owner; or
3. a reviewed campaign-wide policy for csplit aliases that remains
   fail-closed and is not specific to this object.
