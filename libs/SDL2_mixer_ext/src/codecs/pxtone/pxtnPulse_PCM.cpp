
#include "./pxtn.h"

#include "./pxtnMem.h"
#include "./pxtnPulse_PCM.h"

typedef struct
{
	uint16_t formatID;     // PCM:0x0001
	uint16_t ch;           //
	uint32_t sps;          //
	uint32_t byte_per_sec; // byte per sec.
	uint16_t block_size;   //
	uint16_t bps;          // bit per sample.
	uint16_t ext;          // no use for pcm.
}
WAVEFORMATCHUNK;

#ifdef px_BIG_ENDIAN
px_FORCE_INLINE void swapEndian( WAVEFORMATCHUNK &format)
{
	format.formatID =       pxtnData::_swap16( format.formatID )    ;
	format.ch =             pxtnData::_swap16( format.ch )          ;
	format.sps =            pxtnData::_swap32( format.sps )         ;
	format.byte_per_sec =   pxtnData::_swap32( format.byte_per_sec );
	format.block_size =     pxtnData::_swap16( format.block_size )  ;
	format.bps =            pxtnData::_swap16( format.bps )         ;
	format.ext =            pxtnData::_swap16( format.ext )         ;
}
#endif

void pxtnPulse_PCM::Release()
{
	if( _p_smp ) { free( _p_smp ); } _p_smp = NULL;
	_ch       =    0;
	_sps      =    0;
	_bps      =    0;
	_smp_head =    0;
	_smp_body =    0;
	_smp_tail =    0;
}

pxtnPulse_PCM::pxtnPulse_PCM( pxtnIO_r io_read, pxtnIO_w io_write, pxtnIO_seek io_seek, pxtnIO_pos io_pos )
{
	_set_io_funcs( io_read, io_write, io_seek, io_pos );
	_p_smp    = NULL;
	Release();
}

pxtnPulse_PCM::~pxtnPulse_PCM()
{
	Release();
}

void *pxtnPulse_PCM::Devolve_SamplingBuffer()
{
	void *p = _p_smp;
	_p_smp = NULL;
	return p;
}


pxtnERR pxtnPulse_PCM::Create( int32_t ch, int32_t sps, int32_t bps, int32_t sample_num )
{
	Release();

	if( bps != 8 && bps != 16 ) return pxtnERR_pcm_unknown;

	int32_t size = 0;

	_p_smp    = NULL;
	_ch       = ch  ;
	_sps      = sps ;
	_bps      = bps ;
	_smp_head =    0;
	_smp_body = sample_num;
	_smp_tail =    0;

	// bit / sample is 8 or 16
	size = _smp_body * _bps * _ch / 8;

	if( !( _p_smp = (uint8_t*)malloc( size ) ) ) return pxtnERR_memory;

	if( _bps == 8 ) memset( _p_smp, 128, size );
	else            memset( _p_smp,   0, size );

	return pxtnOK;
}

pxtnERR pxtnPulse_PCM::read( void* desc )
{
	pxtnERR        res       = pxtnERR_VOID;
	char            buf[ 16 ] = { 0 };
	uint32_t        size      =   0  ;
	WAVEFORMATCHUNK format    = { 0 };

	_p_smp = NULL;

	// 'RIFFxxxxWAVEfmt '
	if( !_io_read( desc, buf, sizeof(char), 16 ) ){ res = pxtnERR_desc_r; goto term; }

	if( buf[ 0] != 'R' || buf[ 1] != 'I' || buf[ 2] != 'F' || buf[ 3] != 'F' ||
		buf[ 8] != 'W' || buf[ 9] != 'A' || buf[10] != 'V' || buf[11] != 'E' ||
		buf[12] != 'f' || buf[13] != 'm' || buf[14] != 't' || buf[15] != ' ' )
	{
		res = pxtnERR_pcm_unknown; goto term;
	}

	// read format.
	if( !_io_read_le32( desc ,&size                   ) ){ res = pxtnERR_desc_r     ; goto term; }
	if( !_io_read( desc, &format,               18, 1 ) ){ res = pxtnERR_desc_r     ; goto term; }
	swapEndian( format );

	if( format.formatID != 0x0001               ){ res = pxtnERR_pcm_unknown; goto term; }
	if( format.ch  != 1 && format.ch  !=  2     ){ res = pxtnERR_pcm_unknown; goto term; }
	if( format.bps != 8 && format.bps != 16     ){ res = pxtnERR_pcm_unknown; goto term; }

	// find 'data'
	if( !_io_seek( desc, SEEK_SET, 12 ) ){ res = pxtnERR_desc_r; goto term; } // skip 'RIFFxxxxWAVE'

	while( 1 )
	{
		if( !_io_read( desc, buf  , sizeof(char), 4 )      ){ res = pxtnERR_desc_r; goto term; }
		if( !_io_read_le32( desc, &size                  ) ){ res = pxtnERR_desc_r; goto term; }
		if( buf[0] == 'd' && buf[1] == 'a' && buf[2] == 't' && buf[3] == 'a' ) break;
		if( !_io_seek( desc, SEEK_CUR, size )              ){ res = pxtnERR_desc_r; goto term; }
	}

	res = Create( format.ch, format.sps, format.bps, size * 8 / format.bps / format.ch );
	if( res != pxtnOK ) goto term;

	if( !_io_read( desc, _p_smp, sizeof(uint8_t), size )   ){ res = pxtnERR_desc_r; goto term; }

#ifdef px_BIG_ENDIAN
	if( format.bps == 16 )
	{
		uint16_t *s = (uint16_t*)_p_smp;
		uint32_t len = size / 2;
		for( uint32_t i = 0; i < len; ++i, ++s ) { *s = pxtnData::_swap16(*s); }
	}
#endif

	res = pxtnOK;
term:

	if( res != pxtnOK && _p_smp ){ free( _p_smp ); _p_smp = NULL; }
	return res;
}

bool pxtnPulse_PCM::write  ( void* desc, const char* pstrLIST ) const
{
	if( !_p_smp ) return false;

	WAVEFORMATCHUNK format;
	bool            b_ret = false;
	uint32_t   riff_size;
	uint32_t   fact_size; // num sample.
	uint32_t   list_size; // num list text.
	uint32_t   isft_size;
	uint32_t   sample_size;

	bool bText;

	char tag_RIFF[4] = {'R','I','F','F'};
	char tag_WAVE[4] = {'W','A','V','E'};
	char tag_fmt_[8] = {'f','m','t',' ', 0x12,0,0,0};
	char tag_fact[8] = {'f','a','c','t', 0x04,0,0,0};
	char tag_data[4] = {'d','a','t','a'};
	char tag_LIST[4] = {'L','I','S','T'};
	char tag_INFO[8] = {'I','N','F','O','I','S','F','T'};


	if( pstrLIST && strlen( pstrLIST ) ) bText = true ;
	else                                 bText = false;

	sample_size         = ( _smp_head + _smp_body + _smp_tail ) * _ch * _bps / 8;

	format.formatID     = pxSwapLE16( 0x0001 );// PCM
	format.ch           = pxSwapLE16( (uint16_t) _ch  );
	format.sps          = pxSwapLE32( (uint32_t) _sps );
	format.bps          = pxSwapLE16( (uint16_t) _bps );
	format.byte_per_sec = pxSwapLE32( (uint32_t)(_sps * _bps * _ch / 8) );
	format.block_size   = pxSwapLE16( (uint16_t)(             _bps * _ch / 8) );
	format.ext          = 0;

	fact_size = ( _smp_head + _smp_body + _smp_tail );
	riff_size  = sample_size;
	riff_size +=  4;// 'WAVE'
	riff_size += 26;// 'fmt '
	riff_size += 12;// 'fact'
	riff_size +=  8;// 'data'

	if( bText )
	{
		isft_size = (uint32_t)strlen( pstrLIST );
		list_size = 4 + 4 + 4 + isft_size; // "INFO" + "ISFT" + size + ver_Text;
		riff_size +=  8 + list_size;// 'LIST'
	}
	else
	{
		isft_size = 0;
		list_size = 0;
	}

	// open file..

	if( !_io_write( desc, tag_RIFF,     sizeof(char    ), 4 ) ) goto End;
	if( !_io_write_le32( desc, &riff_size                   ) ) goto End;
	if( !_io_write( desc, tag_WAVE,     sizeof(char    ), 4 ) ) goto End;
	if( !_io_write( desc, tag_fmt_,     sizeof(char    ), 8 ) ) goto End;
	if( !_io_write( desc, &format,                    18, 1 ) ) goto End;

	if( bText )
	{
		if( !_io_write( desc, tag_LIST,     sizeof(char    ), 4 ) ) goto End;
		if( !_io_write_le32( desc, &list_size                   ) ) goto End;
		if( !_io_write( desc, tag_INFO,     sizeof(char    ), 8 ) ) goto End;
		if( !_io_write_le32( desc, &isft_size                   ) ) goto End;
		if( !_io_write( desc, pstrLIST,     sizeof(char    ), isft_size ) ) goto End;
	}

	if( !_io_write( desc, tag_fact,     sizeof(char    ), 8 ) ) goto End;
	if( !_io_write_le32( desc, &fact_size                   ) ) goto End;
	if( !_io_write( desc, tag_data,     sizeof(char    ), 4 ) ) goto End;
	if( !_io_write_le32( desc, &sample_size                 ) ) goto End;
#ifndef px_BIG_ENDIAN
	if( !_io_write( desc, _p_smp, sizeof(char), sample_size ) ) goto End;
#else
	if( _bps == 16 )
	{
		uint16_t *s = (uint16_t*)_p_smp;
		uint32_t len = sample_size / 2;
		for( uint32_t i = 0; i < len; ++i, ++s ) { if( !_io_write_le16( desc, s ) ) goto End; }
	}
#endif

	b_ret = true;

End:

	return b_ret;
}

// stereo / mono
bool pxtnPulse_PCM::_Convert_ChannelNum( int32_t new_ch )
{
	unsigned char *p_work = NULL;
	int32_t           sample_size;
	int32_t           work_size;
	int32_t           a,b;
	int32_t           temp1;
	int32_t           temp2;

	sample_size = ( _smp_head + _smp_body + _smp_tail ) * _ch * _bps / 8;

	if( _p_smp == NULL   ) return false;
	if( _ch    == new_ch ) return true ;


	// mono to stereo --------
	if( new_ch == 2 )
	{
		work_size = sample_size * 2;
		p_work     = (unsigned char *)malloc( work_size );
		if( !p_work ) return false;

		switch( _bps )
		{
		case  8:
			b = 0;
			for( a = 0; a < sample_size; a++ )
			{
				p_work[b  ] = _p_smp[a];
				p_work[b+1] = _p_smp[a];
				b += 2;
			}
			break;
		case 16:
			b = 0;
			for( a = 0; a < sample_size; a += 2 )
			{
				p_work[b  ] = _p_smp[a  ];
				p_work[b+1] = _p_smp[a+1];
				p_work[b+2] = _p_smp[a  ];
				p_work[b+3] = _p_smp[a+1];
				b += 4;
			}
			break;
		}

	}
	// stereo to mono --------
	else
	{
		work_size = sample_size / 2;
		p_work     = (unsigned char *)malloc( work_size );
		if( !p_work ) return false;

		switch( _bps )
		{
		case  8:
			b = 0;
			for( a = 0; a < sample_size; a+= 2 )
			{
				temp1       = (int32_t)_p_smp[a] + (int32_t)_p_smp[a+1];
				p_work[b  ] = (uint8_t)( temp1 / 2 );
				b++;
			}
			break;
		case 16:
			b = 0;
			for( a = 0; a < sample_size; a += 4 )
			{
				temp1                = *((int16_t *)(&_p_smp[a  ]));
				temp2                = *((int16_t *)(&_p_smp[a+2]));
				*(int16_t *)(&p_work[b]) =   (int16_t)( ( temp1 + temp2 ) / 2 );
				b += 2;
			}
			break;
		}
	}

	// release once.
	free( _p_smp );
	_p_smp = NULL;

	if( !( _p_smp = (uint8_t*)malloc( work_size ) ) ){ free( p_work ); return false; }
	memcpy( _p_smp, p_work, work_size );
	free( p_work );

	// update param.
	_ch = new_ch;

	return true;
}

// change bps
bool pxtnPulse_PCM::_Convert_BitPerSample( int32_t new_bps )
{
	unsigned char *p_work;
	int32_t           sample_size;
	int32_t           work_size;
	int32_t           a,b;
	int32_t           temp1;

	if( !_p_smp         ) return false;
	if( _bps == new_bps ) return true ;

	sample_size = ( _smp_head + _smp_body + _smp_tail ) * _ch * _bps / 8;

	switch( new_bps )
	{
	// 16 to 8 --------
	case  8:
		work_size = sample_size / 2;
		p_work     = (uint8_t*)malloc( work_size );
		if( !p_work ) return false;
		b = 0;
		for( a = 0; a < sample_size; a += 2 )
		{
			temp1 = *((int16_t*)(&_p_smp[a]));
			temp1 = (temp1/0x100) + 128;
			p_work[b] = (uint8_t)temp1;
			b++;
		}
		break;
	//  8 to 16 --------
	case 16:
		work_size = sample_size * 2;
		p_work     = (uint8_t*)malloc( work_size );
		if( !p_work ) return false;
		b = 0;
		for( a = 0; a < sample_size; a++ )
		{
			temp1 = _p_smp[a];
			temp1 = ( temp1 - 128 ) * 0x100;
			*((int16_t *)(&p_work[b])) = (int16_t)temp1;
			b += 2;
		}
		break;

	default: return false;
	}

	// release once.
	free( _p_smp );
	_p_smp = NULL;

	if( !( _p_smp = (uint8_t*)malloc( work_size ) ) ){ free( p_work ); return false; }
	memcpy( _p_smp, p_work, work_size );
	free( p_work );

	// update param.
	_bps = new_bps;

	return true;
}

// sps
bool pxtnPulse_PCM::_Convert_SamplePerSecond( int32_t new_sps )
{
	bool     b_ret = false;
	int32_t  sample_num;
	int32_t  work_size;

	int32_t  head_size, body_size, tail_size;

	uint8_t  *p1byte_data;
	uint16_t *p2byte_data;
	uint32_t *p4byte_data;

	uint8_t  *p1byte_work = NULL;
	uint16_t *p2byte_work = NULL;
	uint32_t *p4byte_work = NULL;


	int32_t a, b;

	if( !_p_smp         ) return false;
	if( _sps == new_sps ) return true ;

	head_size = _smp_head * _ch * _bps / 8;
	body_size = _smp_body * _ch * _bps / 8;
	tail_size = _smp_tail * _ch * _bps / 8;

	head_size = (int32_t)( ( (double)head_size * (double)new_sps + (double)(_sps) - 1 ) / _sps );
	body_size = (int32_t)( ( (double)body_size * (double)new_sps + (double)(_sps) - 1 ) / _sps );
	tail_size = (int32_t)( ( (double)tail_size * (double)new_sps + (double)(_sps) - 1 ) / _sps );

	work_size = head_size + body_size + tail_size;

	// stereo 16bit ========
	if(     _ch == 2 && _bps == 16 )
	{
		_smp_head   = head_size  / 4;
		_smp_body   = body_size  / 4;
		_smp_tail   = tail_size  / 4;
		sample_num  = work_size  / 4;
		work_size   = sample_num * 4;
		p4byte_data = (uint32_t *)_p_smp;
		if( !pxtnMem_zero_alloc( (void **)&p4byte_work, work_size ) ) goto End;
		for( a = 0; a < sample_num; a++ )
		{
			b = (int32_t)( (double)a * (double)(_sps) / (double)new_sps );
			p4byte_work[a] = p4byte_data[b];
		}
	}
	// mono 8bit ========
	else if( _ch == 1 && _bps ==  8 )
	{
		_smp_head = head_size  / 1;
		_smp_body = body_size  / 1;
		_smp_tail = tail_size  / 1;
		sample_num      = work_size  / 1;
		work_size       = sample_num * 1;
		p1byte_data     = (uint8_t*)_p_smp;
		if( !pxtnMem_zero_alloc( (void **)&p1byte_work, work_size ) ) goto End;
		for( a = 0; a < sample_num; a++ )
		{
			b = (int32_t)( (double)a * (double)(_sps) / (double)(new_sps) );
			p1byte_work[a] = p1byte_data[b];
		}
	}
	else
	// mono 16bit / stereo 8bit ========
	{
		_smp_head = head_size  / 2;
		_smp_body = body_size  / 2;
		_smp_tail = tail_size  / 2;
		sample_num      = work_size  / 2;
		work_size       = sample_num * 2;
		p2byte_data     = (uint16_t*)_p_smp;
		if( !pxtnMem_zero_alloc( (void**)&p2byte_work, work_size ) ) goto End;
		for( a = 0; a < sample_num; a++ )
		{
			b = (int32_t)( (double)a * (double)(_sps) / (double)new_sps );
			p2byte_work[a] = p2byte_data[b];
		}
	}

	// release once.
	pxtnMem_free( (void **)&_p_smp );
	if( !pxtnMem_zero_alloc( (void **)&_p_smp, work_size ) ) goto End;

	if(      p4byte_work ) memcpy( _p_smp, p4byte_work, work_size );
	else if( p2byte_work ) memcpy( _p_smp, p2byte_work, work_size );
	else if( p1byte_work ) memcpy( _p_smp, p1byte_work, work_size );
	else goto End;

	// update.
	_sps = new_sps;

	b_ret = true;
End:

	if( !b_ret )
	{
		pxtnMem_free( (void **)&_p_smp );
		_smp_head = 0;
		_smp_body = 0;
		_smp_tail = 0;
	}

	pxtnMem_free( (void **)&p2byte_work  );
	pxtnMem_free( (void **)&p1byte_work  );
	pxtnMem_free( (void **)&p4byte_work  );

	return b_ret;
}

// convert..
bool pxtnPulse_PCM::Convert( int32_t new_ch, int32_t new_sps, int32_t new_bps )
{
	if( !_Convert_ChannelNum     ( new_ch  ) ) return false;
	if( !_Convert_BitPerSample   ( new_bps ) ) return false;
	if( !_Convert_SamplePerSecond( new_sps ) ) return false;

	return true;
}

bool pxtnPulse_PCM::Convert_Volume( float v )
{
	if( !_p_smp ) return false;

	int32_t sample_num = ( _smp_head + _smp_body + _smp_tail ) * _ch;

	switch( _bps )
	{
	case  8:
		{
			uint8_t *p8 = (uint8_t*)_p_smp;
			for( int32_t i = 0; i < sample_num; i++ )
			{
				*p8 = (uint8_t)( ( ( (float)(*p8) - 128 ) * v ) + 128 );
				p8++;
			}
			break;
		}
	case 16:
		{
			short *p16 = (int16_t *)_p_smp;
			for( int32_t i = 0; i < sample_num; i++ )
			{
				*p16 = (int16_t)( (float)*p16 * v );
				p16++;
			}
			break;
		}
	default:
		return false;
	}
	return true;
}

bool pxtnPulse_PCM::copy_from( const pxtnPulse_PCM *src )
{
	pxtnData::copy_from( src );

	pxtnERR res = pxtnERR_VOID;
	if( !src->_p_smp ){ Release(); return true; }
	res = Create( src->_ch, src->_sps, src->_bps, src->_smp_body );
	if( res != pxtnOK ) return false;
	memcpy( _p_smp, src->_p_smp, ( src->_smp_head + src->_smp_body + src->_smp_tail ) * src->_ch * src->_bps / 8 );
	return true;
}

bool pxtnPulse_PCM::Copy_( pxtnPulse_PCM *p_dst, int32_t start, int32_t end ) const
{
	int32_t size, offset;

	if( _smp_head || _smp_tail     ) return false;
	if( !_p_smp ){ p_dst->Release(); return true; }

	size   = ( end - start ) * _ch * _bps / 8;
	offset =         start   * _ch * _bps / 8;

	if( p_dst->Create( _ch, _sps, _bps, end - start ) != pxtnOK ) return false;

	memcpy( p_dst->_p_smp, &_p_smp[ offset ], size );

	return true;
}

int32_t     pxtnPulse_PCM::get_ch            () const{ return _ch      ; }
int32_t     pxtnPulse_PCM::get_bps           () const{ return _bps     ; }
int32_t     pxtnPulse_PCM::get_sps           () const{ return _sps     ; }
int32_t     pxtnPulse_PCM::get_smp_body      () const{ return _smp_body; }
int32_t     pxtnPulse_PCM::get_smp_head      () const{ return _smp_head; }
int32_t     pxtnPulse_PCM::get_smp_tail      () const{ return _smp_tail; }

const void *pxtnPulse_PCM::get_p_buf         () const{ return _p_smp   ; }
void       *pxtnPulse_PCM::get_p_buf_variable() const{ return _p_smp   ; }

float pxtnPulse_PCM::get_sec   () const
{
	return (float)(_smp_body+_smp_head+_smp_tail) / (float)_sps;
}

int32_t pxtnPulse_PCM::get_buf_size() const
{
	return ( _smp_head + _smp_body + _smp_tail ) * _ch * _bps / 8;
}
