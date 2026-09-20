> Research only: no admission or retained source/header/configuration change.

# Independent review: environment lightmap source reconstruction

**Final recommendation: HOLD the complete replacement.** The factor and named
weights are defensible, but an authorized minimization ablation proves that
the separately unverified branch-local filter copies are needed for exactness.
Under the same topology-evidence standard used for the held triangle packet,
there is not yet enough evidence to admit this complete body. This supersedes
the initial tentative approval based on ordinary per-branch setup being a
reasonable reconstruction. Exactness is not lexical source proof.

Candidate reviewed: `../environment/intensity.c`, SHA256
`fae2a8428afc3ea433f95386b170276d4bc4c4d1d7f80fc4049ac6d0fa77eee4`.
Baseline SHA256:
`55e5f4c4877616075a862f5830d86ac2cddbb2b41974bd9db521ee52712a196c`.
Only `_rasterizer_environment_lightmap_draw` source changes; prefix and suffix
outside this function are equal. The supplied candidate was replayed unchanged;
the separately authorized filter-tail ablation is recorded below.

## Independent verification

Fresh PID-isolated gates from copied sources reproduce 42/44 exact at baseline
and **43/44 exact** for the candidate. The added target function
`__rasterizer_environment_lightmap_draw` is strict exact at **4008 meaningful /
4016 padded bytes**, 248 relocations, normalized SHA256
`7818e559325c1524c20fc78b0857de1e2fb0c63012550d71a7f272029569807e`.

- All 42 inherited exact target functions remain exact.
- Only lightmap draw changes among 51 emitted function sections; the other
  50 sections, including the parked specular spot-light residual, are equal.
- All 88 external/file-static owner records and all 38 non-debug noncode
  sections are equal. COMMON is empty before and after.
- No code owner is added or removed. `_point_from_line3d` remains absent;
  both gates pass the forbidden-emission guard.
- The independent candidate reproduces every emitted function section in the
  worker's candidate object. Fake-match scan: zero review leads.
- Raw alignment displays six differences solely because an inherited
  literal/assert relocation is represented as an undefined target symbol in
  the split and defined noncode in the compile. The existing strict comparator
  proves every byte and semantic relocation identity; it was not modified.

Full inventories, all 248 relocation records, source hashes and assertions are
in `review.json`; reproduction code is `review.py`. `baseline-gate.txt` and
`candidate-gate.txt` contain the independent gate output. No production,
header, configuration, Ninja or commit operation was performed by this review.

## Primary factor evidence and the four temporary variables

I independently reread the authenticated PE, verified **160 instruction byte
records** across the periodic evaluations, factor initialization, six uses,
and two helper bodies, and independently decoded its six RTC aggregate
records. The PE hash agrees with the worker packet:
`740869688354defd295e28adf94bd4ea41385e9d4a98dc08764515285cf05b55`.

At 0x7e01ab..0x7e01b7, the later first-party function copies the float 1.0 from
0x93dd5c into its local EBP-0x78. Its subsequent actual instructions use this
same local in:

1. `(1-primary_value)*factor`, passed to scale_vector3d;
2. `(1-secondary_value)*factor`, passed to scale_vector3d;
3. `factor*primary_value`, passed to point_from_line3d;
4. `factor*secondary_value`, passed to point_from_line3d;
5. the plasma-on scale call;
6. the plasma-off scale call.

The two helper thunks resolve to ordinary vector scaling and point-plus-scaled
vector bodies. This disproves the old negative inference that an intensity
factor was absent merely because the optimized/reconstructed HCEA donor did
not show it. It satisfies the w3c/n3a reopen criterion requiring donor evidence
of a genuine intensity/brightness term. The factor is not justified solely by
the archived exact object's existence.

**Important limit:** the raw later function computes those four products at
their individual helper calls. It does not prove four explicit named scalar
locals, nor that the products were precomputed together in January. HCEA's
tiny PPC stubs provide no scalar declarations. `intensity`, the four scale
names, and their declaration placement remain inferred source.

I nevertheless judge this particular representation admissible. Each scalar
holds one of four independently demonstrated arithmetic weights; all are
assigned before use and each directly supplies its corresponding genuine
helper argument. The source neither invents an operation nor adds an extra
read, discarded store, alias, address-taken dummy, widened type, qualifier,
cast chain, or forced inline directive. The four values have an ordinary
rendering meaning: primary/secondary off/on weights. Naming them explicitly
also retains the float argument boundaries seen in the primary call sequence.
Their precomputation is over initialized scalar locals and a constant; the
intervening pure vector helpers only write the separate output colors.
There is no fabricated dependency on helper behavior.

The factor equals one, so these multiplications simplify mathematically.
That alone does not make them invented inert matching arithmetic: the exact
factor and its six uses are now explicit first-party instructions. The new
evidence addresses the reason the old g2 body was rejected. Conversely, this
does not authorize adding identity products or named temporaries elsewhere
without comparable evidence.

January independently shows the same six helper operations and staging:
off scales at +0xb63/+0xb83, x87-staged on weights at +0xb88/+0xba6 followed by
point calls +0xba1/+0xbbf, and the two constant-1 plasma scale calls
+0xbd7/+0xbef. The complete candidate reproduces that sequence. Exactness
supports this reconstructed spelling; it does not establish its historical
variable names.

## Other patch hunks

| Change | Independent assessment |
| --- | --- |
| Stage-0 MAG/MIN/MIP calls moved into both texture-selection arms | Each runtime path still sets the same three filters once, after its addressing setup and before stage 1. January +0x386..0x406 contains the two texture/address arms and a merged filter tail. Full per-arm texture setup is ordinary source; the January merged tail is compatible with it. The duplicate lexical blocks themselves are inferred, not directly recovered from `/Od`. No new branch or extra runtime call is introduced. |
| `PS_TEXTUREMODES` in both illumination paths | Verified stock XDK `D3D8Types.h:2100` macro and enum values (NONE=0, PROJECT2D=1, CUBEMAP=3). Expansion exactly yields the old two pairs 0x18001/0x18401 and 0x18021/0x18421. January's `sete`, shift 10 and OR encode this genuine texture-stage expression. No local macro or header override is introduced. |
| Self-illumination locals moved into their using `else` | All color/scalar reads and writes are confined to that branch; the no-map branch has no use. RTC independently confirms the four 12-byte color aggregates. Narrow scope is natural, but the original braces and declaration order are not claimed as recovered. |
| Pixel constant conversions after fixed combiner stores | January +0xc1d..0xd27 writes the fixed fields before the conversion calls +0xd31/+0xd3f/+0xd4d/+0xd61/+0xd72. Source preserves the same values, destinations and call order. It restores independently visible store/call ordering rather than inventing stores. |
| Specular-mask switch case moved after the bump-edge cases | Case values, breaks and bodies are unchanged. January body/table order +0xe6a/+0xe92/+0xec4 and table +0xf84 corroborate this layout. No enum or condition is invented. |

Existing helper calls, color/vector casts, shader access and owner declarations
are inherited. This patch neither manually expands helper arithmetic nor
creates a replacement owner. The helper emission/ownership guards pass.

## Remaining limits and admission boundary

The root requested one minimization ablation restoring only the baseline
common MAG/MIN/MIP tail. `common-filters.c/.obj`, `common-filters-gate.txt`, and
`common-filters-ablation.aligned.txt` preserve it. It still has 4016 bytes and
248 identical semantic relocations, but its hash is
`68612e852c70519711820448db7e8aa038807bdc0ef5a30d2fee2e5b18fe7d04` and it gates
42 exact / 2 residual. All siblings, owners, noncode, COMMON and point guards
remain unchanged. The only machine difference is argument-setup scheduling for
the three filter calls at +0x3e1..0x406. Thus unsupported duplication cannot be
omitted while retaining full exactness in this candidate.

January has one already-merged filter tail. The available HCEA entry points
are 4-/16-byte stubs; the later DX9 implementation establishing the intensity
factor does not contain this vector-test texture-selection path. None provides
an independent per-branch witness for the three identical filter writes. The
duplication is semantically sound and source-plausible, but its necessity for
exactness is presently demonstrated by compiler output rather than recovered
source/control topology. That is the specific admission concern, and it is
separate from the now-authenticated factor. Reopen with a real per-branch
source/debug witness or an independently validated source-context mechanism
that closes the same tail without unsupported duplication. Do not use further
filter spelling/declaration variants as a substitute.

The primary executable is a 2020/v140 DX9 instrumented build, with a different
surrounding renderer from January Xbox. Same-source-revision identity remains
unverified. Its narrow arithmetic evidence is reinforced by January's full
code and relocations; do not call the later decompile original January source.
The other residual `_rasterizer_environment_specular_spot_light_begin` remains
parked and unchanged; this is not a whole-object exact admission.

The strict candidate's potential remains +4008 meaningful / +4016 padded / one
function; **recommended credit now is zero**. The factor evidence is a useful
reopening fact and should be retained, but the whole body remains research-only
until the root resolves the branch-topology concern with the stated evidence
standard. No production source was altered by either the review or ablation.

`IDA_AUTOMATION_UNAVAILABLE`. No extra artifact was requested. The only extra
source-review test was the root-authorized common-filter-tail ablation.
