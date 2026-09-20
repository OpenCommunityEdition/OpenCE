# Independent review: authenticated bitmap extraction

**Admissible: `_extract_sequence` gains 720 meaningful / 720 padded bytes.
Bitmap Extract is eligible for whole-object Matching under the existing
admission standard.** The candidate is ordinary defined C with independently
authenticated branch statements, assertions and working bounds. No policy,
header, target, compiler-option or guard change is required.

## Independent frozen baseline

The reviewer compiled the exact source from
`git show 905e5e3b41591af98c82accf3f57fc38118c2e7f:source/bitmaps/bitmap_extract.c`
into a separate scratch object, applied only
`scratch/astra-wave2/extract-authenticated.json` to that frozen text, and
compiled the candidate independently with `gate.py --source --all` plus the
point helper guard. No already-integrated build/base object served as baseline.

| Check | Result |
|---|---|
| Strict target functions | 20/21 before; 21/21 after |
| New exact function | `_extract_sequence`, 720 bytes / 28 relocations |
| Changed code owners | `_extract_sequence` only; all 20 inherited exact siblings unchanged |
| Normalized SHA-256 | `5f106d5c4f2bb3024df015b0b10b2236b3ffc3818cff7588e4fd3a7c7a810e56` |
| Candidate named owners | 112 before and after |
| Target named owners | All 106 present with identical offset/type/storage/flags/size/COMDAT selection |
| Target runtime data | All 85 sections strictly equal, including 44-byte `_extract_data` BSS |
| COMMON / new behavioral owners / forbidden point emission | Zero / zero / zero |
| Fake scan | One complete candidate source; zero review leads |

The target has 13,002 meaningful / 13,168 padded code bytes across 21
functions, with 4,496 canonical report data bytes (.rdata 4,452 plus BSS 44).
Whole-object status adds no second claim for the twenty inherited functions
or the newly accepted 720 bytes. Raw target data payloads total 4,339 bytes;
the canonical report grouping, not a new per-section padding calculation,
determines data credit.

## Correctly changed data and complete ownership

The only changed runtime literals are the three assertion strings. The old
spaced `top >= 0`, `bottom > top`, and `bottom <= extract_data.plate->height`
owners are replaced by the exact January literals `top>=0`, `bottom>=top`,
and `bottom<=extract_data.plate->height`. Thus runtime noncode is intentionally
not byte-identical to the erroneous baseline. Every target data owner now
matches January, including those three corrected strings. The private BSS and
all other runtime content remain unchanged.

The six candidate-only owners are inherited four-byte `SELECT_ANY` float
literals, not behavioral functions. Each symbol is undefined in this January
split object and has exactly one strictly equal selected provider in the
January corpus. Their content and metadata are unchanged from the frozen
baseline; no duplicate named owner exists in the candidate.

| Pooled literal symbol | Selected January provider |
|---|---|
| `__real@00000000` | source/ai/action_charge |
| `__real@3f000000` | source/ai/action_alert |
| `__real@3f800000` | source/ai/action_charge |
| `__real@42c80000` | source/ai/action_uncover |
| `__real@437f0000` | source/bitmaps/libtiff/tif_getimage |
| `__real@bf800000` | source/ai/actor_moving |

These satisfy the established pooled-literal rule in
[the ten-object admission standard](ten_object_admission_batch_20260919.md).
There is no new exemption and no extra literal credit. Bitmap Extract has no
entry in the existing object-admission rejection policy.

## Primary machine and source review

The reviewer independently reopened the raw `tool_symbols.exe` and verified
all **323 saved instruction byte sequences**, from `0xB96A10` through the
terminal return at `0xB96EA8`, against the PE. Its SHA-256 is
`079d20d7792e9565b24ab2cff8115f76a6e48749a8e8ffd190f57954495f0a97`.
Fresh decoding covers the assertion, rectangle initialization, state-ending
tests, working copy and first trim loop. Bytes after the return are RTC data
and padding, not extra code.

The raw source-path string is `d:\p4\halo1\source\bitmaps\bitmap_extract.c`.
The exact three assertion strings above are present at `0x10D7EEC`,
`0x10D7EF4`, and `0x10D7F04`. The compare/jge at `0xB96A74/0xB96A76`
authenticates `bottom>=top`, and x initializes after all assertions.
The x0/y0/x1/y1 store order is explicit at `0xB96B08..0xB96B26`.

The bottom-reference and bitmap booleans are independently initialized for
each column. The raw code first tests the bottom-reference/single-sequence
condition at `0xB96C73..0xB96C9C`, then independently tests state1/no-bitmap at
`0xB96CA0..0xB96CB6`. These are two real source branches. If the first changes
state to2, the second state1 test is false; a single execution does not perform
both state2 stores. Their logical redundancy does not make the reconstructed
first branch synthetic: it has direct independent machine provenance.

The state0 case sets state1 and x0 before falling into the state1 bounds
updates. Color reads, scan limits, state progression and extraction failure
propagation remain coherent. The first/second ending tests together preserve
the same final column state as the inherited state1/no-bitmap test. The
assertion correction intentionally permits an empty top==bottom range, which
advances columns without extracting a bitmap and returns success.

Raw RTC parsing at `0xB96EAC` independently recovers `bounds`, 8 bytes at
EBP-28, and `adjusted_bounds`, 8 bytes at EBP-116. The copy at
`0xB96CE1..0xB96CEA` initializes the complete working rectangle before trim
flag testing. Both trim scans read and update that rectangle; the final call
at `0xB96E67` receives its address. This is a real used aggregate with authentic
name and extent, not declaration filler. Every local read has an initializer
or defining state transition, and no new undefined read, array extent, forced
inline, helper facade, volatile dependency or raw-byte path is introduced.

This is a 2020 cross-build witness. Its source revision relative to January
is unverified, and no original full source file, scalar declaration order or
lexical Block record is claimed. The complete corresponding statements and
fresh strict January code/relocation result jointly support this narrow
reconstruction. The old donor is merely a patch carrier after authentication.

Evidence: `scratch/astra-alias-wave2-extract-review.py/.json` with frozen and
candidate `.c/.obj/.gate.txt`; `scratch/astra-alias-wave2-extract-ownership.py/.json`;
`scratch/astra-alias-wave2-extract-primary.py/.json`; and
`scratch/astra-alias-wave2-extract-fake.json`. The source packet is
[the wave-two bitmap evidence](astra_90pct_wave2_bitmap_evidence_20260920.md).
This reviewer edited no production file, ran no Ninja, made no commit, and
changed no admission status.
