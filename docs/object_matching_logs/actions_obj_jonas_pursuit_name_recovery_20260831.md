# `actions.obj` pursuit-helper name recovery (2026-08-31)

The January symbol atlas and HCEA release PDB recover the private helper at
file offset `0xB3B0` as `actor_action_determine_pursuit_options`, with static
linkage.  The source, caller, symbol atlas comment, and `config/symbols.json`
now use that name instead of `code_0000b3b0`.

Claude's experimental removal of the `controlled_by_group` formal was not
retained.  The later PDB authenticates the complete interface, and deleting a
real parameter merely changed the private register convention.  The parameter
and its call argument remain.  The artificial explanatory `(void)` use was
removed because it had no semantic or machine-code role and claimed more
January provenance than the evidence supports.

This is a naming/authenticity correction, not byte-match credit.  The helper
remains an honest 368-byte, six-relocation residual (98.24428% objdiff), and
`actions.obj` remains 57 exact / 6 residual / 0 unwritten.  The existing
Actions closure ledgers already record the bounded source experiments; no
register-forcing construct, ABI lie, or speculative parameter deletion is
introduced here.
