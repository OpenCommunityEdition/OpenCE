# `libcmt/wcscspn.obj` exact-match ledger

This atomic lane re-audited an older completed patch as untrusted evidence.
Commit `2fdd66cf` was rooted at the old `f8e8e6b` campaign state, so it was not
cherry-picked. Only its proposed source and experiment notes were copied into
a fresh worktree later rebased to authoritative campaign tip `fe4065f6`; all target,
candidate, build, progress, ownership, and regression evidence was regenerated.

## Target inventory

- January object: `libs/libcmt/wcscspn.obj`
- Function: `_wcscspn`
- Padded code size: `0x43` (67 bytes)
- Relocations: none
- Owned data: none

The target walks the input until NUL or until its current unsigned 16-bit
character occurs in the reject set, then returns the input pointer distance in
wide characters.

## Source-shape experiments

### Rejected: nested `for` loops with a post-loop test

The first canonical form used an inner loop followed by `if (*character)
break;`.  It produced 73 bytes instead of 67 because VC7 emitted a second
load/test of the reject-set character after the inner loop.  The target sends
the match edge directly to the final pointer subtraction and sends only the
reject-set NUL edge back to the outer loop.

### Candidate: direct match edge to the shared exit

The second canonical CRT form expresses those distinct edges directly: the
inner loop uses `goto done` on a match, while reject-set exhaustion advances
the input.  It produced the correct 67-byte size and an instruction-identical
loop body, but VC7 shrink-wrapped the callee-save pushes and pops around the
nonempty-input path.  January saves the registers before the empty-input
branch and performs the shared pointer subtraction before restoring them.

### Candidate: explicit empty guard plus `do/while`

The next equivalent CRT form makes the empty-input edge jump explicitly to
the shared `done` label, then expresses the known-nonempty body as a `do/while`.
This tests the only remaining topology difference without changing the loop
semantics or adding an extra return.  VC7 canonicalized it to the exact same
67-byte shrink-wrapped object, so guard spelling alone is not the control.

### Candidate: reject-set cursor initialized outside the loop

The next form initializes the reject-set cursor before the outer condition and
resets it after advancing the input.  This matches classic CRT source and
extends the cursor's source lifetime across the empty-input edge, the most
direct readable lever for preventing prologue shrink-wrapping.

That form also remained 67 bytes, but it made both base pointers source-live:
VC7 assigned the original input to EBX and the reject set to EDI, unlike the
January EAX/EBX roles.

### Candidate: immutable reject-set base plus inner cursor

The next form gives the reject set its own immutable base local before the
outer condition, then initializes the mutable cursor from it inside the loop.
This targets January's exact dataflow: input cursor in EAX, reject-set base in
EBX, mutable reject cursor in EDX.  VC7 eliminated the immutable local and
returned to the prior shrink-wrapped 67-byte object.

### Candidate: identical match and exhaustion returns

The next classic CRT form returns the same pointer difference immediately on
a reject match and again after input exhaustion.  VC7 can cross-jump identical
epilogues; this tests whether owning one return inside the register-using loop
extends the callee-save region across the outer empty-input edge, as January
does.  It passed exactly: VC7 merged the two readable source returns into the
single January machine epilogue.  This is the measured house-rule exception;
forcing one source return reintroduces the proven shrink-wrap mismatch.

No assembly, volatile forcing, undefined behavior, byte patching, compiler
substitution, or comparator exception is used.

## Verification

The identical-return CRT form passes the hardened comparator:

- size: `67/67`
- relocations: `0/0`
- normalized SHA-256, both sides:
  `856fdaf095ac58c9f29a14b9708d3b343d15095fd05392860ac8cd3ee194b5e2`
- owned data in the target: none
- `section_infos_equal`: `true`

The candidate's directive and debug sections are ordinary compiler-generated
metadata.  The sole target COMDAT is strict exact, so the object is safe to
mark `Matching`.

## Current-campaign admission gates

- XDK 3911 `CL.Exe` 13.00.9254.1 rebuilt the source with the configured libcmt
  `/O1 /Gy /I"xbox/include"` flags; no lane-local rule or flag changed.
- The full configured Halo build and the full libcmt build passed.
- `ninja semantic_progress` passed with zero unit errors.
- `ninja progress` credited `wcscspn.obj` as a complete libcmt object. At this
  campaign state libcmt reports 11/476 functions and 502/55,015 code bytes
  exact across 10/212 objects.
- Whole-unit regression snapshot/check passed from clean commit `8d59075c`:
  `_wcscspn` is `still_exact`, with no failures, warnings, newly-exact siblings,
  changed nonexact code, or ownership change.
- `python -m unittest discover -s tools -p 'test_*.py' -v`: 177/177 passed.

## House/Berth and readability audit

The two parameters are each on their own line and all exits use explicit
returns. The identical successful/exhaustion returns are the measured,
documented exception to the preferred single-return rule: VC7 cross-jumps them
into January's one machine epilogue, while forcing one source return produces
the proven shrink-wrapped mismatch. `(void)` formatting is inapplicable because
the function has parameters. There are no tag or object accesses requiring
typed macros.

The source contains no assembly, `volatile`, forced inlining, undefined
behavior, byte patch, alignment directive, optimization pragma, raw structure
offset, comparator exception, or codegen-only cast. No original January bug was
identified, so no bug-preservation comment is needed.
