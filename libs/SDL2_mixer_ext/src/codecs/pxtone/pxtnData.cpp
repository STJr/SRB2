#include "./pxtnData.h"



static bool _v_to_int( uint32_t* p_i, const uint8_t* bytes5, int32_t byte_num )
{
	uint8_t b[ 5 ] = {};

	switch( byte_num )
	{
	case 0:
		break;
	case 1:
		b[0] =  (bytes5[0]&0x7F)>>0;
		break;
	case 2:
		b[0] = ((bytes5[0]&0x7F)>>0) | (bytes5[1]<<7);
		b[1] =  (bytes5[1]&0x7F)>>1;
		break;
	case 3:
		b[0] = ((bytes5[0]&0x7F)>>0) | (bytes5[1]<<7);
		b[1] = ((bytes5[1]&0x7F)>>1) | (bytes5[2]<<6);
		b[2] =  (bytes5[2]&0x7F)>>2;
		break;
	case 4:
		b[0] = ((bytes5[0]&0x7F)>>0) | (bytes5[1]<<7);
		b[1] = ((bytes5[1]&0x7F)>>1) | (bytes5[2]<<6);
		b[2] = ((bytes5[2]&0x7F)>>2) | (bytes5[3]<<5);
		b[3] =  (bytes5[3]&0x7F)>>3;
		break;
	case 5:
		b[0] = ((bytes5[0]&0x7F)>>0) | (bytes5[1]<<7);
		b[1] = ((bytes5[1]&0x7F)>>1) | (bytes5[2]<<6);
		b[2] = ((bytes5[2]&0x7F)>>2) | (bytes5[3]<<5);
		b[3] = ((bytes5[3]&0x7F)>>3) | (bytes5[4]<<4);
		b[4] =  (bytes5[4]&0x7F)>>4;
		break;
	case 6:
	default:
		return false;
	}

	*p_i = static_cast<uint32_t>( (b[0] & 0x000000FF) | ((b[1] << 8) & 0x0000FF00) | ((b[2] << 16) & 0x00FF0000) | ((b[3] << 24) & 0xFF000000) );

	return true;
}

void _int_to_v( uint8_t* bytes5, int32_t* p_byte_num, uint32_t i )
{
	uint8_t a[ 5 ] = {};

	bytes5[ 0 ] = 0; a[ 0 ] = (uint8_t)( (i >> 0 ) & 0xFF );
	bytes5[ 1 ] = 0; a[ 1 ] = (uint8_t)( (i >> 8 ) & 0xFF );
	bytes5[ 2 ] = 0; a[ 2 ] = (uint8_t)( (i >> 16) & 0xFF );
	bytes5[ 3 ] = 0; a[ 3 ] = (uint8_t)( (i >> 24) & 0xFF );
	bytes5[ 4 ] = 0; a[ 4 ] = 0;

	// 1byte(7bit)
	if     ( i < 0x00000080 )
	{
		*p_byte_num = 1;
		bytes5[0]  = a[0];
	}

	// 2byte(14bit)
	else if( i < 0x00004000 )
	{
		*p_byte_num = 2;
		bytes5[0]  =             ((a[0]<<0)&0x7F) | 0x80;
		bytes5[1]  = (a[0]>>7) | ((a[1]<<1)&0x7F);
	}

	// 3byte(21bit)
	else if( i < 0x00200000 )
	{
		*p_byte_num = 3;
		bytes5[0] =             ((a[0]<<0)&0x7F) | 0x80;
		bytes5[1] = (a[0]>>7) | ((a[1]<<1)&0x7F) | 0x80;
		bytes5[2] = (a[1]>>6) | ((a[2]<<2)&0x7F);
	}

	// 4byte(28bit)
	else if( i < 0x10000000 )
	{
		*p_byte_num = 4;
		bytes5[0] =             ((a[0]<<0)&0x7F) | 0x80;
		bytes5[1] = (a[0]>>7) | ((a[1]<<1)&0x7F) | 0x80;
		bytes5[2] = (a[1]>>6) | ((a[2]<<2)&0x7F) | 0x80;
		bytes5[3] = (a[2]>>5) | ((a[3]<<3)&0x7F);
	}

	// 5byte(32bit)
	else
	{
		*p_byte_num = 5;
		bytes5[0] =             ((a[0]<<0)&0x7F) | 0x80;
		bytes5[1] = (a[0]>>7) | ((a[1]<<1)&0x7F) | 0x80;
		bytes5[2] = (a[1]>>6) | ((a[2]<<2)&0x7F) | 0x80;
		bytes5[3] = (a[2]>>5) | ((a[3]<<3)&0x7F) | 0x80;
		bytes5[4] = (a[3]>>4) | ((a[4]<<4)&0x7F);
	}
}

bool pxtnData::_io_read_le16 ( void* user, void* p_dst ) const
{
	uint8_t in[2] = {0, 0};
	uint16_t *out = reinterpret_cast<uint16_t*>( p_dst );

	bool ret = _io_read( user, (void*)in, 1, 2 );
	*out = static_cast<uint16_t>( (in[0] & 0x00FF) | ((in[1] << 8) & 0xFF00) );

	return ret;
}

bool pxtnData::_io_read_le32 ( void* user, void* p_dst ) const
{
	uint8_t in[4] = {0, 0, 0, 0};
	uint32_t *out = reinterpret_cast<uint32_t*>( p_dst );

	bool ret = _io_read( user, (void*)in, 1, 4 );
	*out = static_cast<uint32_t>( (in[0] & 0x000000FF) | ((in[1] << 8) & 0x0000FF00) | ((in[2] << 16) & 0x00FF0000) | ((in[3] << 24) & 0xFF000000) );

	return ret;
}

bool pxtnData::_io_read_le32f ( void *user, void *p_dst ) const
{
	uint8_t in[4] = {0, 0, 0, 0};

	union
	{
		float f;
		uint32_t ui32;
	} swapper;

	float *out = reinterpret_cast<float*>( p_dst );
	bool ret = _io_read( user, (void*)in, 1, 4 );
	swapper.ui32 = static_cast<uint32_t>( (in[0] & 0x000000FF) | ((in[1] << 8) & 0x0000FF00) | ((in[2] << 16) & 0x00FF0000) | ((in[3] << 24) & 0xFF000000) );
	*out = swapper.f;

	return ret;
}

bool pxtnData::_io_write_le16( void *user, const void *p_dst ) const
{
	uint8_t out[2] = {0, 0};
	uint16_t in = *reinterpret_cast<const uint16_t*>( p_dst );

	out[0] = ( (in >> 0) & 0xFF );
	out[1] = ( (in >> 8) & 0xFF );

	return _io_write( user, (void*)out, 1, 2 );
}

bool pxtnData::_io_write_le32(void *user, const void *p_dst ) const
{
	uint8_t out[4] = {0, 0};
	uint32_t in = *reinterpret_cast<const uint32_t*>( p_dst );

	out[0] = ( (in >>  0) & 0xFF );
	out[1] = ( (in >>  8) & 0xFF );
	out[2] = ( (in >> 16) & 0xFF );
	out[3] = ( (in >> 24) & 0xFF );

	return _io_write( user, (void*)out, 1, 4 );
}

bool pxtnData::_io_write_le32f( void *user, const void *p_dst ) const
{
	uint8_t out[4] = {0, 0};
	union
	{
		float f;
		uint32_t ui32;
	} swapper;

	swapper.f = *reinterpret_cast<const float*>( p_dst );

	out[0] = ( (swapper.ui32 >>  0) & 0xFF );
	out[1] = ( (swapper.ui32 >>  8) & 0xFF );
	out[2] = ( (swapper.ui32 >> 16) & 0xFF );
	out[3] = ( (swapper.ui32 >> 24) & 0xFF );

	return _io_write( user, (void*)out, 1, 4 );
}

int32_t pxtnData::_data_check_v_size ( uint32_t v ) const
{
	if     ( v < 0x00000080 ) return 1; // 1byte( 7bit)
	else if( v < 0x00004000 ) return 2; // 2byte(14bit)
	else if( v < 0x00200000 ) return 3; // 3byte(21bit)
	else if( v < 0x10000000 ) return 4; // 4byte(28bit)

	// 5byte(32bit)
	return 5;
}

bool pxtnData::_data_w_v( void* desc, int32_t v, int32_t* p_add ) const
{
	if( !desc ) return false;
	uint8_t a[ 5 ]   = {};
	int32_t byte_num =  0;
	_int_to_v( a, &byte_num, v );
	if( !_io_write( desc, a, 1,  byte_num ) ) return false;
	if( p_add ) *p_add += (  1 * byte_num );
	return true;
}

bool pxtnData::_data_r_v( void* desc, int32_t* pv ) const
{
	if( !desc ) return false;

	uint8_t  a[ 5 ] = {};
	int32_t  count = 0;
	for( count = 0; count < 5; count++ )
	{
		if( !_io_read( desc, &a[ count ], 1, 1 ) ) return false;
		if( !(                a[ count ] & 0x80) ) break;
	}

	uint32_t i = 0;
	if( !_v_to_int( &i, a, count + 1 ) ) return false;
	*pv = i;
	return true;
}

bool pxtnData::_data_get_size( void* desc, int32_t* p_size ) const
{
	int32_t pos = 0, pos_last = 0;
	if( !_io_pos ( desc, &pos          ) ) return false;
	if( !_io_seek( desc, SEEK_END,   0 ) ) return false;
	if( !_io_pos ( desc, &pos_last     ) ) return false;
	if( !_io_seek( desc, SEEK_SET, pos ) ) return false;
	*p_size = pos_last;
	return true;
}

void pxtnData::_set_io_funcs( pxtnIO_r io_read, pxtnIO_w io_write, pxtnIO_seek io_seek, pxtnIO_pos io_pos )
{
	_io_read  = io_read ;
	_io_write = io_write;
	_io_seek  = io_seek ;
	_io_pos   = io_pos  ;
}

bool pxtnData::copy_from( const pxtnData* src )
{
	_b_init   = src->_b_init  ;
	_io_read  = src->_io_read ;
	_io_write = src->_io_write;
	_io_seek  = src->_io_seek ;
	return true;
}


pxtnData::pxtnData()
{
	_b_init = false;
}

void pxtnData::_release()
{
	_b_init = false;
}

pxtnData::~pxtnData()
{
	_release();
}
