#ifndef GAPI_H
#define GAPI_H

int GXOPENDISPLAY(HWND hWnd, DWORD dwFlags);
int GXCLOSEDISPLAY();
void * GXBEGINDRAW();
int GXENDDRAW();
struct GXDisplayProperties GXGETDISPLAYPROPERTIES();
int GXSUSPEND();
int GXRESUME();

#endif