#include "doomtype.h"

#ifndef __R_TAGLIST__
#define __R_TAGLIST__

/// Multitag list.
typedef struct
{
	UINT16* tags;
	UINT16 count;
} taglist_t;

void Tag_Add (taglist_t* list, const UINT16 tag);
#endif //__R_TAGLIST__
