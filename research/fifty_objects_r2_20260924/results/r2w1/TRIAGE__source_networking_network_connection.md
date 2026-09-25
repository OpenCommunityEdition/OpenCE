# source/networking/network_connection

## decision
OWNER

## est_probability
0.4

## route
The owner rules on two items: (1) _network_server_close_client_connection: the authentic N+1 capacity loop bound (MAXIMUM_NUMBER_OF_LOCAL_PLAYERS + 1, with a BUG comment), which the 2020 /Od build confirms (<0x11 with 16 slots); (2) _network_connection_connect: the redundant unreliable-arm `success = TRUE` (class H; owner packet s6 recommends rejecting it). cand_full gives 23/23 and object_audit PASS.

## why_new
Nothing new. The /Od build has no unreliable path to attest the connect arm.

## prior_negatives_checked
network_connection_obj_* ledgers (4); round-1 wave2 result and review (C1-C15 connect probes; P1-P3 bounds); claude_structural_owner_packet_20260923 s4/s6.

## notes
The object completes only if BOTH rulings are yes. If connect is rejected, fall back to C2 (288/288, one block placement), which stays fuzzy.