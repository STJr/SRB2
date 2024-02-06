// '12/03/06

#include "./pxtn.h"

#include "./pxtnWoice.h"

//////////////////////
// matePCM
//////////////////////

// 24byte =================
typedef struct
{
	uint16_t x3x_unit_no;
	uint16_t basic_key  ;
	uint32_t voice_flags;
	uint16_t ch         ;
	uint16_t bps        ;
	uint32_t sps        ;
	float    tuning     ;
	uint32_t data_size  ;
}
_MATERIALSTRUCT_PCM;

#ifdef px_BIG_ENDIAN
px_FORCE_INLINE void swapEndian( _MATERIALSTRUCT_PCM &pcm)
{
	pcm.x3x_unit_no = pxtnData::_swap16( pcm.x3x_unit_no );
	pcm.basic_key =   pxtnData::_swap16( pcm.basic_key )  ;
	pcm.voice_flags = pxtnData::_swap32( pcm.voice_flags );
	pcm.ch =          pxtnData::_swap16( pcm.ch )         ;
	pcm.bps =         pxtnData::_swap16( pcm.bps )        ;
	pcm.sps =         pxtnData::_swap32( pcm.sps )        ;
	pcm.tuning =      pxtnData::_swap_float( pcm.tuning ) ;
	pcm.data_size =   pxtnData::_swap32( pcm.data_size )  ;
}
#endif

bool pxtnWoice::io_matePCM_w( void* desc ) const
{
	const pxtnPulse_PCM* p_pcm =  _voices[ 0 ].p_pcm;
	pxtnVOICEUNIT*       p_vc  = &_voices[ 0 ];
	_MATERIALSTRUCT_PCM  pcm;

	memset( &pcm, 0, sizeof( _MATERIALSTRUCT_PCM ) );

	pcm.sps         = pxSwapLE32((uint32_t)p_pcm->get_sps     ());
	pcm.bps         = pxSwapLE16((uint16_t)p_pcm->get_bps     ());
	pcm.ch          = pxSwapLE16((uint16_t)p_pcm->get_ch      ());
	pcm.data_size   = pxSwapLE32((uint32_t)p_pcm->get_buf_size());
	pcm.x3x_unit_no = (uint16_t)0;
	pcm.tuning      = pxSwapFloatLE(p_vc->tuning)          ;
	pcm.voice_flags = pxSwapLE32(p_vc->voice_flags)        ;
	pcm.basic_key   = pxSwapLE16((uint16_t)p_vc->basic_key);

	uint32_t size = sizeof( _MATERIALSTRUCT_PCM ) + pcm.data_size;

	if( !_io_write_le32( desc, &size                            ) ) return false;
	if( !_io_write( desc, &pcm , sizeof(_MATERIALSTRUCT_PCM), 1 ) ) return false;
#ifndef px_BIG_ENDIAN
	if( !_io_write( desc, p_pcm->get_p_buf(), 1, pcm.data_size  ) ) return false;
#else
	if( p_pcm->get_bps() == 16 )
	{
		uint16_t *s = (uint16_t*)p_pcm->get_p_buf();
		uint32_t len = p_pcm->get_buf_size() / 2;
		for(uint32_t i = 0; i < len; ++i, ++s) { if ( !_io_write_le16( desc, s ) ) return false; }
	}
#endif

	return true;
}

pxtnERR pxtnWoice::io_matePCM_r( void* desc )
{
	pxtnERR             res  = pxtnERR_VOID;
	_MATERIALSTRUCT_PCM pcm  = {0};
	int32_t             size =  0 ;

	if( !_io_read_le32( desc, &size                              ) ) return pxtnERR_desc_r;
	if( !_io_read( desc, &pcm , sizeof( _MATERIALSTRUCT_PCM ), 1 ) ) return pxtnERR_desc_r;
	swapEndian( pcm );

	if( ((int32_t)pcm.voice_flags) & PTV_VOICEFLAG_UNCOVERED )return pxtnERR_fmt_unknown;

	if( !Voice_Allocate( 1 ) ){ res = pxtnERR_memory; goto term; }

	{
		pxtnVOICEUNIT* p_vc = &_voices[ 0 ];

		p_vc->type = pxtnVOICE_Sampling;

		res   = p_vc->p_pcm->Create( pcm.ch, pcm.sps, pcm.bps, pcm.data_size / ( pcm.bps / 8 * pcm.ch ) );
		if( res != pxtnOK ) goto term;
		if( !_io_read( desc, p_vc->p_pcm->get_p_buf_variable(), 1, pcm.data_size ) ){ res = pxtnERR_desc_r; goto term; }
		_type = pxtnWOICE_PCM;

#ifdef px_BIG_ENDIAN
		if( pcm.bps == 16 )
		{
			uint16_t *s = (uint16_t*)p_vc->p_pcm->get_p_buf_variable();
			uint32_t len = pcm.data_size / 2;
			for( uint32_t i = 0; i < len; ++i, ++s ) { *s = pxtnData::_swap16(*s); }
		}
#endif

		p_vc->voice_flags = pcm.voice_flags;
		p_vc->basic_key   = pcm.basic_key  ;
		p_vc->tuning      = pcm.tuning     ;
		_x3x_basic_key    = pcm.basic_key  ;
		_x3x_tuning       =               0;
	}
	res = pxtnOK;
term:

	if( res != pxtnOK ) Voice_Release();
	return res;
}


/////////////
// matePTN
/////////////

// 16byte =================
typedef struct
{
	uint16_t x3x_unit_no;
	uint16_t basic_key  ;
	uint32_t voice_flags;
	float    tuning     ;
	int32_t  rrr        ; // 0: -v.0.9.2.3
	                      // 1:  v.0.9.2.4-
}
_MATERIALSTRUCT_PTN;

#ifdef px_BIG_ENDIAN
px_FORCE_INLINE void swapEndian( _MATERIALSTRUCT_PTN &ptv)
{
	ptv.x3x_unit_no = pxtnData::_swap16( ptv.x3x_unit_no );
	ptv.basic_key =   pxtnData::_swap16( ptv.basic_key )  ;
	ptv.voice_flags = pxtnData::_swap32( ptv.voice_flags );
	ptv.tuning =      pxtnData::_swap_float( ptv.tuning ) ;
	ptv.rrr =         pxtnData::_swap32( ptv.rrr )        ;
}
#endif


bool pxtnWoice::io_matePTN_w( void* desc ) const
{
	_MATERIALSTRUCT_PTN ptn ;
	pxtnVOICEUNIT*      p_vc;
	int32_t                 size = 0;

	// ptv -------------------------
	memset( &ptn, 0, sizeof( _MATERIALSTRUCT_PTN ) );
	ptn.x3x_unit_no   = (uint16_t)0;

	p_vc = &_voices[ 0 ];
	ptn.tuning      =           p_vc->tuning     ;
	ptn.voice_flags =           p_vc->voice_flags;
	ptn.basic_key   = (uint16_t)p_vc->basic_key  ;
	ptn.rrr         =                           1;

	swapEndian( ptn );

	// pre
	if( !_io_write_le32( desc, &size ) ) return false;
	if( !_io_write( desc, &ptn,  sizeof(_MATERIALSTRUCT_PTN), 1 ) ) return false;

	size += sizeof(_MATERIALSTRUCT_PTN);

	if( !p_vc->p_ptn->write( desc, &size )                        ) return false;

	if( !_io_seek ( desc, SEEK_CUR, -size -sizeof(int32_t) )      ) return false;
	if( !_io_write( desc, &size, sizeof(int32_t),             1 ) ) return false;
	if( !_io_seek ( desc, SEEK_CUR,  size                  )      ) return false;

	return true;
}


pxtnERR pxtnWoice::io_matePTN_r( void* desc )
{
	pxtnERR             res  = pxtnERR_VOID;
	_MATERIALSTRUCT_PTN ptn  = {0};
	int32_t             size =  0 ;

	if( !_io_read_le32( desc, &size                            ) ) return pxtnERR_desc_r;
	if( !_io_read( desc, &ptn,  sizeof(_MATERIALSTRUCT_PTN), 1 ) ) return pxtnERR_desc_r;
	swapEndian( ptn );

	if     ( ptn.rrr > 1 ) return pxtnERR_fmt_unknown;
	else if( ptn.rrr < 0 ) return pxtnERR_fmt_unknown;

	if( !Voice_Allocate( 1 ) ) return pxtnERR_memory;

	{
		pxtnVOICEUNIT *p_vc = &_voices[ 0 ];

		p_vc->type = pxtnVOICE_Noise;

		res = p_vc->p_ptn->read( desc ); if( res != pxtnOK ) goto term;

		_type = pxtnWOICE_PTN;

		p_vc->voice_flags = ptn.voice_flags;
		p_vc->basic_key   = ptn.basic_key  ;
		p_vc->tuning      = ptn.tuning     ;
	}

	_x3x_basic_key = ptn.basic_key;
	_x3x_tuning    =             0;

	res = pxtnOK;
term:
	if( res != pxtnOK ) Voice_Release();
	return res;
}

/////////////////
// matePTV
/////////////////

// 24byte =================
typedef struct
{
	uint16_t x3x_unit_no;
	uint16_t rrr        ;
	float    x3x_tuning ;
	int32_t  size       ;
}
_MATERIALSTRUCT_PTV;

#ifdef px_BIG_ENDIAN
px_FORCE_INLINE void swapEndian( _MATERIALSTRUCT_PTV &ptv)
{
	ptv.x3x_unit_no = pxtnData::_swap16( ptv.x3x_unit_no )   ;
	ptv.rrr =         pxtnData::_swap16( ptv.rrr )           ;
	ptv.x3x_tuning =  pxtnData::_swap_float( ptv.x3x_tuning );
	ptv.size =        pxtnData::_swap32( ptv.size )          ;
}
#endif

bool pxtnWoice::io_matePTV_w( void* desc ) const
{
	_MATERIALSTRUCT_PTV ptv;
	int32_t             head_size = sizeof(_MATERIALSTRUCT_PTV) + sizeof(int32_t);
	int32_t             size      = 0;

	// ptv -------------------------
	memset( &ptv, 0, sizeof( _MATERIALSTRUCT_PTV ) );
	ptv.x3x_unit_no = (uint16_t)0;
	ptv.x3x_tuning  =           0;//1.0f;//p_w->tuning;
	ptv.size        =           0;

	swapEndian( ptv );

	// pre write
	if( !_io_write_le32( desc, &size ) ) return false;
	if( !_io_write( desc, &ptv,  sizeof(_MATERIALSTRUCT_PTV), 1 ) ) return false;
	swapEndian( ptv );

	if( !PTV_Write( desc, &ptv.size ) ) return false;

	if( !_io_seek( desc, SEEK_CUR, -( ptv.size + head_size ) ) ) return false;

	size = ptv.size +  sizeof(_MATERIALSTRUCT_PTV);

	if( !_io_write_le32( desc, &size ) ) return false;
	if( !_io_write( desc, &ptv,  sizeof(_MATERIALSTRUCT_PTV), 1 ) ) return false;

	if( !_io_seek ( desc, SEEK_CUR, ptv.size ) ) return false;

	return true;
}

pxtnERR pxtnWoice::io_matePTV_r( void* desc )
{
	pxtnERR             res  = pxtnERR_VOID;
	_MATERIALSTRUCT_PTV ptv  = {0};
	int32_t             size =  0 ;

	if( !_io_read_le32( desc, &size                              ) ) return pxtnERR_desc_r;
	if( !_io_read( desc, &ptv,  sizeof( _MATERIALSTRUCT_PTV ), 1 ) ) return pxtnERR_desc_r;
	swapEndian( ptv );

	if( ptv.rrr ) return pxtnERR_fmt_unknown;

	res = PTV_Read( desc ); if( res != pxtnOK ) goto term;

	if( ptv.x3x_tuning != 1.0 ) _x3x_tuning = ptv.x3x_tuning;
	else                        _x3x_tuning =              0;

	res = pxtnOK;
term:

	return res;
}


#ifdef  pxINCLUDE_OGGVORBIS

//////////////////////
// mateOGGV
//////////////////////

// 16byte =================
typedef struct
{
	uint16_t xxx        ; //ch;
	uint16_t basic_key  ;
	uint32_t voice_flags;
	float    tuning     ;
}
_MATERIALSTRUCT_OGGV;

#ifdef px_BIG_ENDIAN
px_FORCE_INLINE void swapEndian( _MATERIALSTRUCT_OGGV &mate)
{
	mate.xxx =           pxtnData::_swap16( mate.xxx )        ;
	mate.basic_key =     pxtnData::_swap16( mate.basic_key )  ;
	mate.voice_flags =   pxtnData::_swap32( mate.voice_flags );
	mate.tuning =        pxtnData::_swap_float( mate.tuning ) ;
}
#endif

bool pxtnWoice::io_mateOGGV_w( void* desc ) const
{
	if( !_voices ) return false;

	_MATERIALSTRUCT_OGGV mate = {0};
	pxtnVOICEUNIT*       p_vc = &_voices[ 0 ];

	if( !p_vc->p_oggv ) return false;

	int32_t oggv_size = p_vc->p_oggv->GetSize();

	mate.tuning      =        pxSwapFloatLE( p_vc->tuning )     ;
	mate.voice_flags =           pxSwapLE16( p_vc->voice_flags );
	mate.basic_key   = pxSwapLE16( (uint16_t)p_vc->basic_key )  ;

	uint32_t size = sizeof( _MATERIALSTRUCT_OGGV ) + oggv_size;

	if( !_io_write_le32( desc, &size ) ) return false;
	if( !_io_write( desc, &mate, sizeof(_MATERIALSTRUCT_OGGV), 1 ) ) return false;

	if( !p_vc->p_oggv->pxtn_write( desc ) ) return false;

	return true;
}

pxtnERR pxtnWoice::io_mateOGGV_r( void* desc )
{
	pxtnERR              res  = pxtnERR_VOID;
	_MATERIALSTRUCT_OGGV mate = {};
	int32_t              size =  0;

	if( !_io_read_le32( desc, &size                               ) ) return pxtnERR_desc_r;
	if( !_io_read( desc, &mate, sizeof( _MATERIALSTRUCT_OGGV ), 1 ) ) return pxtnERR_desc_r;

	swapEndian( mate );

	if( ((int32_t)mate.voice_flags) & PTV_VOICEFLAG_UNCOVERED ) return pxtnERR_fmt_unknown;

	if( !Voice_Allocate( 1 ) ) goto End;

	{
		pxtnVOICEUNIT *p_vc = &_voices[ 0 ];

		p_vc->type = pxtnVOICE_OggVorbis;

		if( !p_vc->p_oggv->pxtn_read( desc ) ) { res= pxtnERR_desc_r; goto End;}

		p_vc->voice_flags  = mate.voice_flags;
		p_vc->basic_key    = mate.basic_key  ;
		p_vc->tuning       = mate.tuning     ;
	}

	_x3x_basic_key = mate.basic_key;
	_x3x_tuning    =              0;
	_type          = pxtnWOICE_OGGV;

	res = pxtnOK;
End:
	if( res != pxtnOK ) Voice_Release();
	return res;
}

#endif
