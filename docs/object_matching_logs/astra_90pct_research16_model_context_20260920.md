> Research-only packet16: zero production change and zero exact credit. Validated production remains wave13.

# Packet 16: bounded model source context

**Closed at the historical boundaries. No distinct new source premise, source shape, C compiler call, production change, or exact credit. Evidence released.**

Baseline is `29ff9e6f934c3fb4308e712268727a1d10b75b9f`. Scope is only the two requested written owners in `source/rasterizer/xbox/rasterizer_xbox_models.c`, totaling 8,268 target meaningful bytes. The September 9 reconciliation and September 14 wave C, W1 and W3 matching reports were read before current instruction inspection.

## Current January comparison

| Owner | Meaningful bytes | January padded / relocations | Current padded / relocations | January / current instructions | January / current direct calls |
| --- | ---: | --- | --- | --- | --- |
| `__rasterizer_model_draw` | 5,165 | 5,168 / 350 | 5,168 / 348 | 1,458 / 1,454 | 128 / 126 |
| `_rasterizer_model_draw_environment_shader` | 3,103 | 3,104 / 235 | 3,104 / 235 | 832 / 833 | 88 / 88 |

Current normalized hashes reproduce the previously documented source state exactly:

- Draw: `23ca80471861b904938ae52617a1931ea48279e746341e540d1874a7701dbd35`; January `99d8b95f1ce011781cedc3a123887065a0f8af4a54c842194fcd90c22fa2cc4e`.
- Environment shader: `5179768ee4ad9fc8a971e3c4ef621f2ab0b6f5d9866a4fe5af747b7f02e28725`; January `d4c4ee8c3b174dadb7364ae989fb815d8f7fc5cec2be5f2251abd438f7b36ace`.

The existing atlas similarities are 95.32371% and 95.87124%, respectively. These are inherited objdiff values; no new compiler or objdiff run occurred.

## Draw: known helper/type boundary

The only missing direct calls are January `_subtract_vectors3d` at `+0x5d1` and `_point_from_line3d` at `+0x604`. There are no extra current calls. These are the same self-illumination color interpolation calls documented in wave C and W1/W3, not a newly discovered missing operation. The current component-wise RGB expression was retained deliberately after the incompatible color/point/vector casts were removed.

The old separate-call and nested-call forms already measured the subtract helper staying inline, with an unwanted emitted helper owner and a remaining nonexact body. Replaying them would repeat the rejected ownership/type mechanism. The existing plasma accessor fold and seed/phase/color-slot differences remain additional gaps. A partial correction cannot supply complete admissible credit.

Reopening requires independent source/type/API evidence permitting both January helper calls with genuine ownership under the existing rules, plus evidence addressing the remaining instruction differences. A helper exception, TU-local facade, prohibited representation cast, or target rewrite is not a proposed next step.

## Environment shader: known consumed values, unresolved source context

All 88 direct-call identities occur in the same order, and the complete symbolic relocation multisets agree. The current unmodified alignment retains the documented differences:

- January loads the alpha-test byte directly into EDI at `+0x19f` and masks it; current zeroes EAX, loads AL and transfers the result to EDI at `+0x1ab`. The selected field and bit are the established ones.
- January's detail-scale load at `+0x3fc` reads `[esi+0xb4]`, copies EDX to ECX at `+0x402`, then stores the pair at `+0x451/+0x454`. Current reads the same field through the x87 at `+0x44f` and stores the two components at `+0x45c/+0x466`. This is precisely the old call-result-base CSE/copy hold.
- Color/brightness local grouping, fog multiplication/load order, later pixel register permutation and argument-register choices remain the previously described gaps. No new named field, immediate, output width, missing use or helper-call discrepancy was found in the inspected differences.

Prior wave C/W1/W3 already tested the relevant `set_real_vector4d` forms, named-real local, chained assignments, array copy, direct-field duplicate read, pointer alias/substructure view, scoped const and ALPHATESTENABLE cast forms. The genuine frame-statistics prototype-only test was inert for this function. No such spelling or declaration-context test is repeated here.

The precise missing evidence remains an authenticated source/context distinction that explains the detail-scale shared integer copy and the associated color/alpha lifetime choices. Current January bytes establish the consumed field; they do not uniquely establish the original C expression. No distinct discrepancy qualified a fresh `/Od`, DIA or Ghidra extraction, so none was run merely to reconfirm the old map.

## Limits and artifacts

Equal call sequences or relocation multisets do not prove every argument value equal. This is a bounded comparison with the documented holds, not a proof that no future admissible source exists. Later-build original lexical text and a shared January source revision remain unverified. The transparent-submit static split is outside scope and was not retried.

`current_audit.py` / `current.json` reproduce current fingerprints and call/relocation differences for the two owners. The two `*-split.asm` / `*-base.asm` pairs and `*.alndiff` files preserve unmodified instruction evidence. Alignment text includes `symbol` versus `defined-noncode` annotations and shifted relocation positions; these annotations are not new unresolved alias identities. Prior report paths and hashes are retained in the JSON.

Only `scratch/astra-wave16/model-context/` was written. No candidate exists, so no new sibling/owner/data/COMMON/point gate or whole-object admission is claimed.
