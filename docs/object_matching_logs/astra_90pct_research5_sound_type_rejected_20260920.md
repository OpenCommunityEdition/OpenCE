> Research-only: zero new exact credit; no production changes.

# Independent result: coherent short-owner test rejected

The first falsifier failed. The coherent two-file `long` -> `short` parameter change regresses the already exact `_sound_definition_find_pitch_range_by_pitch`. **Do not proceed to consumer gates, tune the body, delete casts, or revive the incompatible short-header/long-definition proposal. No exact credit is available.**

I independently read the root's frozen, freshly rebuilt before, candidate after, and original January owner objects using the unchanged COFF comparator. No independent source/header edit or compile was performed. Baseline is `11746616760fffd6066354ffb7e5e0077eac06e6`.

The frozen and freshly rebuilt baseline function measurements agree. All seven listed owner-unit functions are strict exact before; only six remain exact after. The precise change is at function offset `+0x9`:

```asm
January / baseline: mov esi, dword ptr [ebp+0x10]  ; 3 bytes
coherent short:     mov si,  word ptr [ebp+0x10]  ; 4 bytes
```

The operand-size prefix adds one byte. The remaining operation sequence is unchanged, while the two `_tag_block_get_element_with_size` call relocation sites shift from `+0x31/+0x85` to `+0x32/+0x86`. The function consumes one byte of padding: the code extent changes from 253 to 254 bytes, with padded size still 256 and relocation count still two. Equal size and count therefore do not preserve strict equality.

| Object | Normalized SHA256 |
| --- | --- |
| January / before | `a2d60b28f22526b6adc7f21c9bd8210dee343da2a4f3a9acc4cfad61f66ffe5e` |
| Coherent short after | `2718c8db7507624ed852f08c41bbba1df95b89379e378fb6ba96c457dd762b71` |

All six other listed exact functions remain exact. Only the pitch-range owner changes; no defined owner is added or removed. All seven non-debug noncode sections remain byte-and-relocation equal, COMMON is equal, and no `_point_from_line3d` owner appears. This isolates the failure to the proposed true parameter type rather than unrelated build drift.

The primary evidence remains valid within its limits. HCEA's named parameter is a signed short, and January only consumes its signed low16 value. The new measurement shows that these facts do **not** suffice to reproduce the January owner with a short declaration under the unchanged source body. It does not prove a universal compiler-width law or that the later source revision is identical. The old header-only mismatch stays rejected; the new coherent alternative is now measured and rejected too.

Root restored both files. Independent byte comparison against the frozen copies passes:

- `source/sound/sound_definitions.c`: SHA256 `21dba7728a5152b36c883191173ad3d8cff93f4a3a6efdaa3572032ba0a15f5a`.
- `source/sound/sound_definitions.h`: SHA256 `a12a953ac5896f639486c4d0f697e1af55fe3cf6ab6d78b8f67d199fe70ca3be`.

The source/header git diff is empty. No after objects exist for any other header consumer, as required by the early-stop condition. The 205-byte caller was not probed or admitted; the 1,359-byte looping caller remains outside this failed test. No source shape should be spent to recover the callee's initial dword load.

Receipts in this directory: `independent_review.py`, `independent-review.json`, `independent-review.stdout.txt`, `independent-{frozen,fresh,after,target}.asm`, and `independent-owner.aln.txt`. The prior `independent-primary.md` is the preflight rationale, not an approval after this failed gate. All ownership is released.
