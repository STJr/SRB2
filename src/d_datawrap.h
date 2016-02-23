// Basically SDL_RWops I guess.
#include <setjmp.h>

typedef struct DataWrap_s {
	const void *data, *p;
	size_t len;
	jmp_buf	*eofjmp;

	UINT8 (*ReadUINT8)(struct DataWrap_s *);
	UINT16 (*ReadUINT16)(struct DataWrap_s *);

	INT16 (*ReadINT16)(struct DataWrap_s *);

	char *(*ReadStringn)(struct DataWrap_s *, size_t n);
} *DataWrap;

DataWrap D_NewDataWrap(const void *data, size_t len, jmp_buf *eofjmp);
