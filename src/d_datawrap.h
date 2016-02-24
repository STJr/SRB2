// Basically SDL_RWops I guess.
#include <setjmp.h>

typedef struct DataWrap_s {
	const void *data, *p;
	size_t len;
	jmp_buf	*eofjmp;
} *DataWrap;

UINT8 DW_ReadUINT8(struct DataWrap_s *);
UINT16 DW_ReadUINT16(struct DataWrap_s *);
UINT32 DW_ReadUINT32(struct DataWrap_s *);

SINT8 DW_ReadSINT8(struct DataWrap_s *);
INT16 DW_ReadINT16(struct DataWrap_s *);

char *DW_ReadStringn(struct DataWrap_s *, size_t n);

DataWrap D_NewDataWrap(const void *data, size_t len, jmp_buf *eofjmp);
