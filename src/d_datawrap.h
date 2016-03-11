// Basically SDL_RWops I guess.
#include <setjmp.h>

typedef struct DataWrap_s {
	const void *data, *p;
	size_t len;
	jmp_buf	*eofjmp;
} *DataWrap;

UINT8 DW_ReadUINT8(DataWrap);
UINT16 DW_ReadUINT16(DataWrap);
UINT32 DW_ReadUINT32(DataWrap);

SINT8 DW_ReadSINT8(DataWrap);
INT16 DW_ReadINT16(DataWrap);
INT32 DW_ReadINT32(DataWrap);

fixed_t DW_ReadFixed(DataWrap);

char *DW_ReadStringn(DataWrap, size_t n);

DataWrap D_NewDataWrap(const void *data, size_t len, jmp_buf *eofjmp);
