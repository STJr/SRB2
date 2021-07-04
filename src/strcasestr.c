/*
strcasestr -- case insensitive substring searching function.
*/
/*
Copyright 2019-2020 James R.
All rights reserved.

Redistribution and use in source forms, with or without modification, is
permitted provided that the following condition is met:

1. Redistributions of source code must retain the above copyright notice, this
   condition and the following disclaimer.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

static inline int
trycmp (char **pp, char *cp,
		const char *q, size_t qn)
{
	char *p;
	p = (*pp);
	if (strncasecmp(p, q, qn) == 0)
		return 0;
	(*pp) = strchr(&p[1], (*cp));
	return 1;
}

static inline void
swapp (char ***ppap, char ***ppbp, char **cpap, char **cpbp)
{
	char **pp;
	char  *p;

	pp    = *ppap;
	*ppap = *ppbp;
	*ppbp =  pp;

	p     = *cpap;
	*cpap = *cpbp;
	*cpbp =   p;
}

char *
strcasestr (const char *s, const char *q)
{
	size_t  qn;

	char    uc;
	char    lc;

	char   *up;
	char   *lp;

	char **ppa;
	char **ppb;

	char  *cpa;
	char  *cpb;

	uc = toupper(*q);
	lc = tolower(*q);

	up = strchr(s, uc);
	lp = strchr(s, lc);

	if (!( (intptr_t)up|(intptr_t)lp ))
		return 0;

	if (!lp || ( up && up < lp ))
	{
		ppa = &up;
		ppb = &lp;

		cpa = &uc;
		cpb = &lc;
	}
	else
	{
		ppa = &lp;
		ppb = &up;

		cpa = &lc;
		cpb = &uc;
	}

	qn = strlen(q);

	for (;;)
	{
		if (trycmp(ppa, cpa, q, qn) == 0)
			return (*ppa);

		if (!( (intptr_t)up|(intptr_t)lp ))
			break;

		if (!(*ppa) || ( (*ppb) && (*ppb) < (*ppa) ))
			swapp(&ppa, &ppb, &cpa, &cpb);
	}

	return 0;
}
