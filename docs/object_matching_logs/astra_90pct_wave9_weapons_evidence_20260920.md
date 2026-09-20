> Pre-landing evidence. Root subsequently admitted only weapon_update after full wave9 validation.

# Packet 9 — weapon_update recharge outputs

One primary-supported scratch candidate makes `_weapon_update` strict exact: **2,788 meaningful / 2,800 padded bytes, 97 relocations**, normalized SHA-256 `308bb149c4e2928e58ecbdd45ee8e41650b8c8da698a047c3f526fc5b708e0eb`. The unit moves from 75 exact / 4 residual to 76 exact / 3 residual in the scratch gate. Independent review is pending; admitted credit remains zero until root approval and integration. No production source/header/configuration/Ninja output or commit was changed by this lane.

## History and why this is new

Read before probing: the weapons Codex checkpoint and owner/inventory reconciliation records; September 12 historical packet and September 13 owner park; September 14 100K, wave C, 150K W1 and W3; September 15/16 Tier B; and September 19 evidence closeout. The current unit has no applicable unresolved `weapon_update` park entry, but its documentary do-not-repeat boundary was treated as binding.

The old D1 form removes a redundant scalar trigger-state snapshot and consistently reads the existing two-byte trigger array. The old rD form captures the **input** recharge rate in one short. D1+rD already reached 2,800 bytes/97 relocations, but retained a recharge-block register/store difference. Many local type/scope/order, MIN/clamp, increment, statement-order and operand-order variants were exhausted. They were not repeated. Targeted reads of the archived wave-C patch snippets and throughput `w2_weapons/wu1..wu6.py` confirm those forms.

The new evidence is a different, meaningful value flow: two short **outputs**, one whole-round quotient and one fractional-round remainder. This is not a new placement/type spelling for the previous input local.

## Primary correspondence

Actual Ghidra 12.1.2 ran read-only against the supplied `halo_cache_symbols.exe`, exporting `0x6aa140` to `ghidra_006aa140.asm` and `.c`. `collect.py` and `collection.json` preserve the execution. Executable SHA-256 is `740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`. `primary.py` independently reads the PE bytes and emits `magazine-recharge.raw.asm` and `primary.json`, verifying the decompiler's relevant inference.

Raw machine value flow:

| Operation | Supplied /Od counterpart |
| --- | --- |
| Require positive short recharge rate and loaded rounds below capacity | `0x6aabb5..0x6aabd4` |
| Signed rate / 30, then short quotient store at EBP-0x44 | `0x6aabda..0x6aabe9` |
| Signed rate % 30, then short remainder store at EBP-0x48 | `0x6aabed..0x6aabfc` |
| Read quotient, add to loaded word | `0x6aac00..0x6aac10` |
| Read remainder, add to fractional word | `0x6aac14..0x6aac31` |
| Fractional carry increments loaded count and subtracts 30 | `0x6aac35..0x6aac5f` |
| Clamp loaded rounds to capacity | `0x6aac63..0x6aac7f` |

January target `+0x3e8..+0x429` independently proves the same guard and quotient/remainder operations, with a single commoned signed IDIV at `+0x407`. It uses the remainder at `+0x409`, quotient at `+0x40d`, a fractional carry at `+0x41e..+0x425`, and a capacity clamp at `+0x429..+0x433`. The candidate's two ordinary C quotient/remainder expressions compile into that complete January sequence. There is no arithmetic reassociation or manual division expansion.

The output-local names are descriptive reconstruction, **not recovered identifiers**. Short signed value widths and consumed output values are supported by the raw stores and signed reloads; fresh DIA does not name these scalar locals. With a signed-short positive rate, quotient and remainder fit in short (quotient at most 1092, remainder at most 29), so the two assignments introduce no truncation or overflow behavior.

Fresh DIA `hcea_weapon_update_sym.txt` identifies parameter `long weapon_index` and local `unsigned char[2] trigger_down`; `_lines.txt` identifies `d:\projects\code\hcex\sources\items\weapons.c`, MD5 `9EA3D60FA4F1F61C85A268B760EFCA15`. The supplied PE's RTC descriptor at `0x6ab790` independently names `trigger_down`, size 2 at EBP-0x10. `primary.py` re-parses that descriptor. Raw `0x6aaee9` writes the toggled result into the array, and `0x6aaef1` reads that array directly for the released-since-last-shot test. This corroborates the previously January-supported D1 operation; absence of a scalar in DIA alone would not prove its nonexistence.

Same-revision source identity is **not authenticated**. The later executable adds network/profiling paths; no such path is imported. Its corresponding recharge operations and array accesses are used as primary reconstruction evidence, while January bytes/relocations remain the final authority. Ghidra-generated return types, local names, unreachable-block rewrites and source layouts are not promoted to original-source facts.

## Candidate and semantics

`paired-recharge-outputs.patch` makes exactly two coherent changes inside `weapon_update`:

1. Compute short `rounds_recharged = rate / TICKS_PER_SECOND` and `fractional_rounds_recharged = rate % TICKS_PER_SECOND` before their respective additions, matching the independently observed output-value lifetimes.
2. Apply the established D1 array form: remove `boolean trigger_down` and its snapshot assignment; use `triggers_down[trigger_index]` at the same tests.

The array is initialized before use, remains local, is never passed to a helper, and has no writes after the old scalar snapshot point within an iteration. Re-reading it therefore preserves the existing snapshot behavior across helper calls. Index bounds and all flag operations are unchanged. All other function bodies, helper ownership, declarations outside the function, headers, constants and interfaces remain unchanged.

Exactly **one new shape** was compiled. No follow-up variants or ablations were needed once it passed.

## Strict gate and ownership

```text
python tools/campaign/gate.py source/items/weapons --source scratch/astra-wave9/weapons/paired-recharge-outputs.c --all --out scratch/astra-wave9/weapons/paired-recharge-outputs.obj --forbid-emitted-symbol _point_from_line3d
```

Result: `exact 76 residual 3 unwritten 0`, point guard passed. `audit.py` checks full normalized section bytes and relocation identities, not a stack-erased diagnostic.

| Check | Result |
| --- | --- |
| Inherited strict controls | all 75 preserved |
| Changed existing function | `_weapon_update` only |
| Emitted function owners | 95 before and after; none added/removed |
| Non-debug named owner records | 127 before and after; unchanged |
| Non-debug noncode sections | 33; bytes, relocations and owners unchanged |
| COMMON | empty before/after |
| `_point_from_line3d` | absent before/after |
| Other three residual bodies | unchanged |
| Production source and base object | byte-identical to freeze |

January and candidate both have 851 disassembled instructions. `paired-recharge-outputs.alndiff.txt` still displays defined-noncode versus symbolic literal annotations, an existing representation difference accepted by the strict canonical relocation comparator. It is not claimed to have zero textual diff hunks; full strict equality is the authoritative result.

Frozen current source SHA-256: `62a1a05b9a39a7f0ddc42cd3062330734929de4ffcace84904a84c6f0e59e4d5`. The source/object paths and hashes for independent review are in `candidate.json`; complete COFF preservation measurements are in `audit.json`.

## Decision requested from root

Independently review the two short output lifetimes against the raw instructions and verify the complete candidate under current headers. If approved, integrate only this function's patch and the provenance report, then run root's normal checks. No shared-header change, COMDAT exception, ABI ruling or metadata rewrite is needed. The three remaining weapons residuals stay outside this packet.
