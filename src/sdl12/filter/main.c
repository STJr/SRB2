#include "filters.h"

int main(int argc, char *argv[])
{
	SDL_Surface *src = NULL;
	SDL_Surface *dst = NULL;
	src = SDL_LoadBMP("src.bmp"); //load
	if(!src) return -1; //check
	dst = filter_2x(src, NULL, hq2x32); //prcoess
	SDL_FreeSurface(src); //free
	if(!dst) return 0; //error
	SDL_SaveBMP(dst, "dst.bmp"); //save
	SDL_FreeSurface(dst); //free
	return 1; //good
}
