> Pre-landing evidence. Root subsequently admitted only weapon_update after full wave9 validation.

# Independent weapon_update review: approve function landing

**APPROVE the reviewed function delta**, followed by root's normal production checkpoint and admission accounting. The independent scratch rebuild reproduces all **2,800 padded bytes and 97 relocations**, preserving every one of the **75 inherited exact controls**. The measured potential gain is **2,788 meaningful / 2,800 padded bytes**. No whole-object admission is approved; the other three weapons residuals are unchanged.

Reviewed baseline: `5fe5f0012206c416b133542063598a0bcebf6146`. Candidate source SHA-256: `868d3c928ffa1f431491b29e05020fa1f5fc742a4d39c488dc255672c8a87d1b`. This recommendation concerns exactly `../weapons/paired-recharge-outputs.c` and its published patch. Production source and base object still equal the frozen inputs at review time. Packet 8 was not modified.

## Independent source and primary check

The patch has two bounded changes inside `weapon_update`: replace the input-rate temporary with a short quotient and short remainder, both consumed by the existing recharge additions; and apply the historical D1 form, reading the local trigger array instead of retaining a redundant scalar snapshot. Independent text reconstruction confirms every other byte outside those operations is unchanged, including all other function bodies, interfaces and helper definitions.

The September 14 wave C and September 19 closeout records establish that D1 and the rD input-rate forms were already researched. They remain prior controls, not discoveries. The new paired **output** values differ from rD's input local. A supplementary scan of 78 C files in the preserved wave-C and throughput `w2_weapons` directories found no paired short quotient/remainder form. This is a bounded duplicate check, not a claim that every historical artifact was searched.

Independent Capstone decoding from the supplied PE confirms the complete relevant chain, without relying on the generated Ghidra C:

| Raw operation | Address |
| --- | --- |
| Positive-rate and below-capacity guards | `0x6aabbe`, `0x6aabd4` |
| Signed divide, quotient stored as a word at EBP-0x44 | `0x6aabe7`, `0x6aabe9` |
| Signed divide, remainder stored as a word at EBP-0x48 | `0x6aabfa`, `0x6aabfc` |
| Signed quotient/remainder reloads for the two additions | `0x6aac00`, `0x6aac14` |

These are consumed short results, not dummy pressure variables or inferred filler. The descriptive C names are reconstructed. DIA does not recover their names or January declarations, and an unoptimized later store is not a literal copy of original source text. The raw value flow is concrete support for this natural representation; complete January equality supplies the final binary check.

January independently uses one signed divide by 30 at `+0x407`, consumes remainder at `+0x409` and quotient at `+0x40d`, then performs the carry and maximum clamp. The ordinary two C expressions optimize into this exact sequence. No manual division implementation, arithmetic rearrangement, volatile, assembly, helper copy or compiler change is involved.

The array context also has independent positive evidence. Re-reading RTC descriptor `0x6ab790` recovers a two-byte `trigger_down` object at EBP-0x10. HCEA identifies `unsigned char[2] trigger_down`. Direct raw decoding finds nine indexed reads after the final toggle store, at `0x6aaef1`, `0x6ab0eb`, `0x6ab182`, `0x6ab1e8`, `0x6ab277`, `0x6ab390`, `0x6ab3be`, `0x6ab41f` and `0x6ab596`. Thus D1 is supported by actual repeated reads, not merely the absence of a scalar in debug records.

The supplied PE SHA-256 is `740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`. Its source revision relative to January remains unverified; later network/profiling paths were not imported. HCEA source MD5 is `9ea3d60fa4f1f61c85a268b760efca15`. This review approves a supported reconstruction, not a claim of recovered original January identifiers or lexical source.

## Semantic and ownership checks

There is no intended behavior change for valid objects. The positive signed-short rate is at most 32,767. Dividing by 30 produces a quotient from 0 through 1,092 and a remainder from 0 through 29, so both new short assignments are exact and cannot introduce overflow, divide-by-zero or truncation. The review checked all 32,767 positive input values. Existing compound additions, carry and maximum clamp remain unchanged; their integer promotions and narrowing behavior are not widened or used as a matching device.

The trigger array is initialized before use. Its address never escapes, and no array element is assigned after the old scalar snapshot within an iteration. The index is unchanged until the loop advances. Helper calls therefore cannot make the repeated array reads differ from that snapshot for valid objects. Existing bounds and state transitions remain intact; no new uninitialized read or undefined behavior is introduced.

Full COFF checks cover the original candidate, independently rebuilt candidate and frozen baseline:

- Exactly `_weapon_update` changes among inherited functions, and it becomes strict exact. Both candidates have 76 exact / 3 residual / 0 unwritten of 79 January target owners. Target and both candidates have normalized SHA-256 `308bb149c4e2928e58ecbdd45ee8e41650b8c8da698a047c3f526fc5b708e0eb`.
- All 95 emitted function owners and all 127 named nondebug owner records are preserved, including section flags and COMDAT selection metadata. No helper owner is added or removed.
- Every other runtime section is unchanged. All 33 nondebug noncode sections / 2,221 bytes, their relocations and owners are preserved. COMMON remains empty. `_point_from_line3d` is absent throughout.
- The independent rebuild equals the producer object across all nondebug runtime sections. Baseline and candidate fake scans both have zero findings.
- `_trigger_create_projectiles`, `_weapon_magazine_finish_reload` and `_weapon_place` retain their frozen residuals. No ownership exception, type/ABI ruling, target edit, alias rewrite or comparator weakening is requested.

The diagnostic text alignment contains symbolic-versus-defined-literal annotations. The existing canonical strict comparator correctly matches those identities; all ordered relocation offsets, types and destinations compare equal. This is not an instruction-erasing comparison or a claim of empty textual diff output.

## Reproduction and release

One independent reproduction compile was run, with zero new source shapes:

```text
python tools/campaign/gate.py source/items/weapons --source scratch/astra-wave9/weapons/paired-recharge-outputs.c --all --out scratch/astra-wave9/weapons-independent/rebuilt.obj --forbid-emitted-symbol _point_from_line3d
python scratch/astra-wave9/weapons-independent/review.py
```

The review script reads raw PE bytes, frozen source and objects, preserved historical C files and the independent rebuild. It saves `review.json`, raw excerpts and fake-scan receipts. Its production freeze assertions describe the pre-integration state. Full production compilation, campaign accounting and tests belong to root's subsequent checkpoint; they are not claimed here.

**Review ownership is released.** Land only the reviewed source delta if root accepts this recommendation, then verify the normal full checkpoint. Admitted credit at this pre-integration review remains zero. No further variant or minimization probe is requested.
