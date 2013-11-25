/*
 *	Fast block move/copy subroutines
 *	Copyright
 *		(C) 1992 Joseph H. Allen
 *
 *	This file is part of JOE (Joe's Own Editor)
 */
/* This module requires ALIGNED and SIZEOF_INT to be defined correctly */

#include "types.h"

/* Set 'sz' 'int's beginning at 'd' to the value 'c' */
/* Returns address of block.  Does nothing if 'sz' equals zero */

int *msetI(void *dest, int c, int sz)
{
	int *d = (int*)dest;

	for(; d != (int*)(dest)+sz; ++d)
		*d = c;

	return (int*)dest;
}

/* Set 'sz' 'int's beginning at 'd' to the value 'c' */
/* Returns address of block.  Does nothing if 'sz' equals zero */

void **msetP(void **d, void *c, int sz)
{
	void **d = (void**)dest;

	for(; d != (void*)(dest)+sz; ++d)
		*d = c;

	return (void**)dest;
}

int mcnt(unsigned char *blk, unsigned char c, int size)
{
	unsigned char* b = blk;
	int nlines = 0;

	for(; b != blk+size; ++b)
		if (*b == c)
			++nlines;

	return nlines;
}
