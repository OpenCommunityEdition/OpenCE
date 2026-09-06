# Object-matching audit helpers

These scripts preserve focused COFF comparison, donor-search, disassembly,
and residual-ranking utilities used during exact-object reconstruction. Run
them from the repository root unless a script accepts explicit paths.

The disassembly helpers require the Python `capstone` package. All COFF
parsing and exactness decisions come from the repository's tracked
`tools.coff_compare` implementation; these helpers do not alter admission or
matching status.

`error_geometry_runtime_differential.py` compares the real January and rebuilt
Error Geometry caller graphs in separate Unicorn instances. It requires the
Python `unicorn` package, the local `cachebeta.exe`, symbol manifest, and built
objects. Run without Python optimization (`-O` is rejected):

```text
python -B tools/audit/error_geometry_runtime_differential.py --output scratch/error-geometry-runtime.json
```

Use `--target`, `--candidate`, `--expected-target-sha256`, and
`--expected-candidate-sha256` to pin frozen inputs. The 24 bounded cases cover
file lifecycle, names, primitive/bounds calls and variadic comments. Genuine
external APIs are modeled: this does not execute host CRT file operations or
prove locale-rendered output, non-identity transforms, arbitrary inputs,
byte exactness, or whole-object/link ownership. The report records these limits
and input/tool hashes; only the requested JSON is written. Unsupported format
conversions and attempts to overwrite verification inputs fail closed.

Files with dates in their names are retained research snapshots. Their
hard-coded inputs intentionally document the evidence wave they were written
for, while all imports and repository paths are portable.

For a source-level credibility pass, run `python tools/fake_match_scan.py`.
It reports deterministic review leads for inert, contradictory, or explicit
codegen-steering constructs without treating a heuristic hit as proof. See
`docs/fake_match_review.md` for scope, vendored exclusions, and exit codes.
