> Research only: zero admission credit; no retained source/header/configuration change.

# Packet 8 — action-charge perform: one evidenced probe, rejected

Disposition: **no admitted change or exact credit**. One new scratch shape was tested; no second shape was justified. Production source, headers, configuration, Ninja outputs and commits were not changed by this lane. `audit.json` verifies production source and base object still equal the freeze.

## Frozen authority and inherited constraints

The current unit has 20 strict-exact target rows and two residuals. `_action_charge_perform` is 3,216 padded bytes / 84 relocations; January is 3,246 meaningful / 3,248 padded / 84 relocations. The frozen source SHA-256 is `c3781a7634798cede3b29a62479187cc4069fdeaa380b903f4e966d95d440c68`.

The retained `_point_from_line3d` is the genuine selected January owner, 48 bytes and strict exact. It must survive; a blanket forbid-point-emission guard would be wrong here. The inherited helper rename/body package is preserved unchanged.

The August 26 rejection, August 28 public recovery, August 29 reintegration, September 6 authentic-header-owner report, September 14 fresh-graphs and W2 reports, and September 15 W3c report were read before probing. Their exhausted hypotheses include local declaration/scope permutations, dot argument order, commutative arithmetic order, placement before helper definitions, and weapon/LOS spelling. Prior normal-header and local-inline helper variants were residual. The normal-header route also emitted `_magnitude_squared3d`; the August 26 record includes actual LNK2005 against its January canonical owner. None was repeated.

## Fresh primary evidence

`collect.py` ran actual headless Ghidra 12.1.2, opened the supplied project read-only, and exported raw instructions and decompilation for PE `0x430170`. Function identity is corroborated by both `ai\action_charge.c` references (assert lines 288 and 487), the distinctive action-state graph, and its 11 RTC records. `primary.py` independently re-reads PE bytes using Capstone and re-parses every RTC descriptor entry at `0x4316f4`, rather than merely trusting the saved census or decompiler.

The supplied PE SHA-256 is `740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`. Same-revision source identity is **not authenticated**. Compiler/build dates alone are not evidence of a source-revision change. Specific operation differences below limit this function's use as a direct donor.

Fresh DIA exports `hcea_action_charge_perform_sym.txt` and `_lines.txt` identify source `d:\projects\code\hcex\sources\ai\action_charge.c`, MD5 `8771DE8DBB77799C2A97F68170321DD0`, and a `long actor_index`. Recovered typed locals include `real_vector3d vector_to_target`, `aim_vector`, float `minimum_velocity`, `horizontal_velocity`, `vertical_velocity`, and two `real_vector2d` alignment locals. These agree with the current aggregate widths. They do not recover `check_range`/`berserk_ranges`, all scalar locals, January stack-cell ranking, or complete lexical scopes.

The raw RTC descriptor contains these independently checked extents:

| Name | EBP offset | Bytes |
| --- | ---: | ---: |
| vector_to_target | -80 | 12 |
| estimated_target_position | -112 | 12 |
| alignment_vector | -136 | 8 |
| minimum_velocity | -152 | 4 |
| target_point | -172 | 12 |
| aim_vector | -192 | 12 |
| vertical_velocity | -204 | 4 |
| horizontal_velocity | -216 | 4 |
| target_relative_velocity | -236 | 12 |
| target_alignment_vector | -260 | 8 |
| alignment_vector | -284 | 8 |

Raw EBP offsets are authoritative; Ghidra's generated C names have an apparent four-byte naming bias in this function and are not used to infer offsets.

## The one bounded shape

The fresh raw dispatch at `0x430277..0x43028d` compares goals 2 and 3, falls through into the melee arm at `0x430293`, and sends the complementary case to the non-melee arm at `0x430593`. Current C instead spells the complementary non-melee arm first. The proposed natural source ordering swaps the two complete arms, complements `!=/&&` to `==/||`, and preserves every declaration and operation within each arm. No helper, arithmetic, scope, declaration order, ownership, or API change is mixed in.

January's optimized dispatch at `+0xa1..+0xb5` implements the same decision but places non-melee code first. It therefore permits this semantic topology without independently proving its lexical spelling. The /Od physical graph is primary support for the bounded experiment, not recovered January source text.

Files: `shape.py`, `shape.json`, `melee-arm-first.c`, `.patch`, `.obj`, `.gate.txt`, and `.alndiff.txt`.

| Artifact | Padded bytes | Relocations | Instructions | Normalized SHA-256 |
| --- | ---: | ---: | ---: | --- |
| Frozen current | 3216 | 84 | 983 | `2e9beea61ca0c704daa7d73090fae9e2de5cf9d675101967f695e68437c46841` |
| One branch-order shape | 3232 | 84 | 982 | `73dfb3c0e39a1dd684ddef6478aef06f30cdbc6ed9f72fc829ec5569f79ab748` |
| January | 3248 | 84 | 982 | `9ed9d44b1e9fd722f26ca68fb9b299855ce26abbcd9358d7b7c04abd842bb479` |

The shape gives new explanatory evidence: `check_range` moves to January's byte cell `[-2]`, `berserk_ranges` to `[-1]`, and `check_range` stays in BL across `game_time_get`. This is the allocation cause missing from the older local-permutation history. It does **not** close the function. The extra prop reload corresponding to January `+0x256` remains. Earlier stalking/range scheduling now also differs; the source-relevant mismatch includes omitted reloads corresponding to January `+0x164..+0x170` and changed range-bound store placement. The inherited helper-context/aggregate-copy difference also remains. Instruction count or improved one-block alignment provides no admission credit.

## Preservation audit

`audit.py` independently loads full COFF section information; `audit.json` carries the complete measurement and relocation records. All 20 inherited exact rows survive. The only changed existing function is `_action_charge_perform`; `_action_charge_setup` remains byte/relocation-identical to its frozen residual. All 23 emitted function owners and all 45 non-debug named owner records are unchanged. All 23 non-debug noncode sections, including relocation identities and ownership, are unchanged. COMMON is empty before and after. Exactly one `_point_from_line3d` owner remains, strict exact in both objects. No candidate-only helper is emitted.

Gate command:

```text
python tools/campaign/gate.py source/ai/action_charge --source scratch/astra-wave8/action-charge/melee-arm-first.c --all --out scratch/astra-wave8/action-charge/melee-arm-first.obj
```

The aligned textual diff is diagnostic only. Strict full normalized section bytes and relocation addresses/identities determine the rejection.

## Evidence not imported and reopening boundary

Fresh raw `0x430bd8..0x430bf7` and `0x430c1d..0x430c33` confirm the two separate direction aggregate-copy arms already present in current C and January (`+0x6c8..+0x6e9`, `+0x707..+0x722`). They do not license helper-linkage tuning or manual component expansion.

The later raw function also has two extra pre-ballistic `normalize2d` calls at `0x430d05` and `0x430d4a`, a dead-looking adjusted target point, and a different lead-fraction operation sequence: `ratio*0.5 + 0.5` at `0x430b14..0x430b33`. January explicitly uses divide, add 1, then multiply 0.5 at `+0x61d..+0x629`; the current C follows January. These are negative donor-compatibility evidence, not permission to import additional source or rearrange floating-point arithmetic. The spare RTC alignment/target-point records cannot justify fake lifetimes in January.

Reopen only with primary support for the remaining prop lifetime/reload boundary or genuine shared helper-owner/call context that can plausibly close the whole function while preserving the selected point owner and every exact control. Repeating local-order, spelling, operand, or inline sweeps is not justified. No second shape, fidelity-only landing, or exact gain is proposed.
