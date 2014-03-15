// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 1998-2000 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//-----------------------------------------------------------------------------
/// \file
/// \brief DirectX error messages
///	adapted from DirectX6 sample code

#include <stdarg.h>

//#define WIN32_LEAN_AND_MEAN
#define RPC_NO_WINDOWS_H
#include <windows.h>
#define DIRECTSOUND_VERSION     0x0600       /* version 6.0 */
#define DXVERSION_NTCOMPATIBLE  0x0300
#ifdef _MSC_VER
#pragma warning(disable :  4201)
#endif
#include <ddraw.h>
#include <dsound.h>
#include <mmsystem.h>

#include "dx_error.h"

// -----------------
// DXErrorMessageBox
// Displays a message box containing the given formatted string.
// -----------------
/*
VOID DXErrorMessageBox (HRESULT error) LPSTR fmt, ...)
{
	char buff[256];
	va_list args;

	va_start(args, fmt);
	wvsprintf(buff, fmt, args);
	va_end(args);

	lstrcat(buff, "\r\n");
	MessageBoxA(NULL, buff, "DirectX Error:", MB_ICONEXCLAMATION + MB_OK);
}*/


// ---------------
// DXErrorToString
// Returns a pointer to a string describing the given DD, D3D or D3DRM error code.
// ---------------
LPCSTR DXErrorToString (HRESULT error)
{
	switch (error) {
		case DD_OK:
			/* Also includes D3D_OK and D3DRM_OK */
			return "No error.";
		case DDERR_ALREADYINITIALIZED:
			return "This object is already initialized.";
		case DDERR_BLTFASTCANTCLIP:
			return "Return if a clipper object is attached to the source surface passed into a BltFast call.";
		case DDERR_CANNOTATTACHSURFACE:
			return "This surface can not be attached to the requested surface.";
		case DDERR_CANNOTDETACHSURFACE:
			return "This surface can not be detached from the requested surface.";
		case DDERR_CANTCREATEDC:
			return "Windows can not create any more DCs.";
		case DDERR_CANTDUPLICATE:
			return "Can't duplicate primary & 3D surfaces, or surfaces that are implicitly created.";
		case DDERR_CLIPPERISUSINGHWND:
			return "An attempt was made to set a cliplist for a clipper object that is already monitoring an hwnd.";
		case DDERR_COLORKEYNOTSET:
			return "No src color key specified for this operation.";
		case DDERR_CURRENTLYNOTAVAIL:
			return "Support is currently not available.";
		case DDERR_DIRECTDRAWALREADYCREATED:
			return "A DirectDraw object representing this driver has already been created for this process.";
		case DDERR_EXCEPTION:
			return "An exception was encountered while performing the requested operation.";
		case DDERR_EXCLUSIVEMODEALREADYSET:
			return "An attempt was made to set the cooperative level when it was already set to exclusive.";
		case DDERR_GENERIC:
			return "Generic failure.";
		case DDERR_HEIGHTALIGN:
			return "Height of rectangle provided is not a multiple of reqd alignment.";
		case DDERR_HWNDALREADYSET:
			return "The CooperativeLevel HWND has already been set. It can not be reset while the process has surfaces or palettes created.";
		case DDERR_HWNDSUBCLASSED:
			return "HWND used by DirectDraw CooperativeLevel has been subclassed, this prevents DirectDraw from restoring state.";
		case DDERR_IMPLICITLYCREATED:
			return "This surface can not be restored because it is an implicitly created surface.";
		case DDERR_INCOMPATIBLEPRIMARY:
			return "Unable to match primary surface creation request with existing primary surface.";
		case DDERR_INVALIDCAPS:
			return "One or more of the caps bits passed to the callback are incorrect.";
		case DDERR_INVALIDCLIPLIST:
			return "DirectDraw does not support the provided cliplist.";
		case DDERR_INVALIDDIRECTDRAWGUID:
			return "The GUID passed to DirectDrawCreate is not a valid DirectDraw driver identifier.";
		case DDERR_INVALIDMODE:
			return "DirectDraw does not support the requested mode.";
		case DDERR_INVALIDOBJECT:
			return "DirectDraw received a pointer that was an invalid DIRECTDRAW object.";
		case DDERR_INVALIDPARAMS:
			return "One or more of the parameters passed to the function are incorrect.";
		case DDERR_INVALIDPIXELFORMAT:
			return "The pixel format was invalid as specified.";
		case DDERR_INVALIDPOSITION:
			return "Returned when the position of the overlay on the destination is no longer legal for that destination.";
		case DDERR_INVALIDRECT:
			return "Rectangle provided was invalid.";
		case DDERR_LOCKEDSURFACES:
			return "Operation could not be carried out because one or more surfaces are locked.";
		case DDERR_NO3D:
			return "There is no 3D present.";
		case DDERR_NOALPHAHW:
			return "Operation could not be carried out because there is no alpha accleration hardware present or available.";
		case DDERR_NOBLTHW:
			return "No blitter hardware present.";
		case DDERR_NOCLIPLIST:
			return "No cliplist available.";
		case DDERR_NOCLIPPERATTACHED:
			return "No clipper object attached to surface object.";
		case DDERR_NOCOLORCONVHW:
			return "Operation could not be carried out because there is no color conversion hardware present or available.";
		case DDERR_NOCOLORKEY:
			return "Surface doesn't currently have a color key";
		case DDERR_NOCOLORKEYHW:
			return "Operation could not be carried out because there is no hardware support of the destination color key.";
		case DDERR_NOCOOPERATIVELEVELSET:
			return "Create function called without DirectDraw object method SetCooperativeLevel being called.";
		case DDERR_NODC:
			return "No DC was ever created for this surface.";
		case DDERR_NODDROPSHW:
			return "No DirectDraw ROP hardware.";
		case DDERR_NODIRECTDRAWHW:
			return "A hardware-only DirectDraw object creation was attempted but the driver did not support any hardware.";
		case DDERR_NOEMULATION:
			return "Software emulation not available.";
		case DDERR_NOEXCLUSIVEMODE:
			return "Operation requires the application to have exclusive mode but the application does not have exclusive mode.";
		case DDERR_NOFLIPHW:
			return "Flipping visible surfaces is not supported.";
		case DDERR_NOGDI:
			return "There is no GDI present.";
		case DDERR_NOHWND:
			return "Clipper notification requires an HWND or no HWND has previously been set as the CooperativeLevel HWND.";
		case DDERR_NOMIRRORHW:
			return "Operation could not be carried out because there is no hardware present or available.";
		case DDERR_NOOVERLAYDEST:
			return "Returned when GetOverlayPosition is called on an overlay that UpdateOverlay has never been called on to establish a destination.";
		case DDERR_NOOVERLAYHW:
			return "Operation could not be carried out because there is no overlay hardware present or available.";
		case DDERR_NOPALETTEATTACHED:
			return "No palette object attached to this surface.";
		case DDERR_NOPALETTEHW:
			return "No hardware support for 16 or 256 color palettes.";
		case DDERR_NORASTEROPHW:
			return "Operation could not be carried out because there is no appropriate raster op hardware present or available.";
		case DDERR_NOROTATIONHW:
			return "Operation could not be carried out because there is no rotation hardware present or available.";
		case DDERR_NOSTRETCHHW:
			return "Operation could not be carried out because there is no hardware support for stretching.";
		case DDERR_NOT4BITCOLOR:
			return "DirectDrawSurface is not in 4 bit color palette and the requested operation requires 4 bit color palette.";
		case DDERR_NOT4BITCOLORINDEX:
			return "DirectDrawSurface is not in 4 bit color index palette and the requested operation requires 4 bit color index palette.";
		case DDERR_NOT8BITCOLOR:
			return "DirectDrawSurface is not in 8 bit color mode and the requested operation requires 8 bit color.";
		case DDERR_NOTAOVERLAYSURFACE:
			return "Returned when an overlay member is called for a non-overlay surface.";
		case DDERR_NOTEXTUREHW:
			return "Operation could not be carried out because there is no texture mapping hardware present or available.";
		case DDERR_NOTFLIPPABLE:
			return "An attempt has been made to flip a surface that is not flippable.";
		case DDERR_NOTFOUND:
			return "Requested item was not found.";
		case DDERR_NOTLOCKED:
			return "Surface was not locked.  An attempt to unlock a surface that was not locked at all, or by this process, has been attempted.";
		case DDERR_NOTPALETTIZED:
			return "The surface being used is not a palette-based surface.";
		case DDERR_NOVSYNCHW:
			return "Operation could not be carried out because there is no hardware support for vertical blank synchronized operations.";
		case DDERR_NOZBUFFERHW:
			return "Operation could not be carried out because there is no hardware support for zbuffer blitting.";
		case DDERR_NOZOVERLAYHW:
			return "Overlay surfaces could not be z layered based on their BltOrder because the hardware does not support z layering of overlays.";
		case DDERR_OUTOFCAPS:
			return "The hardware needed for the requested operation has already been allocated.";
		case DDERR_OUTOFMEMORY:
			return "There is not enough memory to perform the operation.";
		case DDERR_OUTOFVIDEOMEMORY:
			return "DirectDraw does not have enough memory to perform the operation.";
		case DDERR_OVERLAYCANTCLIP:
			return "The hardware does not support clipped overlays.";
		case DDERR_OVERLAYCOLORKEYONLYONEACTIVE:
			return "Can only have ony color key active at one time for overlays.";
		case DDERR_OVERLAYNOTVISIBLE:
			return "Returned when GetOverlayPosition is called on a hidden overlay.";
		case DDERR_PALETTEBUSY:
			return "Access to this palette is being refused because the palette is already locked by another thread.";
		case DDERR_PRIMARYSURFACEALREADYEXISTS:
			return "This process already has created a primary surface.";
		case DDERR_REGIONTOOSMALL:
			return "Region passed to Clipper::GetClipList is too small.";
		case DDERR_SURFACEALREADYATTACHED:
			return "This surface is already attached to the surface it is being attached to.";
		case DDERR_SURFACEALREADYDEPENDENT:
			return "This surface is already a dependency of the surface it is being made a dependency of.";
		case DDERR_SURFACEBUSY:
			return "Access to this surface is being refused because the surface is already locked by another thread.";
		case DDERR_SURFACEISOBSCURED:
			return "Access to surface refused because the surface is obscured.";
		case DDERR_SURFACELOST:
			return "Access to this surface is being refused because the surface memory is gone. The DirectDrawSurface object representing this surface should have Restore called on it.";
		case DDERR_SURFACENOTATTACHED:
			return "The requested surface is not attached.";
		case DDERR_TOOBIGHEIGHT:
			return "Height requested by DirectDraw is too large.";
		case DDERR_TOOBIGSIZE:
			return "Size requested by DirectDraw is too large, but the individual height and width are OK.";
		case DDERR_TOOBIGWIDTH:
			return "Width requested by DirectDraw is too large.";
		case DDERR_UNSUPPORTED:
			return "Function call not supported.";
		case DDERR_UNSUPPORTEDFORMAT:
			return "FOURCC format requested is unsupported by DirectDraw.";
		case DDERR_UNSUPPORTEDMASK:
			return "Bitmask in the pixel format requested is unsupported by DirectDraw.";
		case DDERR_VERTICALBLANKINPROGRESS:
			return "Vertical blank is in progress.";
		case DDERR_WASSTILLDRAWING:
			return "Informs DirectDraw that the previous Blt which is transfering information to or from this Surface is incomplete.";
		case DDERR_WRONGMODE:
			return "This surface can not be restored because it was created in a different mode.";
		case DDERR_XALIGN:
			return "Rectangle provided was not horizontally aligned on required boundary.";

		//
		// DirectSound errors
		//
		case DSERR_ALLOCATED:
			return "The request failed because resources, such as a priority level, were already in use by another caller.";
		case DSERR_ALREADYINITIALIZED:
			return "The object is already initialized.";
		case DSERR_BADFORMAT:
			return "The specified wave format is not supported.";
		case DSERR_BUFFERLOST:
			return "The buffer memory has been lost and must be restored.";
		case DSERR_CONTROLUNAVAIL:
			return "The control (volume, pan, and so forth) requested by the caller is not available.";
		case DSERR_INVALIDCALL:
			return "This function is not valid for the current state of this object.";
		case DSERR_NOAGGREGATION:
			return "The object does not support aggregation.";
		case DSERR_NODRIVER:
			return "No sound driver is available for use.";
		case DSERR_NOINTERFACE:
			return "The requested COM interface is not available.";
		case DSERR_OTHERAPPHASPRIO:
			return "Another application has a higher priority level, preventing this call from succeeding";
		case DSERR_PRIOLEVELNEEDED:
			return "The caller does not have the priority level required for the function to succeed.";
		case DSERR_UNINITIALIZED:
			return "The IDirectSound::Initialize method has not been called or has not been called successfully before other methods were called.";
		default:
			return "Unrecognized error value.";
	}
}
