
#include "./pxtnMem.h"

bool pxtnMem_zero_alloc( void** pp, uint32_t byte_size )
{
	if( !(  *pp = malloc( byte_size ) ) ) return false;
	memset( *pp, 0,       byte_size );
	return true;
}

bool pxtnMem_free( void** pp )
{
	if( !pp || !*pp ) return false;
	free( *pp ); *pp = NULL;
	return true;
}

bool pxtnMem_zero( void*  p , uint32_t byte_size )
{
	if( !p ) return false;
	char* p_ = (char*)p;
	for( uint32_t i = 0; i < byte_size; i++, p_++ ) *p_ = 0;
	return true;
}
