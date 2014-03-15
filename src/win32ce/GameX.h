/*

  GameX - WindowsCE Game Library for High Performance.

  Copyright (C) 1999 Hayes C. Haugen, all rights reserved.

  */

/*
	* Need better way for host app to keep track of direct vs blit
	drawing.  Right now GetFBAddress() is the way to do it.


  */


#pragma once
#include <windows.h>			// For VK codes.

// Defines

// display property flags

const unsigned long kfDPGrey =		0x0001;
const unsigned long kfDPGrey2Bit =	0x0002;
const unsigned long kfDPGrey4Bit =	0x0004;
const unsigned long kfDPColor =		0x0008;
const unsigned long kfDPColor8Bit =	0x0010;
const unsigned long kfDPColor16Bit = 0x0020;
const unsigned long kfDPColor24Bit = 0x0040;
const unsigned long kfDPColor32Bit = 0x0080;
const unsigned long kfDPFormatNormal = 0x0100;		// fb start is upper left, inc goes across
const unsigned long	kfDPFormatRot270 = 0x0200;		// fb start is lower left, inc goes up

// Machine property flags

const unsigned long kfMPPSPC =	0x0001;		// Palm Sized PC - 240 x 320
const unsigned long kfMPPSPC1 =	0x0002;		// 1st gen pspc
const unsigned long kfMPPSPC2 =	0x0004;		// Wyverns
const unsigned long kfMPHPC =	0x0008;		// Handheld PC
const unsigned long kfMPHPC1 =	0x0010;		// HPC 1, 480 x 240
const unsigned long kfMPHPC2 =	0x0020;		// HPC 2, 640 x 240
const unsigned long kfMPHPC3 =	0x0040;		// HPC Pro, 640 x 240, big keys
const unsigned long kfMPPro =	0x0080;		//
const unsigned long kfMPAutoPC = 0x0100;
const unsigned long kfMPHasKeyboard = 0x0200;
const unsigned long kfMPHasMouse = 0x0400;
const unsigned long kfMPHasRumble = 0x0800;
const unsigned long kfMPHasTouch = 0x1000;

// Rotations

const int kiRotate0 = 0;		// no rotation
const int kiRotate90 = 1;		// 90 degrees clockwise
const int kiRotate180 = 2;		// 180 degrees clockwise (upside down, Rotate 0 flipped)
const int kiRotate270 = 3;		// 270 degrees clockwise (Rotate 1 flipped)

class GameX {
public:
	HWND SetButtonNotificationWindow(HWND hWnd);
	GameX();
	~GameX();

	bool OpenGraphics();
	bool OpenSound();
	bool OpenButtons(HWND hwndButtonNotify);	// Window that will get button messages or NULL if you just want to use GetAsyncKeyState();
	bool CloseGraphics();
	bool CloseSound();
	bool CloseButtons();

	bool IsColor();
	bool IsPSPC();
	bool IsHPC();
	bool IsHPCPro();
	bool HasMouse();
	bool HasKeyboard();					// better than inferring from hpc/pspc/etc.
	bool HasRumble();
	bool HasTouch();					// for completeness.  At least 1 doesn't.

	int IsForeground();
	bool Suspend();						// release buttons, don't draw, etc.
	bool Resume();						// regrab buttons, etc.

	bool BeginDraw();
	bool EndDraw();
	bool FindFrameBuffer();
	void * GetFBAddress();				// simple way to get things that makes code
	long GetFBModulo();					// more readable.
	long GetFBBpp();
	bool GetScreenRect(RECT * prc);
	unsigned long GetDisplayProperties();

	bool GetButton(int VK);				// buttons init themselves on first button call??
	unsigned short GetDefaultButtonID(long id, long rotate);		// gets the best button for use as specified by need and rotation
	bool ReleaseButton(int VK);
	bool BeginDetectButtons();			// grabs all buttons so user can indicate a button in a config dialog
	bool EndDetectButtons();			// releases all buttons (except GetButton() ones)

	bool Rumble();
	bool Backlight(bool fOn);
private:
//	LRESULT CALLBACK TaskBarWndProc(HWND hWnd, UINT message, WPARAM uParam, LPARAM lParam);

private:
	int m_iMP;							// index into amp table for current machine.
	void * m_pvFrameBuffer;
	long m_cbFBModulo;					// count of bytes to next line
	unsigned long m_ffMachineProperties;
	unsigned long m_ffDisplayProperties;
	int m_cBitsPP;

	long m_dwPrevMode;					// for Begin/EndDraw()
	bool m_fActive;						// true if active (resume), false if inactive (suspend).

	HWND m_hwndTaskbar;					// Taskbar is official cap, change TaskBar's
};


/*

kmtCasioE10
kmtCasioE11
kmtCasioE15
kmtCasioE55
kmtCasioE100
kmtLGPhenomII

stuff for memory size and presence/absence of CF

// Rotations that make sense for this device or default rotation:

Rotate0			// no rotation
Rotate1			// 90 degrees clockwise
Rotate2			// 180 degrees clockwise (upside down, Rotate 0 flipped)
Rotate3			// 270 degrees clockwise (Rotate 1 flipped)

kmtAccessNone	// no direct framebuffer access
kmtAccess1		// write to fixed address
kmtAccess2		// find framebuffer memory aperture in GWES.EXE memory space.

OriginUpperLeft
OriginUpperRight
OriginLowerRight
OriginLowerLeft

MappedHorizontal	// increasing fb address goes across screen
MappedVertical		// increasing fb address goes up/down screen (Compaq)

A machine entry:
===========================================
	machine		kmtCasioE10,
	type		kmtPSPC | kmtPSPC1,
	displaytype	kmtGrey | kmtGrey2Bit | kmtAccess1,
	pvFrameBuffer 0xAA000000,
	cbFBModulo	1024					// count of bytes to next line
	cbppFB		2,						// bits per pixel
	cxDisp		240,
	cyDisp		320,
	format		OriginUpperLeft, MappedHorizontal

	rotate0								// for rotate mode 0, these are the best keys
		vkUp	VK_UP
		vkDown	VK_DOWN
		vkLeft	0xC1
		vkRight	0xC2
		vkA		0xC3
		vkB		0
	rotate1
		vkUp	0xC1
		vkDown	0xC2
		vkLeft	VK_DOWN
		...
		...*/

