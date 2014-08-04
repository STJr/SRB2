#ifndef __FILTERS_H__
#define __FILTERS_H__

#ifdef _MSC_VER
#pragma warning(disable : 4514 4214 4244)
#endif

#include "SDL.h"

#ifdef _MSC_VER
#pragma warning(default : 4214 4244)
#endif

typedef enum {
  FILTER_2XSAI  = 0,
  FILTER_SUPER2XSAI,
  FILTER_SUPEREAGLE,
  FILTER_ADVMAME2X ,
  FILTER_TV2X      ,
  FILTER_NORMAL2X  ,
  FILTER_BILINEAR  ,
  FILTER_DOTMATRIX ,
  FILTER_NUM       ,
} t_filter;

typedef void (*filter_2)(Uint8 *srcPtr, Uint32 srcPitch, Uint8 *dstPtr, Uint32 dstPitch, int width, int height);
SDL_Surface *filter_2x(SDL_Surface *src, SDL_Rect *srcclp, filter_2 filter);
SDL_Surface *filter_2xe(SDL_Surface *src, SDL_Rect *srcclp, filter_2 filter,Uint8 R, Uint8 G, Uint8 B);
//Alam_GBC: Header file based on sms_sdl's filter.h
//Note: need 3 lines at the bottom and top?

//int filter_init_2xsai(SDL_PixelFormat *BitFormat);
#define FILTER(src,dst) (Uint8 *)(src->pixels)+src->pitch*3, (Uint32)src->pitch, (Uint8 *)dst->pixels, (Uint32)dst->pitch, src->w, src->h-6
#define SDLFILTER(src,dst) (Uint8 *)src->pixels, (Uint32)src->pitch, (Uint8 *)dst->pixels, (Uint32)dst->pitch, src->w, src->h
int filter_init_2xsai(SDL_PixelFormat *BitFormat); //unless?
void filter_scan50(Uint8 *srcPtr, Uint32 srcPitch, Uint8 *dstPtr, Uint32 dstPitch, int width, int height);
void filter_scan100(Uint8 *srcPtr, Uint32 srcPitch, Uint8 *dstPtr, Uint32 dstPitch,  int width, int height);

void filter_2xsai(Uint8 *srcPtr, Uint32 srcPitch, Uint8 *dstPtr, Uint32 dstPitch, int width, int height);
void filter_super2xsai(Uint8 *srcPtr, Uint32 srcPitch, Uint8 *dstPtr, Uint32 dstPitch, int width, int height);
void filter_supereagle(Uint8 *srcPtr, Uint32 srcPitch, Uint8 *dstPtr, Uint32 dstPitch, int width, int height);
void filter_advmame2x(Uint8 *srcPtr, Uint32 srcPitch, Uint8 *dstPtr, Uint32 dstPitch, int width, int height);
void filter_tv2x(Uint8 *srcPtr, Uint32 srcPitch, Uint8 *dstPtr, Uint32 dstPitch, int width, int height);
void filter_normal2x(Uint8 *srcPtr, Uint32 srcPitch, Uint8 *dstPtr, Uint32 dstPitch, int width, int height);
void filter_bilinear(Uint8 *srcPtr, Uint32 srcPitch, Uint8 *dstPtr, Uint32 dstPitch, int width, int height);
void filter_dotmatrix(Uint8 *srcPtr, Uint32 srcPitch, Uint8 *dstPtr, Uint32 dstPitch, int width, int height);
void filter_bicubic(Uint8 *srcPtr, Uint32 srcPitch, Uint8 *dstPtr, Uint32 dstPitch, int width, int height);
void lq2x16(Uint8 *srcPtr, Uint32 srcPitch, Uint8 *dstPtr, Uint32 dstPitch, int width, int height);
void hq2x16(Uint8 *srcPtr, Uint32 srcPitch, Uint8 *dstPtr, Uint32 dstPitch, int width, int height);

void filter_hq2x(Uint8 *srcPtr, Uint32 srcPitch, Uint8 *dstPtr, Uint32 dstPitch, int width, int height);
void lq2x32(Uint8 *srcPtr, Uint32 srcPitch, Uint8 *dstPtr, Uint32 dstPitch, int width, int height);
void hq2x32(Uint8 *srcPtr, Uint32 srcPitch, Uint8 *dstPtr, Uint32 dstPitch, int width, int height);

#ifdef FILTERS
typedef struct filter_s {  filter_2 filter; int bpp; } filter_t;
#define NUMFILTERS 13
static filter_t filtermode[NUMFILTERS+1] = {
	{NULL             ,  0}, //None
	{filter_normal2x  , 16}, //2xNormal
	{filter_advmame2x , 16}, //AdvMAME2x
	{filter_tv2x      , 16}, //TV2x
	{filter_bilinear  , 16}, //Bilinear
	{filter_dotmatrix , 16}, //DotMatrix
	{lq2x16           , 16}, //16LQ2x
	{hq2x16           , 16}, //16HQ2x
	{lq2x32           , 32}, //32LQ2x
	{hq2x32           , 32}, //32HQ2x
// {filter_bicubic   , 16}, //Slow Bicubic
	// BAD
	{filter_2xsai     , 16}, //2xSAI
	{filter_super2xsai, 16}, //Super2xSAI
	{filter_supereagle, 16}, //SuperEagle
};
CV_PossibleValue_t CV_Filters[] = {{ 0, "None"}, { 1, "2xNormal"},
 { 2, "AdvMAME2x"}, { 3, "TV2x"},       { 4, "Bilinear"}  , { 5, "DotMatrix"},
 { 6, "16LQ2x"},    { 7, "16HQ2x"},     { 8, "32LQ2x"}    , { 9, "32HQ2x"},
 {10, "2xSAI"},     {11, "Super2xSAI"}, {12, "SuperEagle"}, {0, NULL},};
static void Filterchange(void);
consvar_t cv_filter = {"filter", "None", CV_CALL|CV_NOINIT, CV_Filters,Filterchange,0,NULL,NULL,0,0,NULL};
static filter_2 blitfilter = NULL;
static SDL_Surface *preSurface = NULL;
static SDL_Surface *f2xSurface = NULL;

static void Filterchange(void)
{
	if(blitfilter) // only filtering?
	{
		int i=0;
		for(;i < NUMFILTERS; i++)//find old filter
		{
			if(filtermode[i].filter == blitfilter) //Found it
				break; //Stop
		}
		if(i < NUMFILTERS && filtermode[i].bpp == filtermode[cv_filter.value].bpp) //Easy to swap?
			blitfilter = filtermode[cv_filter.value].filter; // Swap with new filter
	}
}

FUNCINLINE static ATTRINLINE void FilterBlit(SDL_Surface *froSurface)
{
	if(froSurface && blitfilter && preSurface && f2xSurface)
	{
		SDL_Rect dstclp = {0,3,0,0};
		int lockedpre = 0, lockedf2x = 0, blitpre = 0;
		blitpre = SDL_BlitSurface(froSurface,NULL,preSurface,&dstclp);
		if(SDL_MUSTLOCK(preSurface)) lockedpre = SDL_LockSurface(preSurface);
		if(SDL_MUSTLOCK(f2xSurface)) lockedf2x = SDL_LockSurface(f2xSurface);
		if(lockedpre == 0 && preSurface->pixels && lockedf2x == 0 && f2xSurface->pixels && blitpre == 0)
		{
			blitfilter(FILTER(preSurface,f2xSurface));
			if(SDL_MUSTLOCK(preSurface)) SDL_UnlockSurface(preSurface);
			if(SDL_MUSTLOCK(f2xSurface)) SDL_UnlockSurface(f2xSurface);
		}
	}
	else
	{
		blitfilter = NULL;
		if(preSurface) SDL_FreeSurface(preSurface);
		preSurface = NULL;
		if(f2xSurface) SDL_FreeSurface(f2xSurface);
		f2xSurface = NULL;
	}
}

FUNCINLINE static ATTRINLINE int Setupf2x(int width, int height, int bpp)
{
	blitfilter = NULL;
	if(preSurface) SDL_FreeSurface(preSurface);
	preSurface = NULL;
	if(f2xSurface) SDL_FreeSurface(f2xSurface);
	f2xSurface = NULL;
	if( !(width%2) && !(height%2) && width >= BASEVIDWIDTH*2 && height >=  BASEVIDHEIGHT*2 && cv_filter.value
	 && cv_filter.value <= NUMFILTERS && filtermode[cv_filter.value].filter && filtermode[cv_filter.value].bpp)
	{
		int hwidth  =  width/2 + 6;
		int heighth = height/2 + 6;
		int hbpp = filtermode[cv_filter.value].bpp;
		switch(hbpp)
		{
			case 8:
				preSurface = SDL_CreateRGBSurface(SDL_SWSURFACE,hwidth,heighth, 8,0x00000000,0x00000000,0x00000000,0x00);
				f2xSurface = SDL_CreateRGBSurface(SDL_HWSURFACE, width,height , 8,0x00000000,0x00000000,0x00000000,0x00);
			case 15:
				preSurface = SDL_CreateRGBSurface(SDL_SWSURFACE,hwidth,heighth,15,0x00007C00,0x000003E0,0x0000001F,0x00);
				f2xSurface = SDL_CreateRGBSurface(SDL_HWSURFACE, width,height ,15,0x00007C00,0x000003E0,0x0000001F,0x00);
				break;
			case 16:
				preSurface = SDL_CreateRGBSurface(SDL_SWSURFACE,hwidth,heighth,16,0x0000F800,0x000007E0,0x0000001F,0x00);
				f2xSurface = SDL_CreateRGBSurface(SDL_HWSURFACE, width,height ,16,0x0000F800,0x000007E0,0x0000001F,0x00);
				break;
			case 24:
				preSurface = SDL_CreateRGBSurface(SDL_SWSURFACE,hwidth,heighth,24,0x00FF0000,0x0000FF00,0x000000FF,0x00);
				f2xSurface = SDL_CreateRGBSurface(SDL_HWSURFACE, width,height ,24,0x00FF0000,0x0000FF00,0x000000FF,0x00);
				break;
			case 32:
				preSurface = SDL_CreateRGBSurface(SDL_SWSURFACE,hwidth,heighth,32,0x00FF0000,0x0000FF00,0x000000FF,0x00);
				f2xSurface = SDL_CreateRGBSurface(SDL_HWSURFACE, width,height ,32,0x00FF0000,0x0000FF00,0x000000FF,0x00);
				break;
			default:
				//I_Error("Filter help");
				break;
		}
		if(preSurface && f2xSurface)
		{
			blitfilter = filtermode[cv_filter.value].filter;
			if(bpp < hbpp) bpp = hbpp;
		}
		else
		{
			if(preSurface) SDL_FreeSurface(preSurface);
			preSurface = NULL;
			if(f2xSurface) SDL_FreeSurface(f2xSurface);
			f2xSurface = NULL;
		}
	}
	return bpp;
}
#else

#ifdef __GNUC__ // __attribute__ ((X))
#if (__GNUC__ > 3) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 1)
#define FUNCINLINE __attribute__((always_inline))
#endif
#define FUNCNOINLINE __attribute__((noinline))
#elif defined(_MSC_VER)
#define inline __inline
#define ATTRNORETURN   __declspec(noreturn)
#define ATTRINLINE __forceinline
#if _MSC_VER > 1200
#define ATTRNOINLINE __declspec(noinline)
#endif
#endif



#ifndef FUNCINLINE
#define FUNCINLINE
#endif
#ifndef FUNCNOINLINE
#define FUNCNOINLINE
#endif
#ifndef ATTRINLINE
#define ATTRINLINE inline
#endif
#ifndef ATTRNOINLINE
#define ATTRNOINLINE
#endif

#endif

#endif
