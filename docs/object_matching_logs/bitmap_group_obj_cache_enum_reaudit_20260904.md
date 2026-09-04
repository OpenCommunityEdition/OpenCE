# Bitmap Group cache-status enum re-audit — 2026-09-04

No bitmap source changed. Promoting the existing cache-status enum into its
owning cache_files.h changes one existing fuzzy function's fingerprint;
the strict parked-policy check correctly stopped publication.

Isolated unchanged-source builds using HEAD/current cache_files.h reproduce
old SHA `2e0bbd84c45f240758cc76a2d4df8c78be689cfe9bc21af6a52e017f218e7ca0`
and new SHA `0bd88e42b1b36f4e9df5a8bed007887a18a1443d6f895f15ea9d275f2f6526c4`.
Both are 992 padded / 989 meaningful bytes, 0x34 frame, with all 59 ordered
relocations identical. All bytes before 0xFF and from 0x106 are identical.
The only difference exchanges these independent stack loads:

```text
old: mov ebx,[ebp-4]; mov di,[ebp+0x1C]
new: mov di,[ebp+0x1C]; mov ebx,[ebp-4]
```

They read separate established stack slots, write distinct registers, and
do not affect flags. No call, store, branch or dependency intervenes.
Aligned streams contain 333 instructions each and no other differences.
All five strict siblings remain exact; the function remains an honest
non-exact park at 60.99099% with unchanged reopening criteria. The reviewed
fingerprint update grants no exact credit and hides no source regression.

Evidence under canonical scratch/: bitmap-cache-header-pristine-20260904.h,
bitmap-cache-header-probe-20260904.json,
bitmap-group-pristine-cache-header-20260904.obj, and
bitmap-group-current-cache-header-20260904.obj. No source/header ordering
search, codegen forcing or bitmap implementation edit was used.
