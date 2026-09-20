> Packet18 final disposition: all three source/header probes restored, zero admission. Qualification packets precede their measured results; production remains wave17.

# Private biped owner correction rejected: inherited exact function lost

Original August/September MAPs mark all three owners private in `bipeds.obj`; fresh HCEA records also identify them as static. The actual source/reference inventory has no cross-owner or function-pointer uses. The September airborne and snap bodies closely correspond to January apart from assertion lines; moving has changed across builds but retains its named call/owner identity. The later snap signature differs and was explicitly excluded. These facts qualified a linkage correction, not replacement source bodies. See `../biped-private-owner/packet.md` and `audit.json`.

Root froze all23 actual/static header consumers before editing. The one fixed shape removes the three public `bipeds.h` declarations, makes the three existing definitions static, and adds only the necessary snap forward declaration to the existing private prototype group. All bodies, January signatures and definition positions remain unchanged. No declaration-order or placement variants were tried.

All23 consumers compiled. **No new exact matches.** Biped's42 inherited exact controls survive, but the unrelated accepted `units::_unit_preprocess_node_orientations` loses its exact match: the same `mov edx,[ebp-4]` is scheduled one instruction earlier at+0x269. This alone rejects the entire proposal under the preservation rule. The already residual `physics::_physics_update_old` also schedules `mov esi,[ebp-0x80]` one instruction earlier at+0xe3a; it remains non-exact and earns no credit. Alignment-aware comparisons are saved beside this report.

There are no added/removed function owners or noncode/COMMON/point changes in the measured records. The three scratch definitions become private as intended. The test checks1,131 inherited exact controls across its23 consumers; one fails in scratch. The source/header/base-object hashes and header timestamp were restored on exit. **No regression was landed.** There is no follow-up prototype, declaration-order, flag or combination search.

Accounting: one source shape,23 root C compiler calls, zero worker calls and zero production/source/header/config changes. Full production and accepted credit remain wave17. The pre-probe raw evidence snapshot is preserved; the reviewer's final qualification, already communicated before the test, is in the final primary report.
