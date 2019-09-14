/* Include the SDL main definition header */
#define SDL_DISABLE_IMMINTRIN_H
#include "SDL_main.h"

#ifdef main
#undef main
int main(int argc, char *argv[])
{
	return(SDL_main(argc, argv));
}
#else
/* Nothing to do on this platform */;
#endif
