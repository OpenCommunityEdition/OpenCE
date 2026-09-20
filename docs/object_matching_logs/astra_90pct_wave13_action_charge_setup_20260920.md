> Root subsequently admitted only action_charge_setup after independent rebuild and whole-tree production checks.

# Action-charge setup: authenticated failure initializer

One source shape is strict exact in scratch: **930 meaningful / 944 padded bytes, 29 relocations**, normalized SHA `34c0c115890ecab8f0ba02d670b180aeeb11963850e56f5b81c2b400b224fa9f`. All20 inherited exact functions remain exact. Independent review and production integration are still pending at this producer checkpoint; this report alone grants no credit.

## Why the historical hold can now be reconsidered

The September14 W2 report already had an exact candidate placing `result = FALSE` after `unit_get`, before the inhibited and missing-target tests. It was held because both failure arms repeat the assignment and there was no independent source evidence for those repetitions. The spelling is historical; the new evidence changes its admission premise.

Fresh Ghidra headless extraction opens the supplied `halo_cache_symbols.exe` project read-only and exports actual instructions and decompilation at0x431e10. Source assertion `ai\action_charge.c`, line47, `state_data`, the goal dispatch, state fields, melee-range outputs and calls establish correspondence. SHA256 is `740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`. Same-revision source identity and January lexical text remain unverified.

`primary.py` independently reads the executable bytes with Capstone. The actual return cell is EBP-9 (the generated decompiler local name has a four-byte bias). Its relevant operations are:

| Address | Operation and role |
| --- | --- |
|0x431e53|Store1: default result.|
|0x431f62|Store0 after unit_get, before the inhibited flag load/test.|
|0x431f78|Store0 again on the inhibited branch.|
|0x431f9c|Store0 again on the missing-target branch.|
|0x4322a3|Store1 on successful melee movement.|
|0x432355|Load the same byte into AL for return.|

The dominating store precedes both branch-owned stores. The deep failure paths rely on that dominating zero; they do not receive an invented new result. No callee receives this local's address. January independently contains its result-cell stores at+0xe4,+0xea,+0x107. The later unoptimized instructions corroborate source-owned assignments; the optimized January output alone is not used to infer arbitrary duplicated source.

Fresh DIA symbol and line exports corroborate action_charge_setup, long actor_index, short goal, charge_state_data pointer and the four meaningful short/float melee outputs. They do not recover full scalar declarations or January lexical braces. Ghidra's inferred undefined8 return is not a C type assertion; actual AL flow and the current owned boolean signature are used.

## Sole candidate and gate

`setup-initializer.patch` moves the existing inner `result = FALSE` assignment before the inhibited test. Both existing branch assignments remain. No new local, repeated source statement, header, prototype, helper, layout, compiler option, ownership exception or result dependency is introduced. The move preserves defined behavior and follows the independently observed original assignment placement.

Root ran exactly one C compiler invocation through the normal campaign gate, with distinct scratch output. The unit has a genuine inherited point_from_line3d owner, so a blanket forbid-point flag would be incorrect. The complete ownership audit instead verifies the same single48-byte exact owner before and after.

The gate changes20exact/2residual to21exact/1residual. Only setup changes; perform remains byte/relocation-identical to its prior residual. `audit.py/.json` verify all23 codeowners,45 named runtime-owner records, all23 noncode sections, empty COMMON, and the inherited point owner. No owner is added or removed; all old exact controls survive. Production C and base object are still frozen-identical at this checkpoint.

## Scope and next gate

The current result is one bounded source correction supported by January and independent first-party unoptimized code. It is not a general permission to duplicate stores or a universal compiler scheduling law. The independent reviewer must re-read primary bytes, assess source credibility, rebuild separately and check every runtime owner before root integration. No header or configuration edit is needed. `IDA_AUTOMATION_UNAVAILABLE`; the local-artifact question remains closed.
