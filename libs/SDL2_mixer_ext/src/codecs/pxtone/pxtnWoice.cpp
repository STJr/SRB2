// '12/03/03

#include "./pxtn.h"

#include "./pxtnWoice.h"
#include "./pxtnEvelist.h"
#include "./pxtnMem.h"

pxtnWoice::pxtnWoice( pxtnIO_r io_read, pxtnIO_w io_write, pxtnIO_seek io_seek, pxtnIO_pos io_pos )
{
	_set_io_funcs( io_read, io_write, io_seek, io_pos );

	memset( _name_buf, 0, sizeof(_name_buf) );
	_name_size =              0;

	_voice_num =              0;
	_type      = pxtnWOICE_None;
	_voices    = NULL          ;
	_voinsts   = NULL          ;
}

pxtnWoice::~pxtnWoice()
{
	Voice_Release();
}

int32_t       pxtnWoice::get_voice_num    () const{ return _voice_num    ; }
int32_t       pxtnWoice::get_x3x_basic_key() const{ return _x3x_basic_key; }
float         pxtnWoice::get_x3x_tuning   () const{ return _x3x_tuning   ; }
pxtnWOICETYPE pxtnWoice::get_type         () const{ return _type         ; }


pxtnVOICEUNIT *pxtnWoice::get_voice_variable( int32_t idx )
{
	if( idx < 0 || idx >= _voice_num ) return NULL;
	return &_voices[ idx ];
}
const pxtnVOICEUNIT *pxtnWoice::get_voice( int32_t idx ) const
{
	if( idx < 0 || idx >= _voice_num ) return NULL;
	return &_voices[ idx ];
}
const pxtnVOICEINSTANCE *pxtnWoice::get_instance( int32_t idx ) const
{
	if( idx < 0 || idx >= _voice_num ) return NULL;
	return &_voinsts[ idx ];
}

bool pxtnWoice::set_name_buf( const char *name, int32_t buf_size )
{
	if( !name || buf_size < 0 || buf_size > pxtnMAX_TUNEWOICENAME ) return false;
	memset( _name_buf, 0, sizeof(_name_buf) );
	if( buf_size ) memcpy( _name_buf, name, buf_size );
	_name_size = buf_size;
	return true;
}

const char* pxtnWoice::get_name_buf( int32_t* p_buf_size ) const
{
	if( p_buf_size ) *p_buf_size = _name_size;
	return _name_buf;
}

bool pxtnWoice::is_name_buf () const
{
	if( _name_size > 0 ) return true;
	return false;
}

static void _Voice_Release( pxtnVOICEUNIT* p_vc, pxtnVOICEINSTANCE* p_vi )
{
	if( p_vc )
	{
		SAFE_DELETE( p_vc->p_pcm  );
		SAFE_DELETE( p_vc->p_ptn  );
#ifdef  pxINCLUDE_OGGVORBIS
		SAFE_DELETE( p_vc->p_oggv );
#endif
		pxtnMem_free( (void**)&p_vc->envelope.points ); memset( &p_vc->envelope, 0, sizeof(pxtnVOICEENVELOPE) );
		pxtnMem_free( (void**)&p_vc->wave.points     ); memset( &p_vc->wave    , 0, sizeof(pxtnVOICEWAVE    ) );
	}
	if( p_vi )
	{
		pxtnMem_free( (void**)&p_vi->p_env           );
		pxtnMem_free( (void**)&p_vi->p_smp_w         );
		memset( p_vi, 0, sizeof(pxtnVOICEINSTANCE) );
	}
}

void pxtnWoice::Voice_Release ()
{
	for( int32_t v = 0; v < _voice_num; v++ ) _Voice_Release( &_voices[ v ], &_voinsts[ v ] );
	pxtnMem_free( (void**)&_voices  );
	pxtnMem_free( (void**)&_voinsts );
	_voice_num = 0;
}

void pxtnWoice::Slim()
{
	for( int32_t i = _voice_num - 1; i >= 0; i-- )
	{
		bool b_remove = false;

		if( !_voices[ i ].volume ) b_remove = true;

		if( _voices[ i ].type == pxtnVOICE_Coodinate && _voices[ i ].wave.num <= 1 ) b_remove = true;

		if( b_remove )
		{
			_Voice_Release( &_voices[ i ], &_voinsts[ i ] );
			_voice_num--;
			for( int32_t j = i; j < _voice_num; j++ ) _voices[ j ] = _voices[ j + 1 ];
			memset( &_voices[ _voice_num ], 0, sizeof(pxtnVOICEUNIT) );
		}
	}
}


bool pxtnWoice::Voice_Allocate( int32_t voice_num )
{
	bool b_ret = false;

	Voice_Release();

	if( !pxtnMem_zero_alloc( (void**)&_voices , sizeof(pxtnVOICEUNIT    ) * voice_num ) ) goto End;
	if( !pxtnMem_zero_alloc( (void**)&_voinsts, sizeof(pxtnVOICEINSTANCE) * voice_num ) ) goto End;
	_voice_num = voice_num;

	for( int32_t i = 0; i < voice_num; i++ )
	{
		pxtnVOICEUNIT *p_vc = &_voices[ i ];
		p_vc->basic_key   = EVENTDEFAULT_BASICKEY;
		p_vc->volume      =  128;
		p_vc->pan         =   64;
		p_vc->tuning      = 1.0f;
		p_vc->voice_flags = PTV_VOICEFLAG_SMOOTH;
		p_vc->data_flags  = PTV_DATAFLAG_WAVE   ;
		p_vc->p_pcm       = new pxtnPulse_PCM  ( _io_read, _io_write, _io_seek, _io_pos );
		p_vc->p_ptn       = new pxtnPulse_Noise( _io_read, _io_write, _io_seek, _io_pos );
#ifdef  pxINCLUDE_OGGVORBIS
		p_vc->p_oggv      = new pxtnPulse_Oggv ( _io_read, _io_write, _io_seek, _io_pos );
#endif
		memset( &p_vc->envelope, 0, sizeof(pxtnVOICEENVELOPE) );
	}

	b_ret = true;
End:

	if( !b_ret ) Voice_Release();

	return b_ret;
}

bool pxtnWoice::Copy( pxtnWoice *p_dst ) const
{
	bool           b_ret = false;
	int32_t        v, size, num;
	pxtnVOICEUNIT* p_vc1 = NULL ;
	pxtnVOICEUNIT* p_vc2 = NULL ;

	if( !p_dst->Voice_Allocate( _voice_num ) ) goto End;

	p_dst->_type = _type;

	memcpy( p_dst->_name_buf, _name_buf, sizeof(_name_buf) );
	p_dst->_name_size = _name_size;

	for( v = 0; v < _voice_num; v++ )
	{
		p_vc1 = &       _voices[ v ];
		p_vc2 = &p_dst->_voices[ v ];

		p_vc2->tuning            = p_vc1->tuning     ;
		p_vc2->data_flags        = p_vc1->data_flags ;
		p_vc2->basic_key         = p_vc1->basic_key  ;
		p_vc2->pan               = p_vc1->pan        ;
		p_vc2->type              = p_vc1->type       ;
		p_vc2->voice_flags       = p_vc1->voice_flags;
		p_vc2->volume            = p_vc1->volume     ;

		// envelope
		p_vc2->envelope.body_num = p_vc1->envelope.body_num;
		p_vc2->envelope.fps      = p_vc1->envelope.fps     ;
		p_vc2->envelope.head_num = p_vc1->envelope.head_num;
		p_vc2->envelope.tail_num = p_vc1->envelope.tail_num;
		num  = p_vc2->envelope.head_num + p_vc2->envelope.body_num + p_vc2->envelope.tail_num;
		size = sizeof(pxtnPOINT) * num;
		if( !pxtnMem_zero_alloc( (void **)&p_vc2->envelope.points, size ) ) goto End;
		memcpy(                            p_vc2->envelope.points, p_vc1->envelope.points, size );

		// wave
		p_vc2->wave.num          = p_vc1->wave.num ;
		p_vc2->wave.reso         = p_vc1->wave.reso;
		size = sizeof(pxtnPOINT) * p_vc2->wave.num ;
		if( !pxtnMem_zero_alloc( (void **)&p_vc2->wave.points, size ) ) goto End;
		memcpy(                            p_vc2->wave.points, p_vc1->wave.points, size );

		if( !p_vc2->p_pcm ->copy_from( p_vc1->p_pcm  ) ) goto End;
		if( !p_vc2->p_ptn ->copy_from( p_vc1->p_ptn  ) ) goto End;
#ifdef  pxINCLUDE_OGGVORBIS
		if( !p_vc1->p_oggv->copy_from( p_vc2->p_oggv ) ) goto End;
#endif
	}

	b_ret = true;
End:
	if( !b_ret ) p_dst->Voice_Release();

	return b_ret;
}


pxtnERR pxtnWoice::read( void* desc, pxtnWOICETYPE type )
{
	pxtnERR res = pxtnERR_VOID;

	switch( type )
	{
	// PCM
	case pxtnWOICE_PCM:
		{
			pxtnVOICEUNIT *p_vc; if( !Voice_Allocate( 1 ) ) goto term; p_vc = &_voices[ 0 ]; p_vc->type = pxtnVOICE_Sampling;
			res = p_vc->p_pcm->read( desc ); if( res != pxtnOK ) goto term;
			// if under 0.005 sec, set LOOP.
			if(p_vc->p_pcm->get_sec() < 0.005f ) p_vc->voice_flags |=  PTV_VOICEFLAG_WAVELOOP;
			else                                 p_vc->voice_flags &= ~PTV_VOICEFLAG_WAVELOOP;
			_type      = pxtnWOICE_PCM;
		}
		break;

	// PTV
	case pxtnWOICE_PTV:
		{
			res = PTV_Read( desc ); if( res != pxtnOK ) goto term;
		}
		break;

	// PTN
	case pxtnWOICE_PTN:
		if( !Voice_Allocate( 1 ) ){ res = pxtnERR_memory; goto term; }
		{
			pxtnVOICEUNIT *p_vc = &_voices[ 0 ]; p_vc->type = pxtnVOICE_Noise;
			res = p_vc->p_ptn->read( desc ); if( res != pxtnOK ) goto term;
			_type = pxtnWOICE_PTN;
		}
		break;

	// OGGV
	case pxtnWOICE_OGGV:
#ifdef  pxINCLUDE_OGGVORBIS
		if( !Voice_Allocate( 1 ) ){ res = pxtnERR_memory; goto term; }
		{
			pxtnVOICEUNIT *p_vc;  p_vc = &_voices[ 0 ]; p_vc->type = pxtnVOICE_OggVorbis;
			res = p_vc->p_oggv->ogg_read( desc ); if( res != pxtnOK ) goto term;
			_type      = pxtnWOICE_OGGV;
		}
#else
		res = pxtnERR_ogg_no_supported; goto term;
#endif
		break;

	default: goto term;
	}

	res = pxtnOK;
term:

	return res;
}


void pxtnWoice::_UpdateWavePTV( pxtnVOICEUNIT* p_vc, pxtnVOICEINSTANCE* p_vi, int32_t  ch, int32_t  sps, int32_t  bps )
{
	double  work, osc;
	int32_t long_;
	int32_t pan_volume[ 2 ] = {64, 64};
	bool    b_ovt;

	pxtnPulse_Oscillator osci( _io_read, _io_write, _io_seek, _io_pos );

	if( ch == 2 )
	{
		if( p_vc->pan > 64 ) pan_volume[ 0 ] = ( 128 - p_vc->pan );
		if( p_vc->pan < 64 ) pan_volume[ 1 ] = (       p_vc->pan );
	}

	osci.ReadyGetSample( p_vc->wave.points, p_vc->wave.num, p_vc->volume, p_vi->smp_body_w, p_vc->wave.reso );

	if( p_vc->type == pxtnVOICE_Overtone ) b_ovt = true ;
	else                                   b_ovt = false;

	p_vi->b_sine_over = false;

	//  8bit
	if( bps ==  8 )
	{
		uint8_t* p = (uint8_t*)p_vi->p_smp_w;
		for( int32_t s = 0; s < p_vi->smp_body_w; s++ )
		{
			if( b_ovt ) osc = osci.GetOneSample_Overtone ( s );
			else        osc = osci.GetOneSample_Coodinate( s );
			for( int32_t c = 0; c < ch; c++ )
			{
				work = osc * pan_volume[ c ] / 64;
				if( work >  1.0 ){ work =  1.0; p_vi->b_sine_over = true; }
				if( work < -1.0 ){ work = -1.0; p_vi->b_sine_over = true; }
				long_  = (int32_t )( work * 127 );
				p[ s * ch + c ] = (uint8_t)(long_ + 128);
			}
		}

	// 16bit
	}
	else
	{
		int16_t* p = (int16_t*)p_vi->p_smp_w;
		for( int32_t s = 0; s < p_vi->smp_body_w; s++ )
		{
			if( b_ovt ) osc = osci.GetOneSample_Overtone ( s );
			else        osc = osci.GetOneSample_Coodinate( s );
			for( int32_t c = 0; c < ch; c++ )
			{
				work = osc * pan_volume[ c ] / 64;
				if( work >  1.0 ){ work =  1.0; p_vi->b_sine_over = true; }
				if( work < -1.0 ){ work = -1.0; p_vi->b_sine_over = true; }
				long_  = (int32_t )( work * 32767 );
				p[ s * ch + c ] = (int16_t)long_;
			}
		}
	}
}

pxtnERR pxtnWoice::Tone_Ready_sample( const pxtnPulse_NoiseBuilder *ptn_bldr )
{
	pxtnERR            res   = pxtnERR_VOID;
	pxtnVOICEINSTANCE* p_vi  = NULL ;
	pxtnVOICEUNIT*     p_vc  = NULL ;
	pxtnPulse_PCM      pcm_work( _io_read, _io_write, _io_seek, _io_pos );

	int32_t            ch    =     2;
	int32_t            sps   = 44100;
	int32_t            bps   =    16;

	for( int32_t v = 0; v < _voice_num; v++ )
	{
		p_vi = &_voinsts[ v ];
		pxtnMem_free( (void **)&p_vi->p_smp_w );
		p_vi->smp_head_w = 0;
		p_vi->smp_body_w = 0;
		p_vi->smp_tail_w = 0;
	}

	for( int32_t v = 0; v < _voice_num; v++ )
	{
		p_vi = &_voinsts[ v ];
		p_vc = &_voices [ v ];

		switch( p_vc->type )
		{
		case pxtnVOICE_OggVorbis:

#ifdef pxINCLUDE_OGGVORBIS
			res = p_vc->p_oggv->Decode( &pcm_work );
			if( res != pxtnOK ) goto term;
			if( !pcm_work.Convert( ch, sps, bps  ) ) goto term;
			p_vi->smp_head_w = pcm_work.get_smp_head();
			p_vi->smp_body_w = pcm_work.get_smp_body();
			p_vi->smp_tail_w = pcm_work.get_smp_tail();
			p_vi->p_smp_w    = (uint8_t*)pcm_work.Devolve_SamplingBuffer();
#else
			res = pxtnERR_ogg_no_supported; goto term;
#endif
			break;

		case pxtnVOICE_Sampling:

			if( !pcm_work.copy_from( p_vc->p_pcm  ) ){ res = pxtnERR_pcm_unknown; goto term; }
			if( !pcm_work.Convert  ( ch, sps, bps ) ){ res = pxtnERR_pcm_convert; goto term; }
			p_vi->smp_head_w = pcm_work.get_smp_head();
			p_vi->smp_body_w = pcm_work.get_smp_body();
			p_vi->smp_tail_w = pcm_work.get_smp_tail();
			p_vi->p_smp_w    = (uint8_t*)pcm_work.Devolve_SamplingBuffer();
			break;

		case pxtnVOICE_Overtone :
		case pxtnVOICE_Coodinate:
			{
				p_vi->smp_body_w =  400;
				int32_t size = p_vi->smp_body_w * ch * bps / 8;
				if( !( p_vi->p_smp_w = (uint8_t*)malloc( size ) ) ){ res = pxtnERR_memory; goto term; }
				memset( p_vi->p_smp_w, 0x00, size );
				_UpdateWavePTV( p_vc, p_vi, ch, sps, bps );
				break;
			}

		case pxtnVOICE_Noise:
			{
				pxtnPulse_PCM *p_pcm = NULL;
				if( !ptn_bldr ){ res = pxtnERR_ptn_init; goto term; }
				if( !( p_pcm = ptn_bldr->BuildNoise( p_vc->p_ptn, ch, sps, bps ) ) ){ res = pxtnERR_ptn_build; goto term; }
				p_vi->p_smp_w    = (uint8_t*)p_pcm->Devolve_SamplingBuffer();
				p_vi->smp_body_w = p_vc->p_ptn->get_smp_num_44k();
				delete p_pcm;
				break;
			}
		}
	}

	res = pxtnOK;
term:
	if( res != pxtnOK )
	{
		for( int32_t v = 0; v < _voice_num; v++ )
		{
			p_vi = &_voinsts[ v ];
			pxtnMem_free( (void **)&p_vi->p_smp_w );
			p_vi->smp_head_w = 0;
			p_vi->smp_body_w = 0;
			p_vi->smp_tail_w = 0;
		}
	}

	return res;
}

pxtnERR pxtnWoice::Tone_Ready_envelope( int32_t sps )
{
	pxtnERR    res     = pxtnERR_VOID;
	int32_t    e       =            0;
	pxtnPOINT* p_point = NULL        ;

	for( int32_t v = 0; v < _voice_num; v++ )
	{
		pxtnVOICEINSTANCE* p_vi   = &_voinsts[ v ] ;
		pxtnVOICEUNIT*     p_vc   = &_voices [ v ] ;
		pxtnVOICEENVELOPE* p_enve = &p_vc->envelope;
		int32_t            size   =               0;

		pxtnMem_free( (void**)&p_vi->p_env );

		if( p_enve->head_num )
		{
			for( e = 0; e < p_enve->head_num; e++ ) size += p_enve->points[ e ].x;
			p_vi->env_size = (int32_t)( (double)size * sps / p_enve->fps );
			if( !p_vi->env_size ) p_vi->env_size = 1;

			if( !pxtnMem_zero_alloc( (void**)&p_vi->p_env, p_vi->env_size                       ) ){ res = pxtnERR_memory; goto term; }
			if( !pxtnMem_zero_alloc( (void**)&p_point    , sizeof(pxtnPOINT) * p_enve->head_num ) ){ res = pxtnERR_memory; goto term; }

			// convert points.
			int32_t  offset   = 0;
			int32_t  head_num = 0;
			for( e = 0; e < p_enve->head_num; e++ )
			{
				if( !e || p_enve->points[ e ].x ||  p_enve->points[ e ].y )
				{
					offset        += (int32_t)( (double)p_enve->points[ e ].x * sps / p_enve->fps );
					p_point[ e ].x = offset;
					p_point[ e ].y =                p_enve->points[ e ].y;
					head_num++;
				}
			}

			pxtnPOINT start;
			e = start.x = start.y = 0;
			for( int32_t  s = 0; s < p_vi->env_size; s++ )
			{
				while( e < head_num && s >= p_point[ e ].x )
				{
					start.x = p_point[ e ].x;
					start.y = p_point[ e ].y;
					e++;
				}

				if(    e < head_num )
				{
					p_vi->p_env[ s ] = (uint8_t)(
												start.y + ( p_point[ e ].y - start.y ) *
												(              s - start.x ) /
												( p_point[ e ].x - start.x ) );
				}
				else
				{
					p_vi->p_env[ s ] = (uint8_t)start.y;
				}
			}

			pxtnMem_free( (void**)&p_point );
		}

		if( p_enve->tail_num )
		{
			p_vi->env_release = (int32_t)( (double)p_enve->points[ p_enve->head_num ].x * sps / p_enve->fps );
		}
		else
		{
			p_vi->env_release = 0;
		}
	}

	res = pxtnOK;
term:

	pxtnMem_free( (void**)&p_point );

	if( res != pxtnOK ){ for( int32_t v = 0; v < _voice_num; v++ ) pxtnMem_free( (void**)&_voinsts[ v ].p_env ); }

	return res;
}

pxtnERR pxtnWoice::Tone_Ready( const pxtnPulse_NoiseBuilder *ptn_bldr, int32_t sps )
{
	pxtnERR res = pxtnERR_VOID;
	res = Tone_Ready_sample  ( ptn_bldr ); if( res != pxtnOK ) return res;
	res = Tone_Ready_envelope( sps      ); if( res != pxtnOK ) return res;
	return pxtnOK;
}
