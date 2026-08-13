/*
DAYS.C

symbols in this file:
	__lpdays (.data, 0x34)
	__days (.data, 0x34)
*/

/* ---------- public data */

int _lpdays[13] =
{
	-1, 30, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365
};

int _days[13] =
{
	-1, 30, 58, 89, 119, 150, 180, 211, 242, 272, 303, 333, 364
};
