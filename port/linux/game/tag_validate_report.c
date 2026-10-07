/*
TAG_VALIDATE_REPORT.C

Where the tag validator's messages go in the game (tag_validate.c): the
debug log. (tools/map_validate.c prints them.)
*/

/* ---------- headers */

#include "cseries.h"
#include "errors.h"
#include "tag_schema.h"

/* ---------- public code */

void tag_validate_report(
	char const *message)
{
	error(_error_silent, "%s", message);

	return;
}
