> Research-only packet10: zero production change and zero exact credit. Latest validated production remains wave9.

# Packet10: actor_look_update — saved major-validation outputs

**One new primary-backed source shape measured; byte-inert, rejected for production, zero credit.** No second shape is justified. The assigned function remains nonexact at 4,704 padded bytes / 159 relocations against January 4,720 padded / 4,714 meaningful bytes / 159 relocations. All source changes stay in this scratch directory. No production/header/configuration/Ninja/commit changes were made. `IDA_AUTOMATION_UNAVAILABLE`.

## History and distinct evidence

Read the 100K/wave-C, 150K w1/w3 and September19 evidence-closeout ledgers, original 150K worker notes, and the current wave2 evidence/independent review. Previous scope, declaration order, saved-minor flag, conditional/switch, stationary-copy and compiler-option families remain closed. Wave2 already installed its authenticated float-array/private-pointer type, initial store order, primary clears and independent override equalities. Those are baseline, not new work.

A focused archive check in `prepare.py` reads 139 C files / 129 unique contents from the original 100K, wave-C, 150K, w3 and throughput `w2_actor_looking` portfolios. No file contains the two saved **major** validation-output assignments proposed here. This census is distinct from the historical closeout's 139 unique candidates; it does not claim another exhaustive Git/artifact search. The earlier `m1` saved-minor-direction probe is a different site and was not repeated.

The retained actual Ghidra export of the authenticated 2020/v140 PE identifies the two major-validation helper paths. `prepare.py` freshly reads the executable bytes and checks these precise instructions, independent of decompiler local-name guesses:

| Value flow | Raw PE addresses |
|---|---|
| Aim helper returns AL; store existing major result byte | `0x4609cb` call, `0x4609d3` store at EBP-0xb1 |
| Test saved result before vector-copy effects | `0x4609d9` reload, `0x4609e0` test |
| Look helper returns AL into the same result byte | `0x460a45` call, `0x460a4d` store |
| Test saved result before vector-copy effects | `0x460a53` reload, `0x460a5a` test |
| Test the same result again for major-success handling | `0x460a81` reload, `0x460a88` test |

January independently has the corresponding two helper calls at +0xacf and +0xb11 and compatible success/failure branches through +0xb35. Optimization removes the explicit major-result stores; January does not uniquely recover the lexical C assignments. The later raw return/store/read graph supplies the narrow source reconstruction evidence. Same-source-revision is unverified, and no later network, layout, dead-flag or extra-control behavior is imported.

Existing HCEA DIA records and freshly re-parsed PE RTC confirm the current aggregate types and extents: two-element float deviations, primary/secondary/major/minor vectors, a 16-byte direction specification and three 2D vectors. They do not authenticate the missing January scalar lifetime. The reused Ghidra/DIA file hashes are saved in `review.json`; no Ghidra project was opened or rerun for this packet.

## The single source shape and measurement

The two current branches test a helper return and set `major_valid = TRUE` after their vector stores. The scratch candidate instead assigns the helper return into the **existing** `major_valid`, tests it, and performs the same vector stores on success. Its later major-success test is unchanged. It adds no local, call, helper, cast, qualifier, header or artificial use. On failure the flag remains false; on success its only consumers test truth, so retaining the helper's Boolean value preserves behavior. The new fact was communicated to root before compiling.

| Object | Padded / relocations | Normalized SHA-256 |
|---|---|---|
| Frozen production and fresh baseline | 4,704 / 159 | `4bc222ca36d696b3a8bfebfb48c402e06df583ac82cac6fbd3f79d305adc9f7d` |
| Saved-major-results candidate | 4,704 / 159 | same full hash and ordered relocations as baseline |
| January | 4,720 / 159 | `47ee3e6abf3917b8284bf08f17bd5268190a8088988a1c457f53ca6fffdf2379` |

There were two C compiler invocations: one fresh baseline replay and one candidate, using normal PID-isolated gates and unchanged flags. Both report 14 exact / 2 residual / 0 unwritten. Every emitted function remains identical, including the second residual `_actor_look_idle_find_prop`. January and candidate retain the same external relocation multiset; seven of the eight internal relocation offsets differ along with the residual code and are not claimed equal.

`audit.py` verifies full non-debug runtime equality, including normalized bytes, ordered relocation identities, section flags, owner names/types/storage/offsets, COMDAT selections and COMMON. All 33 function owners, 117 named runtime owners (excluding compiler-local `$`/section labels), 84 noncode sections, empty COMMON and absence of `_point_from_line3d` are preserved. Current production source and base-object SHA-256 values still match their frozen copies. The result is an inert correction hypothesis, not coverage progress or a production recommendation.

## Concrete remaining boundary

The same secondary-selection allocation problem remains: January saves the secondary aim-valid result at EBP-7 and keeps the desired-facing pointer in EBX; current code keeps the result in BL and changes pointer reloads and later stack homes. Neither the nine RTC aggregates nor HCEA's retained typed records prove a missing address-taking use or lifetime for that scalar. This newly authenticated major-result flow does not change it.

Reopen only for a new January-compatible scalar use/lifetime, genuine call/type boundary, or measured compiler law that predicts that allocation while preserving every exact control and selected owner. Do not repeat these saved-major assignments, minor-valid flag, local/scope/order/type permutations, dead cross-build flags or header visibility sweeps. No second candidate or production patch is proposed.

## Files

All new evidence is under `scratch/astra-wave10/actor-looking/`: `prepare.py`, `preparation.json` (archive census and raw assertions), `major-validation-return-flow.raw.asm`, `saved-major-results.c/.json/.patch/.obj/.gate.txt/.asm`; frozen `baseline.c/.obj/.asm`, `baseline-fresh.obj/.gate.txt/.asm`, `january.obj/.asm`; and `audit.py`, `review.json`, `summary.json`, this `packet.md`. The candidate JSON is a rejected research probe, not an admission proposal. Ownership released.
