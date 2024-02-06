
#include "./pxtn.h"

#ifdef pxINCLUDE_OGGVORBIS

#if defined(pxINCLUDE_OGGVORBIS_STB)
#ifdef pxUseSDL
#   include "SDL_stdinc.h"
#   include "SDL_version.h"
#   include "SDL_rwops.h"
#   include "SDL_assert.h"
#   define STB_VORBIS_SDL 1 /* for SDL_mixer-specific stuff. */
#   define STB_FORCEINLINE SDL_FORCE_INLINE
#   define STB_VORBIS_NO_CRT 1
#endif
#define STB_VORBIS_NO_STDIO 1
#define STB_VORBIS_NO_PUSHDATA_API 1
#define STB_VORBIS_MAX_CHANNELS 8   /* For 7.1 surround sound */
#if px_IS_BIG_ENDIAN
#   define STB_VORBIS_BIG_ENDIAN 1
#endif

#ifdef pxUseSDL
#   define STBV_CDECL SDLCALL /* for SDL_qsort */
#   ifdef assert
#       undef assert
#   endif
#   ifdef memset
#       undef memset
#   endif
#   ifdef memcpy
#       undef memcpy
#   endif
#   define assert SDL_assert
#   define memset SDL_memset
#   define memcmp SDL_memcmp
#   define memcpy SDL_memcpy
#   define qsort SDL_qsort
#   define malloc SDL_malloc
#   define realloc SDL_realloc
#   define free SDL_free

#   define pow SDL_pow
#   define floor SDL_floor
#   define ldexp(v, e) SDL_scalbn((v), (e))
#   define abs(x) SDL_abs(x)
#   define cos(x) SDL_cos(x)
#   define sin(x) SDL_sin(x)
#   define log(x) SDL_log(x)
#   if SDL_VERSION_ATLEAST(2, 0, 9)
#       define exp(x) SDL_exp(x) /* Available since SDL 2.0.9 */
#   endif
#else
#   include <stddef.h>
#   include <assert.h>
#   include <string.h>
#   include <math.h>
#endif // pxUseSDL

/* Workaround to don't conflict with another statically-linked stb-vorbis */
#define stb_vorbis_get_info _pxtn_stb_vorbis_get_info
#define stb_vorbis_get_comment _pxtn_stb_vorbis_get_comment
#define stb_vorbis_get_error _pxtn_stb_vorbis_get_error
#define stb_vorbis_close _pxtn_stb_vorbis_close
#define stb_vorbis_get_sample_offset _pxtn_stb_vorbis_get_sample_offset
#define stb_vorbis_get_playback_sample_offset _pxtn_stb_vorbis_get_playback_sample_offset
#define stb_vorbis_get_file_offset _pxtn_stb_vorbis_get_file_offset
#define stb_vorbis_open_pushdata _pxtn_stb_vorbis_open_pushdata
#define stb_vorbis_decode_frame_pushdata _pxtn_stb_vorbis_decode_frame_pushdata
#define stb_vorbis_flush_pushdata _pxtn_stb_vorbis_flush_pushdata
#define stb_vorbis_decode_filename _pxtn_stb_vorbis_decode_filename
#define stb_vorbis_decode_memory _pxtn_stb_vorbis_decode_memory
#define stb_vorbis_open_memory _pxtn_stb_vorbis_open_memory
#define stb_vorbis_open_filename _pxtn_stb_vorbis_open_filename
#define stb_vorbis_open_file _pxtn_stb_vorbis_open_file
#define stb_vorbis_open_file_section _pxtn_stb_vorbis_open_file_section
#define stb_vorbis_open_rwops_section _pxtn_stb_vorbis_open_rwops_section
#define stb_vorbis_open_rwops _pxtn_stb_vorbis_open_rwops
#define stb_vorbis_seek_frame _pxtn_stb_vorbis_seek_frame
#define stb_vorbis_seek _pxtn_stb_vorbis_seek
#define stb_vorbis_seek_start _pxtn_stb_vorbis_seek_start
#define stb_vorbis_stream_length_in_samples _pxtn_stb_vorbis_stream_length_in_samples
#define stb_vorbis_stream_length_in_seconds _pxtn_stb_vorbis_stream_length_in_seconds
#define stb_vorbis_get_frame_float _pxtn_stb_vorbis_get_frame_float
#define stb_vorbis_get_frame_short_interleaved _pxtn_stb_vorbis_get_frame_short_interleaved
#define stb_vorbis_get_frame_short _pxtn_stb_vorbis_get_frame_short
#define stb_vorbis_get_samples_float_interleaved _pxtn_stb_vorbis_get_samples_float_interleaved
#define stb_vorbis_get_samples_float _pxtn_stb_vorbis_get_samples_float
#define stb_vorbis_get_samples_short_interleaved _pxtn_stb_vorbis_get_samples_short_interleaved
#define stb_vorbis_get_samples_short _pxtn_stb_vorbis_get_samples_short
/* Workaround to don't conflict with another statically-linked stb-vorbis: END */

#include "../stb_vorbis/stb_vorbis.h"

#elif defined(pxINCLUDE_OGGVORBIS_TREMOR)
#include <tremor/ivorbisfile.h>
#else
#define OV_EXCLUDE_STATIC_CALLBACKS
#include <vorbis/codec.h>
#include <vorbis/vorbisfile.h>
#endif

#include "./pxtnPulse_Oggv.h"

#if defined(__3DS__)
typedef int pxogg_int32_t;
#else
typedef int32_t pxogg_int32_t;
#endif

#if !defined(pxINCLUDE_OGGVORBIS_STB)
typedef struct
{
    char*   p_buf; // ogg vorbis-data on memory.s
    int32_t size ; //
    int32_t pos  ; // reading position.
}
OVMEM;


// 4 callbacks below:

static size_t _mread( void *p, size_t size, size_t nmemb, void* p_void )
{
	OVMEM *pom = (OVMEM*)p_void;

	if( !pom                  ) return -1;
	if( pom->pos >= pom->size ) return  0;
	if( pom->pos == -1        ) return  0;

	int32_t left = pom->size - pom->pos;

	if( (int32_t)(size * nmemb) >= left )
	{
		memcpy( p, &pom->p_buf[ pom->pos ], pom->size - pom->pos );
		pom->pos = pom->size;
		return left / size;
	}

	memcpy( p, &pom->p_buf[ pom->pos ], nmemb * size );
	pom->pos += (int32_t)( nmemb * size );

	return nmemb;
}

static pxogg_int32_t _mseek( void* p_void, ogg_int64_t offset, pxogg_int32_t mode )
{
	pxogg_int32_t newpos;
	OVMEM *pom = (OVMEM*)p_void;

	if( !pom || pom->pos < 0 )       return -1;
	if( offset < 0 ){ pom->pos = -1; return -1; }

	switch( mode )
	{
	case SEEK_SET: newpos =             (pxogg_int32_t)offset; break;
	case SEEK_CUR: newpos = pom->pos  + (pxogg_int32_t)offset; break;
	case SEEK_END: newpos = pom->size + (pxogg_int32_t)offset; break;
	default: return -1;
	}
	if( newpos < 0 ) return -1;

	pom->pos = newpos;

	return 0;
}

static long _mtell( void* p_void )
{
	OVMEM* pom = (OVMEM*)p_void;
	if( !pom ) return -1;
	return pom->pos;
}

static pxogg_int32_t _mclose_dummy( void* p_void )
{
	OVMEM* pom = (OVMEM*)p_void;
	if( !pom ) return -1;
	return 0;
}
#endif

bool pxtnPulse_Oggv::_SetInformation()
{
	bool b_ret = false;

#if !defined(pxINCLUDE_OGGVORBIS_STB)
	OVMEM ovmem;
	ovmem.p_buf = _p_data;
	ovmem.pos   =       0;
	ovmem.size  = _size  ;

	// set callback func.
	ov_callbacks   oc;
	oc.read_func  = _mread       ;
	oc.seek_func  = _mseek       ;
	oc.close_func = _mclose_dummy;
	oc.tell_func  = _mtell       ;

	bool           vf_loaded = false;
	OggVorbis_File vf;

	vorbis_info*  vi;

	switch( ov_open_callbacks( &ovmem, &vf, NULL, 0, oc ) )
	{
	case OV_EREAD     : goto End; //{printf("A read from media returned an error.\n");exit(1);}
	case OV_ENOTVORBIS: goto End; //{printf("Bitstream is not Vorbis data. \n");exit(1);}
	case OV_EVERSION  : goto End; //{printf("Vorbis version mismatch. \n");exit(1);}
	case OV_EBADHEADER: goto End; //{printf("Invalid Vorbis bitstream header. \n");exit(1);}
	case OV_EFAULT    : goto End; //{printf("Internal logic fault; indicates a bug or heap/stack corruption. \n");exit(1);}
	default:
		break;
	}

	vf_loaded = true;

	vi = ov_info( &vf,-1 );

	_ch      = vi->channels;
	_sps2    = vi->rate    ;
	_smp_num = (int32_t)ov_pcm_total( &vf, -1 );

	// end.
#else

	stb_vorbis *vf = NULL;

	stb_vorbis_info vi;

	vf = stb_vorbis_open_memory( (const uint8_t*)_p_data, _size, NULL, NULL );
	if( vf == NULL ) { goto End; }

	vi = stb_vorbis_get_info( vf );

	_ch      = vi.channels;
	_sps2    = vi.sample_rate;
	_smp_num = (int32_t)stb_vorbis_stream_length_in_samples( vf );

#endif

	b_ret = true;

End:
#if !defined(pxINCLUDE_OGGVORBIS_STB)
    if( vf_loaded ) ov_clear( &vf );
#else
    if( vf ) stb_vorbis_close( vf );
#endif
    return b_ret;

}


/////////////////
// global
/////////////////


pxtnPulse_Oggv::pxtnPulse_Oggv( pxtnIO_r io_read, pxtnIO_w io_write, pxtnIO_seek io_seek, pxtnIO_pos io_pos )
{
	_set_io_funcs( io_read, io_write, io_seek, io_pos );

	_p_data  = NULL;
	_ch      = 0;
	_sps2    = 0;
	_smp_num = 0;
	_size    = 0;
}

pxtnPulse_Oggv::~pxtnPulse_Oggv()
{
	Release();
}

void pxtnPulse_Oggv::Release()
{
	if( _p_data ) { free( _p_data ); } _p_data = NULL;
	_ch      = 0;
	_sps2    = 0;
	_smp_num = 0;
	_size    = 0;
}

pxtnERR pxtnPulse_Oggv::ogg_read( void* desc )
{
	pxtnERR res = pxtnERR_VOID;

	if( !_data_get_size( desc, &_size ) || !_size ){ res = pxtnERR_desc_r; goto term; }
	if( !( _p_data = (char*)malloc( _size ) )     ){ res = pxtnERR_memory; goto term; }
	if( !_io_read( desc, _p_data, 1, _size )      ){ res = pxtnERR_desc_r; goto term; }
	if( !_SetInformation()                        ) goto term;

	res = pxtnOK;
term:

	if( res != pxtnOK )
	{
		if( _p_data ) { free( _p_data ); } _p_data = NULL; _size = 0;
	}
	return res;
}

pxtnERR pxtnPulse_Oggv::Decode( pxtnPulse_PCM * p_pcm ) const
{
	pxtnERR res = pxtnERR_VOID;

#if !defined(pxINCLUDE_OGGVORBIS_STB)
	bool           vf_loaded = false;
	OggVorbis_File vf;
	vorbis_info*   vi;
	ov_callbacks   oc;

	pxogg_int32_t current_section;
	char    pcmout[ 4096 ] = {0};

	OVMEM ovmem;

	ovmem.p_buf = _p_data;
	ovmem.pos   =       0;
	ovmem.size  = _size  ;

	// set callback func.
	oc.read_func  = _mread       ;
	oc.seek_func  = _mseek       ;
	oc.close_func = _mclose_dummy;
	oc.tell_func  = _mtell       ;

	switch( ov_open_callbacks( &ovmem, &vf, NULL, 0, oc ) )
	{
	case OV_EREAD     : res = pxtnERR_ogg; goto term; //{printf("A read from media returned an error.\n");exit(1);}
	case OV_ENOTVORBIS: res = pxtnERR_ogg; goto term; //{printf("Bitstream is not Vorbis data. \n");exit(1);}
	case OV_EVERSION  : res = pxtnERR_ogg; goto term; //{printf("Vorbis version mismatch. \n");exit(1);}
	case OV_EBADHEADER: res = pxtnERR_ogg; goto term; //{printf("Invalid Vorbis bitstream header. \n");exit(1);}
	case OV_EFAULT    : res = pxtnERR_ogg; goto term; //{printf("Internal logic fault; indicates a bug or heap/stack corruption. \n");exit(1);}
	default: break;
	}

	vf_loaded = true;

	vi    = ov_info( &vf,-1 );

	//take 4k out of the data segment, not the stack
	{
		int32_t smp_num = (int32_t)ov_pcm_total( &vf, -1 );
		// uint32_t bytes = vi->channels * 2 * smp_num;

		res = p_pcm->Create( vi->channels, vi->rate, 16, smp_num );
		if( res != pxtnOK ) goto term;
	}
	// decode..
	{
		int32_t ret = 0;
		uint8_t  *p  = (uint8_t*)p_pcm->get_p_buf_variable();
		do
		{
#if defined(pxINCLUDE_OGGVORBIS_TREMOR)
			ret = ov_read( &vf, pcmout, 4096, &current_section );
#else
			ret = ov_read( &vf, pcmout, 4096, px_IS_BIG_ENDIAN, 2, 1, &current_section );
#endif
			if( ret > 0 ) memcpy( p, pcmout, ret ); //fwrite( pcmout, 1, ret, of );
			p += ret;
		}
		while( ret );
	}

	// end.
#else // pxINCLUDE_OGGVORBIS_STB

	int error;
	stb_vorbis *vf = NULL;
	stb_vorbis_info vi;
	char    pcmout[ 4096 ] = {0};

	vf = stb_vorbis_open_memory( (const uint8_t*)_p_data, _size, &error, NULL );
	if( vf == NULL ) { res = pxtnERR_ogg; goto term; }

	vi = stb_vorbis_get_info( vf );

	//take 4k out of the data segment, not the stack
	{
		int32_t smp_num = (int32_t)stb_vorbis_stream_length_in_samples( vf );
		// uint32_t bytes = vi->channels * 2 * smp_num;

		res = p_pcm->Create( vi.channels, vi.sample_rate, 16, smp_num );
		if( res != pxtnOK ) goto term;
	}

	// decode..
	{
		int32_t ret = 0;
		uint8_t  *p  = (uint8_t*)p_pcm->get_p_buf_variable();
		do
		{
			ret = stb_vorbis_get_samples_short_interleaved( vf, vi.channels, (short*)pcmout, sizeof(pcmout) / sizeof(short) );
			ret *= vi.channels * sizeof(short);
			if( ret > 0 ) memcpy( p, pcmout, ret ); //fwrite( pcmout, 1, ret, of );
			p += ret;
		}
		while( ret );
	}

#endif // pxINCLUDE_OGGVORBIS_STB

	res = pxtnOK;

term:
#if !defined(pxINCLUDE_OGGVORBIS_STB)
    if( vf_loaded ) ov_clear( &vf );
#else
    if( vf ) stb_vorbis_close( vf );
#endif

    return res;
}

bool pxtnPulse_Oggv::GetInfo( int32_t* p_ch, int32_t* p_sps, int32_t* p_smp_num )
{
	if( !_p_data ) return false;

	if( p_ch      ) *p_ch      = _ch     ;
	if( p_sps     ) *p_sps     = _sps2   ;
	if( p_smp_num ) *p_smp_num = _smp_num;

	return true;
}

int32_t  pxtnPulse_Oggv::GetSize() const
{
	if( !_p_data ) return 0;
	return sizeof(int32_t)*4 + _size;
}



bool pxtnPulse_Oggv::ogg_write( void* desc ) const
{
	bool    b_ret  = false;

	if( !_io_write( desc, _p_data, 1,_size ) ) goto End;

	b_ret = true;
End:
	return b_ret;
}

bool pxtnPulse_Oggv::pxtn_write( void* desc ) const
{
	if( !_p_data ) return false;

	if( !_io_write_le32( desc, &_ch                      ) ) return false;
	if( !_io_write_le32( desc, &_sps2                    ) ) return false;
	if( !_io_write_le32( desc, &_smp_num                 ) ) return false;
	if( !_io_write_le32( desc, &_size                    ) ) return false;
	if( !_io_write( desc,  _p_data , sizeof(char), _size ) ) return false;

	return true;
}

bool pxtnPulse_Oggv::pxtn_read( void* desc )
{
	bool  b_ret  = false;

	if( !_io_read_le32( desc, &_ch      ) ) goto End;
	if( !_io_read_le32( desc, &_sps2    ) ) goto End;
	if( !_io_read_le32( desc, &_smp_num ) ) goto End;
	if( !_io_read_le32( desc, &_size    ) ) goto End;

	if( !_size ) goto End;

	if( !( _p_data = (char*)malloc( _size ) ) ) goto End;
	if( !_io_read( desc, _p_data, 1,      _size )   ) goto End;

	b_ret = true;
End:

	if( !b_ret )
	{
		if( _p_data ) { free( _p_data ); } _p_data = NULL; _size = 0;
	}

	return b_ret;
}

bool pxtnPulse_Oggv::copy_from ( const pxtnPulse_Oggv* src )
{
	pxtnData::copy_from( src );

	Release();
	if( !src->_p_data ) return true;

	if( !( _p_data = (char*)malloc( src->_size ) ) ) return false;
	memcpy( _p_data, src->_p_data, src->_size );

	_ch      = src->_ch     ;
	_sps2    = src->_sps2   ;
	_size    = src->_size   ;
	_smp_num = src->_smp_num;

	return true;
}

#endif
