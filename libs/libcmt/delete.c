/*
DELETE.C

symbols in this file:
	??3@YAXPAX@Z (.text, 0x5)
*/

#include <stdlib.h>

void operator delete(
	void *block)
{
	free(block);

	return;
}
