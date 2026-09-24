# `models.obj`: authentic default-state BSS ownership

The four file-private render-model defaults have the correct types and names, but their tentative definitions previously produced different `.bss` offsets from January despite all 12 functions being exact. Declaring each with an explicit zero initializer is a normal source spelling for an intentional fallback default; VC7 still emits a 136-byte `.bss` section, but now places the records in January's order:

| Default | January offset | Rebuilt offset | Size |
| --- | ---: | ---: | ---: |
| `default_function_values` | 0 | 0 | 16 |
| `default_render_model_change_colors` | 16 | 16 | 48 |
| `default_render_model_effect` | 64 | 64 | 40 |
| `default_render_model_region_permutation_indices` | 104 | 104 | 32 |

Both sections retain flags `0xc0300080`. No aggregate, padding, renamed steering symbol, custom section, or compiler directive is involved. An ablation leaving only the first two explicit zero initializers reverted to the old order, so the result is genuinely tied to source initialization rather than declaration order. The later first-party HCEX PDB confirms `default_function_values` as a private `float[4]`; January's own COFF establishes all four offsets and private storage. This closes the previously recorded BSS-owner hold rather than claiming new code or data bytes.

The full build and one-TU strict gate pass; `models.obj` remains 12/12 exact, and objdiff shows all four sections at 100%. The board-wide stable sweep against the pre-edit snapshot reports zero gained and zero regressed functions; this is an ownership closure, not new code credit. Parks remain valid (zero stale or invalid), the admission audit has zero contradictions, the changed-file fake scan has zero leads, and pytest passes (1,157 passed, 5 skipped, 26 subtests). The object status is now Matching and only its BSS-layout admission hold was removed.
