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

static UINT8 ReadUINT8(DataWrap dw)
{
	CheckEOF(dw, 1);
	return READUINT8(dw->p);
}

static UINT16 ReadUINT16(DataWrap dw)
{
	CheckEOF(dw, 2);
	return READUINT16(dw->p);
}

static char *ReadStringn(DataWrap dw, size_t n)
{
	char *string = ZZ_Alloc(n+1);
	char *p = string;
	int i;
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

	dw->ReadUINT8 = ReadUINT8;
	dw->ReadUINT16 = ReadUINT16;
	dw->ReadStringn = ReadStringn;
	return dw;
}
