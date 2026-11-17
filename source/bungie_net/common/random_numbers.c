/* ---------- headers */

#include "cseries.h"
#include "bungie_net/common/random_numbers.h"

#include <time.h>

/* ---------- constants */

/* ---------- macros */

/* ---------- structures */

/* ---------- globals */

static boolean random_numbers_initialized = FALSE;

/* ---------- public code */

long randomrange(long min, long max)
{
	if (!random_numbers_initialized)
	{
		srand(time(NULL));
		random_numbers_initialized= TRUE;
	}
	return (long)((double)rand()*(unsigned long)max/((unsigned long)min+32767.0))+min;
}

void
randomrange64(
	struct qword_value const *min,
	struct qword_value const *max,
	struct qword_value *result)
{
	struct qword_value offset;

	match_assert("c:\\halo\\SOURCE\\bungie_net\\common\\random_numbers.c", 46, min && max && result);

	if (!random_numbers_initialized)
	{
		srand(time(NULL));
		random_numbers_initialized= TRUE;
	}

	offset.qword = (unsigned __int64)((double)rand() * max->qword / (min->qword + 32767.0));
	add64(min, &offset, result);

	match_assert("c:\\halo\\SOURCE\\bungie_net\\common\\random_numbers.c", 58, result->qword >= min->qword);
	match_assert("c:\\halo\\SOURCE\\bungie_net\\common\\random_numbers.c", 59, result->qword <= max->qword);
}

/* ---------- private code */
