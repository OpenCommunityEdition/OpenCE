# source/hs/hs_runtime (storage: 12 converters)

## decision
ADMISSION

## est_probability
0.9

## route
Add `static` to the prototypes (hs_runtime.c:469-491) and definitions (around :1540-1660) of hs_long_to_boolean, hs_short_to_boolean, hs_string_to_boolean, hs_data_to_void, hs_short_to_real, hs_long_to_real, hs_enum_to_real, hs_real_to_short, hs_real_to_long, hs_long_to_short, hs_object_name_to_object_list and hs_object_to_object_list. symbols.json already marks all 12 `static: true`, and the split has storage 3, so no csplit regen is needed. Gate should confirm the change is byte-inert.

## why_new
object_audit and pdb_storage show 12 storage 3/2 disagreements. All 12 are absent from the cachebeta publics, and they are the 10 NO-JANUARY-PROVIDER link_fail rows. They are referenced only through the conversion table inside hs_runtime.c (grep), so they are address-taken and a private ABI is impossible. No prior ledger addressed them: the round-1 hs storage packet covered hs.c only.

## prior_negatives_checked
hs_runtime_obj_opus5_250k_w3c A1 static-linkage refutation concerns hs_global_data/hs_thread_data/hs_syntax_data (UNDEF externals), not these converters; round-1 hs ownership packet + review (hs.c only).

## notes
Not measured. Triage made no candidate edits; the expectation of byte-inertness rests on the table-only references. Necessary but not sufficient for hs_runtime completion.