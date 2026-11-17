/*
CREWMAN_HANDS.H

The seated crewmen's hands drawn on the desks they work, not in them
(crewman_hands.c).
*/

#ifndef __CREWMAN_HANDS_H
#define __CREWMAN_HANDS_H

struct real_matrix4x3;

/* ---------- prototypes */

/* as an Xbox map's tags load (scenario_tags_load): the crewman's arms and
the vertices their forearms and hands hold, read from his model */
void crewman_hands_tags_loaded(
	void);
/* as any map's tags unload: none */
void crewman_hands_tags_unloaded(
	void);
/* a unit's node matrices, made for the tick (objects.c,
object_postprocess_node_matrices): a seated crewman's, each arm bent at the
elbow or raised at the shoulder until his forearm and hand are out of what
is before them; how many arms were bent */
short crewman_hands_postprocess_node_matrices(
	long object_index,
	struct real_matrix4x3 *node_matrices);

#endif
