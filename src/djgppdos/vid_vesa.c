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
/// \brief extended vesa VESA2.0 video modes i/o

#include <stdlib.h>

#include "../i_system.h"        //I_Error()
#include "vid_vesa.h"
#include "../doomdef.h"         //MAXVIDWIDTH, MAXVIDHEIGHT
#include "../screen.h"

#include <dpmi.h>
#include <go32.h>
#include <sys/farptr.h>
#include <sys/movedata.h>
#include <sys/segments.h>
#include <sys/nearptr.h>

#include "../console.h"
#include "../command.h"            //added:21-03-98: vid_xxx commands
#include "../i_video.h"


// PROTOS
static vmode_t *VID_GetModePtr (int modenum);
static int  VID_VesaGetModeInfo (int modenum);
static void VID_VesaGetExtraModes (void);
static INT32  VID_VesaInitMode (viddef_t *lvid, vmode_t *pcurrentmode);

static void VID_Command_NumModes_f (void);
static void VID_Command_ModeInfo_f (void);
static void VID_Command_ModeList_f (void);
static void VID_Command_Mode_f (void);

consvar_t cv_vidwait = {"vid_wait", "On", CV_SAVE, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};
static consvar_t cv_stretch = {"stretch", "On", CV_SAVE|CV_NOSHOWHELP, CV_OnOff, NULL, 0, NULL, NULL, 0, 0, NULL};

#define VBEVERSION      2       // we need vesa2 or higher

// -----------------------------------------------------
#define MASK_LINEAR(addr)     (addr & 0x000FFFFF)
#define RM_TO_LINEAR(addr)    (((addr & 0xFFFF0000) >> 12) + (addr & 0xFFFF))
#define RM_OFFSET(addr)       (addr & 0xF)
#define RM_SEGMENT(addr)      ((addr >> 4) & 0xFFFF)
// -----------------------------------------------------

static int totalvidmem;

static vmode_t      vesa_modes[MAX_VESA_MODES] = {{NULL, NULL, 0, 0, 0, 0, 0, 0, NULL, NULL, 0}};
static vesa_extra_t vesa_extra[MAX_VESA_MODES];

//this is the only supported non-vesa mode : standard 320x200x256c.
#define NUMVGAVIDMODES  1
static INT32 VGA_InitMode (viddef_t *lvid, vmode_t *pcurrentmode);
static char vgamode1[] ="320x200";
static vmode_t      vgavidmodes[NUMVGAVIDMODES] = {
  {
	NULL,
	vgamode1,
	320, 200,  //(200.0/320.0)*(320.0/240.0),
	320, 1,    // rowbytes, bytes per pixel
	0, 1,
	NULL,
	VGA_InitMode, 0
  }
};

static char         names[MAX_VESA_MODES][10];

//----------------------------i_video.c------------------------------------
// these ones should go to i_video.c, but I prefer keep them away from the
// doom sources until the vesa stuff is ok.
static int     numvidmodes;   //total number of video modes, vga, vesa1, vesa2.
static vmode_t *pvidmodes;    //start of videomodes list.
static vmode_t *pcurrentmode; // the current active videomode.
//----------------------------i_video.c------------------------------------



// table des modes videos.
// seul le mode 320x200x256c standard VGA est support‚ sans le VESA.
// ce mode est le mode num‚ro 0 dans la liste.
typedef struct
{
	int modenum;            // vesa vbe2.0 modenum
	int mode_attributes;
	int winasegment;
	int winbsegment;
	int bytes_per_scanline; // bytes per logical scanline (+16)
	int win;                // window number (A=0, B=1)
	int win_size;           // window size (+6)
	int granularity;        // how finely i can set the window in vid mem (+4)
	int width, height;      // displayed width and height (+18, +20)
	int bits_per_pixel;     // er, better be 8, 15, 16, 24, or 32 (+25)
	int bytes_per_pixel;    // er, better be 1, 2, or 4
	int memory_model;       // and better be 4 or 6, packed or direct color (+27)
	int num_pages;          // number of complete frame buffer pages (+29)
	int red_width;          // the # of bits in the red component (+31)
	int red_pos;            // the bit position of the red component (+32)
	int green_width;        // etc.. (+33)
	int green_pos;          // (+34)
	int blue_width;         // (+35)
	int blue_pos;           // (+36)
	int pptr;
	int pagesize;
	int numpages;
} modeinfo_t;

static vbeinfoblock_t vesainfo;
static vesamodeinfo_t vesamodeinfo;

// ------------------------------------------------------------------------
// DOS stuff
// ------------------------------------------------------------------------
static unsigned long conventional_memory = (unsigned long)-1;

FUNCINLINE static ATTRINLINE void map_in_conventional_memory(void)
{
	if (conventional_memory == (unsigned long)-1)
	{
		if (__djgpp_nearptr_enable())
		{
			conventional_memory = __djgpp_conventional_base;
		}
	}
}

// Converts a flat 32 bit ptr to a realmode 0x12345 type ptr (seg<<4 + offs)
#if 0
unsigned int ptr2real(void *ptr)
{
	map_in_conventional_memory();
	return (int)ptr - conventional_memory;
}
#endif

// Converts 0x12345 (seg<<4+offs) realmode ptr to a flat 32bit ptr
FUNCINLINE static ATTRINLINE void *real2ptr(unsigned int real)
{
	map_in_conventional_memory();
	return (void *) (real + conventional_memory);
}

// ------------------------------------------------------------------------


/* ======================================================================== */
// Add the standard VGA video modes (only one now) to the video modes list.
/* ======================================================================== */
static inline void VGA_Init(void)
{
	vgavidmodes[NUMVGAVIDMODES-1].pnext = pvidmodes;
	pvidmodes = &vgavidmodes[0];
	numvidmodes += NUMVGAVIDMODES;
}


//added:30-01-98: return number of video modes in pvidmodes list
INT32 VID_NumModes(void)
{
	return numvidmodes;
}

//added:21-03-98: return info on video mode
FUNCINLINE static ATTRINLINE const char *VID_ModeInfo (int modenum, char **ppheader)
{
	static const char *badmodestr = "Bad video mode number\n";
	vmode_t     *pv;

	pv = VID_GetModePtr (modenum);

	if (!pv)
	{
		if (ppheader)
			*ppheader = NULL;
		return badmodestr;
	}
	else
	{
		//if (ppheader)
		//    *ppheader = pv->header;
		return pv->name;
	}
}



//added:03-02-98: return a video mode number from the dimensions
INT32 VID_GetModeForSize( INT32 w, INT32 h)
{
	vmode_t *pv;
	int modenum;

	pv = pvidmodes;
	for (modenum=0; pv!=NULL; pv=pv->pnext,modenum++ )
	{
		if ( pv->width==(unsigned)w && pv->height==(unsigned)h )
			return modenum;
	}

	return 0;
}


/* ======================================================================== */
//
/* ======================================================================== */
void    VID_Init (void)
{
	COM_AddCommand ("vid_nummodes", VID_Command_NumModes_f);
	COM_AddCommand ("vid_modeinfo", VID_Command_ModeInfo_f);
	COM_AddCommand ("vid_modelist", VID_Command_ModeList_f);
	COM_AddCommand ("vid_mode", VID_Command_Mode_f);
	CV_RegisterVar (&cv_vidwait);
	CV_RegisterVar (&cv_stretch);

	//setup the videmodes list,
	// note that mode 0 must always be VGA mode 0x13
	pvidmodes = NULL;
	pcurrentmode = NULL;
	numvidmodes = 0;
	// setup the vesa_modes list
	VID_VesaGetExtraModes ();

	// the game boots in 320x200 standard VGA, but
	// we need a highcolor mode to run the game in highcolor
	if (highcolor && numvidmodes==0)
		I_Error ("No 15bit highcolor VESA2 video mode found, cannot run in highcolor.\n");

	// add the vga modes at the start of the modes list
	VGA_Init();


#ifdef DEBUG
	CONS_Printf("VID_SetMode(%d)\n",vid.modenum);
#endif
	VID_SetMode (0); //vid.modenum);


#ifdef DEBUG
	CONS_Printf("after VID_SetMode\n");
	CONS_Printf("vid.width    %d\n",vid.width);
	CONS_Printf("vid.height   %d\n",vid.height);
	CONS_Printf("vid.buffer   %x\n",vid.buffer);
	CONS_Printf("vid.rowbytes %d\n",vid.rowbytes);
	CONS_Printf("vid.numpages %d\n",vid.numpages);
	CONS_Printf("vid.recalc   %d\n",vid.recalc);
	CONS_Printf("vid.direct   %x\n",vid.direct);
#endif

}


// ========================================================================
// Returns a vmode_t from the video modes list, given a video mode number.
// ========================================================================
vmode_t *VID_GetModePtr (int modenum)
{
	vmode_t *pv;

	pv = pvidmodes;
	if (!pv)
		I_Error ("VID_error 1\n");

	while (modenum--)
	{
		pv = pv->pnext;
		if (!pv)
			I_Error ("VID_error 2\n");
	}

	return pv;
}


//added:30-01-98:return the name of a video mode
const char *VID_GetModeName (INT32 modenum)
{
	return (VID_GetModePtr(modenum))->name;
}


// ========================================================================
// Sets a video mode
// ========================================================================
INT32 VID_SetMode (INT32 modenum)  //, UINT8 *palette)
{
	int     vstat;
	vmode_t *pnewmode, *poldmode;

	if ((modenum >= numvidmodes) || (modenum < 0))
	{
		if (pcurrentmode == NULL)
		{
			modenum = 0;    // mode hasn't been set yet, so initialize to base
							//  mode since they gave us an invalid initial mode
		}
		else
		{
			//nomodecheck = true;
			I_Error ("Unknown video mode: %d\n", modenum);
			//nomodecheck = false;
			return 0;
		}
	}

	pnewmode = VID_GetModePtr (modenum);

	if (pnewmode == pcurrentmode)
		return 1;   // already in the desired mode

	// initialize the new mode
	poldmode = pcurrentmode;
	pcurrentmode = pnewmode;

	// initialize vidbuffer size for setmode
	vid.width  = pcurrentmode->width;
	vid.height = pcurrentmode->height;
	//vid.aspect = pcurrentmode->aspect;
	vid.rowbytes = pcurrentmode->rowbytes;
	vid.bpp      = pcurrentmode->bytesperpixel;

	//debug
	//if (vid.rowbytes != vid.width)
	//    I_Error("vidrowbytes (%d) <> vidwidth(%d)\n",vid.rowbytes,vid.width);

	vstat = (*pcurrentmode->setmode) (&vid, pcurrentmode);

	if (vstat < 1)
	{
		if (vstat == 0)
		{
			// harware could not setup mode
			//if (!VID_SetMode (vid.modenum))
			//    I_Error ("VID_SetMode: couldn't set video mode (hard failure)");
			I_Error("Couldn't set video mode %d\n", modenum);
		}
		else
		if (vstat == -1)
		{
			CONS_Printf ("Not enough mem for VID_SetMode...\n");

			// not enough memory; just put things back the way they were
			pcurrentmode = poldmode;
			vid.width = pcurrentmode->width;
			vid.height = pcurrentmode->height;
			vid.rowbytes = pcurrentmode->rowbytes;
			vid.bpp      = pcurrentmode->bytesperpixel;
			return 0;
		}
	}

	vid.modenum = modenum;

	//printf ("%s\n", VID_ModeInfo (vid.modenum, NULL));

	//added:20-01-98: recalc all tables and realloc buffers based on
	//                vid values.
	vid.recalc = 1;

	if (!cv_stretch.value && (float)vid.width/vid.height != ((float)BASEVIDWIDTH/BASEVIDHEIGHT))
		vid.height = (int)(vid.width * ((float)BASEVIDHEIGHT/BASEVIDWIDTH));// Adjust the height to match

	return 1;
}

void VID_CheckRenderer(void) {}
void VID_CheckGLLoaded(rendermode_t oldrender) {}



// converts a segm:offs 32bit pair to a 32bit flat ptr
#if 0
void *VID_ExtraFarToLinear (void *ptr)
{
	int     temp;

	temp = (int)ptr;
	return real2ptr (((temp & 0xFFFF0000) >> 12) + (temp & 0xFFFF));
}
#endif



// ========================================================================
// Helper function for VID_VesaGetExtraModes
// In:  vesa mode number, from the vesa videomodenumbers list
// Out: false, if no info for given modenum
// ========================================================================
int VID_VesaGetModeInfo (int modenum)
{
	int     bytes_per_pixel;
	unsigned int          i;
	__dpmi_regs regs;

	for (i=0; i<sizeof (vesamodeinfo_t); i++)
		_farpokeb(_dos_ds, MASK_LINEAR(__tb)+i, 0);

	regs.x.ax = 0x4f01;
	regs.x.di = RM_OFFSET(__tb);
	regs.x.es = RM_SEGMENT(__tb);
	regs.x.cx = modenum;
	__dpmi_int(0x10, &regs);
	if (regs.h.ah)
		return false;
	else
	{
		dosmemget (MASK_LINEAR(__tb), sizeof (vesamodeinfo_t), &vesamodeinfo);

		bytes_per_pixel = (vesamodeinfo.BitsPerPixel+1)/8;

		// we add either highcolor or lowcolor video modes, not the two
		if (highcolor && (vesamodeinfo.BitsPerPixel != 15))
			return false;
		if (!highcolor && (vesamodeinfo.BitsPerPixel != 8))
			return false;

		if ((bytes_per_pixel > 2) ||
			(vesamodeinfo.XResolution > MAXVIDWIDTH) ||
			(vesamodeinfo.YResolution > MAXVIDHEIGHT))
		{
			return false;
		}

		// we only want color graphics modes that are supported by the hardware
		if ((vesamodeinfo.ModeAttributes &
			(MODE_SUPPORTED_IN_HW | COLOR_MODE | GRAPHICS_MODE) ) !=
			 (MODE_SUPPORTED_IN_HW | COLOR_MODE | GRAPHICS_MODE))
		{
			return false;
		}

		// we only work with linear frame buffers, except for 320x200,
		// which is linear when banked at 0xA000
		if (!(vesamodeinfo.ModeAttributes & LINEAR_FRAME_BUFFER))
		{
			if ((vesamodeinfo.XResolution != 320) ||
				(vesamodeinfo.YResolution != 200))
			{
				return false;
			}
		}

		// pagesize
		if ((vesamodeinfo.BytesPerScanLine * vesamodeinfo.YResolution)
		    > totalvidmem)
		{
			return false;
		}

		vesamodeinfo.NumberOfImagePages = 1;


#ifdef DEBUG
		CONS_Printf("VID: (VESA) info for mode 0x%x\n", modeinfo.modenum);
		CONS_Printf("  mode attrib = 0x%0x\n", modeinfo.mode_attributes);
		CONS_Printf("  win a attrib = 0x%0x\n", *(UINT8 *)(infobuf+2));
		CONS_Printf("  win b attrib = 0x%0x\n", *(UINT8 *)(infobuf+3));
		CONS_Printf("  win a seg 0x%0x\n", (int) modeinfo.winasegment);
		CONS_Printf("  win b seg 0x%0x\n", (int) modeinfo.winbsegment);
		CONS_Printf("  bytes per scanline = %d\n",
				modeinfo.bytes_per_scanline);
		CONS_Printf("  width = %d, height = %d\n", modeinfo.width,
				modeinfo.height);
		CONS_Printf("  win = %c\n", 'A' + modeinfo.win);
		CONS_Printf("  win granularity = %d\n", modeinfo.granularity);
		CONS_Printf("  win size = %d\n", modeinfo.win_size);
		CONS_Printf("  bits per pixel = %d\n", modeinfo.bits_per_pixel);
		CONS_Printf("  bytes per pixel = %d\n", modeinfo.bytes_per_pixel);
		CONS_Printf("  memory model = 0x%x\n", modeinfo.memory_model);
		CONS_Printf("  num pages = %d\n", modeinfo.num_pages);
		CONS_Printf("  red width = %d\n", modeinfo.red_width);
		CONS_Printf("  red pos = %d\n", modeinfo.red_pos);
		CONS_Printf("  green width = %d\n", modeinfo.green_width);
		CONS_Printf("  green pos = %d\n", modeinfo.green_pos);
		CONS_Printf("  blue width = %d\n", modeinfo.blue_width);
		CONS_Printf("  blue pos = %d\n", modeinfo.blue_pos);
		CONS_Printf("  phys mem = %x\n", modeinfo.pptr);
#endif
	}

	return true;
}


// ========================================================================
// Get extended VESA modes information, keep the ones that we support,
// so they'll be available in the game Video menu.
// ========================================================================
#define MAXVESADESC 100
static char vesadesc[MAXVESADESC] = "";

void VID_VesaGetExtraModes (void)
{
	unsigned int    i;
	unsigned long   addr;
	int             nummodes;
	__dpmi_meminfo  phys_mem_info;
	unsigned long   mode_ptr;
	__dpmi_regs     regs;

	// make a copy of the video modes list! else trash in __tb
	UINT16          vmode[MAX_VESA_MODES+1];
	int             numvmodes;
	UINT16          vesamode;

	// new ugly stuff...
	for (i=0; i<sizeof (vbeinfoblock_t); i++)
		_farpokeb(_dos_ds, MASK_LINEAR(__tb)+i, 0);

	dosmemput("VBE2", 4, MASK_LINEAR(__tb));

	// see if VESA support is available
	regs.x.ax = 0x4f00;
	regs.x.di = RM_OFFSET(__tb);
	regs.x.es = RM_SEGMENT(__tb);
	__dpmi_int(0x10, &regs);
	if (regs.h.ah)
		goto no_vesa;

	dosmemget(MASK_LINEAR(__tb), sizeof (vbeinfoblock_t), &vesainfo);

	if (strncmp((void *)vesainfo.VESASignature, "VESA", 4) != 0)
	{
no_vesa:
		CONS_Printf ("No VESA driver\n");
		return;
	}

	if (vesainfo.VESAVersion < (VBEVERSION<<8))
	{
		CONS_Printf ("VESA VBE %d.0 not available\n", VBEVERSION);
		return;
	}

	//
	// vesa version number
	//
	CONS_Printf ("%4.4s %d.%d (", vesainfo.VESASignature,
	             vesainfo.VESAVersion>>8,
	             vesainfo.VESAVersion&0xFF);

	//
	// vesa description string
	//
	i = 0;
	addr = RM_TO_LINEAR(vesainfo.OemStringPtr);
	_farsetsel(_dos_ds);
	while (_farnspeekb(addr) != 0)
	{
		vesadesc[i++] = _farnspeekb(addr++);
		if (i == MAXVESADESC-1)
			break;
	}
	vesadesc[i]=0;
	CONS_Printf ("%s)\n",vesadesc);

	totalvidmem = vesainfo.TotalMemory << 16;

   //
   // find 8 bit modes
   //
	numvmodes = 0;
	mode_ptr = RM_TO_LINEAR(vesainfo.VideoModePtr);
	while ((vmode[numvmodes] = _farpeekw(_dos_ds, mode_ptr)) != 0xFFFF)
	{
		numvmodes++;
		if ( numvmodes == MAX_VESA_MODES )
			break;
		mode_ptr += 2;
	}
	vmode[numvmodes] = 0xffff;

	nummodes = 0;       // number of video modes accepted for the game

	numvmodes=0;  // go again through vmodes table
	while ( ((vesamode=vmode[numvmodes++]) != 0xFFFF) && (nummodes < MAX_VESA_MODES) )
	{
		//fill the modeinfo struct.
		if (VID_VesaGetModeInfo (vesamode))
		{
			vesa_modes[nummodes].pnext = &vesa_modes[nummodes+1];
			if (vesamodeinfo.XResolution > 999)
			{
				if (vesamodeinfo.YResolution > 999)
				{
					sprintf (&names[nummodes][0], "%4dx%4d", vesamodeinfo.XResolution,
					         vesamodeinfo.YResolution);
					names[nummodes][9] = 0;
				}
				else
				{
					sprintf (&names[nummodes][0], "%4dx%3d", vesamodeinfo.XResolution,
					         vesamodeinfo.YResolution);
					names[nummodes][8] = 0;
				}
			}
			else
			{
				if (vesamodeinfo.YResolution > 999)
				{
					sprintf (&names[nummodes][0], "%3dx%4d", vesamodeinfo.XResolution,
					         vesamodeinfo.YResolution);
					names[nummodes][8] = 0;
				}
				else
				{
					sprintf (&names[nummodes][0], "%3dx%3d", vesamodeinfo.XResolution,
					         vesamodeinfo.YResolution);
					names[nummodes][7] = 0;
				}
			}

			vesa_modes[nummodes].name = &names[nummodes][0];
			vesa_modes[nummodes].width = vesamodeinfo.XResolution;
			vesa_modes[nummodes].height = vesamodeinfo.YResolution;

			//added:20-01-98:aspect ratio to be implemented...
			vesa_modes[nummodes].rowbytes = vesamodeinfo.BytesPerScanLine;
			vesa_modes[nummodes].windowed = 0;
			vesa_modes[nummodes].pextradata = &vesa_extra[nummodes];
			vesa_modes[nummodes].setmode = VID_VesaInitMode;

			if (vesamodeinfo.ModeAttributes & LINEAR_FRAME_BUFFER)
			{
			// add linear bit to mode for linear modes
				vesa_extra[nummodes].vesamode = vesamode | LINEAR_MODE;
				vesa_modes[nummodes].numpages = 1; //vesamodeinfo.NumberOfImagePages;

				phys_mem_info.address = (int)vesamodeinfo.PhysBasePtr;
				phys_mem_info.size = 0x400000;

				// returns -1 on error
				if (__dpmi_physical_address_mapping(&phys_mem_info))
				{
					//skip this mode, it doesnt work
					continue;
				}

				// if physical mapping was ok... convert the selector:offset
				vesa_extra[nummodes].plinearmem =
				 real2ptr (phys_mem_info.address);

				// lock the region
				__dpmi_lock_linear_region (&phys_mem_info);
			}
			else
			{
			// banked at 0xA0000
				vesa_extra[nummodes].vesamode = vesamode;
				//vesa_extra[nummodes].pages[0] = 0;
				vesa_extra[nummodes].plinearmem =
				 real2ptr (vesamodeinfo.WinASegment<<4);

				vesa_modes[nummodes].numpages = 1; //modeinfo.numpages;
			}

			vesa_modes[nummodes].bytesperpixel = (vesamodeinfo.BitsPerPixel+1)/8;

			nummodes++;
		}
	}

// add the VESA modes at the start of the mode list (if there are any)
	if (nummodes)
	{
		vesa_modes[nummodes-1].pnext = NULL; //pvidmodes;
		pvidmodes = &vesa_modes[0];
		numvidmodes += nummodes;
	}

}


// ========================================================================
// Free the video buffer of the last video mode,
// allocate a new buffer for the video mode to set.
// ========================================================================
static boolean VID_FreeAndAllocVidbuffer (viddef_t *lvid)
{
	int  vidbuffersize;

	vidbuffersize = (lvid->width * lvid->height * lvid->bpp * NUMSCREENS);  //status bar

	// free allocated buffer for previous video mode
	if (lvid->buffer!=NULL)
	{
		free(lvid->buffer);
	}

	// allocate the new screen buffer
	if ( (lvid->buffer = (UINT8 *) malloc(vidbuffersize))==NULL )
		return false;

	// initially clear the video buffer
	memset (lvid->buffer, 0x00, vidbuffersize);

#ifdef DEBUG
	CONS_Printf("VID_FreeAndAllocVidbuffer done, vidbuffersize: %x\n",vidbuffersize);
#endif
	return true;
}


// ========================================================================
// Set video mode routine for STANDARD VGA MODES (NO HIGHCOLOR)
// Out: 1 ok,
//      0 hardware could not set mode,
//     -1 no mem
// ========================================================================
static INT32 VGA_InitMode (viddef_t *lvid, vmode_t *currentmodep)
{
	__dpmi_regs   regs;

	if (!VID_FreeAndAllocVidbuffer (lvid))
		return -1;                  //no mem

	//added:26-01-98: should clear video mem here

	//set mode 0x13
	regs.h.ah = 0;
	regs.h.al = 0x13;
	__dpmi_int(0x10, &regs);

	// here it is the standard VGA 64k window, not an LFB
	// (you could have 320x200x256c with LFB in the vesa modes)
	lvid->direct = (UINT8 *) real2ptr (0xa0000);
	lvid->u.numpages = 1;
	lvid->bpp = currentmodep->bytesperpixel;

	return 1;
}


// ========================================================================
// Set video mode routine for VESA video modes, see VID_SetMode()
// Out: 1 ok,
//      0 hardware could not set mode,
//     -1 no mem
// ========================================================================
INT32 VID_VesaInitMode (viddef_t *lvid, vmode_t *currentmodep)
{
	vesa_extra_t    *pextra;
	__dpmi_regs     regs;

	pextra = currentmodep->pextradata;

#ifdef DEBUG
	CONS_Printf("VID_VesaInitMode...\n");
	CONS_Printf(" currentmodep->name %s\n",currentmodep->name);
	CONS_Printf("               width %d\n",currentmodep->width);
	CONS_Printf("               height %d\n",currentmodep->height);
	CONS_Printf("               rowbytes %d\n",currentmodep->rowbytes);
	CONS_Printf("               windowed %d\n",currentmodep->windowed);
	CONS_Printf("               numpages %d\n",currentmodep->numpages);
	CONS_Printf(" currentmodep->pextradata :\n");
	CONS_Printf("                ->vesamode %x\n",pextra->vesamode);
	CONS_Printf("                ->plinearmem %x\n\n",pextra->plinearmem);
#endif

	//added:20-01-98:no page flipping now... TO DO!!!
	lvid->u.numpages = 1;

	// clean up any old vid buffer lying around, alloc new if needed
	if (!VID_FreeAndAllocVidbuffer (lvid))
		return -1;                  //no mem


	//added:20-01-98: should clear video mem here


	// set the mode
	regs.x.ax = 0x4f02;
	regs.x.bx = pextra->vesamode;
	__dpmi_int (0x10, &regs);

	if (regs.x.ax != 0x4f)
		return 0;               // could not set mode

//added:20-01-98: should setup wait_vsync flag, currentpage here...
//                plus check for display_enable bit

//added:20-01-98: here we should set the page if page flipping...

	// points to LFB, or the start of VGA mem.
	lvid->direct = pextra->plinearmem;
	lvid->bpp    = currentmodep->bytesperpixel;

	return 1;
}

// ========================================================================
//                     VIDEO MODE CONSOLE COMMANDS
// ========================================================================


//  vid_nummodes
//
//added:21-03-98:
void VID_Command_NumModes_f (void)
{
	int     nummodes;

	nummodes = VID_NumModes ();
	CONS_Printf ("%d video mode(s) available(s)\n", nummodes);
}


//  vid_modeinfo <modenum>
//
void VID_Command_ModeInfo_f (void)
{
	vmode_t     *pv;
	int         modenum;

	if (COM_Argc()!=2)
		modenum = vid.modenum;          // describe the current mode
	else
		modenum = atoi (COM_Argv(1));   //    .. the given mode number

	if (modenum >= VID_NumModes())
	{
		CONS_Printf ("No such video mode\n");
		return;
	}

	pv = VID_GetModePtr (modenum);

	CONS_Printf ("%s\n", VID_ModeInfo (modenum, NULL));
	CONS_Printf ("width : %d\n"
	             "height: %d\n"
	             "bytes per scanline: %d\n"
	             "bytes per pixel: %d\n"
	             "numpages: %d\n",
	             pv->width,
	             pv->height,
	             pv->rowbytes,
	             pv->bytesperpixel,
	             pv->numpages );
}


//  vid_modelist
//
void VID_Command_ModeList_f (void)
{
	int         i, nummodes;
	const char  *pinfo;
	char        *pheader;
	vmode_t     *pv;
	boolean     na;

	na = false;

	nummodes = VID_NumModes ();
	for (i=0 ; i<nummodes ; i++)
	{
		pv = VID_GetModePtr (i);
		pinfo = VID_ModeInfo (i, &pheader);

		if (i==0 || pv->bytesperpixel==1)
			CONS_Printf ("%d: %s\n", i, pinfo);
		else
			CONS_Printf ("%d: %s (hicolor)\n", i, pinfo);
	}

}


//  vid_mode <modenum>
//
void VID_Command_Mode_f (void)
{
	int         modenum;

	if (COM_Argc()!=2)
	{
		CONS_Printf ("vid_mode <modenum> : set video mode\n");
		return;
	}

	modenum = atoi(COM_Argv(1));

	if (modenum >= VID_NumModes())
		CONS_Printf ("No such video mode\n");
	else
		// request vid mode change
		setmodeneeded = modenum+1;
}
