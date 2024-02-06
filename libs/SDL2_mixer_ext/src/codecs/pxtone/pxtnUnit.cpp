// '12/03/03

#include "./pxtn.h"

#include "./pxtnUnit.h"
#include "./pxtnEvelist.h"

pxtnUnit::pxtnUnit( pxtnIO_r io_read, pxtnIO_w io_write, pxtnIO_seek io_seek, pxtnIO_pos io_pos )
{
	_set_io_funcs( io_read, io_write, io_seek, io_pos );
	_bPlayed   = true;
	_bOperated = true;
	strcpy( _name_buf, "no name" );
	_name_size = strlen( _name_buf );
}

pxtnUnit::~pxtnUnit()
{
}

void pxtnUnit::Tone_Init()
{
	_v_GROUPNO            = EVENTDEFAULT_GROUPNO ;
	_v_VELOCITY           = EVENTDEFAULT_VELOCITY;
	_v_VOLUME             = EVENTDEFAULT_VOLUME  ;
	_v_TUNING             = EVENTDEFAULT_TUNING  ;
	_portament_sample_num =                     0;
	_portament_sample_pos =                     0;

	for( int32_t i = 0; i < pxtnMAX_CHANNEL; i++ )
	{
		_pan_vols [ i ] = 64;
		_pan_times[ i ] =  0;
	}
}

void pxtnUnit::Tone_Clear()
{
	for( int32_t i = 0; i < pxtnMAX_CHANNEL; i++ ) memset( _pan_time_bufs[ i ], 0, sizeof(int32_t) * pxtnBUFSIZE_TIMEPAN );
}

void pxtnUnit::Tone_Reset_and_2prm( int32_t voice_idx, int32_t env_rls_clock, float offset_freq )
{
	pxtnVOICETONE* p_tone = &_vts[ voice_idx ];
	p_tone->life_count    = 0;
	p_tone->on_count      = 0;
	p_tone->smp_pos       = 0;
	p_tone->smooth_volume = 0;
	p_tone->env_release_clock = env_rls_clock;
	p_tone->offset_freq       = offset_freq  ;
}

bool pxtnUnit::set_woice( const pxtnWoice *p_woice )
{
	if( !p_woice ) return false;
	_p_woice    = p_woice;
	_key_now    = EVENTDEFAULT_KEY;
	_key_margin = 0;
	_key_start  = EVENTDEFAULT_KEY;
	return true;
}

bool pxtnUnit::set_name_buf( const char *name, int32_t buf_size )
{
	if( !name || buf_size < 0 || buf_size > pxtnMAX_TUNEUNITNAME ) return false;
	memset( _name_buf, 0, sizeof(_name_buf) );
	if( buf_size ) memcpy( _name_buf, name, buf_size );
	_name_size = buf_size;
	return true;
}

const char* pxtnUnit::get_name_buf( int32_t* p_buf_size ) const
{
	if( p_buf_size ) *p_buf_size = _name_size;
	return _name_buf;
}

bool pxtnUnit::is_name_buf () const{ if( _name_size > 0 ) return true; return false; }


void pxtnUnit::set_operated( bool b )
{
	_bOperated = b;
}

void pxtnUnit::set_played  ( bool b )
{
	_bPlayed   = b;
}

bool pxtnUnit::get_operated() const{ return _bOperated; }
bool pxtnUnit::get_played  () const{ return _bPlayed  ; }

void pxtnUnit::Tone_ZeroLives()
{
	for( int32_t i = 0; i < pxtnMAX_CHANNEL; i++ ) _vts[ i ].life_count = 0;
}

void pxtnUnit::Tone_KeyOn()
{
	_key_now    = _key_start + _key_margin;
	_key_start  = _key_now;
	_key_margin = 0;
}

void pxtnUnit::Tone_Key( int32_t  key )
{
	_key_start            = _key_now;
	_key_margin           = key - _key_start;
	_portament_sample_pos = 0;
}

void pxtnUnit::Tone_Pan_Volume( int32_t ch, int32_t  pan )
{
	_pan_vols[ 0 ] = 64;
	_pan_vols[ 1 ] = 64;
	if( ch == 2 )
	{
		if( pan >= 64 )_pan_vols[ 0 ] = 128 - pan;
		else           _pan_vols[ 1 ] =       pan;
	}
}

void pxtnUnit::Tone_Pan_Time( int32_t ch, int32_t  pan, int32_t sps )
{
	_pan_times[ 0 ] = 0;
	_pan_times[ 1 ] = 0;

	if( ch == 2 )
	{
		if( pan >= 64 )
		{
			_pan_times[ 0 ] = pan - 64; if( _pan_times[ 0 ] > 63 ) _pan_times[ 0 ] = 63;
			_pan_times[ 0 ] = (_pan_times[ 0 ] * 44100) / sps;
		}
		else
		{
			_pan_times[ 1 ] = 64 - pan; if( _pan_times[ 1 ] > 63 ) _pan_times[ 1 ] = 63;
			_pan_times[ 1 ] = (_pan_times[ 1 ] * 44100) / sps;
		}
	}
}

void pxtnUnit::Tone_Velocity ( int32_t val ){ _v_VELOCITY           = val; }
void pxtnUnit::Tone_Volume   ( int32_t val ){ _v_VOLUME             = val; }
void pxtnUnit::Tone_Portament( int32_t val ){ _portament_sample_num = val; }
void pxtnUnit::Tone_GroupNo  ( int32_t val ){ _v_GROUPNO            = val; }
void pxtnUnit::Tone_Tuning   ( float   val ){ _v_TUNING             = val; }

void pxtnUnit::Tone_Envelope()
{
	if( !_p_woice ) return;

	for( int32_t v = 0; v < _p_woice->get_voice_num(); v++ )
	{
		const pxtnVOICEINSTANCE *p_vi = _p_woice->get_instance( v );
		pxtnVOICETONE           *p_vt = &_vts                 [ v ];

		if( p_vt->life_count > 0 && p_vi->env_size )
		{
			if( p_vt->on_count > 0 )
			{
				if( p_vt->env_pos < p_vi->env_size )
				{
					p_vt->env_volume = p_vi->p_env[ p_vt->env_pos ];
					p_vt->env_pos++;
				}
			}
			// release.
			else
			{
				p_vt->env_volume = p_vt->env_start + ( 0 - p_vt->env_start ) * p_vt->env_pos / p_vi->env_release;
				p_vt->env_pos++;
			}
		}
	}
}

void pxtnUnit::Tone_Sample( bool b_mute_by_unit, int32_t ch_num, int32_t  time_pan_index, int32_t  smooth_smp )
{
	if( !_p_woice ) return;

	if( b_mute_by_unit && !_bPlayed )
	{
		for( int32_t ch = 0; ch < ch_num; ch++ ) _pan_time_bufs[ ch ][ time_pan_index ] = 0;
		return;
	}

	for( int32_t  ch = 0; ch < pxtnMAX_CHANNEL; ch++ )
	{
		int32_t  time_pan_buf = 0;

		for( int32_t v = 0; v < _p_woice->get_voice_num(); v++ )
		{
			pxtnVOICETONE*           p_vt = &_vts                 [ v ];
			const pxtnVOICEINSTANCE* p_vi = _p_woice->get_instance( v );

			int32_t  work = 0;

			if( p_vt->life_count > 0 )
			{
				int32_t pos = (int32_t)p_vt->smp_pos * 4 + ch * 2;
				work += *( (short*)&p_vi->p_smp_w[ pos ] );

				if( ch_num == 1 )
				{
					work += *( (short*)&p_vi->p_smp_w[ pos + 2 ] );
					work = work / 2;
				}

				work = ( work * _v_VELOCITY )   / 128;
				work = ( work * _v_VOLUME   )   / 128;
				work =   work * _pan_vols[ ch ] /  64;

				if( p_vi->env_size ) work = work * p_vt->env_volume / 128;

				// smooth tail
				if( _p_woice->get_voice( v )->voice_flags & PTV_VOICEFLAG_SMOOTH && p_vt->life_count < smooth_smp )
				{
					work = work * p_vt->life_count / smooth_smp;
				}
			}
			time_pan_buf += work;
		}
		_pan_time_bufs[ ch ][ time_pan_index ] = time_pan_buf;
	}
}

void pxtnUnit::Tone_Supple( int32_t  *group_smps, int32_t ch, int32_t  time_pan_index ) const
{
	int32_t  idx = ( time_pan_index - _pan_times[ ch ] ) & ( pxtnBUFSIZE_TIMEPAN - 1 );
	group_smps[ _v_GROUPNO ] += _pan_time_bufs[ ch ][ idx ];
}

int32_t  pxtnUnit::Tone_Increment_Key()
{
	// prtament..
	if( _portament_sample_num && _key_margin )
	{
		if( _portament_sample_pos < _portament_sample_num )
		{
			_portament_sample_pos++;
			_key_now = (int32_t)( _key_start + (double)_key_margin * _portament_sample_pos / _portament_sample_num );
		}
		else
		{
			_key_now    = _key_start + _key_margin;
			_key_start  = _key_now;
			_key_margin = 0;
		}
	}
	else
	{
		_key_now = _key_start + _key_margin;
	}
	return _key_now;
}

void pxtnUnit::Tone_Increment_Sample( float freq )
{
	if( !_p_woice ) return;

	for( int32_t v = 0; v < _p_woice->get_voice_num(); v++ )
	{
		const pxtnVOICEINSTANCE* p_vi = _p_woice->get_instance( v );
		pxtnVOICETONE*           p_vt = &_vts                 [ v ];

		if( p_vt->life_count > 0 ) p_vt->life_count--;
		if( p_vt->life_count > 0 )
		{
			p_vt->on_count--;

			p_vt->smp_pos += p_vt->offset_freq * _v_TUNING * freq;

			if( p_vt->smp_pos >= p_vi->smp_body_w )
			{
				if( _p_woice->get_voice( v )->voice_flags & PTV_VOICEFLAG_WAVELOOP )
				{
					if( p_vt->smp_pos >= p_vi->smp_body_w ) p_vt->smp_pos -= p_vi->smp_body_w;
					if( p_vt->smp_pos >= p_vi->smp_body_w ) p_vt->smp_pos  = 0;
				}
				else
				{
					p_vt->life_count = 0;
				}
			}

			// OFF
			if( p_vt->on_count == 0 && p_vi->env_size )
			{
				p_vt->env_start = p_vt->env_volume;
				p_vt->env_pos   = 0;
			}
		}
	}
}

const pxtnWoice *pxtnUnit::get_woice() const{ return _p_woice; }

pxtnVOICETONE *pxtnUnit::get_tone( int32_t voice_idx )
{
	return &_vts[ voice_idx ];
}


// v1x (20byte) =================
typedef struct
{
	char     name[ pxtnMAX_TUNEUNITNAME ];
	uint16_t type ;
	uint16_t group;
}
_x1x_UNIT;

#ifdef px_BIG_ENDIAN
px_FORCE_INLINE void swapEndian( _x1x_UNIT &unit)
{
	unit.type =  pxtnData::_swap16( unit.type ) ;
	unit.group = pxtnData::_swap16( unit.group );
}
#endif

bool pxtnUnit::Read_v1x( void* desc, int32_t *p_group )
{
	_x1x_UNIT unit;
	int32_t   size;

	if( !_io_read_le32( desc, &size                    ) ) return false;
	if( !_io_read( desc, &unit, sizeof( _x1x_UNIT ), 1 ) ) return false;
	swapEndian( unit );

	if( (pxtnWOICETYPE)unit.type != pxtnWOICE_PCM        ) return false;

	memcpy( _name_buf, unit.name, pxtnMAX_TUNEUNITNAME ); _name_buf[ pxtnMAX_TUNEUNITNAME ] = '\0';
	*p_group = unit.group;
	return true;
}

///////////////////
// pxtnUNIT x3x
///////////////////

typedef struct
{
	uint16_t type ;
	uint16_t group;
}
_x3x_UNIT;

#ifdef px_BIG_ENDIAN
px_FORCE_INLINE void swapEndian( _x3x_UNIT &unit)
{
	unit.type =  pxtnData::_swap16( unit.type ) ;
	unit.group = pxtnData::_swap16( unit.group );
}
#endif

pxtnERR pxtnUnit::Read_v3x( void* desc, int32_t *p_group )
{
	_x3x_UNIT unit = {0};
	int32_t   size =  0 ;

	if( !_io_read_le32( desc, &size                    ) ) return pxtnERR_desc_r;
	if( !_io_read( desc, &unit, sizeof( _x3x_UNIT ), 1 ) ) return pxtnERR_desc_r;
	swapEndian( unit );

	if( (pxtnWOICETYPE)unit.type != pxtnWOICE_PCM &&
		(pxtnWOICETYPE)unit.type != pxtnWOICE_PTV &&
		(pxtnWOICETYPE)unit.type != pxtnWOICE_PTN ) return pxtnERR_fmt_unknown;
	*p_group = unit.group;

	return pxtnOK;
}
