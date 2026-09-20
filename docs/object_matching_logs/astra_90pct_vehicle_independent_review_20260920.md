# Independent review: vehicle desirable

**Admissible function change: 272 meaningful / 272 padded bytes.** The candidate
preserves all 14 inherited strict exact functions and becomes strict exact with
all seven relocation identities unchanged. No whole-object admission is
recommended: two target functions remain residual and ten inherited helper
owners are absent from the target split object.

The baseline was freshly compiled from
`git show 47d2e7b69f1c1277123e979296b6f0a7de00c80b:source/ai/action_vehicle.c`.
The candidate applied only `scratch/astra-context-vehicle-authenticated.json`
to that frozen text. Both used `gate.py --source --all` with the point helper
guard and independent scratch objects. This does not use a subsequently
integrated build/base object as its baseline.

| Independent check | Result |
|---|---|
| Target strict functions | 14/17 before; 15/17 after; zero regressions |
| Desirable code | 288 bytes before; 272 after, matching January |
| Desirable relocations | Seven; all strict identities equal |
| Normalized SHA-256 | `e3b1ecf64be4fb82e7bad92a4bce38d449f57b5b5acba6da82576f649dee1219` |
| Source function sections | 27 before and after; 25 unchanged |
| Named runtime owners | 50 before and after; names, offsets, types, storage and flags equal |
| Runtime noncode | Every section equal by owner and relocation identity |
| COMMON / forbidden point emissions | Zero / zero |
| Fake scan | One complete candidate source scanned; zero review leads |

The only collateral code change is `_action_vehicle_setup_impromptu`, which
inlines the edited routine. Its before, candidate and target sizes are all
464 bytes. It is residual before and after and receives **zero credit**.
`_action_vehicle_find_destination` is unchanged and remains residual. All ten
target-absent helper definitions are inherited and unchanged; this review does
not convert them into whole-object ownership approval.

The patch moves the existing FALSE initialization from the ordinary arm to
the real `desirable` declaration and retains the dead-arm FALSE store. Its
semantics remain defined: dead vehicles produce FALSE, scripted requests TRUE,
and ordinary requests start FALSE until the range/inside test succeeds. Speed
and upright vetoes still restore FALSE. The moved local has no escaped address;
no call, branch predicate, threshold, parameter width, or return policy changes.

The primary unoptimized body at `0x004417E0` independently shows the entry byte
store at `0x00441827`, dead-arm repeat at `0x0044183A`, scripted TRUE store at
`0x0044184B`, and direct ordinary-arm transition from maximum-distance setup
to `object_get_origin` without a second FALSE store. The out-of-range edge at
`0x004418BB` uses the entry initializer; the successful arm stores TRUE at
`0x004418BD`. Thus the redundant dead-arm store is authenticated source
topology, not justified merely by an optimized-byte hit.

The cross-build function identification uses the complete distinctive
actor/vehicle/seven-argument branch and call topology plus neighboring functions;
the RTC `origin` record is corroboration, not a recovered function-name symbol.
Its 2020/v140 source-revision identity to January remains unverified. January
strict bytes and relocations are the final code witness. No UB, arbitrary
filler, forced compilation mode or fake dependency was introduced.

Evidence: `scratch/astra-alias-vehicle-review.py` and `.json`; independently
compiled `scratch/astra-alias-vehicle-{head,candidate}.{c,obj,gate.txt}`;
`scratch/astra-alias-vehicle-fake.json`; and primary disassembly
`scratch/astra-context-action_vehicle_desirable-2020.disasm.txt`.
The detailed source-context provenance is in
[the context packet](astra_90pct_context_packet_20260920.md).
This reviewer edited no production source/configuration, ran no Ninja, and
made no commit or admission change.
