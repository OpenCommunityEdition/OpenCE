/* ---------- headers */

#include "cseries.h"
#include "ai/ai.h"
#include "objects.h"
#include "game/players.h"

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- prototypes */

/* ---------- globals */

object_deleted_proc object_deleted_procs[3] =
{
	objects_fix_for_deleted_object,
	ai_handle_deleted_object,
	players_handle_deleted_object
};

/* ---------- public code */

void object_deleted_procs_call(
	long deleted_object_index)
{
	object_deleted_proc *deleted_proc = object_deleted_procs;
	long deleted_proc_count = NUMBEROF(object_deleted_procs);

	do
	{
		(*deleted_proc)(deleted_object_index);
		deleted_proc++;
	}
	while (--deleted_proc_count);
}

/* ---------- private code */
