/* Compile-only test callers; runtime dependencies are bounded test doubles. */
#include "cseries/cseries.h"

long assertion_condition_probe(
	void);
char *assertion_diagnostic_probe(
	void);

void assertion_plain(
	void)
{
	assert(assertion_condition_probe());
	return;
}

void assertion_diagnostic(
	void)
{
	dassert(assertion_condition_probe(), assertion_diagnostic_probe());
	return;
}

void assertion_formatted(
	void)
{
	vassert(assertion_condition_probe(),
		csprintf(temporary, "diagnostic: %s", assertion_diagnostic_probe()));
	return;
}

void assertion_matching_plain(
	void)
{
	match_assert("original.c", 8505, assertion_condition_probe());
	return;
}

void assertion_matching_diagnostic(
	void)
{
	match_dassert("original.c", 8505,
		assertion_condition_probe(), assertion_diagnostic_probe());
	return;
}

void assertion_matching_formatted(
	void)
{
	match_vassert("original.c", 8505, assertion_condition_probe(),
		csprintf(temporary, "diagnostic: %s", assertion_diagnostic_probe()));
	return;
}

void assertion_disconnected_weapon(
	void)
{
	/* January _object_connected_to_map_bit is 11; no object layout is needed. */
	match_vassert("original.c", 8505,
		!TEST_FLAG(assertion_condition_probe(), 11),
		csprintf(temporary, "diagnostic: %s", assertion_diagnostic_probe()));
	return;
}

void warning_plain(
	void)
{
	warn(assertion_condition_probe());
	return;
}

void warning_diagnostic(
	void)
{
	dwarn(assertion_condition_probe(), assertion_diagnostic_probe());
	return;
}

void warning_formatted(
	void)
{
	vwarn(assertion_condition_probe(), csprintf(temporary, "diagnostic: %s", assertion_diagnostic_probe()));
	return;
}

void warning_matching_plain(
	void)
{
	match_warn("original.c", 8505, assertion_condition_probe());
	return;
}

void warning_matching_diagnostic(
	void)
{
	match_dwarn("original.c", 8505, assertion_condition_probe(), assertion_diagnostic_probe());
	return;
}

void warning_matching_formatted(
	void)
{
	match_vwarn("original.c", 8505, assertion_condition_probe(), csprintf(temporary, "diagnostic: %s", assertion_diagnostic_probe()));
	return;
}

void halting_plain(
	void)
{
	halt();
	return;
}

void halting_diagnostic(
	void)
{
	dhalt(assertion_diagnostic_probe());
	return;
}

void halting_formatted(
	void)
{
	vhalt(csprintf(temporary, "diagnostic: %s", assertion_diagnostic_probe()));
	return;
}

void halting_matching_plain(
	void)
{
	match_halt("original.c", 8505);
	return;
}

void halting_matching_diagnostic(
	void)
{
	match_dhalt("original.c", 8505, assertion_diagnostic_probe());
	return;
}

void halting_matching_formatted(
	void)
{
	match_vhalt("original.c", 8505, csprintf(temporary, "diagnostic: %s", assertion_diagnostic_probe()));
	return;
}
