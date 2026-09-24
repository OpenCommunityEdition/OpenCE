# review4 rasterizer_xbox_models ownership - reviewer ledger
- read WORKER_BRIEF 0-9, house rules, lane ledger held table, worker LEDGER, unit xdk_wrapper_aliases ledger.
- patch apply --check clean; independent derivation == worker copy; --directory apply == worker copy.
- publics: names absent; no public at RVAs 0x15a150..0x15a9c0; 17-19 image copies per wrapper; D3DINLINE static.
- csplit regen (slug): only models.obj differs (5 storage bytes); base regen == build/split; no UNDEF refs board-wide.
- audit prod/g6 vs regen: 58/58 symbols, FAIL(2) (residual .text only); pdb_storage 0 disagreements.
- gate prod 12/2 unchanged; objdiff 3.3.1 cur==regen (96.91414, 4111/12379, data 1912/1912).
- surplus_identity 3 IDENTICAL; provider_link PASS.
- VERDICT: APPROVE as-is. See REVIEW.md.
