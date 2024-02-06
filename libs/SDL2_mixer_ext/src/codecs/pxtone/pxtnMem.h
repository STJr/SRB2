// '16/12/16 pxtnMem.

#ifndef pxtnMem_H
#define pxtnMem_H

#include "./pxtn.h"

bool pxtnMem_zero_alloc( void** pp, uint32_t byte_size );
bool pxtnMem_free      ( void** pp );
bool pxtnMem_zero      ( void*  p , uint32_t byte_size );

#endif
