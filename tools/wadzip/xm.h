void *xm(size_t size, size_t nmemb);
void *xr(void *p, size_t size, size_t nmemb);
void *xpnd(void *p, int nit, int *sit, size_t sz);
#define XPND(p, num, spc) (p = xpnd(p, num, &spc, sizeof *p))
