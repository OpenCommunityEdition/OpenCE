# Main update-time: independent W7 reconciliation

Canonical baseline: `a88549404523ecf34d510c62765a3cfdc89dbd36`.
Donor: `26684ca8f57ab2b9b418526b92ddab7b8ed7e480`, branch
`claude/compiler-application-20260925`.
The owner requested reconciliation and publication if the function passed.
Only `source/main/main.c` and its one park removal are taken from the donor;
other research, scorer proposals and held candidates are excluded.

## Result

`_main_update_time` is independently strict exact: **1,440 credited code
bytes / 1,440 padded bytes / 117 relocations**. Normalized SHA256:
`b32677fb6c4cc6bcd664b98bfc5cc962b68bdf65228a31759b52574becdf200b`.

Halo credited code advances **1,588,626 -> 1,590,066 / 1,770,166 (89.83%)**.
Halo credited functions advance **7,457 -> 7,458 / 7,574**. Complete objects
stay **388/468**; credited data stays **2,587,011**. Main has 93 exact,
one residual (`_main_frame_rate_debug`) and one unwritten (`_main_crash`)
target row, so this is not a whole-object completion.

## Source credibility

The two throttle branches now assign the existing signed-short field
`main_globals.vblank_interval_current` directly, then the join uses that
field for the target-index addition. Previously both branches selected a
short local and the join stored that local to the field. The debug calls
precede the assignment in both forms; there is no intervening side effect
between the new assignment and read-back. The global is nonvolatile. The
same selected values, truncation and arithmetic are preserved on each path.

The existing field is at offset `0x3B6`, already protected by an in-tree
offset assertion. It is correctly aligned for a short (two bytes), but not
for the compiler's attempted four-byte widening. No structure, padding,
cast, qualifier, barrier, dummy operation or compiler directive is changed.
The remaining local now holds only the requested interval, so the descriptive
rename `selected_interval` -> `requested_interval` reflects its narrower role;
the donor's F1/F1b controls show that rename is byte-inert.

January has a 16-bit copy at `+0x3FF`, sign extension at `+0x402` and the
field store at `+0x40B`. W7's corrected, stock-equal C2 traces associate the
narrow copy with the genuine field assignment: field provenance prevents
widening, and promotion merges the final write-back. This is a compatible,
predictive mechanism for natural source, not proof of unique original C.
There is no corresponding later `/Od` implementation; HCEX corroborates
field types only. The rejected compiler-barrier experiment is not admitted.

Source evidence and failed controls remain in the donor's
`research/compiler_application_20260925/workers/W7/`, especially `CARDS.md`,
F1/F1b, corrected T7b, and the widening/store labs. Supplied patch SHA256:
`3d2ad57ed7379493eaf7de32693fe486814f7cc04556f96d4b05dc0b6ca4b754`.

## Fresh verification

- Full `ninja` before and after: pass. Objdiff remains 3.3.1; no scorer,
  normalization, compiler, header or symbol-manifest changes.
- Fresh target gate: exact, followed by retirement of only this function's
  11-line unclassified park entry. All other entries remain unchanged.
- Whole-board stable sweep: **8,252 owners; 7,629 -> 7,630 exact**,
  exactly this gain and **zero regressions**.
- Independent whole-TU comparison: 254 sections before/after; only the target
  section changes. The other 196 non-debug and 57 debug sections are equal.
  All 414 owner records, COMMON and COMDAT selections/associations are unchanged.
- One internal compiler label in `_halt_and_catch_fire` is renamed
  `$L29500` -> `$L29502` at the same offset `0x291`. Its section bytes,
  extent and relocations are equal, with no incoming relocations to either
  label. This is not a new runtime owner or changed helper.
- Parks: **76 active / 0 stale / 0 invalid**.
- Admission unchanged: **10 candidates / 0 contradicted / 1 rejected /
  0 revoked**.
- `/W3` unchanged: C4146 x1, C4244 x20, C4013 x4 (all inherited).
- Fake scan: zero leads in main.c, 26 inherited board-wide leads.
- Tools tests: **1,161 passed, 5 skipped, 26 subtests passed**.
- `git diff --check`: clean; original source/config CRLF preserved.

Production source and park changes equal the donor's named two-file delta.
Private receipts are in `scratch/reconcile_main_time_20260926/`: original
source/object, build logs, before/after stable snapshots and ledgers, fresh
gate, warning census, parks/admission/fake reports, tools-test log, and
`main_independent.json` with the complete section/owner audit.

The function and park retirement are committed together. This adds no new
source-form exception and makes no whole-program linking or boot claim.
