# `cseries.obj` closed: the two-cursor assignment order (Jonas, 2026-08-28)

**`source/cseries/cseries.obj` is complete: 26/26 target code owners and 20/20
runtime data owners strict exact.** The sole residual `_csstrcasecmp` reaches
192 padded bytes, 11 relocations, normalized SHA-256
`9fb7ecf4631bd2d54ae281b836b5f974242c9527483666ccd00a04fefce84efa` — identical
to January — with `tools.coff_compare.section_infos_equal` returning true and
relocations at the target addresses `23, 28, 38, 43, 48, 55, 62, 75, 88, 121,
134`. Configuration index 359 moves `NonMatching` → `Matching`.

## The finding

This closes the residual left by
`cseries_obj_jonas_two_cursor_20260825.md`, whose C1 one-shot reached the
correct 192/11 shape and the target `c1 = ESI` allocation but selected the
wrong induction variable, emitting `8f1984c1…` instead of `9fb7ecf4…`.

**The decisive factor is the order in which the two cursors are first
assigned, not their declaration order, their increment order, or their types.**

```c
second = s2;                 /* second-string cursor assigned FIRST */
first = s1;
```

Assigning the second-string cursor first makes VC7 retain it as the advancing
EDI induction variable and reconstruct the first stream from the invariant
delta, which is January's topology. Reversing just those two assignment
statements reproduces the prior lane's C1 result exactly — `8f1984c1…`, the
mirrored induction variable — confirming that this single ordering is the whole
residual and that nothing else in C1 was wrong.

Measured inert once the assignment order is right: declaration order of the two
cursors (any of four), `register` present or absent, the loop's increment order
(`second++; first++` and `first++; second++` are both exact). Measured
harmful: swapping the two `towlower` loads in the loop body (208 bytes), and
computing `c2` before `c1` at the head (208 bytes).

## Accepted source

```c
long csstrcasecmp(
	const char *s1,
	const char *s2)
{
	int c1;
	int c2;
	const char *first;
	const char *second;

	cseries_match_assert("c:\\halo\\SOURCE\\cseries\\cseries.c", 397, s1 && s2);

	c1 = towlower(*s1);
	c2 = towlower(*s2);
	if (c1 == 0)
		goto c1_zero;

	second = s2;
	first = s1;
loop:
	if (c2 == 0)
		goto c2_zero;
	if (c1 != c2)
		goto not_equal;
	second++;
	first++;
	c1 = towlower(*first);
	c2 = towlower(*second);
	if (c1 != 0)
		goto loop;
	...
```

Three independent policy improvements come with it, none of them forced:

- the obsolete `register` storage class is **removed** (the exact donor kept it;
  it is measured unnecessary);
- the `offset = s1 - s2` pointer subtraction between **unrelated** pointers —
  undefined behaviour, inherited from the baseline — is removed entirely;
- the comparison polarity is corrected to `c1 != c2`, which is what January
  emits (`cmp esi,eax`).

No assembly, volatility, barrier, intrinsic, pragma, attribute, inline control,
raw offset or address, pointer or union pun, inactive-union access, synthetic
anchor, tool or flag change, object-byte patch, or comparator exception. The
existing `goto` topology and three explicit returns are retained: January's
back-edge and three distinct return paths are the documented exactness
exception to the single-return preference. One parameter per line. No header,
no other function, and no other unit changed.

## Method note

The prior two lanes worked this residual through frozen one-shot waves — one
compile each, no retry — which is why a two-statement ordering survived them.
This lane first reproduced their recorded results **out of tree**: an in-memory
edit plus a real VC7 full-TU compile plus `section_infos_equal`, at roughly two
translation units per second. The baseline reproduced `65cce7e9…`, the polarity
correction reproduced their recorded intermediate `cbae75b4…`, and a reversed
cursor order reproduced their C1 `8f1984c1…`, all bit-for-bit. Only after the
exact spelling was identified and the whole-object census verified at 26/0/0
was a single production emission made. Harness in the scratch lab
`c2lab-20260826/frontier20260827/` (`batch.py`, `cens2.py`).

## Validation

- Pinned toolchain verified by SHA-256 before use: `cachebeta.exe`
  `4cc87b45…`, `CL.Exe` 13.00.9254.1 `483e00c4…`, `csplit` `708f957c…`
  (re-verified after ninja re-downloaded it), `objdiff-cli` `090987aa…`.
- Isolated worktree `jonas/cseries-two-cursor-close-20260828` branched from the
  observed integration HEAD `6b229430`; baseline source blob `439de058…`
  confirmed before editing.
- Pre-edit whole-TU regression snapshot at commit `6b229430`.
- Real VC7 `/Zs` parse-only: clean, no diagnostics, object hash unchanged.
- Dry run before emission: exactly **one** CL edge,
  `build\base\source\cseries\cseries.obj` — containment proven, no header
  touched.
- Exactly one ordinary emission. First-shot object preserved unmodified at
  `build/audit/cseries-close-first-shot/cseries.obj`, phase-local raw SHA-256
  `63fe7f36…`; a later clean rebuild reproduced it byte-for-byte.
- Regression gate: `ok: true`, 0 failures, **25 still_exact, 1 newly_exact, 0
  changed_nonexact**, no data or symbol-ownership drift.
- `halobetacache_build` and `libcmt_build` both satisfied.
- `build/report.json` and `build/semantic_report.json` regenerated; the unit
  reports 26/26 functions, 2507/2507 code bytes, 972/972 data bytes, 100%.
- Audits: semantic (`accepted_exact` 4216, `unit_errors` 0), parked (no stale,
  no invalid), object admission (`candidate_count` 0, `revoked_count` 0).
- All **179** tooling tests pass.
- `git diff --check` clean; the tracked diff is exactly
  `source/cseries/cseries.c` and one line of `config/config.json`.

**Pre-existing and unrelated:** the admission audit reports one
`completion-label-contradicted`, `source/shell/shell_xbox` (function_gap 1,
data_gap 32). That unit is already labelled `Matching` at HEAD, is untouched by
this wave, and its contradiction predates it. It is recorded here so the next
lane does not attribute it to this change; it deserves its own adjudication.

## Note on `build.ninja` regeneration

The `progress` target runs `configure.py`, which regenerates `build.ninja` and
reverts the CL command to the relative forward-slash spelling that fails
`CreateProcess` in this environment. Re-apply the worktree-absolute respelling
after it, as documented. Reports were generated before `progress` for that
reason.
