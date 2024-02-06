// '17/10/11 pxtnData.

#ifndef pxtnData_H
#define pxtnData_H

#include "pxtn.h"

#ifdef px_BIG_ENDIAN
#   define pxSwapLE16(x)    pxtnData::_swap16(x)
#   define pxSwapLE32(x)    pxtnData::_swap32(x)
#   define pxSwapFloatLE(x) pxtnData::_swap_float(x)
#else
#   define pxSwapLE16(x)    (x)
#   define pxSwapLE32(x)    (x)
#   define pxSwapFloatLE(x) (x)
#   define swapEndian(x)    (void)x
#endif

typedef bool (* pxtnIO_r   )( void* user,       void* p_dst, int32_t size, int32_t num );
typedef bool (* pxtnIO_w   )( void* user, const void* p_dst, int32_t size, int32_t num );
typedef bool (* pxtnIO_seek)( void* user,     int32_t mode , int32_t size              );
typedef bool (* pxtnIO_pos )( void* user,                    int32_t* p_pos            );

class pxtnData
{
private:

	void operator = (const pxtnData& src){ (void)src; }
	pxtnData        (const pxtnData& src){ (void)src; }

protected:

	bool        _b_init  ;

	pxtnIO_r    _io_read ;
	bool		_io_read_le16   ( void* user, void* p_dst )       const;
	bool		_io_read_le32   ( void* user, void* p_dst )       const;
	bool		_io_read_le32f  ( void* user, void* p_dst )       const;
	pxtnIO_w    _io_write;
	bool		_io_write_le16  ( void* user, const void* p_dst ) const;
	bool		_io_write_le32  ( void* user, const void* p_dst ) const;
	bool		_io_write_le32f ( void* user, const void* p_dst ) const;
	pxtnIO_seek _io_seek ;
	pxtnIO_pos  _io_pos  ;

	void _release();

	bool    _data_w_v     ( void* desc, int32_t   v, int32_t* p_add ) const;
	bool    _data_r_v     ( void* desc, int32_t* pv                 ) const;
	bool    _data_get_size( void* desc, int32_t* p_size             ) const;
	int32_t _data_check_v_size( uint32_t v ) const;

	void _set_io_funcs( pxtnIO_r io_read, pxtnIO_w io_write, pxtnIO_seek io_seek, pxtnIO_pos io_pos );

public :

	pxtnData();
	virtual ~pxtnData();

	bool copy_from( const pxtnData* src );

	bool init();
	bool Xxx ();


	// Misc. functions

	static inline float cast_to_float( int32_t in )
	{
		union
		{
			float f;
			int32_t i;
		} v;
		v.i = in;
		return v.f;
	}

	static inline int32_t cast_to_int( float in )
	{
		union
		{
			float f;
			int32_t i;
		} v;
		v.f = in;
		return v.i;
	}

#ifdef px_BIG_ENDIAN
	static inline uint16_t _swap16( uint16_t x )
	{
		return static_cast<uint16_t>(((x << 8) & 0xFF00) | ((x >> 8) & 0x00FF));
	}

	static inline uint32_t _swap32( uint32_t x )
	{
		return static_cast<uint32_t>(((x << 24) & 0xFF000000) | ((x << 8) & 0x00FF0000) | ((x >> 8) & 0x0000FF00) | ((x >> 24) & 0x000000FF));
	}

	static inline float _swap_float( float x )
	{
		union
		{
			float f;
			uint32_t ui32;
		} swapper;
		swapper.f = x;
		swapper.ui32 = _swap32( swapper.ui32 );
		return swapper.f;
	}
#endif /* PXTONE_BIG_ENDIAN */
};

#endif
