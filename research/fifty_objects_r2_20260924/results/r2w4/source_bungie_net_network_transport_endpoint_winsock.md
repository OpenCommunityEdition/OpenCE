# source/bungie_net/network/transport_endpoint_winsock

## verdict
ADMISSION_FIX_PROPOSED

## production_changes
patches/03: names only. connection_thread_list_add->add_connect_thread, connection_thread_list_mark_for_disposal->mark_connection_thread_as_terminated, create_endpoint_socket->create_socket, connect_endpoint_process@4->connect_async_thread_proc@4. The patch also renames the key of the config/parked.json entry for the parked residual and appends an evidence note; it must land in the same commit because parked validation keys on the name. Zero credit.

## evidence
All 4 are identical to Sept (Aug-2001 agrees); callers match. Gate 29/1 with SAME rows; objcmp identical; only the owner object changes in the split; objdiff unchanged. Under the new name the parked measurements (304/25 and both sha256 values) still match.

## blockers
_connect_async_thread_proc@4 stays parked: January reads a stack slot that was never initialised. object_audit FAIL(1) is unchanged.

## reopen_criteria
A contradicting first-party name.

## manifest
C:\halo-worktrees\claude-fifty-objects-r2-20260924\scratch\w\critic_3\MANIFEST.md