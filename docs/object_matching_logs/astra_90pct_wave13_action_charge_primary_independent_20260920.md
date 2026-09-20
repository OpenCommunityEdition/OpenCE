> Root subsequently admitted only action_charge_setup after independent rebuild and whole-tree production checks.

# Action charge setup: independent source and primary review

**APPROVE the one bounded source shape.** This decision rests on new independent primary corroboration of the previously held store topology, not on the old candidate's exact bytes.

The September 14 W2 report and September 15 W3c report already record the same exact candidate and the precise hold: January clears the returned result byte before the inhibited test and clears it again in the inhibited/no-target arms. The accepted fuzzy form kept the common failure initialization inside the deeper melee-attempt arm. Nonredundant alternatives and scope permutations were exhausted. This source spelling is historical; the newly decoded unoptimized operation graph supplies the missing corroboration.

`primary-review.py` independently reads the supplied PE, verifies its hash, decodes the complete function at `0x431e10..0x432378`, and checks its control-flow graph. The byte at `[EBP-9]` is initialized to TRUE at `0x431e53`; after the real unit getter returns, it is cleared at `0x431f62`. That store dominates the inhibited test and both failure arms. The same byte is cleared again at `0x431f78` for inhibited melee and `0x431f9c` for no target, set on success at `0x4322a3`, and loaded into AL at `0x432355` for return. The final RTC check preserves that returned register. Raw source-assertion strings identify `ai\action_charge.c`, assertion `state_data`, line 47. The getter's direct jump thunk and the failure decision values corroborate the surrounding call/branch roles.

January independently uses the same result cell with the prebranch clear at `+0xe4`, inhibited clear at `+0xea`, no-target clear at `+0x107`, and returned AL load at `+0x39a`. Its instruction scheduler places the common store between the condition test and branch; the later unoptimized body makes its prebranch placement explicit. The review does not infer a general compiler rule from this example.

The proposed source moves exactly one existing `result = FALSE;` from the inner melee-attempt arm to immediately after `unit_get`, before the inhibited test. All declarations, existing branch assignments, helper calls and other statements remain unchanged. Every result read is initialized. The local is private and not address-taken by source code; moving its clear earlier adds no externally visible access, uninitialized read, filler, fake dependency or UB. Extra later-build local initializations are not imported.

The candidate source SHA-256 is `5ad8a10528acbbbf4697de32f30ede6354e3acbce290aded0c81c73e3343f0f1`. HCEA/DIA supports signature, source identity and meaningful output-local types. It does not supply January lexical text or prove the later PE has the identical source revision. These limits remain explicit.

This primary review used zero C compiler calls and grants no credit. Root's reported strict candidate is eligible for the separately authorized independent rebuild and complete runtime-owner review before a landing recommendation.
