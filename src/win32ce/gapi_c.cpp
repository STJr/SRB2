#include <stdlib.h>
#include <windows.h>

#define _USE_GAPI_


#ifndef _USE_GAPI_
	#include "GameX.h"

struct GXDisplayProperties
{
	DWORD cxWidth;
	DWORD cyHeight;
	long cbxPitch;
	long cbyPitch;
	long cBPP;
	DWORD ffFormat;
};

#define kfPalette	0x10		// Pixel values are indexes into a palette
#define kfDirect565	0x80		// 5 red bits, 6 green bits and 5 blue bits per pixel


	GameX* gx = new GameX;
#else
	#include "gx.h"
#endif

extern "C"
{
	#include "gapi_c.h"
}

extern "C" int GXOPENDISPLAY(HWND hWnd, DWORD dwFlags)
{
	#ifndef _USE_GAPI_
		if(!gx)
			return 0;

		//gx->OpenSound();
		if(!gx->OpenGraphics())
		{
			delete gx;
			gx = 0;
			return 0;
		}

		return TRUE;

	#else
		return GXOpenDisplay(hWnd,dwFlags);
	#endif
}

extern "C" int GXCLOSEDISPLAY()
{
	#ifndef _USE_GAPI_
		gx->CloseGraphics();
		return TRUE;
	#else
		return GXCloseDisplay();
	#endif
}

extern "C" void * GXBEGINDRAW()
{
	#ifndef _USE_GAPI_
		if(gx->BeginDraw())
			return gx->GetFBAddress();

		return NULL;
	#else
		return GXBeginDraw();
	#endif
}

extern "C" int GXENDDRAW()
{
	#ifndef _USE_GAPI_
		return gx->EndDraw();
	#else
		return GXEndDraw();
	#endif

}

extern "C" struct GXDisplayProperties GXGETDISPLAYPROPERTIES()
{
	#ifndef _USE_GAPI_
		RECT				r;
		GXDisplayProperties	gxdp;

		gxdp.cbyPitch	= gx->GetFBModulo();
		gxdp.cBPP		= gx->GetFBBpp();
		gxdp.cbxPitch	= (gxdp.cBPP >> 3);

		gx->GetScreenRect(&r);
		gxdp.cxWidth	= (r.right - r.left);
		gxdp.cyHeight	= (r.bottom - r.top);

		if(gxdp.cBPP = 16)
			gxdp.ffFormat = kfDirect565;
		else if(gxdp.cBPP = 8)
			gxdp.ffFormat = kfPalette;

		return gxdp;

	#else
		return GXGetDisplayProperties();
	#endif
}

extern "C" int GXSUSPEND()
{
	#ifndef _USE_GAPI_
		return gx->Suspend();
	#else
		return GXSuspend();
	#endif
}

extern "C" int GXRESUME()
{
	#ifndef _USE_GAPI_
		return gx->Resume();
	#else
		return GXResume();
	#endif
}