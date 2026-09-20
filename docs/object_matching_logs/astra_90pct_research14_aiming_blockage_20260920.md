> Research-only packet14: zero production change and zero exact credit. Validated production remains wave13.

# Packet14: aiming-blockage assignment evidence

**One historical evidence gap is resolved, but no full-match source mechanism is established. Zero source shapes, C compiler calls or exact credit.** The function remains parked at 385 meaningful / 400 padded target bytes; current output is 384 padded bytes, with ten relocations on each side.

The September14 W3 candidate `avb_body3b.c` already used structured positive conditions, `normalize2d(&horizontal_aiming)>0.0f`, a single result variable and separate vertical 2/1/0 branches. It reached 400 padded bytes but remained nonexact. Its two recorded holds were the apparently redundant vertical zero assignment and the normalization reciprocal's x87 lifetime. Its five old source shapes are not repeated here.

Fresh Ghidra extraction opens the supplied symbol-build project read-only at `0x4710a0`; `primary.py` separately reads the original PE bytes with Capstone. The PE hash is `740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`. Raw RTC descriptors name `source_planar_direction` (8 bytes), `friend_vector` (12) and `friend_perpendicular_vector` (12). These aggregate identities, the four geometry arguments, all range constants, call graph and conditional return flow independently establish function correspondence. Fresh HCEA DIA lines and parameters corroborate the private function and `ai/actor_perception.c`; exact January source revision and lexical text remain unverified.

The actual return word is EBP-0x18, not Ghidra's biased local name. Raw instructions show:

| Address | Operation |
| --- | --- |
| 0x4710c2 | Initialize the return word to zero before normalization and all range tests. |
| 0x4711a3 | Assign two in the narrow vertical range. |
| 0x4711ca | Assign one in the wider vertical range. |
| 0x4711d2 | Assign zero in the vertical out-of-range arm. |
| 0x4711d6 | Read that word and skip horizontal tests when it is nonpositive. |
| 0x471210 | Assign one in the wider horizontal range. |
| 0x471218 | Assign zero outside the horizontal range. |
| 0x47121c | Load the same word into AX for return, preserving it across the RTC call. |

The first zero dominates the vertical zero store, whose independently witnessed repetition answers the old provenance objection. The horizontal zero is also witnessed, but is semantically needed after a result of one or two. No meaningful callee receives the result variable's address. Ghidra's inferred `void` return is not used as type evidence; the actual AX flow is decisive for the returned value.

The later implementation computes planar magnitude before dot product, calls the known three-component point operation, and conditionally calls the vector negate helper. Those operations do not establish a new January source lifetime or helper-emission rule. The affine helper is the already authenticated operation corresponding to `point_from_line3d`; its later call does not authorize a surplus January owner.

The independent normalization lane checks nine strict exact controls. Both same-expression actor-looking validators use the current consumed-reciprocal form, whereas blockage's January instructions retain the reciprocal through the j store and then pop it. The old universal twenty-site explanation is therefore not an established rule. The separate independent review also rechecks the archived `avb3b.obj`: it remains nonexact, including its magnitude/projection stack sequence and epilogue/pop placement. There is no supported shared-header edit, scalar-width correction or additional consumed value that predicts closure while preserving the exact controls. The same reviewer decodes HCEA's raw procedure type and confirms a signed-short return with four pointer parameters.

Consequently, the old near-match is not recompiled and no fidelity-only source lands. Reopening now requires new January-compatible caller/lifetime/type/call-boundary evidence for the remaining reciprocal difference. The repeated-store evidence should be retained rather than rediscovered. Production source, objects, headers, configuration, parked state and admission rules remain unchanged. `IDA_AUTOMATION_UNAVAILABLE`; no repeated artifact request or duplicate-copy search.

Receipts: `primary.py`, `primary.json`, raw disassemblies, fresh Ghidra/DIA captures and the separate independent review. Java launcher compilation is a tool step, not a C source probe.
