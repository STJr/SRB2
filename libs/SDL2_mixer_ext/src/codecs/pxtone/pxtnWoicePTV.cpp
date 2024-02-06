// '12/03/03

#include "./pxtn.h"

#include "./pxtnMem.h"
#include "./pxtnWoice.h"

//                          01234567
static const char *_code = "PTVOICE-";
//             _version  =  20050826;
//             _version  =  20051101; // support coodinate
static int32_t _version  =  20060111; // support no-envelope

bool pxtnWoice::_Write_Wave( void* desc, const pxtnVOICEUNIT *p_vc, int32_t *p_total ) const
{
	bool    b_ret = false;
	int32_t num, i, size ;
	int8_t  sc    =     0;
	uint8_t uc    =     0;

	if( !_data_w_v( desc,  p_vc->type, p_total ) ) goto term;

	switch( p_vc->type )
	{
	// Coodinate (3)
	case pxtnVOICE_Coodinate:

		if( !_data_w_v( desc, p_vc->wave.num , p_total ) ) goto term;
		if( !_data_w_v( desc, p_vc->wave.reso, p_total ) ) goto term;
		num = p_vc->wave.num;
		for( i = 0; i < num; i++ )
		{
			uc = (int8_t)p_vc->wave.points[ i ].x; if( !_io_write( desc, &uc, 1, 1 ) ) goto term; (*p_total)++;
			sc = (int8_t)p_vc->wave.points[ i ].y; if( !_io_write( desc, &sc, 1, 1 ) ) goto term; (*p_total)++;
		}
		break;

	// Overtone (2)
	case pxtnVOICE_Overtone:

		if( !_data_w_v( desc, p_vc->wave.num, p_total ) ) goto term;
		num = p_vc->wave.num;
		for( i = 0; i < num; i++ )
		{
			if( !_data_w_v( desc, p_vc->wave.points[ i ].x, p_total ) ) goto term;
			if( !_data_w_v( desc, p_vc->wave.points[ i ].y, p_total ) ) goto term;
		}
		break;

	// sampling (7)
	case pxtnVOICE_Sampling:
		if( !_data_w_v( desc, p_vc->p_pcm->get_ch      (), p_total ) ) goto term;
		if( !_data_w_v( desc, p_vc->p_pcm->get_bps     (), p_total ) ) goto term;
		if( !_data_w_v( desc, p_vc->p_pcm->get_sps     (), p_total ) ) goto term;
		if( !_data_w_v( desc, p_vc->p_pcm->get_smp_head(), p_total ) ) goto term;
		if( !_data_w_v( desc, p_vc->p_pcm->get_smp_body(), p_total ) ) goto term;
		if( !_data_w_v( desc, p_vc->p_pcm->get_smp_tail(), p_total ) ) goto term;

		size = p_vc->p_pcm->get_buf_size();

		if( !_io_write( desc, p_vc->p_pcm->get_p_buf(), 1, size )      ) goto term;
		*p_total += size;
		break;

		case pxtnVOICE_OggVorbis: goto term; // not support.

	default:
		goto term; // unknown.
	}

	b_ret = true;
term:

	return b_ret;
}

bool pxtnWoice::_Write_Envelope( void* desc, const pxtnVOICEUNIT *p_vc, int32_t *p_total ) const
{
	bool b_ret = false;
	int32_t num, i;

	// envelope. (5)
	if( !_data_w_v( desc, p_vc->envelope.fps,      p_total ) ) goto term;
	if( !_data_w_v( desc, p_vc->envelope.head_num, p_total ) ) goto term;
	if( !_data_w_v( desc, p_vc->envelope.body_num, p_total ) ) goto term;
	if( !_data_w_v( desc, p_vc->envelope.tail_num, p_total ) ) goto term;

	num = p_vc->envelope.head_num + p_vc->envelope.body_num + p_vc->envelope.tail_num;
	for( i = 0; i < num; i++ )
	{
		if( !_data_w_v( desc, p_vc->envelope.points[ i ].x, p_total ) ) goto term;
		if( !_data_w_v( desc, p_vc->envelope.points[ i ].y, p_total ) ) goto term;
	}

	b_ret = true;
term:

	return b_ret;
}



pxtnERR pxtnWoice::_Read_Wave( void* desc, pxtnVOICEUNIT *p_vc )
{
	int32_t i, num;
	int8_t  sc;
	uint8_t uc;

	if( !_data_r_v( desc, (int32_t*)&p_vc->type ) ) return pxtnERR_desc_r;

	switch( p_vc->type )
	{
	// coodinate (3)
	case pxtnVOICE_Coodinate:
		if( !_data_r_v( desc, &p_vc->wave.num  ) ) return pxtnERR_desc_r;
		if( !_data_r_v( desc, &p_vc->wave.reso ) ) return pxtnERR_desc_r;
		num = p_vc->wave.num;
		if( !pxtnMem_zero_alloc( (void **)&p_vc->wave.points, sizeof(pxtnPOINT) * num ) ) return pxtnERR_memory;
		for( i = 0; i < num; i++ )
		{
			if( !_io_read( desc, &uc, 1, 1 ) ) { return pxtnERR_desc_r; } p_vc->wave.points[ i ].x = uc;
			if( !_io_read( desc, &sc, 1, 1 ) ) { return pxtnERR_desc_r; } p_vc->wave.points[ i ].y = sc;
		}
		num = p_vc->wave.num;
		break;
	// overtone (2)
	case pxtnVOICE_Overtone:

		if( !_data_r_v( desc, &p_vc->wave.num ) ) return pxtnERR_desc_r;
		num = p_vc->wave.num;
		if( !pxtnMem_zero_alloc( (void **)&p_vc->wave.points, sizeof(pxtnPOINT) * num ) ) return pxtnERR_memory;
		for( i = 0; i < num; i++ )
		{
			if( !_data_r_v( desc, &p_vc->wave.points[ i ].x ) ) return pxtnERR_desc_r;
			if( !_data_r_v( desc, &p_vc->wave.points[ i ].y ) ) return pxtnERR_desc_r;
		}
		break;

	// p_vc->sampring. (7)
	case pxtnVOICE_Sampling: return pxtnERR_fmt_unknown; // un-support

		//if( !_data_r_v( desc, &p_vc->pcm.ch       ) ) goto term;
		//if( !_data_r_v( desc, &p_vc->pcm.bps      ) ) goto term;
		//if( !_data_r_v( desc, &p_vc->pcm.sps      ) ) goto term;
		//if( !_data_r_v( desc, &p_vc->pcm.smp_head ) ) goto term;
		//if( !_data_r_v( desc, &p_vc->pcm.smp_body ) ) goto term;
		//if( !_data_r_v( desc, &p_vc->pcm.smp_tail ) ) goto term;
		//size = ( p_vc->pcm.smp_head + p_vc->pcm.smp_body + p_vc->pcm.smp_tail ) * p_vc->pcm.ch * p_vc->pcm.bps / 8;
		//if( !_malloc_zero( (void **)&p_vc->pcm.p_smp,    size )          ) goto term;
		//if( !_io_read( desc,        p_vc->pcm.p_smp, 1, size ) ) goto term;
		//break;

	default: return pxtnERR_ptv_no_supported; // un-support
	}

	return pxtnOK;
}

pxtnERR pxtnWoice::_Read_Envelope( void* desc, pxtnVOICEUNIT *p_vc )
{
	pxtnERR res = pxtnOK;
	int32_t  num, i;

	//p_vc->envelope. (5)
	if( !_data_r_v( desc, &p_vc->envelope.fps      ) ){ res = pxtnERR_desc_r     ; goto term; }
	if( !_data_r_v( desc, &p_vc->envelope.head_num ) ){ res = pxtnERR_desc_r     ; goto term; }
	if( !_data_r_v( desc, &p_vc->envelope.body_num ) ){ res = pxtnERR_desc_r     ; goto term; }
	if( !_data_r_v( desc, &p_vc->envelope.tail_num ) ){ res = pxtnERR_desc_r     ; goto term; }
	if( p_vc->envelope.body_num                 ){ res = pxtnERR_fmt_unknown; goto term; }
	if( p_vc->envelope.tail_num != 1            ){ res = pxtnERR_fmt_unknown; goto term; }

	num = p_vc->envelope.head_num + p_vc->envelope.body_num + p_vc->envelope.tail_num;
	if( !pxtnMem_zero_alloc( (void **)&p_vc->envelope.points, sizeof(pxtnPOINT) * num ) ){ res = pxtnERR_memory; goto term; }
	for( i = 0; i < num; i++ )
	{
		if( !_data_r_v( desc, &p_vc->envelope.points[ i ].x ) ){ res = pxtnERR_desc_r; goto term; }
		if( !_data_r_v( desc, &p_vc->envelope.points[ i ].y ) ){ res = pxtnERR_desc_r; goto term; }
	}

	res = pxtnOK;
term:
	if( res != pxtnOK ) pxtnMem_free( (void**)&p_vc->envelope.points );

	return res;
}


////////////////////////
// publics..
////////////////////////

bool pxtnWoice::PTV_Write( void* desc, int32_t *p_total ) const
{
	bool           b_ret = false;
	pxtnVOICEUNIT* p_vc  = NULL ;
	uint32_t       work  =     0;
	int32_t        v     =     0;
	int32_t        total =     0;

	if( !_io_write( desc, _code   , 1, 8 ) ) goto term;
	if( !_io_write_le32( desc, &_version ) ) goto term;
	if( !_io_write_le32( desc, &total    ) ) goto term;

	work = 0;

	// p_ptv-> (5)
	if( !_data_w_v( desc, work      , &total ) ) goto term; // basic_key (no use)
	if( !_data_w_v( desc, work      , &total ) ) goto term;
	if( !_data_w_v( desc, work      , &total ) ) goto term;
	if( !_data_w_v( desc, _voice_num, &total ) ) goto term;

	for( v = 0; v < _voice_num; v++ )
	{
		// p_ptvv-> (9)
		p_vc = &_voices[ v ];
		if( !p_vc ) goto term;

		if( !_data_w_v( desc, p_vc->basic_key  , &total ) ) goto term;
		if( !_data_w_v( desc, p_vc->volume     , &total ) ) goto term;
		if( !_data_w_v( desc, p_vc->pan        , &total ) ) goto term;
		memcpy( &work, &p_vc->tuning, sizeof( 4 ) );
		if( !_data_w_v( desc, work             , &total ) ) goto term;
		if( !_data_w_v( desc, p_vc->voice_flags, &total ) ) goto term;
		if( !_data_w_v( desc, p_vc->data_flags , &total ) ) goto term;

		if( p_vc->data_flags & PTV_DATAFLAG_WAVE     && !_Write_Wave    ( desc, p_vc, &total ) ) goto term;
		if( p_vc->data_flags & PTV_DATAFLAG_ENVELOPE && !_Write_Envelope( desc, p_vc, &total ) ) goto term;
	}

	// total size
	if( !_io_seek ( desc, SEEK_CUR, -(total + 4) ) ) goto term;
	if( !_io_write_le32( desc, &total            ) ) goto term;
	if( !_io_seek ( desc, SEEK_CUR,  (total    ) ) ) goto term;

	if( p_total ) *p_total = 16 + total;
	b_ret  = true;
term:

	return b_ret;
}



pxtnERR pxtnWoice::PTV_Read( void* desc )
{
	pxtnERR        res       = pxtnERR_VOID;
	pxtnVOICEUNIT* p_vc      = NULL ;
	uint8_t        code[ 8 ] = { 0 };
	int32_t        version   =     0;
	int32_t        work1     =     0;
	int32_t        work2     =     0;
	int32_t        total     =     0;
	int32_t        num       =     0;

	if( !_io_read( desc, code    ,   1, 8 ) ){ res = pxtnERR_desc_r  ; goto term; }
	if( !_io_read_le32( desc, &version    ) ){ res = pxtnERR_desc_r  ; goto term; }
	if( memcmp( code, _code, 8 )            ){ res = pxtnERR_inv_code; goto term; }
	if( !_io_read_le32( desc, &total      ) ){ res = pxtnERR_desc_r  ; goto term; }
	if( version > _version                  ){ res = pxtnERR_fmt_new ; goto term; }

	// p_ptv-> (5)
	if( !_data_r_v( desc, &_x3x_basic_key ) ){ res = pxtnERR_desc_r     ; goto term; }
	if( !_data_r_v( desc, &work1          ) ){ res = pxtnERR_desc_r     ; goto term; }
	if( !_data_r_v( desc, &work2          ) ){ res = pxtnERR_desc_r     ; goto term; }
	if( work1 || work2                      ){ res = pxtnERR_fmt_unknown; goto term; }
	if( !_data_r_v( desc, &num )            ){ res = pxtnERR_desc_r     ; goto term; }
	if( !Voice_Allocate(  num )             ){ res = pxtnERR_memory     ; goto term; }

	for( int32_t v = 0; v < _voice_num; v++ )
	{
		// p_ptvv-> (8)
		p_vc = &_voices[ v ];
		if( !p_vc                                       ){ res = pxtnERR_FATAL ; goto term; }
		if( !_data_r_v( desc, &p_vc->basic_key )             ){ res = pxtnERR_desc_r; goto term; }
		if( !_data_r_v( desc, &p_vc->volume    )             ){ res = pxtnERR_desc_r; goto term; }
		if( !_data_r_v( desc, &p_vc->pan       )             ){ res = pxtnERR_desc_r; goto term; }
		if( !_data_r_v( desc, &work1           )             ){ res = pxtnERR_desc_r; goto term; }
		memcpy( &p_vc->tuning, &work1, sizeof( 4 )     );
		if( !_data_r_v( desc, (int32_t*)&p_vc->voice_flags )     ){ res = pxtnERR_desc_r; goto term; }
		if( !_data_r_v( desc, (int32_t*)&p_vc->data_flags  )     ){ res = pxtnERR_desc_r; goto term; }

		// no support.
		if( p_vc->voice_flags & PTV_VOICEFLAG_UNCOVERED ){ res = pxtnERR_fmt_unknown; goto term; }
		if( p_vc->data_flags  & PTV_DATAFLAG_UNCOVERED  ){ res = pxtnERR_fmt_unknown; goto term; }
		if( p_vc->data_flags  & PTV_DATAFLAG_WAVE       ){ res = _Read_Wave    ( desc, p_vc ); if( res != pxtnOK ) goto term; }
		if( p_vc->data_flags  & PTV_DATAFLAG_ENVELOPE   ){ res = _Read_Envelope( desc, p_vc ); if( res != pxtnOK ) goto term; }
	}
	_type = pxtnWOICE_PTV;

	res = pxtnOK;
term:

	return res;
}
