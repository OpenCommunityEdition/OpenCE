/*
NLSDATA1.C

symbols in this file:
	___mb_cur_max (.data, 0x0)
	___decimal_point (.data, 0x4)
	___decimal_point_length (.data, 0x8)
*/

/* ---------- globals */

int __mb_cur_max = 1;
char __decimal_point[4] = ".";
int __decimal_point_length = 1;
