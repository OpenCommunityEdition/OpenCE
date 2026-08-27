# Object-matching audit helpers

These scripts preserve focused COFF comparison, donor-search, disassembly,
and residual-ranking utilities used during exact-object reconstruction. Run
them from the repository root unless a script accepts explicit paths.

The disassembly helpers require the Python `capstone` package. All COFF
parsing and exactness decisions come from the repository's tracked
`tools.coff_compare` implementation; these helpers do not alter admission or
matching status.

Files with dates in their names are retained research snapshots. Their
hard-coded inputs intentionally document the evidence wave they were written
for, while all imports and repository paths are portable.
