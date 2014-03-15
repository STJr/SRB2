#include <math.h>
#include "filters.h"

//Alam_GBC: C file based on sms_sdl's filter.c

/* 2X SAI Filter */
static Uint32 colorMask = 0xF7DEF7DE;
static Uint32 lowPixelMask = 0x08210821;
static Uint32 qcolorMask = 0xE79CE79C;
static Uint32 qlowpixelMask = 0x18631863;
static Uint32 redblueMask = 0xF81F;
static Uint32 greenMask = 0x7E0;

SDL_Surface *filter_2x(SDL_Surface *src, SDL_Rect *srcclp, filter_2 filter)
{
	return filter_2xe(src,srcclp,filter,0,0,0);
}

SDL_Surface *filter_2xe(SDL_Surface *src, SDL_Rect *srcclp, filter_2 filter,Uint8 R, Uint8 G, Uint8 B)
{
	SDL_Surface *srcfilter = NULL;
	SDL_Rect dstclp = {0,3,0,0};
	SDL_Surface *dstfilter = NULL;
	Uint32 Fillcolor = 0;
	if(!src || !filter) return NULL; // Need src and filter
	if(srcclp) // size by clp
	{
		dstclp.w = srcclp->w; //clp's width
		dstclp.h = srcclp->h; //clp's height
	}
	else // size by src
	{
		dstclp.w = (Uint16)src->w; //src's width
		dstclp.h = (Uint16)src->h; //src's height
	}
	if(filter == hq2x32 || filter == lq2x32) // src 0888 surface
		srcfilter = SDL_CreateRGBSurface(SDL_SWSURFACE,dstclp.w,dstclp.h+6,32,0x00FF0000,0x0000FF00,0x000000FF,0x00);
	else // src 565 surface
		srcfilter = SDL_CreateRGBSurface(SDL_SWSURFACE,dstclp.w,dstclp.h+6,16,0x0000F800,0x000007E0,0x0000001F,0x00);
	if(!srcfilter) return NULL; //No Memory?
	Fillcolor = SDL_MapRGB(srcfilter->format,R,G,B); //Choose color
	SDL_FillRect(srcfilter,NULL,Fillcolor); //fill it
	if(filter == filter_hq2x || filter == hq2x32 || filter == lq2x32) // dst 0888 surface
		dstfilter = SDL_CreateRGBSurface(SDL_SWSURFACE,dstclp.w*2,dstclp.h*2,32,0x00FF0000,0x0000FF00,0x000000FF,0x00);
	else // dst 565 surface
		dstfilter = SDL_CreateRGBSurface(SDL_SWSURFACE,dstclp.w*2,dstclp.h*2,16,0x0000F800,0x000007E0,0x0000001F,0x00);
	if(!dstfilter || SDL_BlitSurface(src,srcclp,srcfilter,&dstclp) == -1) // No dstfilter or Blit failed
	{
		SDL_FreeSurface(srcfilter); // Free memory
		return NULL; //No Memory?
	}
	else // have dstfilter ready and srcfilter done
	{
		SDL_FillRect(dstfilter,NULL,Fillcolor); //fill it too
		filter(FILTER(srcfilter,dstfilter)); //filtering
		SDL_FreeSurface(srcfilter); //almost
	}
	return dstfilter; //done
}


int filter_init_2xsai(SDL_PixelFormat *BitFormat)
{
	if (!BitFormat || BitFormat->BytesPerPixel != 2 ||BitFormat->Amask != 0x0)
	{
		return 0;
	}
	else if (BitFormat->Rmask == 0xF800 && BitFormat->Gmask == 0x7E0
		&& BitFormat->Bmask == 0x1F && BitFormat->BitsPerPixel == 16) //565
	{
		colorMask = 0xF7DEF7DE;
		lowPixelMask = 0x08210821;
		qcolorMask = 0xE79CE79C;
		qlowpixelMask = 0x18631863;
		redblueMask = 0xF81F;
		greenMask = 0x7E0;
	}
	else if (BitFormat->Rmask == 0x7C00 && BitFormat->Gmask == 0x3E0
		&& BitFormat->Bmask == 0x1F && BitFormat->BitsPerPixel == 15) //555
	{
		colorMask = 0x7BDE7BDE;
		lowPixelMask = 0x04210421;
		qcolorMask = 0x739C739C;
		qlowpixelMask = 0x0C630C63;
		redblueMask = 0x7C1F;
		greenMask = 0x3E0;
	}
	else
	{
		return 0;
	}
#ifdef MMX
	if(BitFormat->Gmask == 0x7E0) Init_2xSaIMMX(565);
	else Init_2xSaIMMX(555);
#endif
	return 1;
}


FUNCINLINE static ATTRINLINE int GetResult1 (Uint32 A, Uint32 B, Uint32 C, Uint32 D, Uint32 E)
{
  int x = 0;
  int y = 0;
  int r = 0;
  (void)E;

  if (A == C)
	x += 1;
  else if (B == C)
	  y += 1;
  if (A == D)
    x += 1;
  else if (B == D)
	  y += 1;
  if (x <= 1)
	  r += 1;
  if (y <= 1)
	  r -= 1;
  return r;
}

FUNCINLINE static ATTRINLINE int GetResult2 (Uint32 A, Uint32 B, Uint32 C, Uint32 D, Uint32 E)
{
  int x = 0;
  int y = 0;
  int r = 0;
  (void)E;

  if (A == C)
	  x += 1;
  else if (B == C)
    y += 1;
  if (A == D)
    x += 1;
  else if (B == D)
    y += 1;
  if (x <= 1)
    r -= 1;
  if (y <= 1)
    r += 1;
  return r;
}

FUNCINLINE static ATTRINLINE int GetResult (Uint32 A, Uint32 B, Uint32 C, Uint32 D)
{
  int x = 0;
  int y = 0;
  int r = 0;

  if (A == C)
    x += 1;
  else if (B == C)
    y += 1;
  if (A == D)
    x += 1;
  else if (B == D)
    y += 1;
  if (x <= 1)
    r += 1;
  if (y <= 1)
    r -= 1;
  return r;
}

FUNCINLINE static ATTRINLINE Uint32 INTERPOLATE (Uint32 A, Uint32 B)
{
  if (A != B)
  {
    return (((A & colorMask) >> 1) + ((B & colorMask) >> 1) +
		(A & B & lowPixelMask));
  }
  else
	  return A;
}

FUNCINLINE static ATTRINLINE Uint32 Q_INTERPOLATE (Uint32 A, Uint32 B, Uint32 C, Uint32 D)
{
  register Uint32 x = ((A & qcolorMask) >> 2) +
	((B & qcolorMask) >> 2) +
	((C & qcolorMask) >> 2) + ((D & qcolorMask) >> 2);
  register Uint32 y = (A & qlowpixelMask) +
	(B & qlowpixelMask) + (C & qlowpixelMask) + (D & qlowpixelMask);
  y = (y >> 2) & qlowpixelMask;
  return x + y;
}

#define BLUE_MASK565 0x001F001F
#define RED_MASK565 0xF800F800
#define GREEN_MASK565 0x07E007E0

#define BLUE_MASK555 0x001F001F
#define RED_MASK555 0x7C007C00
#define GREEN_MASK555 0x03E003E0

void filter_super2xsai(Uint8 *srcPtr, Uint32 srcPitch,
		  Uint8 *dstPtr, Uint32 dstPitch,
		 int width, int height)
{
	Uint16 *bP;
    Uint8  *dP;
    Uint32 inc_bP;
    Uint32 Nextline = srcPitch >> 1;

	Uint32 finish;
	inc_bP = 1;

	for (; height; height--)
	{
	    bP = (Uint16 *) srcPtr;
	    dP = (Uint8 *) dstPtr;

	    for (finish = width; finish; finish -= inc_bP)
	    {
		Uint32 color4, color5, color6;
		Uint32 color1, color2, color3;
		Uint32 colorA0, colorA1, colorA2, colorA3,
		    colorB0, colorB1, colorB2, colorB3, colorS1, colorS2;
		Uint32 product1a, product1b, product2a, product2b;

//---------------------------------------    B1 B2
//                                         4  5  6 S2
//                                         1  2  3 S1
//                                           A1 A2

		colorB0 = *(bP - Nextline - 1);
		colorB1 = *(bP - Nextline);
		colorB2 = *(bP - Nextline + 1);
		colorB3 = *(bP - Nextline + 2);

		color4 = *(bP - 1);
		color5 = *(bP);
		color6 = *(bP + 1);
		colorS2 = *(bP + 2);

		color1 = *(bP + Nextline - 1);
		color2 = *(bP + Nextline);
		color3 = *(bP + Nextline + 1);
		colorS1 = *(bP + Nextline + 2);

		colorA0 = *(bP + Nextline + Nextline - 1);
		colorA1 = *(bP + Nextline + Nextline);
		colorA2 = *(bP + Nextline + Nextline + 1);
		colorA3 = *(bP + Nextline + Nextline + 2);

//--------------------------------------
		if (color2 == color6 && color5 != color3)
		{
		    product2b = product1b = color2;
		}
		else if (color5 == color3 && color2 != color6)
		{
		    product2b = product1b = color5;
		}
		else if (color5 == color3 && color2 == color6)
		{
		    register int r = 0;

		    r += GetResult (color6, color5, color1, colorA1);
		    r += GetResult (color6, color5, color4, colorB1);
		    r += GetResult (color6, color5, colorA2, colorS1);
		    r += GetResult (color6, color5, colorB2, colorS2);

		    if (r > 0)
			product2b = product1b = color6;
		    else if (r < 0)
			product2b = product1b = color5;
		    else
		    {
			product2b = product1b = INTERPOLATE (color5, color6);
		    }
		}
		else
		{
		    if (color6 == color3 && color3 == colorA1
			    && color2 != colorA2 && color3 != colorA0)
			product2b =
			    Q_INTERPOLATE (color3, color3, color3, color2);
		    else if (color5 == color2 && color2 == colorA2
			     && colorA1 != color3 && color2 != colorA3)
			product2b =
			    Q_INTERPOLATE (color2, color2, color2, color3);
		    else
			product2b = INTERPOLATE (color2, color3);

		    if (color6 == color3 && color6 == colorB1
			    && color5 != colorB2 && color6 != colorB0)
			product1b =
			    Q_INTERPOLATE (color6, color6, color6, color5);
		    else if (color5 == color2 && color5 == colorB2
			     && colorB1 != color6 && color5 != colorB3)
			product1b =
			    Q_INTERPOLATE (color6, color5, color5, color5);
		    else
			product1b = INTERPOLATE (color5, color6);
		}

		if (color5 == color3 && color2 != color6 && color4 == color5
			&& color5 != colorA2)
		    product2a = INTERPOLATE (color2, color5);
		else
		    if (color5 == color1 && color6 == color5
			&& color4 != color2 && color5 != colorA0)
		    product2a = INTERPOLATE (color2, color5);
		else
		    product2a = color2;

		if (color2 == color6 && color5 != color3 && color1 == color2
			&& color2 != colorB2)
		    product1a = INTERPOLATE (color2, color5);
		else
		    if (color4 == color2 && color3 == color2
			&& color1 != color5 && color2 != colorB0)
		    product1a = INTERPOLATE (color2, color5);
		else
		    product1a = color5;

#ifdef LSB_FIRST
		product1a = product1a | (product1b << 16);
		product2a = product2a | (product2b << 16);
#else
    product1a = (product1a << 16) | product1b;
    product2a = (product2a << 16) | product2b;
#endif
		*((Uint32 *) dP) = product1a;
		*((Uint32 *) (dP + dstPitch)) = product2a;

		bP += inc_bP;
		dP += sizeof (Uint32);
	    }			// end of for ( finish= width etc..)

	    srcPtr   += srcPitch;
	    dstPtr   += dstPitch * 2;
	}			// endof: for (; height; height--)
}

void filter_supereagle(Uint8 *srcPtr, Uint32 srcPitch, /* Uint8 *deltaPtr,  */
		 Uint8 *dstPtr, Uint32 dstPitch, int width, int height)
{
    Uint8  *dP;
    Uint16 *bP;
    Uint32 inc_bP;



	Uint32 finish;
	Uint32 Nextline = srcPitch >> 1;

	inc_bP = 1;

	for (; height ; height--)
	{
	    bP = (Uint16 *) srcPtr;
	    dP = dstPtr;
	    for (finish = width; finish; finish -= inc_bP)
	    {
		Uint32 color4, color5, color6;
		Uint32 color1, color2, color3;
		Uint32 colorA1, colorA2, colorB1, colorB2, colorS1, colorS2;
		Uint32 product1a, product1b, product2a, product2b;
		colorB1 = *(bP - Nextline);
		colorB2 = *(bP - Nextline + 1);

		color4 = *(bP - 1);
		color5 = *(bP);
		color6 = *(bP + 1);
		colorS2 = *(bP + 2);

		color1 = *(bP + Nextline - 1);
		color2 = *(bP + Nextline);
		color3 = *(bP + Nextline + 1);
		colorS1 = *(bP + Nextline + 2);

		colorA1 = *(bP + Nextline + Nextline);
		colorA2 = *(bP + Nextline + Nextline + 1);
		// --------------------------------------
		if (color2 == color6 && color5 != color3)
		{
		    product1b = product2a = color2;
		    if ((color1 == color2) || (color6 == colorB2))
		    {
			product1a = INTERPOLATE (color2, color5);
			product1a = INTERPOLATE (color2, product1a);
//                       product1a = color2;
		    }
		    else
		    {
			product1a = INTERPOLATE (color5, color6);
		    }

		    if ((color6 == colorS2) || (color2 == colorA1))
		    {
			product2b = INTERPOLATE (color2, color3);
			product2b = INTERPOLATE (color2, product2b);
//                       product2b = color2;
		    }
		    else
		    {
			product2b = INTERPOLATE (color2, color3);
		    }
		}
		else if (color5 == color3 && color2 != color6)
		{
		    product2b = product1a = color5;

		    if ((colorB1 == color5) || (color3 == colorS1))
		    {
			product1b = INTERPOLATE (color5, color6);
			product1b = INTERPOLATE (color5, product1b);
//                       product1b = color5;
		    }
		    else
		    {
			product1b = INTERPOLATE (color5, color6);
		    }

		    if ((color3 == colorA2) || (color4 == color5))
		    {
			product2a = INTERPOLATE (color5, color2);
			product2a = INTERPOLATE (color5, product2a);
//                       product2a = color5;
		    }
		    else
		    {
			product2a = INTERPOLATE (color2, color3);
		    }

		}
		else if (color5 == color3 && color2 == color6)
		{
		    register int r = 0;

		    r += GetResult (color6, color5, color1, colorA1);
		    r += GetResult (color6, color5, color4, colorB1);
		    r += GetResult (color6, color5, colorA2, colorS1);
		    r += GetResult (color6, color5, colorB2, colorS2);

		    if (r > 0)
		    {
			product1b = product2a = color2;
			product1a = product2b = INTERPOLATE (color5, color6);
		    }
		    else if (r < 0)
		    {
			product2b = product1a = color5;
			product1b = product2a = INTERPOLATE (color5, color6);
		    }
		    else
		    {
			product2b = product1a = color5;
			product1b = product2a = color2;
		    }
		}
		else
		{
		    product2b = product1a = INTERPOLATE (color2, color6);
		    product2b =
			Q_INTERPOLATE (color3, color3, color3, product2b);
		    product1a =
			Q_INTERPOLATE (color5, color5, color5, product1a);

		    product2a = product1b = INTERPOLATE (color5, color3);
		    product2a =
			Q_INTERPOLATE (color2, color2, color2, product2a);
		    product1b =
			Q_INTERPOLATE (color6, color6, color6, product1b);

//                    product1a = color5;
//                    product1b = color6;
//                    product2a = color2;
//                    product2b = color3;
		}
#ifdef LSB_FIRST
		product1a = product1a | (product1b << 16);
		product2a = product2a | (product2b << 16);
#else
    product1a = (product1a << 16) | product1b;
    product2a = (product2a << 16) | product2b;
#endif

		*((Uint32 *) dP) = product1a;
		*((Uint32 *) (dP + dstPitch)) = product2a;

		bP += inc_bP;
		dP += sizeof (Uint32);
	    }			// end of for ( finish= width etc..)
	    srcPtr += srcPitch;
	    dstPtr += dstPitch * 2;
	}			// endof: for (height; height; height--)
}

void filter_2xsai (Uint8 *srcPtr, Uint32 srcPitch,
	     Uint8 *dstPtr, Uint32 dstPitch, int width, int height)
{
    Uint8  *dP;
    Uint16 *bP;
    Uint32 inc_bP;


	Uint32 finish;
	Uint32 Nextline = srcPitch >> 1;
	inc_bP = 1;


	for (; height; height--)
	{
	    bP = (Uint16 *) srcPtr;
	    dP = dstPtr;

	    for (finish = width; finish; finish -= inc_bP)
	    {

		register Uint32 colorA, colorB;
		Uint32 colorC, colorD,
		    colorE, colorF, colorG, colorH,
		    colorI, colorJ, colorK, colorL,

		    colorM, colorN, colorO, colorP;
		Uint32 product, product1, product2;

//---------------------------------------
// Map of the pixels:                    I|E F|J
//                                       G|A B|K
//                                       H|C D|L
//                                       M|N O|P
		colorI = *(bP - Nextline - 1);
		colorE = *(bP - Nextline);
		colorF = *(bP - Nextline + 1);
		colorJ = *(bP - Nextline + 2);

		colorG = *(bP - 1);
		colorA = *(bP);
		colorB = *(bP + 1);
		colorK = *(bP + 2);

		colorH = *(bP + Nextline - 1);
		colorC = *(bP + Nextline);
		colorD = *(bP + Nextline + 1);
		colorL = *(bP + Nextline + 2);

		colorM = *(bP + Nextline + Nextline - 1);
		colorN = *(bP + Nextline + Nextline);
		colorO = *(bP + Nextline + Nextline + 1);
		colorP = *(bP + Nextline + Nextline + 2);

		if ((colorA == colorD) && (colorB != colorC))
		{
		    if (((colorA == colorE) && (colorB == colorL)) ||
			    ((colorA == colorC) && (colorA == colorF)
			     && (colorB != colorE) && (colorB == colorJ)))
		    {
			product = colorA;
		    }
		    else
		    {
			product = INTERPOLATE (colorA, colorB);
		    }

		    if (((colorA == colorG) && (colorC == colorO)) ||
			    ((colorA == colorB) && (colorA == colorH)
			     && (colorG != colorC) && (colorC == colorM)))
		    {
			product1 = colorA;
		    }
		    else
		    {
			product1 = INTERPOLATE (colorA, colorC);
		    }
		    product2 = colorA;
		}
		else if ((colorB == colorC) && (colorA != colorD))
		{
		    if (((colorB == colorF) && (colorA == colorH)) ||
			    ((colorB == colorE) && (colorB == colorD)
			     && (colorA != colorF) && (colorA == colorI)))
		    {
			product = colorB;
		    }
		    else
		    {
			product = INTERPOLATE (colorA, colorB);
		    }

		    if (((colorC == colorH) && (colorA == colorF)) ||
			    ((colorC == colorG) && (colorC == colorD)
			     && (colorA != colorH) && (colorA == colorI)))
		    {
			product1 = colorC;
		    }
		    else
		    {
			product1 = INTERPOLATE (colorA, colorC);
		    }
		    product2 = colorB;
		}
		else if ((colorA == colorD) && (colorB == colorC))
		{
		    if (colorA == colorB)
		    {
			product = colorA;
			product1 = colorA;
			product2 = colorA;
		    }
		    else
		    {
			register int r = 0;

			product1 = INTERPOLATE (colorA, colorC);
			product = INTERPOLATE (colorA, colorB);

			r +=
			    GetResult1 (colorA, colorB, colorG, colorE,
					colorI);
			r +=
			    GetResult2 (colorB, colorA, colorK, colorF,
					colorJ);
			r +=
			    GetResult2 (colorB, colorA, colorH, colorN,
					colorM);
			r +=
			    GetResult1 (colorA, colorB, colorL, colorO,
					colorP);

			if (r > 0)
			    product2 = colorA;
			else if (r < 0)
			    product2 = colorB;
			else
			{
			    product2 =
				Q_INTERPOLATE (colorA, colorB, colorC,
					       colorD);
			}
		    }
		}
		else
		{
		    product2 = Q_INTERPOLATE (colorA, colorB, colorC, colorD);

		    if ((colorA == colorC) && (colorA == colorF)
			    && (colorB != colorE) && (colorB == colorJ))
		    {
			product = colorA;
		    }
		    else
			if ((colorB == colorE) && (colorB == colorD)
			    && (colorA != colorF) && (colorA == colorI))
		    {
			product = colorB;
		    }
		    else
		    {
			product = INTERPOLATE (colorA, colorB);
		    }

		    if ((colorA == colorB) && (colorA == colorH)
			    && (colorG != colorC) && (colorC == colorM))
		    {
			product1 = colorA;
		    }
		    else
			if ((colorC == colorG) && (colorC == colorD)
			    && (colorA != colorH) && (colorA == colorI))
		    {
			product1 = colorC;
		    }
		    else
		    {
			product1 = INTERPOLATE (colorA, colorC);
		    }
		}
#ifdef LSB_FIRST
		product = colorA | (product << 16);
		product1 = product1 | (product2 << 16);
#else
    product = (colorA << 16) | product;
    product1 = (product1 << 16) | product2;
#endif
		*((Uint32 *) dP) = product;
		*((Uint32 *) (dP + dstPitch)) = product1;

		bP += inc_bP;
		dP += sizeof (Uint32);
	    }			// end of for ( finish= width etc..)

	    srcPtr += srcPitch;
	    dstPtr += dstPitch * 2;
	}			// endof: for (height; height; height--)
}

#if 0
static inline Uint32 Bilinear(Uint32 A, Uint32 B, Uint32 x)
{
    unsigned long areaA, areaB;
    unsigned long result;

    if (A == B)
	return A;

    areaB = (x >> 11) & 0x1f;	// reduce 16 bit fraction to 5 bits
    areaA = 0x20 - areaB;

    A = (A & redblueMask) | ((A & greenMask) << 16);
    B = (B & redblueMask) | ((B & greenMask) << 16);

    result = ((areaA * A) + (areaB * B)) >> 5;

    return (result & redblueMask) | ((result >> 16) & greenMask);

}

static inline Uint32 Bilinear4 (Uint32 A, Uint32 B, Uint32 C, Uint32 D, Uint32 x,
			 Uint32 y)
{
    unsigned long areaA, areaB, areaC, areaD;
    unsigned long result, xy;

    x = (x >> 11) & 0x1f;
    y = (y >> 11) & 0x1f;
    xy = (x * y) >> 5;

    A = (A & redblueMask) | ((A & greenMask) << 16);
    B = (B & redblueMask) | ((B & greenMask) << 16);
    C = (C & redblueMask) | ((C & greenMask) << 16);
    D = (D & redblueMask) | ((D & greenMask) << 16);

    areaA = 0x20 + xy - x - y;
    areaB = x - xy;
    areaC = y - xy;
    areaD = xy;

    result = ((areaA * A) + (areaB * B) + (areaC * C) + (areaD * D)) >> 5;

    return (result & redblueMask) | ((result >> 16) & greenMask);
}
#endif


void filter_advmame2x(Uint8 *srcPtr, Uint32 srcPitch,
                      Uint8 *dstPtr, Uint32 dstPitch,
							        int width, int height)
{
	unsigned int nextlineSrc = srcPitch / sizeof(short);
	short *p = (short *)srcPtr;

	unsigned int nextlineDst = dstPitch / sizeof(short);
	short *q = (short *)dstPtr;

	while(height--) {
    int i = 0, j = 0;
		for(i = 0; i < width; ++i, j += 2) {
			short B = *(p + i - nextlineSrc);
			short D = *(p + i - 1);
			short E = *(p + i);
			short F = *(p + i + 1);
			short H = *(p + i + nextlineSrc);

			*(q + j) = (short)(D == B && B != F && D != H ? D : E);
			*(q + j + 1) = (short)(B == F && B != D && F != H ? F : E);
			*(q + j + nextlineDst) = (short)(D == H && D != B && H != F ? D : E);
			*(q + j + nextlineDst + 1) = (short)(H == F && D != H && B != F ? F : E);
		}
		p += nextlineSrc;
		q += nextlineDst << 1;
	}
}


void filter_tv2x(Uint8 *srcPtr, Uint32 srcPitch,
                 Uint8 *dstPtr, Uint32 dstPitch,
                 int width, int height)
{
  unsigned int nextlineSrc = srcPitch / sizeof(Uint16);
	Uint16 *p = (Uint16 *)srcPtr;

	unsigned int nextlineDst = dstPitch / sizeof(Uint16);
	Uint16 *q = (Uint16 *)dstPtr;

	while(height--) {
    int i = 0, j = 0;
		for(; i < width; ++i, j += 2) {
			Uint16 p1 = *(p + i);
      Uint32 pi;

			pi = (((p1 & redblueMask) * 7) >> 3) & redblueMask;
			pi |= (((p1 & greenMask) * 7) >> 3) & greenMask;

      *(q + j) = (Uint16)p1;
      *(q + j + 1) = (Uint16)p1;
			*(q + j + nextlineDst) = (Uint16)pi;
			*(q + j + nextlineDst + 1) = (Uint16)pi;
	  }
	  p += nextlineSrc;
	  q += nextlineDst << 1;
	}
}

void filter_normal2x(Uint8 *srcPtr, Uint32 srcPitch,
               Uint8 *dstPtr, Uint32 dstPitch,
               int width, int height)
{
	unsigned int nextlineSrc = srcPitch / sizeof(Uint16);
	Uint16 *p = (Uint16 *)srcPtr;

	unsigned int nextlineDst = dstPitch / sizeof(Uint16);
	Uint16 *q = (Uint16 *)dstPtr;

	while(height--) {
	int i = 0, j = 0;
		for(; i < width; ++i, j += 2) {
			Uint16 color = *(p + i);

			*(q + j) = color;
			*(q + j + 1) = color;
			*(q + j + nextlineDst) = color;
			*(q + j + nextlineDst + 1) = color;
		}
		p += nextlineSrc;
		q += nextlineDst << 1;
	}
}

void filter_scan50(Uint8 *srcPtr, Uint32 srcPitch,
                   Uint8 *dstPtr, Uint32 dstPitch,
                   int width, int height)
{

  unsigned int nextlineSrc = srcPitch / sizeof(Uint16);
	Uint16 *p = (Uint16 *)srcPtr;

	unsigned int nextlineDst = dstPitch / sizeof(Uint16);
	Uint16 *q = (Uint16 *)dstPtr;

  while(height--) {
    int i = 0, j = 0;
    for(; i < width; ++i, j += 2) {
	    Uint16 p1 = *(p + i);
	    Uint16 p2 = *(p + i + nextlineSrc);
	    // 0111 1011 1110 1111 == 0x7BEF
      Uint16 pm = (Uint16)(((p1 + p2) >> 2) & 0x7BEF);

      *(q + j) = p1;
      *(q + j + 1) = p1;
			*(q + j + nextlineDst) = pm;
			*(q + j + nextlineDst + 1) = pm;

    }
		p += nextlineSrc;
		q += nextlineDst << 1;
  }
}


void filter_scan100(Uint8 *srcPtr, Uint32 srcPitch,
                    Uint8 *dstPtr, Uint32 dstPitch,
                    int width, int height)
{
  unsigned int nextlineSrc = srcPitch / sizeof(Uint16);
	Uint16 *p = (Uint16 *)srcPtr;

	unsigned int nextlineDst = dstPitch / sizeof(Uint16);
	Uint16 *q = (Uint16 *)dstPtr;

  while(height--) {
    int i = 0, j = 0;
    for(; i < width; ++i, j += 2) {
      *(q + j) = *(q + j + 1) = *(p + i);
    }
		p += nextlineSrc;
		q += nextlineDst << 1;
  }
}


FUNCINLINE static ATTRINLINE Uint16 DOT_16(Uint16 c, int j, int i) {
  static const Uint16 dotmatrix[16] = {
	  0x01E0, 0x0007, 0x3800, 0x0000,
	  0x39E7, 0x0000, 0x39E7, 0x0000,
	  0x3800, 0x0000, 0x01E0, 0x0007,
	  0x39E7, 0x0000, 0x39E7, 0x0000
  };
  return (Uint16)(c - ((c >> 2) & *(dotmatrix + ((j & 3) << 2) + (i & 3))));
}

void filter_dotmatrix(Uint8 *srcPtr, Uint32 srcPitch,
                      Uint8 *dstPtr, Uint32 dstPitch,
					            int width, int height)
{
	unsigned int nextlineSrc = srcPitch / sizeof(Uint16);
	Uint16 *p = (Uint16 *)srcPtr;

	unsigned int nextlineDst = dstPitch / sizeof(Uint16);
	Uint16 *q = (Uint16 *)dstPtr;

  int i, ii, j, jj;
	for(j = 0, jj = 0; j < height; ++j, jj += 2) {
		for(i = 0, ii = 0; i < width; ++i, ii += 2) {
			Uint16 c = *(p + i);
			*(q + ii) = DOT_16(c, jj, ii);
			*(q + ii + 1) = DOT_16(c, jj, ii + 1);
			*(q + ii + nextlineDst) = DOT_16(c, jj + 1, ii);
			*(q + ii + nextlineDst + 1) = DOT_16(c, jj + 1, ii + 1);
		}
		p += nextlineSrc;
		q += nextlineDst << 1;
	}
}


void filter_bilinear(Uint8 *srcPtr, Uint32 srcPitch,
                     Uint8 *dstPtr, Uint32 dstPitch,
                     int width, int height)
{
  unsigned int nextlineSrc = srcPitch / sizeof(Uint16);
  Uint16 *p = (Uint16 *)srcPtr;
  unsigned int nextlineDst = dstPitch / sizeof(Uint16);
  Uint16 *q = (Uint16 *)dstPtr;

  while(height--) {
    int i, ii;
    for(i = 0, ii = 0; i < width; ++i, ii += 2) {
      Uint16 A = *(p + i);
      Uint16 B = *(p + i + 1);
      Uint16 C = *(p + i + nextlineSrc);
      Uint16 D = *(p + i + nextlineSrc + 1);
      *(q + ii) = A;
      *(q + ii + 1) = (Uint16)INTERPOLATE(A, B);
      *(q + ii + nextlineDst) = (Uint16)INTERPOLATE(A, C);
      *(q + ii + nextlineDst + 1) = (Uint16)Q_INTERPOLATE(A, B, C, D);
    }
    p += nextlineSrc;
    q += nextlineDst << 1;
  }
}


// NEED_OPTIMIZE
static void MULT(Uint16 c, float* r, float* g, float* b, float alpha) {
  *r += alpha * ((c & RED_MASK565  ) >> 11);
  *g += alpha * ((c & GREEN_MASK565) >>  5);
  *b += alpha * ((c & BLUE_MASK565 ) >>  0);
}

static Uint16 MAKE_RGB565(float r, float g, float b) {
  return (Uint16)
  (((((Uint8)r) << 11) & RED_MASK565  ) |
  ((((Uint8)g) <<  5) & GREEN_MASK565) |
  ((((Uint8)b) <<  0) & BLUE_MASK565 ));
}

FUNCINLINE static ATTRINLINE float CUBIC_WEIGHT(float x) {
  // P(x) = { x, x>0 | 0, x<=0 }
  // P(x + 2) ^ 3 - 4 * P(x + 1) ^ 3 + 6 * P(x) ^ 3 - 4 * P(x - 1) ^ 3
  double r = 0.;
  if(x + 2 > 0) r +=      pow(x + 2, 3);
  if(x + 1 > 0) r += -4 * pow(x + 1, 3);
  if(x     > 0) r +=  6 * pow(x    , 3);
  if(x - 1 > 0) r += -4 * pow(x - 1, 3);
  return (float)r / 6;
}

void filter_bicubic(Uint8 *srcPtr, Uint32 srcPitch,
                    Uint8 *dstPtr, Uint32 dstPitch,
                    int width, int height)
{
  unsigned int nextlineSrc = srcPitch / sizeof(Uint16);
  Uint16 *p = (Uint16 *)srcPtr;
  unsigned int nextlineDst = dstPitch / sizeof(Uint16);
  Uint16 *q = (Uint16 *)dstPtr;
  int dx = width << 1, dy = height << 1;
  float fsx = (float)width / dx;
	float fsy = (float)height / dy;
	float v = 0.0f;
	int j = 0;
	for(; j < dy; ++j) {
	  float u = 0.0f;
	  int iv = (int)v;
    float decy = v - iv;
    int i = 0;
	  for(; i < dx; ++i) {
		  int iu = (int)u;
		  float decx = u - iu;
      float r, g, b;
      int m;
      r = g = b = 0.;
      for(m = -1; m <= 2; ++m) {
        float r1 = CUBIC_WEIGHT(decy - m);
        int n;
        for(n = -1; n <= 2; ++n) {
          float r2 = CUBIC_WEIGHT(n - decx);
          Uint16* pIn = p + (iu  + n) + (iv + m) * nextlineSrc;
          MULT(*pIn, &r, &g, &b, r1 * r2);
        }
      }
      *(q + i) = MAKE_RGB565(r, g, b);
      u += fsx;
	  }
    q += nextlineDst;
	  v += fsy;
  }
}
