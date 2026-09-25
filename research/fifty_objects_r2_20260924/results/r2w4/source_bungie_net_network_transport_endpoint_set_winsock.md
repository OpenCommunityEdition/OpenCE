# source/bungie_net/network/transport_endpoint_set_winsock

## verdict
ADMISSION_FIX_PROPOSED

## production_changes
patches/04: transport_endpoint_set_get_next_index->get_next_available_set_array_index and transport_endpoint_set_compare_entries->poll_ep_array_compare_proc, plus the parked.json key rename and evidence note, which must land together. Zero credit.

## evidence
Both bodies are identical to Sept. The current name came from an atlas C++ propagation (poll_endpoint_array_compare_proc) that contradicts the Sept map ground truth. Gate 23/3 with SAME rows; objcmp identical; objdiff unchanged; the parked measurements hold under the new name.

## blockers
Residuals _poll_ep_array_compare_proc (parked, EAX/ECX), _poll_endpoint_set and _transport_initialize; object_audit FAIL(3) is unchanged.

## reopen_criteria
A contradicting first-party name.

## manifest
C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\critic_3\MANIFEST.md