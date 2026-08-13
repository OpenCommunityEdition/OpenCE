/*
NLSDATA2.C

symbols in this file:
	___lc_clike (.data, 0x4)
	___lc_handle (.bss, 0x18)
	___lc_codepage (.bss, 0x4)
	___lc_collate_cp (.bss, 0x4)
*/

/* ---------- public data */

int __lc_clike = 1;

unsigned long __lc_handle[6] = { 0 };
unsigned int __lc_codepage = 0;
unsigned int __lc_collate_cp = 0;
