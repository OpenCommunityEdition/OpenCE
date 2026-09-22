# Independent adversarial review — `_ai_scripting_migrate_and_speak` (commit `9eca0524`)

Reviewer: independent, did not author the change. Worktree
`C:/halo-worktrees/claude-lane-a-ai-core-20260920`, branch
`claude/lane-a-ai-core-20260920`, HEAD `3e44bc78` (working tree clean;
`9eca0524` is an ancestor). No file under `source/`, `config/` or any header was
touched; no `ninja`, no `configure.py`, no mutating git command. Everything I
created lives under `scratch/review-ai-script/`.

---

## VERDICT

**CONFIRMED WITH CORRECTIONS.** The match is real and strict, the arity defect
is target-proven from January's own bytes and `.rdata`, and no lawful
alternative gates exact — but one of the five claims in the BUG comment ("so no
shipping configuration runs it") is an unproven overstatement, and the wave
report contains a dangling policy citation plus a distinction the cited owner
text does not make.

---

## 1. Is the match real and strict?

```
$ python tools/campaign/gate.py source/ai/ai_script --all
EXACT       224  _ai_scripting_migrate_and_speak
== exact 116  residual 0  unwritten 0  (of 116 listed)
```

No sibling regressed: **exact 116 / residual 0 / unwritten 0**, as required. The
same census holds with the ownership guard:

```
$ python tools/campaign/gate.py source/ai/ai_script --all --forbid-emitted-symbol _point_from_line3d
== exact 116  residual 0  unwritten 0  (of 116 listed)      EXIT=0
```

```
$ python tools/campaign/relocdiff.py source/ai/ai_script _ai_scripting_migrate_and_speak
target=...\build\split\source\ai\ai_script.obj
ours=...\build\base\source\ai\ai_script.obj
shape: size 224/224, relocations 16/16, sha equal
summary: 16 target, 16 candidate, 0 differing row(s)
```

All 16 rows print `=`. Three rows differ only in how the tool spells the same
target (`['symbol', …]` vs `['defined-noncode', '.rdata', …] symbolic=['symbol', …]`),
which `relocdiff` resolves symbolically and scores equal.

`tinfo` on both trees, compared by me field-by-field:

| | `build/split` (January) | `build/base` (ours) |
|---|---|---|
| `size` | 224 | 224 |
| `relocation_count` | 16 | 16 |
| `normalized_sha256` | `6fb5a427f629d9b8470851e22e67cee095122693df8eebef6e108316afc68521` | `6fb5a427f629d9b8470851e22e67cee095122693df8eebef6e108316afc68521` |

(The two objects sit in different section numbers — 51 vs 125 — which is
expected and is not part of the identity.)

I also re-ran the recorded regression diff, read-only:

```
$ python -m tools.campaign.stable_verdicts diff scratch/lane-a-before.json scratch/lane-a-waveA1.json
gained source/ai/ai_script::section:51 _ai_scripting_migrate_and_speak -> _ai_scripting_migrate_and_speak 224
gained 1 224 bytes; regressions 0     EXIT=0
```

**Freshness control.** `gate.py --all` reports the already-built `build/base`
object, so on its own it does not prove the committed source produces it. I
copied the committed `source/ai/ai_script.c` byte-for-byte into
`scratch/review-ai-script/ai_script_v0_landed.c` (`diff` against the original is
empty) and recompiled it through `gate.py --source`. It reproduces
`EXACT 224 / exact 116 residual 0 unwritten 0`. The build is not stale.

I could **not** independently verify the tree-wide totals in the commit message
(`1,444,193 / 1,770,166`, `7,292 / 7,574`) because that requires `ninja`, which
the brief forbids. I did verify the structural reason they cannot have moved
elsewhere: `git show --stat 9eca0524` touches exactly two files,
`source/ai/ai_script.c` and one doc. **No header, no config.** Every other
object compiles from unchanged inputs. The two cited audits do reproduce:
`tools.parked_functions` → `active 184, stale 0, invalid 0`;
`tools/audit_object_admission.py` → `candidate 19, contradicted 0, rejected 8,
revoked 0`.

---

## 2. Is the BUG target-proven?

### January's bytes, disassembled by me

```
$ python scratch/orch/dumpfn.py build/split/source/ai/ai_script.obj _ai_scripting_migrate_and_speak 0x0 0xe0
     0  push     ebp
     1  mov      ebp, esp
     3  sub      esp, 0x404
     9  mov      al, byte ptr [0x97]        ; -> _ai_debug
     e  test     al, al
    10  push     ebx
    11  mov      ebx, dword ptr [ebp + 8]
    14  push     esi
    15  push     edi
    16  mov      edi, dword ptr [ebp + 0xc]
    19  jne      0x24
    1b  mov      al, byte ptr [0x99]        ; -> _ai_debug
    20  test     al, al
    22  je       0x7d
    ...
    5a  lea      edx, [ebp - 0x204]
    60  push     edx
    61  lea      eax, [ebp - 0x404]
    67  push     eax
    68  call     ...                        ; -> _hs_runtime_get_executing_thread_name
    6d  push     eax
    6e  push     0                          ; -> ??_C@_0CC@JKLCPGKI@…
    73  push     2
    75  call     ...                        ; -> _error
    7a  add      esp, 0x14
    7d  mov      esi, dword ptr [ebp + 0x10]
```

`build/base` disassembles **identically** over the whole `0x0..0xe0` range.

### The format literal, read from the raw `.rdata` bytes

I wrote my own COFF walker (`scratch/review-ai-script/rdata_dump.py`) rather
than trusting the demangled name:

```
symbol : ??_C@_0CC@JKLCPGKI@?$CFs?3?5ai_migrate_and_speak?5?$CFs?5?$CFs?5?$CF@
section: #161 .rdata  storage_class=2  value=0x0  raw_len=34
bytes  : 25 73 3a 20 61 69 5f 6d 69 67 72 61 74 65 5f 61 6e 64 5f 73 70 65 61 6b 20 25 73 20 25 73 20 25 73 00
ascii  : b'%s: ai_migrate_and_speak %s %s %s'
strlen : 33 (with NUL: 34)
%-signs: 4   %s count: 4   other conversions: []
```

Four `%s`, no other conversion, no `%%`. The relocation at `+0x6f` in January's
function resolves to exactly this symbol.

### Plain statement of the three facts

- **January cleans `0x14` = 20 bytes = five dwords** at `+0x7a`.
- Five dwords = `_error_silent` (the `push 2` at `+0x73`) + the format pointer +
  **three** values. The format requires **four** values. One conversion has no
  argument.
- **`speech_type` provably is not one of the pushed arguments.** It lives at
  `[ebp+0x10]`. Across the entire function, `[ebp+0x10]` is read exactly once,
  at `+0x7d` (`mov esi, dword ptr [ebp+0x10]`) — *after* the `call _error` at
  `+0x75`. The only other parameter loads are `[ebp+8]`→ebx at `+0x11` and
  `[ebp+0xc]`→edi at `+0x16`. Nothing between `+0x24` and `+0x7a` touches
  `[ebp+0x10]`.

### Two extra checks the author did not report

- **The literal has exactly one reference in the whole object.** I walked every
  relocation in `build/split/source/ai/ai_script.obj`
  (`scratch/review-ai-script/xref_literal.py`): `total relocations referencing
  'JKLCPGKI': 1`, at `section #51 owner=_ai_scripting_migrate_and_speak
  offset=0x006f`. So "the four-`%s` literal was authored for some other,
  correct, four-value call site" is refuted.
- **The 2020 cross-build corroboration is real**, and I verified it myself
  rather than taking the wave report's word. Reading
  `C:/Users/isabe/Downloads/haloleak2024/halo1/h1_tags/halo_cache_symbols.exe`
  (read-only; never executed) with `scratch/review-ai-script/crossbuild_check.py`:

  ```
  literal  file 0x00547264  VA 0x00948864  len=33  pct_s=4  '%s: ai_migrate_and_speak %s %s %s'
  xref at VA 0x004b45bd:
      004b45bc  push    eax
   >> 004b45bd  push    0x948864
      004b45c2  push    2
      004b45c4  call    0x408607
      004b45c9  add     esp, 0x14
      004b45cc  mov     esi, esp
      004b45ce  push    0x944b4c
      004b45d3  mov     edx, dword ptr [ebp + 0x10]
  ```

  Same four-`%s` literal, same `add esp, 0x14`, `speech_type` (`[ebp+0x10]`)
  still loaded only after. Corroboration only, as the intake document requires —
  but the wave report's description of it is accurate.

**Conclusion: the bug is target-proven.**

---

## 3. Is the BUG comment accurate? — claim by claim

The comment sits at `source/ai/ai_script.c:2763-2773`.

| # | Claim | Verdict |
|---|---|---|
| a | format has four `%s`, only three values supplied | **TRUE** — 34 raw `.rdata` bytes, `%s` count 4, other conversions none; five dwords cleaned |
| b | January cleans `0x14` bytes (five arguments) here | **TRUE** — `7a add esp, 0x14` |
| c | `speech_type` is loaded only afterwards, at `+0x7d` | **TRUE** — `7d mov esi, dword ptr [ebp+0x10]`, sole read of `[ebp+0x10]` |
| d | the fourth slot lands on the saved-EDI home, so `vsprintf` dereferences the caller's entry EDI as a `char *` | **TRUE** — arithmetic re-derived below |
| e | reachable only when `ai_debug.print_migration` or `print_scripting` is set | **TRUE**, but the sentence it is attached to ("so no shipping configuration runs it") is **OVERSTATED** — see below |

### Claim (d), re-derived from the prologue myself

From `push ebp / mov ebp,esp / sub esp,0x404 / push ebx / push esi / push edi`:

| address | holds |
|---|---|
| `ebp-0x404` | `source_name[512]` (`lea eax,[ebp-0x404]`) |
| `ebp-0x204` | `target_name[512]` (`lea edx,[ebp-0x204]`) |
| `ebp-0x4` | `advancing_stimulus` |
| `ebp-0x408` | saved EBX |
| `ebp-0x40c` | saved ESI |
| **`ebp-0x410`** | **saved EDI** (pushed at `+0x15`, *before* `mov edi,[ebp+0xc]` at `+0x16`, so it is the caller's EDI) |
| `ebp-0x414` | pushed `target_name` |
| `ebp-0x418` | pushed `source_name` |
| `ebp-0x41c` | pushed thread name — first vararg |
| `ebp-0x420` | pushed format |
| `ebp-0x424` | pushed `2` (`_error_silent`), = `esp` at the `call` |

`error()` does `va_start(argument_list, format); vsprintf(string, format,
argument_list);` (`source/cseries/errors.c:245`), so the va_list starts at
`ebp-0x41c` and walks up. Conversions 1/2/3 read `ebp-0x41c` / `ebp-0x418` /
`ebp-0x414`; conversion 4 reads **`ebp-0x410`, the saved-EDI home**. The wave
report's shorthand `[esp+0x14] = ebp-0x410` is also correct
(`-0x424 + 0x14 = -0x410`). `_error_silent` is `2` in the
`source/cseries/errors.h` enum (`_error_immediate=0, _error_delayed, _error_silent`),
matching `push 2`. **The arithmetic is right.**

### Claim (e) — the one real error

The gate itself is verified twice over: the bytes test `ai_debug+0x97` and
`ai_debug+0x99`, and `source/hs/hs_globals_external.c:1468,1470` independently
pins those offsets:

```
{ "ai_print_migration",  _hs_type_boolean, 0, ai_debug + 0x97 },
{ "ai_print_scripting",  _hs_type_boolean, 0, ai_debug + 0x99 },
```

But that same evidence **undercuts** the comment's conclusion. Both flags are
entries in `hs_external_global_definitions[442]` — HaloScript external globals,
settable at runtime by a scenario script or the debug console. They are not
compile-time-excluded debug code. "So no shipping configuration runs it" is an
inference about builds outside the campaign's evidence base, and it is
contradicted by the fact that the switch is script-reachable in the very build
being matched.

**Corrected wording for that sentence:**

> ` * The branch runs only when ai_debug.print_migration or`
> ` * ai_debug.print_scripting is set. Both default to false and are exposed as`
> ` * the script globals ai_print_migration / ai_print_scripting, so ordinary`
> ` * gameplay does not reach it.`

### One presentational note on claim (a)'s evidence sentence

The comment introduces the cleanup as `Evidence:` for the four-vs-three claim.
Strictly, the cleanup proves *three values*; the *four conversions* come from
the `.rdata` literal. My variant **v4** makes this concrete: a lawful
three-conversion format compiles to byte-identical code —
`75 call _error / 7a add esp, 0x14 / 7d mov esi,[ebp+0x10]` — and fails only on
the literal's identity (`[reloc-identity]`). So the cleanup evidence alone does
**not** establish the defect; the 34-byte literal is load-bearing. The comment
is not wrong (the four-`%s` format is on the adjacent source line, and the
relocation proves it is January's), but it would be stronger as:

> ` * Evidence: January's .rdata literal is 34 bytes, "%s: ai_migrate_and_speak`
> ` * %s %s %s" (four %s), while January cleans 0x14 bytes (five dwords) at this`
> ` * call site — severity, format and only three values — and loads speech_type`
> ` * only afterwards, at +0x7d.`

---

## 4. Is there a lawful alternative that also gates exact?

I compiled **nine** variants of my own (generator:
`scratch/review-ai-script/make_variants.py`; each is a full copy of
`ai_script.c` with only the `error()` call rewritten), through
`gate.py --source … --all`:

| variant | shape | gate row | unit census |
|---|---|---|---|
| `v0_landed` | committed source, recompiled | `EXACT 224` | `exact 116 residual 0` |
| `v1_four_values_speech_last` | 4 values, `speech_type` last (= pre-commit) | `residual 224 [sha]` | `exact 115 residual 1` |
| `v2_four_values_speech_second` | 4 values, `speech_type` reordered second | `residual 224 [sha]` | `exact 115 residual 1` |
| `v3_three_values_source_twice` | 3 values, different third (`source_name` twice) | `residual 224 [sha]` | `exact 115 residual 1` |
| `v4_three_conversions_lawful` | 3 conversions, 3 values (lawful, no bug) | `residual 224 [reloc-identity]` | `exact 115 residual 1` |
| `v5_four_values_literal_fourth` | 4th value = `""` literal | `residual 224 [relocs 17!=16, sha]` | `exact 115 residual 1` |
| `v6_four_values_target_twice` | 4th value = `target_name` again | `residual 224 [sha]` | `exact 115 residual 1` |
| `v7_three_values_reordered` | 3 values, thread name last | `residual 224 [sha]` | `exact 115 residual 1` |
| `v8_corrected_form` | the comment's own recommended fix | `residual 224 [sha]` | `exact 115 residual 1` |

**No lawful variant gates exact.** The refutation attempt fails, and the reason
is structural, not empirical: `error` is `cdecl` variadic, so *every* supplied
value costs one pushed dword. Four values therefore always clean `0x18`, which I
confirmed by disassembling v1:

```
    5d  push     esi                    ; speech_type, hoisted
    ...
    79  call     ...                    ; -> _error
    7e  add      esp, 0x18              ; SIX dwords
```

`v1` is byte-for-byte the pre-commit source (I extracted
`git show 9eca0524^:source/ai/ai_script.c` and compared EOL-normalised: identical,
123,465 bytes). So the reported "before" state is authentic.

I also closed the two loopholes the wave report asserts without showing:

- "read the format as three conversions plus a literal percent" — refuted by the
  raw bytes (`%`-count 4, `%s`-count 4, `other conversions: []`; no `%%`).
- "a wrapper or macro supplying the fourth conversion" — refuted structurally
  (any such wrapper still pushes a dword; v5/v6 demonstrate it for two different
  fourth values) **and** by the xref census: the literal is referenced exactly
  once in the object.

I could not verify the wave report's "23-blob `branch_sweep` census; best
historical result 113" — that is a claim about a historical-source sweep I have
no artifact for.

---

## 5. House rules and fake matching

```
$ python tools/fake_match_scan.py --format json source/ai/ai_script.c
{"files_scanned": 1, "findings": [], "skipped_vendored": []}

$ python tools/fake_match_scan.py --format json source/ai
{"files_scanned": 73, "findings": [ …2 findings, both source/ai/ai.c:2889 and :3034,
  rule "codegen-directive", the pre-existing #pragma optimize("", off/on) pair… ]}
```

**The clean scan is not clearance, and structurally cannot be.** I listed the
scanner's rules (`--list-rules`): `self-assignment`, `goto-next-label`,
`empty-if`, `empty-then-else`, `empty-else`, `identical-branches`,
`fixed-boolean-condition`, `self-order-comparison`, `volatile-reinterpret-cast`,
`inline-assembly`, `raw-byte-emission`, `codegen-directive`. **Not one of the
twelve examines variadic arity or format-string conversions.** A missing vararg
is invisible to it by construction. The two `source/ai` findings are inherited
and untouched by this commit.

Judged manually against `docs/matching_methodology.md` "Source-credibility gate"
and `docs/campaign_house_rules.md`:

- The gate's prohibited list is *inert, contradictory, undefined or artificial
  source whose only credible purpose is to steer VC7*. This source is none of
  those in purpose: it is an ordinary `error()` call, and the defect is the
  **absence** of an argument, not an added construct. A byte-steering hack adds
  things; this removes one. The narrow "target-proven original bug" exception is
  explicitly carved out in the same paragraph.
- Rule 2 (each parameter on its own line) is honoured; rules 7/8/9/13/19 are
  untouched; no `#pragma`, no asm, no volatile.
- Rule 20/21 (reject fake matching / nonsensical logic that happens to match):
  the logic is not nonsensical — it is a plausible human slip in a diagnostic
  string, of exactly the kind that already exists 27 lines' worth of function
  away at `ai_script.c:1597`, and it is corroborated in a second, independent
  compiler's output 18 years later.
- `docs/matching_methodology.md` "Original-bug policy" prescribes the exact
  comment template used, and "Keep the safe fix disabled in the exact build" is
  satisfied (the fix is described, not applied).

I record one honest residual risk for the owner: this leaves the repository's C
containing a real out-of-bounds varargs read. It is disclosed, debug-gated and
matches in-tree precedent, but it is not free.

---

## 6. Precedent check

Counted by me, not taken on trust:

```
$ grep -rn "BUG (preserved"  source/ --include=*.c --include=*.h | wc -l   ->  33
$ grep -rn "BUG (original"   source/ --include=*.c --include=*.h | wc -l   ->  22
$ git grep -nE "BUG \((preserved|original)" 9eca0524^ -- 'source/*.c' 'source/*.h' | wc -l  ->  54
```

**54 before the commit, 55 now.** The claim is accurate. And this lane added
exactly one:

```
$ git diff 6130661c…HEAD -- 'source/*.c' 'source/*.h' | grep -E "^\+.*BUG \((preserved|original)"
+		/* BUG (preserved for exact matching): the format has four %s conversions
```

Both cited precedents exist, at the exact lines claimed:

- `source/bitmaps/bitmap_extract.c:2101` —
  `// BUG (preserved): January does not bound the final run before reading its next sequence index.`
  It sits over `extract_data.bitmaps[first_bitmap_index + bitmap_count].sequence_index`
  in an unbounded `while`, i.e. a genuine **UB-class out-of-bounds read**. The
  characterisation is correct.
- `source/ai/ai_script.c:1597` —
  `/* BUG (preserved for exact matching): January formats ai_reference again instead of
  target_ai_reference, so the log prints the first name twice. … */`
  A **January diagnostic-argument defect in this same file**. Correct. (Note it
  is a *wrong-argument* defect, not an arity/UB defect, so `bitmap_extract` is
  the load-bearing precedent for the UB class; the wave report says exactly
  this.)

---

## Factual errors and overstatements found

### In the BUG comment (`source/ai/ai_script.c:2763-2773`)

1. **"so no shipping configuration runs it" — OVERSTATED, and contradicted by
   in-tree evidence.** `ai_debug.print_migration` and `.print_scripting` are
   HaloScript external globals (`source/hs/hs_globals_external.c:1468,1470`,
   `ai_debug + 0x97` / `+ 0x99`), settable at runtime from a scenario script or
   the debug console in the very build being matched. Corrected wording is given
   in §3 above.
2. **"Evidence: January cleans 0x14 bytes …" — incomplete attribution** (not
   false). The cleanup proves *three values*; the *four conversions* come from
   the `.rdata` literal, and my variant v4 shows a lawful three-conversion
   format produces byte-identical code. Suggested stronger wording in §3.
3. *Not an error, a clarity nit:* "the caller's entry EDI" would read better as
   "the EDI value saved on entry, i.e. the caller's EDI". EDI is callee-saved;
   the slot is written at `+0x15` before `edi` is reloaded at `+0x16`.

Everything else in the comment — the four-`%s` claim, the `0x14`/five-dword
claim, the `+0x7d` claim, the saved-EDI stack arithmetic, the 2020-build claim,
and the recommended fix (which I compiled as v8: it is valid C) — is accurate.

### In the wave report (`docs/object_matching_logs/claude_lane_a_waveA1_ai_script_20260920.md`)

4. **"(`docs/matching_methodology.md`, house rule 24)" — dangling citation.**
   `docs/campaign_house_rules.md` contains **22** numbered rules; there is no
   rule 24 anywhere in the tree (`grep -rn "rule 24" docs/` returns only this
   report, twice). The referent is the lane brief, which is not in the
   repository. Corrected wording: *"landed under the original-bug policy
   (`docs/matching_methodology.md`, 'Original-bug policy'), which
   `docs/campaign_house_rules.md` rules 20-21 defer to as the narrow exception."*
5. **"This distinguishes it from the two cases the owner previously excluded …
   which are on always-executed paths" — a distinction the cited text does not
   make.** `astra_90pct_rejected_hypotheses_20260920.md:103-115` frames those
   exclusions as *"An archived exact donor is admissible because it matches —
   prohibited"*, listing the uninitialized read/return among **donor**
   properties. Reachability is never mentioned. I could not verify the
   always-executed characterisation either, because the prohibited donors are
   not in the tree. Corrected wording: *"The owner's exclusion of
   `_dead_camera_update` / `_compare_profile_sections` is framed as a bar on
   re-admitting archived donors, not on disclosed defects; this shape is not a
   donor at all — it is recovered from January's bytes and its literal. Its
   debug-only reachability further limits the blast radius."*
6. **"so no shipping configuration executes it"** — same overstatement as (1),
   repeated in the "Blast radius" bullet and again in
   `claude_lane_a_ai_core_HANDOFF_20260920.md:114-115`.

### In the admission manifest (`…_admission_manifest_20260920.json`)

7. **`"source_commit": "7356576d…"` is the lane tip at manifest time, not the
   landing commit.** All 8 portfolio entries carry the identical value, and
   `7356576d` ("wave A5: restore the print_allegiance incident report") touches
   only `source/ai/ai_communication.c`. An auditor following that field for this
   function finds nothing. The landing commit is `9eca0524`. Suggest renaming
   the field to `manifest_tip_commit` or adding a per-entry `landing_commit`.
   (The manifest's `evidence_basis` text is otherwise accurate and, to its
   credit, does *not* repeat the "no shipping configuration" claim.)

### In the commit message

8. No factual error found. Every verifiable number reproduces: 224/224, 16/16,
   `sha equal`, 0 differing rows, `stable_verdicts` gained 1 / 224 / 0
   regressions, parks 184/0/0, admission audit 19/0/8/0, fake scan 0 findings in
   the changed file and the same 2 inherited findings across `source/ai`. It
   repeats the "reachable only under `ai_debug.print_migration` /
   `print_scripting`" statement, which is true, and — correctly — does **not**
   add the "no shipping configuration" gloss.

---

## Things I could not verify

- The tree-wide byte/function totals (`1,444,193 / 1,770,166`; `7,292 / 7,574`;
  `overall 1,465,425 (7,559)`). Verifying them requires `ninja`, which the brief
  forbids. Mitigation: the commit touches one `.c` and one doc, no header and no
  config, so no other translation unit's inputs changed; and the unit census and
  `stable_verdicts` diff both reproduce with 0 regressions.
- The "23-blob `branch_sweep` census; best historical result 113" claim about
  historical source revisions — no artifact available to me.
- Whether `_dead_camera_update`'s uninitialized read and
  `_compare_profile_sections`'s uninitialized return are in fact on
  always-executed paths. The prohibited donors are not in the tree.
- Whether a *retail/shipping* Halo build retains the `ai_print_migration` /
  `ai_print_scripting` externals. This is precisely why claim (e)'s "no shipping
  configuration runs it" should be withdrawn rather than defended: it is
  unfalsifiable from campaign evidence, and the January build it *is* made about
  does expose the switch to scripts.

---

## Process observation: another agent is mutating this worktree concurrently

At the start of this review `git status --porcelain` was **empty**. During it,
two files changed underneath me without any action of mine:

```
 M docs/object_matching_logs/claude_lane_a_ai_core_HANDOFF_20260920.md   (mtime 13:45:05)
 M docs/object_matching_logs/claude_lane_a_ai_core_rejected_hypotheses_20260920.md (13:44:37)
?? lab.obj                                                               (13:46:13)
```

The doc edits record an unrelated **owner ruling on `_actor_emotion_update`**
("do not land the out-of-bounds read while the function remains fuzzy… must
deliver STRICT EXACT before it can be reconsidered"). `lab.obj` at the repo root
comes from `tools/c2dbg32/gen_config_lab.py`, which I never invoked; none of the
tools I ran (`gate.py`, `relocdiff.py`, `tinfo.py`, `stable_verdicts`,
`parked_functions`, `audit_object_admission.py`, `fake_match_scan.py`) writes to
`docs/` or the repo root. So a second process is working in
`C:/halo-worktrees/claude-lane-a-ai-core-20260920` right now.

This did **not** invalidate my measurements: `git status --porcelain source/
config/` is empty throughout, and a final `gate.py source/ai/ai_script --all`
re-run at the end of the review still reports
`== exact 116  residual 0  unwritten 0`. But the caller should know that a
review and another lane were sharing one working tree, and that lab.obj is
stray.

Worth noting for the owner: that concurrent ruling is *adjacent in kind* to this
one — an out-of-bounds read the owner declined to land **because it did not
reach strict exact**. That is consistent with accepting the ai_script landing,
which does reach strict exact, and it sharpens the real decision rule: strict
exactness is the price of admission for a disclosed defect, not debug-only
reachability.

## Recommendation

**Keep the landing.** The 214 bytes are real, the defect is proven from
January's own instruction stream, `.rdata` literal and relocation census, it is
corroborated in an independent 2020 build, and nine compiled alternatives —
including every one the wave report claims to have refuted — fail to gate exact.
Before this is considered closed, make the three one-line documentation
corrections above: strike "so no shipping configuration runs it" from the BUG
comment (items 1/6), fix the "house rule 24" citation (item 4), and restate the
`_dead_camera_update` / `_compare_profile_sections` distinction in the owner's
own terms (item 5). None of them touches a byte of code.
