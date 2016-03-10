#include "doomdef.h"
#include "doomstat.h"
#include "byteptr.h"
#include "d_datawrap.h"
#include "z_zone.h"

static void CheckEOF(DataWrap dw, size_t l)
{
	if (dw->p - dw->data + l > dw->len)
	{
		Z_Free(dw);
		longjmp(*dw->eofjmp, 1);
	}
}

UINT8 DW_ReadUINT8(DataWrap dw)
{
	CheckEOF(dw, 1);
	return READUINT8(dw->p);
}

UINT16 DW_ReadUINT16(DataWrap dw)
{
	CheckEOF(dw, 2);
	return READUINT16(dw->p);
}

UINT32 DW_ReadUINT32(DataWrap dw)
{
	CheckEOF(dw, 4);
	return READUINT32(dw->p);
}

SINT8 DW_ReadSINT8(DataWrap dw)
{
	CheckEOF(dw, 1);
	return READSINT8(dw->p);
}

INT16 DW_ReadINT16(DataWrap dw)
{
	CheckEOF(dw, 2);
	return READINT16(dw->p);
}

char *DW_ReadStringn(DataWrap dw, size_t n)
{
	char *string = ZZ_Alloc(n+1);
	char *p = string;
	size_t i;
	for (i = 0; i < n; i++, p++)
	{
		CheckEOF(dw,1);
		*p = READUINT8(dw->p);
		if (!*p)
			break;
	}
	*p = '\0';
	return string;
}

DataWrap D_NewDataWrap(const void *data, size_t len, jmp_buf *eofjmp)
{
	DataWrap dw = ZZ_Alloc(sizeof(struct DataWrap_s));
	dw->data = dw->p = data;
	dw->len = len;
	dw->eofjmp = eofjmp;
	return dw;
}
