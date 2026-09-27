# Cinematic render and lens-flare fallback intake — 2026-09-26

**No newly supported source hypothesis, no production change, zero strict
gain.** Both current-context whole-TU gates and both later first-party
disassemblies were freshly generated. Known source packets and failed
controls were inspected rather than rerun as allegedly new experiments.

Base: `fdf76bd0caf859aa46e5a6ca41b33164635350d0`.

| Target | Fresh TU | Padded / relocations | Fresh target-function candidate hash |
| --- | --- | --- | --- |
| `_cinematic_render` | 16 exact / 1 residual | 1280 / 57 | `1a9d49fc4eace03cb02268a6434f252b45496477fac007eba679c72e4b5c8d1c` |
| `_rasterizer_lens_flares_draw` | 11 exact / 2 residual | 2240 / 115 | `25834daf80e88b4175ab92e01e17a5cc917acdeb58f57fc1b9856c22e317fb03` |

Both frozen-flag diagnostic compiles give zero warnings; complete function
sections equal the reviewed gate output. Source snapshots are unchanged.
No new code/data/COMMON definitions. Rasterizer lights' other remaining row
is the 48-byte reset function; its owner-gated overrun was not instantiated.
The already exact 400-byte submit_for_cluster row was not edited.

## Cinematic render

Fresh disassembly is `later_od_cinematic_render.txt`, function
`0x559f00..0x55a715` (primary supplied `/Od` image analyzed as data).
The title pack at `0x55a500..0x55a640` has repeated PIN evaluations passed
inline, no named shadow-alpha local, and an unsigned alpha conversion.
The current named local and cast chain therefore are not the later source
spelling. That fact is **already** the prior source-fidelity packet:

- Lane D `cutscene__cinematics/REPORT.md` s1–s3 strips the casts/local; all
  bytes outside the same four-instruction residual remain identical.
- `research/fifty_objects_20260925/w/cinematics/LEDGER.md` p1–p7 supplies the
  nested checks, RTC `text_color`, combined initialized local scopes,
  direct title-time conversions and direct global letterbox assignments.
  p6 couples the known factors, p7 adds the attested nested scopes; neither
  closes the residual. Authentic `realcmp` is also already tested inert.
- The complete optional source-fidelity patch is preserved by that packet;
  no old whole-file donor was copied over current canonical.

Fresh January/current comparison remains 389 instructions each. The only
meaningful difference is:

```text
January +0x45d: shl eax,24; and ecx,0xffffff; or eax,ecx; push eax
Current +0x45d: and ecx,0xffffff; shl eax,24; or ecx,eax; push ecx
```

`research/compiler_application_20260925/workers/E1/CARDS.md` T3–T3e contains
stock-equal compiler traces of the OR operand key. In the measured p1/p7
family, the remaining switch is a C2-created base-id mod-4 term, not a newly
identified source operation. That is a diagnostic model under its traced
conditions, not original-source proof and not a universal impossibility
claim. No independent source fact supplying a different upstream expression
or call was found here. Manufacturing one extra compiler temporary or
changing clamp counts would be prohibited steering, not reconstruction.

Thus this intake did not repeat p1–p7, scalar/prototype counts, operand
permutations or an arbitrary helper/wrapper insertion. The park permits
investigation but grants no reason to bypass the missing source evidence.

## Lens-flare draw fallback

Fresh `later_od_lens_flares_draw.txt` is function `0x82fa10..0x830abe`.
Unlike flag_update, this function **does** have the genuine point helper:
at `0x8308a5..0x8308c5` it passes `&point`, `reflection->offset`, the corona
axis and corona position to `point_from_line3d` (`0x42e0d0`). It also stores
the texture helper result as a byte at `0x8308e9`. The light brightness
initializer is visible at `0x82fb92`; the fade expression has the already
recorded conditional-expression temporary at `0x82fcad..0x82fd8b`.

These are all **known**, fully tested factors, not newly missed calls:
`research/fifty_objects_20260925/w/rasterizer_lights/LEDGER.md` d1/e1/e2/q1
and its wave-4 result packet measured the real helper, initializer and
ternary, upper-bound scalar locals, texture-result byte and second-loop
counter. The minimal helper changes dot-product schedules; the broader
packet remains nonexact at direction-dot `+0x31c` and texture setup
`+0x738..+0x74e`. The packet discloses that any four of five attested locals
recover its fuzzy hash through a count effect; this is not a new exact
route or authority to choose source by count.

Fresh canonical comparison has 669 instructions on both sides and retains
the same known three regions: brightness product `+0x202`, direction-dot
i-term `+0x31c`, and texture-argument interleaving `+0x738..+0x74e`.
The texture sequence consumes the same short field through the same short
API; an upper-half zeroing register instruction is not, by itself, proof
that the field or API should be changed to unsigned. `/Od` also zero-extends
that short argument, consistent with the existing ABI.

The previous packet directly checked scopes, declaration placement,
genuine helper combinations, argument order, scalar lifetimes, reciprocal
forms and the unused-aggregate/count controls. No newly attested
direction definition/escape, camera-offset definition or distinct argument
evaluation was found beyond those records. Accordingly no new source
candidate was generated just to revisit those controls.

## Boundary and artifacts

This is a concrete negative evidence intake, not a claim that either
function is permanently frozen. Reopen with an independently supported
January-applicable expression/helper/type/lifetime fact, not a desired
compiler ID or added reference. Existing per-site helper ownership and
original-bug holds remain unchanged.

`audit.py` invokes reviewed `gate.py --source --out` and a same-flags
diagnostic compile, compares all function sections and emits bound hashes,
all function rows, symbol deltas and raw diagnostics in `results.json`.
For the fallback use:

```text
python -B scratch/astra_five_functions_20260926/cinematics/audit.py source/rasterizer/rasterizer_lights _rasterizer_lens_flares_draw lens_flares
```

The default invocation measures cinematics. Per-unit receipts include
`baseline.gate.txt`, `baseline.compile.txt`, and the selected function's
aligned diff. Lens receipts are in `lens_flares/`. Fresh first-party
disassemblies are beside this report. All writes are scratch artifacts;
no headers, production, config, git state, ninja/configure, held construct
or comparator normalization changed. New source-shape probes: **0**.
