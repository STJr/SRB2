// '12/03/03

#include "./pxtnData.h"

#include "./pxtnMaster.h"
#include "./pxtnEvelist.h"

pxtnMaster::pxtnMaster( pxtnIO_r io_read, pxtnIO_w io_write, pxtnIO_seek io_seek, pxtnIO_pos io_pos )
{
	_set_io_funcs( io_read, io_write, io_seek, io_pos );
	Reset();
}

pxtnMaster::~pxtnMaster()
{
}

void pxtnMaster::Reset()
{
	_beat_num    = EVENTDEFAULT_BEATNUM  ;
	_beat_tempo  = EVENTDEFAULT_BEATTEMPO;
	_beat_clock  = EVENTDEFAULT_BEATCLOCK;
	_meas_num    = 1;
	_repeat_meas = 0;
	_last_meas   = 0;
}

void  pxtnMaster::Set( int32_t    beat_num, float    beat_tempo, int32_t    beat_clock )
{
	_beat_num   = beat_num;
	_beat_tempo = beat_tempo;
	_beat_clock = beat_clock;
}

void  pxtnMaster::Get( int32_t *p_beat_num, float *p_beat_tempo, int32_t *p_beat_clock, int32_t* p_meas_num ) const
{
	if( p_beat_num   ) *p_beat_num   = _beat_num  ;
	if( p_beat_tempo ) *p_beat_tempo = _beat_tempo;
	if( p_beat_clock ) *p_beat_clock = _beat_clock;
	if( p_meas_num   ) *p_meas_num   = _meas_num  ;
}

int32_t   pxtnMaster::get_beat_num   ()const{ return _beat_num   ;}
float     pxtnMaster::get_beat_tempo ()const{ return _beat_tempo ;}
int32_t   pxtnMaster::get_beat_clock ()const{ return _beat_clock ;}
int32_t   pxtnMaster::get_meas_num   ()const{ return _meas_num   ;}
int32_t   pxtnMaster::get_repeat_meas()const{ return _repeat_meas;}
int32_t   pxtnMaster::get_last_meas  ()const{ return _last_meas  ;}

int32_t   pxtnMaster::get_last_clock ()const
{
	return _last_meas * _beat_clock * _beat_num;
}

int32_t   pxtnMaster::get_play_meas  ()const
{
	if( _last_meas ) return _last_meas;
	return _meas_num;
}

int32_t  pxtnMaster::get_this_clock( int32_t meas, int32_t beat, int32_t clock ) const
{
	return _beat_num * _beat_clock * meas + _beat_clock * beat + clock;
}

void pxtnMaster::AdjustMeasNum( int32_t clock )
{
	int32_t m_num;
	int32_t b_num;

	b_num   = ( clock + _beat_clock  - 1 ) / _beat_clock;
	m_num   = ( b_num + _beat_num    - 1 ) / _beat_num;
	if( _meas_num    <= m_num       ) _meas_num    = m_num;
	if( _repeat_meas >= _meas_num   ) _repeat_meas = 0;
	if( _last_meas   >  _meas_num   ) _last_meas   = _meas_num;
}

void pxtnMaster::set_meas_num( int32_t meas_num )
{
	if( meas_num < 1                ) meas_num = 1;
	if( meas_num <= _repeat_meas    ) meas_num = _repeat_meas + 1;
	if( meas_num <  _last_meas      ) meas_num = _last_meas;
	_meas_num = meas_num;
}

void  pxtnMaster::set_repeat_meas( int32_t meas ){ if( meas < 0 ) meas = 0; _repeat_meas = meas; }
void  pxtnMaster::set_last_meas  ( int32_t meas ){ if( meas < 0 ) meas = 0; _last_meas   = meas; }

void  pxtnMaster::set_beat_clock ( int32_t beat_clock ){ if( beat_clock < 0 ) beat_clock = 0; _beat_clock = beat_clock; }





bool pxtnMaster::io_w_v5( void* desc, int32_t rough ) const
{

	uint32_t   size   =          15;
	int16_t   bclock = _beat_clock / rough;
	int32_t   clock_repeat = bclock * _beat_num * get_repeat_meas();
	int32_t   clock_last   = bclock * _beat_num * get_last_meas  ();
	int8_t    bnum   = _beat_num  ;
	float btempo = _beat_tempo;
	if( !_io_write_le32( desc, &size                         ) ) return false;
	if( !_io_write_le16( desc, &bclock                       ) ) return false;
	if( !_io_write( desc, &bnum        , sizeof(int8_t  ), 1 ) ) return false;
	if( !_io_write_le32f( desc, &btempo                      ) ) return false;
	if( !_io_write_le32( desc, &clock_repeat                 ) ) return false;
	if( !_io_write_le32( desc, &clock_last                   ) ) return false;

	return true;
}

pxtnERR pxtnMaster::io_r_v5( void* desc )
{
//	pxtnERR   res = pxtnERR_VOID;
	int16_t   beat_clock   = 0;
	int8_t    beat_num     = 0;
	float     beat_tempo   = 0;
	int32_t   clock_repeat = 0;
	int32_t   clock_last   = 0;

	uint32_t  size         = 0;

	if( !_io_read_le32( desc, &size ) ) return pxtnERR_desc_r;
	if( size != 15                              ) return pxtnERR_fmt_unknown;

	if( !_io_read_le16( desc, &beat_clock                 ) ) return pxtnERR_desc_r;
	if( !_io_read( desc, &beat_num    ,sizeof(int8_t ), 1 ) ) return pxtnERR_desc_r;
	if( !_io_read_le32f( desc, &beat_tempo                ) ) return pxtnERR_desc_r;
	if( !_io_read_le32( desc, &clock_repeat               ) ) return pxtnERR_desc_r;
	if( !_io_read_le32( desc, &clock_last                 ) ) return pxtnERR_desc_r;

	_beat_clock = beat_clock;
	_beat_num   = beat_num  ;
	_beat_tempo = beat_tempo;

	set_repeat_meas( clock_repeat / ( beat_num * beat_clock ) );
	set_last_meas  ( clock_last   / ( beat_num * beat_clock ) );

	return pxtnOK;
}

int32_t pxtnMaster::io_r_v5_EventNum( void* desc )
{
	uint32_t size;
	if( !_io_read_le32( desc, &size            ) ) return 0;
	if( size != 15                               ) return 0;
	int8_t buf[ 15 ];
	if( !_io_read( desc,  buf , sizeof(int8_t ), 15 )  ) return 0;
	return 5;
}

/////////////////////////////////
// file io
/////////////////////////////////

// master info(8byte) ================
typedef struct
{
	uint16_t data_num ;        // data-num is 3 ( clock / status / volume ）
	uint16_t rrr      ;
	uint32_t event_num;
}
_x4x_MASTER;

#ifdef px_BIG_ENDIAN
px_FORCE_INLINE void swapEndian( _x4x_MASTER &mast)
{
	mast.data_num =  pxtnData::_swap16( mast.data_num ) ;
	mast.rrr =       pxtnData::_swap16( mast.rrr )      ;
	mast.event_num = pxtnData::_swap32( mast.event_num );
}
#endif

// read( project )
pxtnERR pxtnMaster::io_r_x4x( void* desc )
{
	_x4x_MASTER mast     ={0};
	int32_t     size     = 0;
	int32_t     e        = 0;
	int32_t     status   = 0;
	int32_t     clock    = 0;
	int32_t     volume   = 0;
	int32_t     absolute = 0;

	int32_t     beat_clock, beat_num, repeat_clock, last_clock;
	float       beat_tempo = 0;

	if( !_io_read_le32( desc, &size                      ) ) return pxtnERR_desc_r;
	if( !_io_read( desc, &mast, sizeof( _x4x_MASTER ), 1 ) ) return pxtnERR_desc_r;
	swapEndian( mast );

	// unknown format
	if( mast.data_num != 3 ) return pxtnERR_fmt_unknown;
	if( mast.rrr           ) return pxtnERR_fmt_unknown;

	beat_clock   = EVENTDEFAULT_BEATCLOCK;
	beat_num     = EVENTDEFAULT_BEATNUM;
	beat_tempo   = EVENTDEFAULT_BEATTEMPO;
	repeat_clock = 0;
	last_clock   = 0;

	absolute = 0;

	for( e = 0; e < (int32_t)mast.event_num; e++ )
	{
		if( !_data_r_v( desc, &status ) ) break;
		if( !_data_r_v( desc, &clock  ) ) break;
		if( !_data_r_v( desc, &volume ) ) break;
		absolute += clock;
		clock     = absolute;

		switch( status )
		{
		case EVENTKIND_BEATCLOCK: beat_clock   =  volume;                        if( clock  ) return pxtnERR_desc_broken; break;
		case EVENTKIND_BEATTEMPO: memcpy( &beat_tempo, &volume, sizeof(float) ); if( clock  ) return pxtnERR_desc_broken; break;
		case EVENTKIND_BEATNUM  : beat_num     =  volume;                        if( clock  ) return pxtnERR_desc_broken; break;
		case EVENTKIND_REPEAT   : repeat_clock =  clock ;                        if( volume ) return pxtnERR_desc_broken; break;
		case EVENTKIND_LAST     : last_clock   =  clock ;                        if( volume ) return pxtnERR_desc_broken; break;
		default: return pxtnERR_fmt_unknown;
		}
	}

	if( e != (int32_t)mast.event_num ) return pxtnERR_desc_broken;

	_beat_num   = beat_num  ;
	_beat_tempo = beat_tempo;
	_beat_clock = beat_clock;

	set_repeat_meas( repeat_clock / ( beat_num * beat_clock ) );
	set_last_meas  ( last_clock   / ( beat_num * beat_clock ) );

	return pxtnOK;
}

int32_t pxtnMaster::io_r_x4x_EventNum( void* desc )
{
	_x4x_MASTER mast;
	int32_t     size;
	int32_t     work;
	int32_t     e   ;

	memset( &mast, 0, sizeof( _x4x_MASTER ) );
	if( !_io_read_le32( desc, &size                      ) ) return 0;
	if( !_io_read( desc, &mast, sizeof( _x4x_MASTER ), 1 ) ) return 0;
	swapEndian( mast );

	if( mast.data_num != 3 ) return 0;

	for( e = 0; e < (int32_t)mast.event_num; e++ )
	{
		if( !_data_r_v( desc, &work ) ) return 0;
		if( !_data_r_v( desc, &work ) ) return 0;
		if( !_data_r_v( desc, &work ) ) return 0;
	}

	return mast.event_num;
}
