/* Fail-proof memory allocation by Graue <graue@oceanbase.org>; public domain */

#include <stdlib.h>
#include <limits.h>
#include "err.h"

/* MinGW (for example) appears to lack SIZE_T_MAX. */
#ifndef SIZE_T_MAX
#define SIZE_T_MAX ULONG_MAX
#endif

/*
 * The usual xmalloc and xrealloc type routines, for allocating memory
 * and bombing out if it fails.
 *
 * xm and xr here take two values: size and nmemb, like fread/fwrite.
 * The idea is to be able to check for overflow, to prevent some weird
 * crashes and security problems that can arise if you multiply size
 * and nmemb and they overflow.
 */

/* FIXME: use %zu (size_t) in these format arguments if library is C99 */

/*
 * Multiply size and number of elements, bombing out if overflow would
 * occur.

 * Note: In the case where size*nmemb is exactly equal to SIZE_T_MAX,
 * this function considers that an overflow, even though it isn't
 * really. But it's not likely the program will get that much memory,
 * anyway.
 */
static size_t sizmul(size_t size, size_t nmemb)
{
	if (SIZE_T_MAX / size <= nmemb)
	{
		errx(1, "allocating %lu objects of size %lu "
			"would overflow", (unsigned long)nmemb,
			(unsigned long)size);
	}
	return size * nmemb; /* won't overflow */
}

static void nomem(size_t size, size_t nmemb)
{
	err(1, "cannot allocate %lu objects of size %lu "
		"(total %lu bytes)", (unsigned long)nmemb,
		(unsigned long)size, (unsigned long)(nmemb*size));
}

void *xm(size_t size, size_t nmemb)
{
	const size_t bytes = sizmul(size, nmemb);
	void *p;

	p = malloc(bytes);
	if (p == NULL) nomem(size, nmemb);
	return p;
}

void *xr(void *p, size_t size, size_t nmemb)
{
	const size_t bytes = sizmul(size, nmemb);

	p = realloc(p, bytes);
	if (p == NULL) nomem(size, nmemb);
	return p;
}

/* expand array */
void *xpnd(void *p, int nit, int *sit, size_t sz)
{
	if (nit < *sit) return p;
	else if (*sit > 0) return xr(p, sz, (*sit *= 2));
	else return xm(sz, (*sit = 10));
}
