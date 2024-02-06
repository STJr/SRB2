
#include "./pxtn.h"

#include "./pxtnMem.h"
#include "./pxtnPulse_Noise.h"


void pxtnPulse_Noise::set_smp_num_44k( int32_t num )
{
	_smp_num_44k = num;
}

int32_t pxtnPulse_Noise::get_unit_num() const
{
	return _unit_num;
}

int32_t pxtnPulse_Noise::get_smp_num_44k() const
{
	return _smp_num_44k;
}

float pxtnPulse_Noise::get_sec() const
{
	return (float)_smp_num_44k / 44100;
}


pxNOISEDESIGN_UNIT *pxtnPulse_Noise::get_unit( int32_t u )
{
	if( !_units || u < 0 || u >= _unit_num ) return NULL;
	return &_units[ u ];
}


pxtnPulse_Noise::pxtnPulse_Noise( pxtnIO_r io_read, pxtnIO_w io_write, pxtnIO_seek io_seek, pxtnIO_pos io_pos )
{
	_set_io_funcs( io_read, io_write, io_seek, io_pos );

	_units       = NULL;
	_unit_num    =    0;
	_smp_num_44k =    0;
}

pxtnPulse_Noise::~pxtnPulse_Noise()
{
	Release();
}

#define NOISEDESIGNLIMIT_SMPNUM (48000 * 10)
#define NOISEDESIGNLIMIT_ENVE_X ( 1000 * 10)
#define NOISEDESIGNLIMIT_ENVE_Y (  100     )
#define NOISEDESIGNLIMIT_OSC_FREQUENCY 44100.0f
#define NOISEDESIGNLIMIT_OSC_VOLUME      200.0f
#define NOISEDESIGNLIMIT_OSC_OFFSET      100.0f

static void _FixUnit( pxNOISEDESIGN_OSCILLATOR *p_osc )
{
	if( p_osc->type   >= pxWAVETYPE_num                 ) p_osc->type   = pxWAVETYPE_None;
	if( p_osc->freq   >  NOISEDESIGNLIMIT_OSC_FREQUENCY ) p_osc->freq   = NOISEDESIGNLIMIT_OSC_FREQUENCY;
	if( p_osc->freq   <= 0                              ) p_osc->freq   = 0;
	if( p_osc->volume >  NOISEDESIGNLIMIT_OSC_VOLUME    ) p_osc->volume = NOISEDESIGNLIMIT_OSC_VOLUME;
	if( p_osc->volume <= 0                              ) p_osc->volume = 0;
	if( p_osc->offset >  NOISEDESIGNLIMIT_OSC_OFFSET    ) p_osc->offset = NOISEDESIGNLIMIT_OSC_OFFSET;
	if( p_osc->offset <= 0                              ) p_osc->offset = 0;
}

void pxtnPulse_Noise::Fix()
{
	pxNOISEDESIGN_UNIT *p_unit;
	int32_t i, e;

	if( _smp_num_44k > NOISEDESIGNLIMIT_SMPNUM ) _smp_num_44k = NOISEDESIGNLIMIT_SMPNUM;

	for( i = 0; i < _unit_num; i++ )
	{
		p_unit = &_units[ i ];
		if( p_unit->bEnable )
		{
			for( e = 0; e < p_unit->enve_num; e++ )
			{
				if( p_unit->enves[ e ].x > NOISEDESIGNLIMIT_ENVE_X ) p_unit->enves[ e ].x = NOISEDESIGNLIMIT_ENVE_X;
				if( p_unit->enves[ e ].x <                       0 ) p_unit->enves[ e ].x =                       0;
				if( p_unit->enves[ e ].y > NOISEDESIGNLIMIT_ENVE_Y ) p_unit->enves[ e ].y = NOISEDESIGNLIMIT_ENVE_Y;
				if( p_unit->enves[ e ].y <                       0 ) p_unit->enves[ e ].y =                       0;
			}
			if( p_unit->pan < -100 ) p_unit->pan = -100;
			if( p_unit->pan >  100 ) p_unit->pan =  100;
			_FixUnit( &p_unit->main );
			_FixUnit( &p_unit->freq );
			_FixUnit( &p_unit->volu );
		}
	}
}

#define MAX_NOISEEDITUNITNUM     4
#define MAX_NOISEEDITENVELOPENUM 3

#define NOISEEDITFLAG_XX1       0x0001
#define NOISEEDITFLAG_XX2       0x0002
#define NOISEEDITFLAG_ENVELOPE  0x0004
#define NOISEEDITFLAG_PAN       0x0008
#define NOISEEDITFLAG_OSC_MAIN  0x0010
#define NOISEEDITFLAG_OSC_FREQ  0x0020
#define NOISEEDITFLAG_OSC_VOLU  0x0040
#define NOISEEDITFLAG_OSC_PAN   0x0080

#define NOISEEDITFLAG_UNCOVERED 0xffffff83

//                          01234567
static const char *_code = "PTNOISE-";
//                    _ver =  20051028 ; -v.0.9.2.3
static const uint32_t _ver =  20120418 ; // 16 wave types.

bool pxtnPulse_Noise::_WriteOscillator( const pxNOISEDESIGN_OSCILLATOR *p_osc, void* desc, int32_t *p_add ) const
{
	int32_t work;
	work = (int32_t) p_osc->type        ; if( !_data_w_v( desc, work, p_add ) ) return false;
	work = (int32_t) p_osc->b_rev       ; if( !_data_w_v( desc, work, p_add ) ) return false;
	work = (int32_t)(p_osc->freq   * 10); if( !_data_w_v( desc, work, p_add ) ) return false;
	work = (int32_t)(p_osc->volume * 10); if( !_data_w_v( desc, work, p_add ) ) return false;
	work = (int32_t)(p_osc->offset * 10); if( !_data_w_v( desc, work, p_add ) ) return false;
	return true;
}

pxtnERR pxtnPulse_Noise::_ReadOscillator( pxNOISEDESIGN_OSCILLATOR *p_osc, void* desc )
{
	int32_t work;
	if( !_data_r_v( desc, &work )     ) { return pxtnERR_desc_r     ; } p_osc->type     = (pxWAVETYPE)work;
	if( p_osc->type >= pxWAVETYPE_num ) { return pxtnERR_fmt_unknown; }
	if( !_data_r_v( desc, &work )     ) { return pxtnERR_desc_r     ; } p_osc->b_rev    = work ? true : false;
	if( !_data_r_v( desc, &work )     ) { return pxtnERR_desc_r     ; } p_osc->freq     = (float)work / 10;
	if( !_data_r_v( desc, &work )     ) { return pxtnERR_desc_r     ; } p_osc->volume   = (float)work / 10;
	if( !_data_r_v( desc, &work )     ) { return pxtnERR_desc_r     ; } p_osc->offset   = (float)work / 10;

	return pxtnOK;
}

uint32_t pxtnPulse_Noise::_MakeFlags( const pxNOISEDESIGN_UNIT *pU ) const
{
	uint32_t flags = 0;
	flags |= NOISEEDITFLAG_ENVELOPE;
	if( pU->pan                          ) flags |= NOISEEDITFLAG_PAN     ;
	if( pU->main.type != pxWAVETYPE_None ) flags |= NOISEEDITFLAG_OSC_MAIN;
	if( pU->freq.type != pxWAVETYPE_None ) flags |= NOISEEDITFLAG_OSC_FREQ;
	if( pU->volu.type != pxWAVETYPE_None ) flags |= NOISEEDITFLAG_OSC_VOLU;
	return flags;
}

bool pxtnPulse_Noise::write( void* desc, int32_t *p_add ) const
{
	bool  b_ret = false;
	int32_t   u, e, seek, num_seek, flags;
	char  byte;
	char  unit_num = 0;
	const pxNOISEDESIGN_UNIT *pU;

	if( p_add ) seek = *p_add;
	else        seek =      0;

	if( !_io_write( desc,_code, 1, 8 ) ) goto End;
	if( !_io_write_le32( desc, &_ver ) ) goto End;
	seek += 12;
	if( !_data_w_v( desc, _smp_num_44k, &seek ) ) goto End;

	if( !_io_write( desc,&unit_num , 1, 1 ) ) goto End;
	num_seek = seek;
	seek += 1;

	for( u = 0; u < _unit_num; u++ )
	{
		pU = &_units[ u ];
		if( pU->bEnable )
		{
			// フラグ
			flags = _MakeFlags( pU );
			if( !_data_w_v( desc, flags, &seek ) ) goto End;
			if( flags & NOISEEDITFLAG_ENVELOPE )
			{
				if( !_data_w_v( desc, pU->enve_num, &seek ) ) goto End;
				for( e = 0; e < pU->enve_num; e++ )
				{
					if( !_data_w_v( desc, pU->enves[ e ].x, &seek ) ) goto End;
					if( !_data_w_v( desc, pU->enves[ e ].y, &seek ) ) goto End;
				}
			}
			if( flags & NOISEEDITFLAG_PAN      )
			{
				byte = (char)pU->pan;
				if( !_io_write( desc,&byte, 1, 1 ) ) goto End;
				seek++;
			}
			if( flags & NOISEEDITFLAG_OSC_MAIN ){ if( !_WriteOscillator( &pU->main, desc, &seek ) ) goto End; }
			if( flags & NOISEEDITFLAG_OSC_FREQ ){ if( !_WriteOscillator( &pU->freq, desc, &seek ) ) goto End; }
			if( flags & NOISEEDITFLAG_OSC_VOLU ){ if( !_WriteOscillator( &pU->volu, desc, &seek ) ) goto End; }
			unit_num++;
		}
	}

	// update unit_num.
	_io_seek( desc, SEEK_CUR, num_seek - seek    );
	if( !_io_write( desc,&unit_num, 1, 1 ) ) goto End;
	_io_seek( desc, SEEK_CUR, seek - num_seek -1 );
	if( p_add ) *p_add = seek;

	b_ret = true;
End:

	return b_ret;
}

pxtnERR pxtnPulse_Noise::read( void* desc )
{
	pxtnERR  res       = pxtnERR_VOID;
	uint32_t flags     =            0;
	char     unit_num  =            0;
	char     byte      =            0;
	uint32_t ver       =            0;

	pxNOISEDESIGN_UNIT* pU = NULL;

	char       code[ 8 ] = {0};

	Release();

	if( !_io_read( desc, code, 1, 8 )         ){ res = pxtnERR_desc_r     ; goto term; }
	if( memcmp( code, _code, 8 )        ){ res = pxtnERR_inv_code   ; goto term; }
	if( !_io_read_le32( desc, &ver )    ){ res = pxtnERR_desc_r     ; goto term; }
	if( ver > _ver                      ){ res = pxtnERR_fmt_new    ; goto term; }
	if( !_data_r_v( desc, &_smp_num_44k )    ){ res = pxtnERR_desc_r     ; goto term; }
	if( !_io_read( desc, &unit_num, 1, 1 )    ){ res = pxtnERR_desc_r     ; goto term; }
	if( unit_num < 0                    ){ res = pxtnERR_inv_data   ; goto term; }
	if( unit_num > MAX_NOISEEDITUNITNUM ){ res = pxtnERR_fmt_unknown; goto term; }
	_unit_num = unit_num;

	if( !pxtnMem_zero_alloc( (void**)&_units, sizeof(pxNOISEDESIGN_UNIT) * _unit_num ) ){ res = pxtnERR_memory; goto term; }

	for( int32_t u = 0; u < _unit_num; u++ )
	{
		pU = &_units[ u ];
		pU->bEnable = true;

		if( !_data_r_v( desc, (int32_t*)&flags ) ){ res = pxtnERR_desc_r     ; goto term; }
		if( flags & NOISEEDITFLAG_UNCOVERED ){ res = pxtnERR_fmt_unknown; goto term; }

		// envelope
		if( flags & NOISEEDITFLAG_ENVELOPE )
		{
			if( !_data_r_v( desc, &pU->enve_num ) ){ res = pxtnERR_desc_r; goto term; }
			if( pU->enve_num > MAX_NOISEEDITENVELOPENUM ){ res = pxtnERR_fmt_unknown; goto term; }
			if( !pxtnMem_zero_alloc( (void**)&pU->enves, sizeof(pxtnPOINT) * pU->enve_num ) ){ res = pxtnERR_memory; goto term; }
			for( int32_t e = 0; e < pU->enve_num; e++ )
			{
				if( !_data_r_v( desc, &pU->enves[ e ].x ) ){ res = pxtnERR_desc_r; goto term; }
				if( !_data_r_v( desc, &pU->enves[ e ].y ) ){ res = pxtnERR_desc_r; goto term; }
			}
		}
		// pan
		if( flags & NOISEEDITFLAG_PAN )
		{
			if( !_io_read( desc, &byte, 1, 1 ) ){ res = pxtnERR_desc_r; goto term; }
			pU->pan = byte;
		}

		if( flags & NOISEEDITFLAG_OSC_MAIN ){ res = _ReadOscillator( &pU->main, desc ); if( res != pxtnOK ) goto term; }
		if( flags & NOISEEDITFLAG_OSC_FREQ ){ res = _ReadOscillator( &pU->freq, desc ); if( res != pxtnOK ) goto term; }
		if( flags & NOISEEDITFLAG_OSC_VOLU ){ res = _ReadOscillator( &pU->volu, desc ); if( res != pxtnOK ) goto term; }
	}

	res = pxtnOK;
term:
	if( res != pxtnOK ) Release();

	return res;
}

void pxtnPulse_Noise::Release()
{
	if( _units )
	{
		for( int32_t u = 0; u < _unit_num; u++ )
		{
			if( _units[ u ].enves ) pxtnMem_free( (void**)&_units[ u ].enves );
		}
		pxtnMem_free( (void**)&_units );
		_unit_num = 0;
	}
}

bool pxtnPulse_Noise::Allocate( int32_t unit_num, int32_t envelope_num )
{
	bool b_ret = false;

	Release();

	_unit_num = unit_num;
	if( !pxtnMem_zero_alloc( (void**)&_units, sizeof(pxNOISEDESIGN_UNIT) * unit_num ) ) goto End;

	for( int32_t u = 0; u < unit_num; u++ )
	{
		pxNOISEDESIGN_UNIT *p_unit = &_units[ u ];
		p_unit->enve_num = envelope_num;
		if( !pxtnMem_zero_alloc( (void**)&p_unit->enves, sizeof(pxtnPOINT) * p_unit->enve_num ) ) goto End;
	}

	b_ret = true;
End:
	if( !b_ret ) Release();

	return b_ret;
}

bool pxtnPulse_Noise::copy_from( const pxtnPulse_Noise* src )
{
	pxtnData::copy_from( src );
	if( !src ) return false;

	bool b_ret = false;

	Release();
	_smp_num_44k = src->_smp_num_44k;

	if( src->_unit_num )
	{
		int32_t enve_num = src->_units[ 0 ].enve_num;
		if( !Allocate( src->_unit_num, enve_num ) ) goto End;
		for( int32_t u = 0; u < src->_unit_num; u++ )
		{
			_units[ u ].bEnable  = src->_units[ u ].bEnable ;
			_units[ u ].enve_num = src->_units[ u ].enve_num;
			_units[ u ].freq     = src->_units[ u ].freq    ;
			_units[ u ].main     = src->_units[ u ].main    ;
			_units[ u ].pan      = src->_units[ u ].pan     ;
			_units[ u ].volu     = src->_units[ u ].volu    ;
			if( !( _units[ u ].enves = (pxtnPOINT*)malloc( sizeof(pxtnPOINT) * enve_num ) ) ) goto End;
			for( int32_t e = 0; e < enve_num; e++ ) _units[ u ].enves[ e ] = src->_units[ u ].enves[ e ];
		}
	}

	b_ret = true;
End:
	if( !b_ret ) Release();

	return b_ret;
}

static int32_t _CompareOsci( const pxNOISEDESIGN_OSCILLATOR *p_osc1, const pxNOISEDESIGN_OSCILLATOR *p_osc2 )
{
	if( p_osc1->type   != p_osc2->type   ) return 1;
	if( p_osc1->freq   != p_osc2->freq   ) return 1;
	if( p_osc1->volume != p_osc2->volume ) return 1;
	if( p_osc1->offset != p_osc2->offset ) return 1;
	if( p_osc1->b_rev  != p_osc2->b_rev  ) return 1;
	return 0;
}

int32_t pxtnPulse_Noise::Compare     ( const pxtnPulse_Noise *p_src ) const
{
	if( !p_src ) return -1;

	if( p_src->_smp_num_44k != _smp_num_44k ) return 1;
	if( p_src->_unit_num    != _unit_num    ) return 1;

	for( int32_t u = 0; u < _unit_num; u++ )
	{
		if( p_src->_units[ u ].bEnable  != _units[ u ].bEnable          ) return 1;
		if( p_src->_units[ u ].enve_num != _units[ u ].enve_num         ) return 1;
		if( p_src->_units[ u ].pan      != _units[ u ].pan              ) return 1;
		if( _CompareOsci( &p_src->_units[ u ].main, &_units[ u ].main ) ) return 1;
		if( _CompareOsci( &p_src->_units[ u ].freq, &_units[ u ].freq ) ) return 1;
		if( _CompareOsci( &p_src->_units[ u ].volu, &_units[ u ].volu ) ) return 1;

		for( int32_t e = 0; e < _units[ u ].enve_num; e++ )
		{
			if( p_src->_units[ u ].enves[ e ].x != _units[ u ].enves[ e ].x ) return 1;
			if( p_src->_units[ u ].enves[ e ].y != _units[ u ].enves[ e ].y ) return 1;
		}
	}
	return 0;
}
