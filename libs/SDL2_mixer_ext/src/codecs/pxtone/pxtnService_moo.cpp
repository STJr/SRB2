
#include "./pxtn.h"

#include "./pxtnMem.h"
#include "./pxtnService.h"

void pxtnService::_moo_constructor()
{
	_moo_b_init         = false;

	_moo_b_valid_data   = false;
	_moo_b_end_vomit    = true ;
	_moo_b_mute_by_unit = false;
	_moo_b_loop         = true ;
	_moo_loops_num      =    -1;

	_moo_fade_fade      =     0;
	_moo_master_vol     =  1.0f;
	_moo_bt_clock       =     0;
	_moo_bt_num         =     0;

	_moo_freq           = NULL ;
	_moo_group_smps     = NULL ;
	_moo_p_eve          = NULL ;

	_moo_smp_count      =     0;
	_moo_smp_end        =     0;
}

bool pxtnService::_moo_release()
{
	if( !_moo_b_init ) return false;
	_moo_b_init = false;
	SAFE_DELETE( _moo_freq );
	if( _moo_group_smps ) { free( _moo_group_smps ); } _moo_group_smps = NULL;
	return true;
}

void pxtnService::_moo_destructer()
{

	_moo_release();
}

bool pxtnService::_moo_init()
{
	bool b_ret = false;

	if( !(_moo_freq = new pxtnPulse_Frequency( _io_read, _io_write, _io_seek, _io_pos ) ) ||  !_moo_freq->Init() ) goto term;
	if( !pxtnMem_zero_alloc( (void **)&_moo_group_smps, sizeof(int32_t) * _group_num ) ) goto term;

	_moo_b_init = true;
	b_ret       = true;
term:
	if( !b_ret ) _moo_release();

	return b_ret;
}


////////////////////////////////////////////////
// Units   ////////////////////////////////////
////////////////////////////////////////////////

bool pxtnService::_moo_ResetVoiceOn( pxtnUnit *p_u, int32_t  w ) const
{
	if( !_moo_b_init ) return false;

	const pxtnVOICEINSTANCE* p_inst;
	const pxtnVOICEUNIT*     p_vc  ;
	const pxtnWoice*         p_wc = Woice_Get( w );

	if( !p_wc ) return false;

	p_u->set_woice( p_wc );

	for( int32_t v = 0; v < p_wc->get_voice_num(); v++ )
	{
		p_inst = p_wc->get_instance( v );
		p_vc   = p_wc->get_voice   ( v );

		float ofs_freq = 0;
		if( p_vc->voice_flags & PTV_VOICEFLAG_BEATFIT )
		{
			ofs_freq = ( p_inst->smp_body_w * _moo_bt_tempo ) / ( 44100 * 60 * p_vc->tuning );
		}
		else
		{
			ofs_freq = _moo_freq->Get( EVENTDEFAULT_BASICKEY - p_vc->basic_key ) * p_vc->tuning;
		}
		p_u->Tone_Reset_and_2prm( v, (int32_t)( p_inst->env_release / _moo_clock_rate ), ofs_freq );
	}
	return true;
}


bool pxtnService::_moo_InitUnitTone()
{
	if( !_moo_b_init ) return false;
	for( int32_t u = 0; u < _unit_num; u++ )
	{
		pxtnUnit *p_u = Unit_Get_variable( u );
		p_u->Tone_Init();
		_moo_ResetVoiceOn( p_u, EVENTDEFAULT_VOICENO );
	}
	return true;
}


bool pxtnService::_moo_PXTONE_SAMPLE( void *p_data )
{
	if( !_moo_b_init ) return false;

	// envelope..
	for( int32_t u = 0; u < _unit_num;  u++ ) _units[ u ]->Tone_Envelope();

	int32_t  clock = (int32_t)( _moo_smp_count / _moo_clock_rate );

	// events..
	for( ; _moo_p_eve && _moo_p_eve->clock <= clock; _moo_p_eve = _moo_p_eve->next )
	{
		int32_t                  u   = _moo_p_eve->unit_no;
		pxtnUnit*                p_u = _units[ u ];
		pxtnVOICETONE*           p_tone;
		const pxtnWoice*         p_wc  ;
		const pxtnVOICEINSTANCE* p_vi  ;

		switch( _moo_p_eve->kind )
		{
		case EVENTKIND_ON       :
			{
				int32_t on_count = (int32_t)( (_moo_p_eve->clock + _moo_p_eve->value - clock) * _moo_clock_rate );
				if( on_count <= 0 ){ p_u->Tone_ZeroLives(); break; }

				p_u->Tone_KeyOn();

				if( !( p_wc = p_u->get_woice() ) ) break;
				for( int32_t v = 0; v < p_wc->get_voice_num(); v++ )
				{
					p_tone = p_u ->get_tone    ( v );
					p_vi   = p_wc->get_instance( v );

					// release..
					if( p_vi->env_release )
					{
						int32_t        max_life_count1 = (int32_t)( ( _moo_p_eve->value - ( clock - _moo_p_eve->clock ) ) * _moo_clock_rate ) + p_vi->env_release;
						int32_t        max_life_count2;
						int32_t        c    = _moo_p_eve->clock + _moo_p_eve->value + p_tone->env_release_clock;
						EVERECORD* next = NULL;
						for( EVERECORD* p = _moo_p_eve->next; p; p = p->next )
						{
							if( p->clock > c ) break;
							if( p->unit_no == u && p->kind == EVENTKIND_ON ){ next = p; break; }
						}
						if( !next ) max_life_count2 = _moo_smp_end - (int32_t)( clock   * _moo_clock_rate );
						else        max_life_count2 = (int32_t)( ( next->clock -      clock ) * _moo_clock_rate );
						if( max_life_count1 < max_life_count2 ) p_tone->life_count = max_life_count1;
						else                                    p_tone->life_count = max_life_count2;
					}
					// no-release..
					else
					{
						p_tone->life_count = (int32_t)( ( _moo_p_eve->value - ( clock - _moo_p_eve->clock ) ) * _moo_clock_rate );
					}

					if( p_tone->life_count > 0 )
					{
						p_tone->on_count  = on_count;
						p_tone->smp_pos   = 0;
						p_tone->env_pos   = 0;
						if( p_vi->env_size ) p_tone->env_volume = p_tone->env_start  =   0; // envelope
						else                 p_tone->env_volume = p_tone->env_start  = 128; // no-envelope
					}
				}
				break;
			}

		case EVENTKIND_KEY       : p_u->Tone_Key       (              _moo_p_eve->value ); break;
		case EVENTKIND_PAN_VOLUME: p_u->Tone_Pan_Volume( _dst_ch_num, _moo_p_eve->value ); break;
		case EVENTKIND_PAN_TIME  : p_u->Tone_Pan_Time  ( _dst_ch_num, _moo_p_eve->value, _dst_sps ); break;
		case EVENTKIND_VELOCITY  : p_u->Tone_Velocity  (              _moo_p_eve->value ); break;
		case EVENTKIND_VOLUME    : p_u->Tone_Volume    (              _moo_p_eve->value ); break;
		case EVENTKIND_PORTAMENT : p_u->Tone_Portament ( (int32_t)(   _moo_p_eve->value * _moo_clock_rate ) ); break;
		case EVENTKIND_BEATCLOCK : break;
		case EVENTKIND_BEATTEMPO : break;
		case EVENTKIND_BEATNUM   : break;
		case EVENTKIND_REPEAT    : break;
		case EVENTKIND_LAST      : break;
		case EVENTKIND_VOICENO   : _moo_ResetVoiceOn   ( p_u, _moo_p_eve->value            ); break;
		case EVENTKIND_GROUPNO   : p_u->Tone_GroupNo   (              _moo_p_eve->value    ); break;
		case EVENTKIND_TUNING    : p_u->Tone_Tuning    ( pxtnData::cast_to_float(_moo_p_eve->value) ); break;
		}
	}

	// sampling..
	for( int32_t u = 0; u < _unit_num; u++ )
	{
		_units[ u ]->Tone_Sample( _moo_b_mute_by_unit, _dst_ch_num, _moo_time_pan_index, _moo_smp_smooth );
	}

	for( int32_t ch = 0; ch < _dst_ch_num; ch++ )
	{
		for( int32_t g = 0; g < _group_num; g++ ) _moo_group_smps[ g ] = 0;
		for( int32_t u = 0; u < _unit_num ; u++ ) _units [ u ]->Tone_Supple(     _moo_group_smps, ch, _moo_time_pan_index );
		for( int32_t o = 0; o < _ovdrv_num; o++ ) _ovdrvs[ o ]->Tone_Supple(     _moo_group_smps );
		for( int32_t d = 0; d < _delay_num; d++ ) _delays[ d ]->Tone_Supple( ch, _moo_group_smps );

		// collect.
		int32_t  work = 0;
		for( int32_t g = 0; g < _group_num; g++ ) work += _moo_group_smps[ g ];

		// fade..
		if( _moo_fade_fade ) work = work * ( _moo_fade_count >> 8 ) / _moo_fade_max;

		// master volume
		work = (int32_t)( work * _moo_master_vol );

		// to buffer..
		if( work >  _moo_top ) work =  _moo_top;
		if( work < -_moo_top ) work = -_moo_top;
		*( (int16_t*)p_data + ch ) = (int16_t)( work );
	}

	// --------------
	// increments..

	_moo_smp_count++;
	_moo_time_pan_index = ( _moo_time_pan_index + 1 ) & ( pxtnBUFSIZE_TIMEPAN - 1 );

	for( int32_t u = 0; u < _unit_num;  u++ )
	{
		int32_t  key_now = _units[ u ]->Tone_Increment_Key();
		_units[ u ]->Tone_Increment_Sample( _moo_freq->Get2( key_now ) *_moo_smp_stride );
	}

	// delay
	for( int32_t d = 0; d < _delay_num; d++ ) _delays[ d ]->Tone_Increment();

	// fade out
	if( _moo_fade_fade < 0 )
	{
		if( _moo_fade_count > 0  ) _moo_fade_count--;
		else return false;
	}
	// fade in
	else if( _moo_fade_fade > 0 )
	{
		if( _moo_fade_count < (_moo_fade_max << 8) ) _moo_fade_count++;
		else                                         _moo_fade_fade = 0;
	}

	if( _moo_smp_count >= _moo_smp_end )
	{
		if( _moo_loops_num > 0)       _moo_loops_num--;
		else if( _moo_loops_num == 0) _moo_b_loop = false;
		if( !_moo_b_loop ) return false;
		_moo_smp_count = _moo_smp_repeat;
		_moo_p_eve     = evels->get_Records();
		_moo_InitUnitTone();
	}
	return true;
}


///////////////////////
// get / set
///////////////////////

bool pxtnService::moo_is_valid_data() const
{
	if( !_moo_b_init ) return false;
	return _moo_b_valid_data;
}


bool pxtnService::moo_is_end_vomit() const
{
	if( !_moo_b_init ) return true;
	return _moo_b_end_vomit ;
}

int32_t pxtnService::moo_get_now_clock() const
{
	if( !_moo_b_init ) return 0;
	if( _moo_clock_rate ) return (int32_t)( _moo_smp_count / _moo_clock_rate );
	return 0;
}

int32_t pxtnService::moo_get_end_clock() const
{
	if( !_moo_b_init ) return 0;
	if( _moo_clock_rate ) return (int32_t)( _moo_smp_end / _moo_clock_rate );
	return 0;
}

bool pxtnService::moo_set_mute_by_unit( bool b ){ if( !_moo_b_init ) return false; _moo_b_mute_by_unit = b; return true; }
bool pxtnService::moo_set_loop        ( bool b ){ if( !_moo_b_init ) return false; _moo_b_loop         = b; return true; }

bool pxtnService::moo_set_loops_num( int32_t n )
{
	if( !_moo_b_init ) return false;
	_moo_loops_num = n;
	if( _moo_loops_num > 0 ) --_moo_loops_num;
	return true;
}

bool pxtnService::moo_set_fade( int32_t  fade, float sec )
{
	if( !_moo_b_init ) return false;
	_moo_fade_max = (int32_t)( (float)_dst_sps * sec ) >> 8;
	if(      fade < 0 ){ _moo_fade_fade  = -1; _moo_fade_count = _moo_fade_max << 8; } // out
	else if( fade > 0 ){ _moo_fade_fade  =  1; _moo_fade_count =  0;                 } // in
	else               { _moo_fade_fade =   0; _moo_fade_count =  0;                 } // off
	return true;
}


////////////////////////////
// preparation
////////////////////////////

// preparation
bool pxtnService::moo_preparation( const pxtnVOMITPREPARATION *p_prep , float tempo_mod )
{
	if( !_moo_b_init || !_moo_b_valid_data || !_dst_ch_num || !_dst_sps || !_dst_byte_per_smp )
	{
		 _moo_b_end_vomit = true ;
		 return false;
	}

	bool    b_ret        = false;
	int32_t start_meas   =     0;
	int32_t start_sample =     0;
	float   start_float  =     0;

	int32_t meas_end     = master->get_play_meas  ();
	int32_t meas_repeat  = master->get_repeat_meas();
	float   fadein_sec   =     0;

	if( p_prep )
	{
		start_meas   = p_prep->start_pos_meas  ;
		start_sample = p_prep->start_pos_sample;
		start_float  = p_prep->start_pos_float ;

		if( p_prep->meas_end     ) meas_end    	= p_prep->meas_end    ;
		if( p_prep->meas_repeat  ) meas_repeat 	= p_prep->meas_repeat ;
		if( p_prep->fadein_sec   ) fadein_sec  	= p_prep->fadein_sec  ;

		if( p_prep->flags & pxtnVOMITPREPFLAG_unit_mute ) _moo_b_mute_by_unit = true ;
		else                                              _moo_b_mute_by_unit = false;
		if( p_prep->flags & pxtnVOMITPREPFLAG_loop      ) _moo_b_loop         = true ;
		else                                              _moo_b_loop         = false;

		_moo_master_vol = p_prep->master_volume;
	}

	_moo_bt_clock   = master->get_beat_clock();
	_moo_bt_num     = master->get_beat_num  ();
	_moo_bt_tempo   = master->get_beat_tempo() * tempo_mod;
	_moo_tempo_mod  = tempo_mod;
	_moo_clock_rate = (float)( 60.0f * (double)_dst_sps / ( (double)_moo_bt_tempo * (double)_moo_bt_clock ) );
	_moo_smp_stride = ( 44100.0f / _dst_sps );
	_moo_top        = 0x7fff;

	_moo_time_pan_index = 0;

	_moo_smp_end    = (int32_t)( (double)meas_end    * (double)_moo_bt_num * (double)_moo_bt_clock * _moo_clock_rate );
	_moo_smp_repeat = (int32_t)( (double)meas_repeat * (double)_moo_bt_num * (double)_moo_bt_clock * _moo_clock_rate );

	if     ( start_float  ){ _moo_smp_start = (int32_t)( (float)moo_get_total_sample() * start_float ); }
	else if( start_sample ){ _moo_smp_start = start_sample; }
	else                   { _moo_smp_start = (int32_t)( (double)start_meas  * (double)_moo_bt_num * (double)_moo_bt_clock * _moo_clock_rate ); }

	_moo_smp_count  = _moo_smp_start;
	_moo_smp_smooth = _dst_sps / 250; // (0.004sec) // (0.010sec)

	if( fadein_sec > 0 ) moo_set_fade( 1, fadein_sec );
	else                 moo_set_fade( 0,          0 );

	tones_clear();

	_moo_p_eve = evels->get_Records();

	_moo_InitUnitTone();

	b_ret = true;
	if( b_ret ) _moo_b_end_vomit = false;
	else        _moo_b_end_vomit = true ;

	return b_ret;
}

bool pxtnService::moo_set_tempo_mod( float tempo_mod )
{
	if( !_moo_b_init || !_moo_b_valid_data || !_dst_ch_num || !_dst_sps || !_dst_byte_per_smp )
	{
		 _moo_b_end_vomit = true ;
		 return false;
	}

	_moo_tempo_mod = tempo_mod;

	int32_t meas_end     = master->get_play_meas  ();
	int32_t meas_repeat  = master->get_repeat_meas();

	float old_pos = _moo_smp_count / _moo_clock_rate;

	_moo_bt_tempo   = master->get_beat_tempo() * _moo_tempo_mod;
	_moo_clock_rate = (float)( 60.0f * (double)_dst_sps / ( (double)_moo_bt_tempo * (double)_moo_bt_clock ) );

	_moo_smp_end    = (int32_t)( (double)meas_end    * (double)_moo_bt_num * (double)_moo_bt_clock * _moo_clock_rate );
	_moo_smp_repeat = (int32_t)( (double)meas_repeat * (double)_moo_bt_num * (double)_moo_bt_clock * _moo_clock_rate );

	_moo_smp_count  = (old_pos * _moo_clock_rate);

	return true;
}

int32_t pxtnService::moo_get_sampling_offset() const
{
	if( !_moo_b_init     ) return 0;
	if( _moo_b_end_vomit ) return 0;
	return _moo_smp_count;
}

int32_t pxtnService::moo_get_sampling_end() const
{
	if( !_moo_b_init     ) return 0;
	if( _moo_b_end_vomit ) return 0;
	return _moo_smp_end;
}

int32_t pxtnService::moo_get_sampling_repeat() const
{
	if( !_moo_b_init     ) return 0;
	if( _moo_b_end_vomit ) return 0;
	return _moo_smp_repeat;
}

int32_t pxtnService::moo_get_total_sample   () const
{
	if( !_b_init           ) return 0;
	if( !_moo_b_valid_data ) return 0;

	int32_t meas_num  ;
	int32_t beat_num  ;
	float   beat_tempo;
	master->Get( &beat_num, &beat_tempo, NULL, &meas_num );
	return pxtnService_moo_CalcSampleNum( meas_num, beat_num, _dst_sps, master->get_beat_tempo() );
}

bool pxtnService::moo_set_master_volume( float v )
{
	if( !_moo_b_init ) return false;
	if( v < 0 ) v = 0;
	if( v > 1 ) v = 1;
	_moo_master_vol = v;
	return true;
}



////////////////////
//
////////////////////

bool pxtnService::Moo( void* p_buf, int32_t  size )
{
	if( !_moo_b_init       ) return false;
	if( !_moo_b_valid_data ) return false;
	if(  _moo_b_end_vomit  ) return false;

	bool b_ret = false;

	int32_t  smp_w = 0;

	if( size % _dst_byte_per_smp ) return false;

	int32_t  smp_num = size / _dst_byte_per_smp;

	{
		int16_t  *p16 = (int16_t*)p_buf;
		int16_t  sample[ 2 ];

		for( smp_w = 0; smp_w < smp_num; smp_w++ )
		{
			if( !_moo_PXTONE_SAMPLE( sample ) ){ _moo_b_end_vomit = true; break; }
			for( int32_t ch = 0; ch < _dst_ch_num; ch++, p16++ ) *p16 = sample[ ch ];
		}
		for( ;          smp_w < smp_num; smp_w++ )
		{
			for( int32_t ch = 0; ch < _dst_ch_num; ch++, p16++ ) *p16 = 0;
		}
	}

	if( _sampled_proc )
	{
//		int32_t clock = (int32_t)( _moo_smp_count / _moo_clock_rate );
		if( !_sampled_proc( _sampled_user, this ) ){ _moo_b_end_vomit = true; goto term; }
	}

	b_ret = true;
term:
	return b_ret;
}

int32_t pxtnService_moo_CalcSampleNum( int32_t meas_num, int32_t beat_num, int32_t sps, float beat_tempo )
{
	uint32_t  total_beat_num;
	uint32_t  sample_num    ;
	if( !beat_tempo ) return 0;
	total_beat_num = meas_num * beat_num;
	sample_num     = (uint32_t )( (double)sps * 60 * (double)total_beat_num / (double)beat_tempo );
	return sample_num;
}
