//
// x1x : v.0.1.2.8 (-2005/06/03) project-info has quality, tempo, clock.
// x2x : v.0.6.N.N (-2006/01/15) no exe version.
// x3x : v.0.7.N.N (-2006/09/30) unit includes voice / basic-key use for only view
//                               no-support event: voice_no, group_no, tuning.
// x4x : v.0.8.3.4 (-2007/10/20) unit has event-list.

#include "./pxtn.h"

#include "./pxtnService.h"


#define _VERSIONSIZE    16
#define _CODESIZE        8

//                                       0123456789012345
static const char* _code_tune_x2x     = "PTTUNE--20050608";
static const char* _code_tune_x3x     = "PTTUNE--20060115";
static const char* _code_tune_x4x     = "PTTUNE--20060930";
static const char* _code_tune_v5      = "PTTUNE--20071119";

static const char* _code_proj_x1x     = "PTCOLLAGE-050227";
static const char* _code_proj_x2x     = "PTCOLLAGE-050608";
static const char* _code_proj_x3x     = "PTCOLLAGE-060115";
static const char* _code_proj_x4x     = "PTCOLLAGE-060930";
static const char* _code_proj_v5      = "PTCOLLAGE-071119";


static const char* _code_x1x_PROJ     = "PROJECT=";
static const char* _code_x1x_EVEN     = "EVENT===";
static const char* _code_x1x_UNIT     = "UNIT====";
static const char* _code_x1x_END      = "END=====";
static const char* _code_x1x_PCM      = "matePCM=";

static const char* _code_x3x_pxtnUNIT = "pxtnUNIT";
static const char* _code_x4x_evenMAST = "evenMAST";
static const char* _code_x4x_evenUNIT = "evenUNIT";

static const char* _code_antiOPER     = "antiOPER"; // anti operation(edit)

static const char* _code_num_UNIT     = "num UNIT";
static const char* _code_MasterV5     = "MasterV5";
static const char* _code_Event_V5     = "Event V5";
static const char* _code_matePCM      = "matePCM ";
static const char* _code_matePTV      = "matePTV ";
static const char* _code_matePTN      = "matePTN ";
static const char* _code_mateOGGV     = "mateOGGV";
static const char* _code_effeDELA     = "effeDELA";
static const char* _code_effeOVER     = "effeOVER";
static const char* _code_textNAME     = "textNAME";
static const char* _code_textCOMM     = "textCOMM";
static const char* _code_assiUNIT     = "assiUNIT";
static const char* _code_assiWOIC     = "assiWOIC";
static const char* _code_pxtoneND     = "pxtoneND";

enum _enum_Tag
{
	_TAG_Unknown  = 0,
	_TAG_antiOPER    ,

	_TAG_x1x_PROJ    ,
	_TAG_x1x_UNIT    ,
	_TAG_x1x_PCM     ,
	_TAG_x1x_EVEN    ,
	_TAG_x1x_END     ,
	_TAG_x3x_pxtnUNIT,
	_TAG_x4x_evenMAST,
	_TAG_x4x_evenUNIT,

	_TAG_num_UNIT    ,
	_TAG_MasterV5    ,
	_TAG_Event_V5    ,
	_TAG_matePCM     ,
	_TAG_matePTV     ,
	_TAG_matePTN     ,
	_TAG_mateOGGV    ,
	_TAG_effeDELA    ,
	_TAG_effeOVER    ,
	_TAG_textNAME    ,
	_TAG_textCOMM    ,
	_TAG_assiUNIT    ,
	_TAG_assiWOIC    ,
	_TAG_pxtoneND

};



pxtnService::pxtnService( pxtnIO_r io_read, pxtnIO_w io_write, pxtnIO_seek io_seek, pxtnIO_pos io_pos )
{
	_set_io_funcs( io_read, io_write, io_seek, io_pos );

	_b_init          = false;
	_b_edit          = false;
	_b_fix_evels_num = false;

	text          = NULL;
	master        = NULL;
	evels         = NULL;

	_delays       = NULL; _delay_max = _delay_num = 0;
	_ovdrvs       = NULL; _ovdrv_max = _ovdrv_num = 0;
	_woices       = NULL; _woice_max = _woice_num = 0;
	_units        = NULL; _unit_max  = _unit_num  = 0;

	_ptn_bldr     = NULL;

	_sampled_proc = NULL;
	_sampled_user = NULL;

	_moo_constructor();
}

bool pxtnService::_release()
{
	if( !_b_init ) return false;
	_b_init = false;

	_moo_destructer();

	SAFE_DELETE( text      );
	SAFE_DELETE( master    );
	SAFE_DELETE( evels     );
	SAFE_DELETE( _ptn_bldr );
	if( _delays ){ for( int32_t i = 0; i < _delay_num; i++ ) SAFE_DELETE( _delays[ i ] ); free( _delays ); _delays = NULL; }
	if( _ovdrvs ){ for( int32_t i = 0; i < _ovdrv_num; i++ ) SAFE_DELETE( _ovdrvs[ i ] ); free( _ovdrvs ); _ovdrvs = NULL; }
	if( _woices ){ for( int32_t i = 0; i < _woice_num; i++ ) SAFE_DELETE( _woices[ i ] ); free( _woices ); _woices = NULL; }
	if( _units  ){ for( int32_t i = 0; i < _unit_num ; i++ ) SAFE_DELETE( _units [ i ] ); free( _units  ); _units  = NULL; }
	return true;
}

pxtnService::~pxtnService()
{
	_release();
}

pxtnERR pxtnService:: init        (){ return _init( 0, false ); }
pxtnERR pxtnService:: init_collage( int32_t fix_evels_num ){ return _init( fix_evels_num, true ); }
pxtnERR pxtnService::_init( int32_t fix_evels_num, bool b_edit )
{
	if( _b_init ) return pxtnERR_INIT;

	pxtnERR res       = pxtnERR_VOID;
	int32_t byte_size =            0;

	if( !( text      = new pxtnText              ( _io_read, _io_write, _io_seek, _io_pos ) ) ){ res = pxtnERR_INIT    ; goto End; }
	if( !( master    = new pxtnMaster            ( _io_read, _io_write, _io_seek, _io_pos ) ) ){ res = pxtnERR_INIT    ; goto End; }
	if( !( evels     = new pxtnEvelist           ( _io_read, _io_write, _io_seek, _io_pos ) ) ){ res = pxtnERR_INIT    ; goto End; }
	if( !( _ptn_bldr = new pxtnPulse_NoiseBuilder( _io_read, _io_write, _io_seek, _io_pos ) ) ){ res = pxtnERR_INIT    ; goto End; }
	if( !  _ptn_bldr->Init() ){ res = pxtnERR_ptn_init; goto End; }

	if( fix_evels_num )
	{
		_b_fix_evels_num = true;
		if( !evels->Allocate( fix_evels_num )   ){ res = pxtnERR_memory; goto End; }
	}
	else
	{
		_b_fix_evels_num = false;
	}

	// delay
	byte_size = sizeof(pxtnDelay*) * pxtnMAX_TUNEDELAYSTRUCT;
	if( !(  _delays = (pxtnDelay**    )malloc( byte_size ) ) ){ res = pxtnERR_memory; goto End; }
	memset( _delays, 0,                        byte_size );
	_delay_max = pxtnMAX_TUNEDELAYSTRUCT;

	// over-drive
	byte_size = sizeof(pxtnOverDrive*) * pxtnMAX_TUNEOVERDRIVESTRUCT;
	if( !(  _ovdrvs = (pxtnOverDrive**)malloc( byte_size ) ) ){ res = pxtnERR_memory; goto End; }
	memset( _ovdrvs, 0,                        byte_size );
	_ovdrv_max = pxtnMAX_TUNEOVERDRIVESTRUCT;

	// woice
	byte_size = sizeof(pxtnWoice*) * pxtnMAX_TUNEWOICESTRUCT;
	if( !(  _woices = (pxtnWoice**    )malloc( byte_size ) ) ){ res = pxtnERR_memory; goto End; }
	memset( _woices, 0,                        byte_size );
	_woice_max = pxtnMAX_TUNEWOICESTRUCT;

	// unit
	byte_size = sizeof(pxtnUnit*) * pxtnMAX_TUNEUNITSTRUCT;
	if( !(  _units = (pxtnUnit**      )malloc( byte_size ) ) ){ res = pxtnERR_memory; goto End; }
	memset( _units, 0,                         byte_size );
	_unit_max = pxtnMAX_TUNEUNITSTRUCT;

	_group_num = pxtnMAX_TUNEGROUPNUM  ;

	if( !_moo_init() ){ res = pxtnERR_moo_init; goto End; }


	if( fix_evels_num ) _moo_b_valid_data = true;

	_b_edit = b_edit;
	res     = pxtnOK;
	_b_init = true  ;
End:
	if( !_b_init ) _release();
	return res;
}

bool pxtnService::AdjustMeasNum()
{
	if( !_b_init ) return false;
	master->AdjustMeasNum( evels->get_Max_Clock() );
	return true;
}

int32_t  pxtnService::Group_Num() const{ return _b_init ? _group_num : 0; }

pxtnERR pxtnService::tones_ready()
{
	if( !_b_init ) return pxtnERR_INIT;

	pxtnERR res        = pxtnERR_VOID;
	int32_t beat_num   = master->get_beat_num  ();
	float   beat_tempo = master->get_beat_tempo();

	for( int32_t i = 0; i < _delay_num; i++ )
	{
		res = _delays[ i ]->Tone_Ready( beat_num, beat_tempo, _dst_sps );
		if( res != pxtnOK ) return res;
	}
	for( int32_t i = 0; i < _ovdrv_num; i++ )
	{
		_ovdrvs[ i ]->Tone_Ready();
	}
	for( int32_t i = 0; i < _woice_num; i++ )
	{
		res = _woices[ i ]->Tone_Ready( _ptn_bldr, _dst_sps );
		if( res != pxtnOK ) return res;
	}
	return pxtnOK;
}

bool pxtnService::tones_clear()
{
	if( !_b_init ) return false;
	for( int32_t i = 0; i < _delay_num; i++ ) _delays[ i ]->Tone_Clear();
	for( int32_t i = 0; i < _unit_num;  i++ ) _units [ i ]->Tone_Clear();
	return true;
}

// ---------------------------
// Delay..
// ---------------------------

int32_t  pxtnService::Delay_Num() const{ return _b_init ? _delay_num : 0; }
int32_t  pxtnService::Delay_Max() const{ return _b_init ? _delay_max : 0; }

bool pxtnService::Delay_Set( int32_t idx, DELAYUNIT unit, float freq, float rate, int32_t group )
{
	if( !_b_init ) return false;
	if( idx >= _delay_num ) return false;
	_delays[ idx ]->Set( unit, freq, rate, group );
	return true;
}

bool pxtnService::Delay_Add( DELAYUNIT unit, float freq, float rate, int32_t group )
{
	if( !_b_init ) return false;
	if( _delay_num >= _delay_max ) return false;
	_delays[ _delay_num ] = new pxtnDelay( _io_read, _io_write, _io_seek, _io_pos );
	_delays[ _delay_num ]->Set( unit, freq, rate, group );
	_delay_num++;
	return true;
}

bool pxtnService::Delay_Remove( int32_t idx )
{
	if( !_b_init ) return false;
	if( idx >= _delay_num ) return false;

	SAFE_DELETE( _delays[ idx ] );
	_delay_num--;
	for( int32_t i = idx; i < _delay_num; i++ ) _delays[ i ] = _delays[ i + 1 ];
	_delays[ _delay_num ] = NULL;
	return true;
}

pxtnDelay *pxtnService::Delay_Get( int32_t idx )
{
	if( !_b_init ) return NULL;
	if( idx < 0 || idx >= _delay_num ) return NULL;
	return _delays[ idx ];
}

pxtnERR pxtnService::Delay_ReadyTone( int32_t idx )
{
	if( !_b_init ) return pxtnERR_INIT;
	if( idx < 0 || idx >= _delay_num ) return pxtnERR_param;
	return _delays[ idx ]->Tone_Ready( master->get_beat_num(), master->get_beat_tempo(), _dst_sps );
}

// ---------------------------
// Over Drive..
// ---------------------------

int32_t  pxtnService::OverDrive_Num() const{ return _b_init ? _ovdrv_num : 0; }
int32_t  pxtnService::OverDrive_Max() const{ return _b_init ? _ovdrv_max : 0; }

bool pxtnService::OverDrive_Set( int32_t idx, float cut, float amp, int32_t group )
{
	if( !_b_init ) return false;
	if( idx >= _ovdrv_num ) return false;
	_ovdrvs[ idx ]->Set( cut, amp, group );
	return true;
}

bool pxtnService::OverDrive_Add( float cut, float amp, int32_t group )
{
	if( !_b_init ) return false;
	if( _ovdrv_num >= _ovdrv_max ) return false;
	_ovdrvs[ _ovdrv_num ] = new pxtnOverDrive( _io_read, _io_write, _io_seek, _io_pos );
	_ovdrvs[ _ovdrv_num ]->Set( cut, amp, group );
	_ovdrv_num++;
	return true;
}

bool pxtnService::OverDrive_Remove( int32_t idx )
{
	if( !_b_init ) return false;
	if( idx >= _ovdrv_num ) return false;

	SAFE_DELETE( _ovdrvs[ idx ] );
	_ovdrv_num--;
	for( int32_t i = idx; i < _ovdrv_num; i++ ) _ovdrvs[ i ] = _ovdrvs[ i + 1 ];
	_ovdrvs[ _ovdrv_num ] = NULL;
	return true;
}

pxtnOverDrive *pxtnService::OverDrive_Get( int32_t idx )
{
	if( !_b_init ) return NULL;
	if( idx < 0 || idx >= _ovdrv_num ) return NULL;
	return _ovdrvs[ idx ];
}

bool pxtnService::OverDrive_ReadyTone( int32_t idx )
{
	if( !_b_init ) return false;
	if( idx < 0 || idx >= _ovdrv_num ) return false;
	_ovdrvs[ idx ]->Tone_Ready();
	return true;
}

// ---------------------------
// Woice..
// ---------------------------

int32_t  pxtnService::Woice_Num() const{ return _b_init ? _woice_num : 0; }
int32_t  pxtnService::Woice_Max() const{ return _b_init ? _woice_max : 0; }

const pxtnWoice *pxtnService::Woice_Get( int32_t idx ) const
{
	if( !_b_init ) return NULL;
	if( idx < 0 || idx >= _woice_num ) return NULL;
	return _woices[ idx ];
}
pxtnWoice       *pxtnService::Woice_Get_variable( int32_t idx )
{
	if( !_b_init ) return NULL;
	if( idx < 0 || idx >= _woice_num ) return NULL;
	return _woices[ idx ];
}


pxtnERR pxtnService::Woice_read( int32_t idx, void* desc, pxtnWOICETYPE type )
{
	if( !_b_init ) return pxtnERR_INIT;
	if( idx < 0 || idx >= _woice_max ) return pxtnERR_param;
	if( idx > _woice_num             ) return pxtnERR_param;
	if( idx == _woice_num ){ _woices[ idx ] = new pxtnWoice( _io_read, _io_write, _io_seek, _io_pos ); _woice_num++; }

	pxtnERR res = pxtnERR_VOID;
	res = _woices[ idx ]->read( desc, type ); if( res != pxtnOK ){ Woice_Remove( idx ); return res; }
	return res;
}

pxtnERR pxtnService::Woice_ReadyTone( int32_t idx )
{
	if( !_b_init ) return pxtnERR_INIT;
	if( idx < 0 || idx >= _woice_num ) return pxtnERR_param;
	return _woices[ idx ]->Tone_Ready( _ptn_bldr, _dst_sps );
}

bool pxtnService::Woice_Remove( int32_t idx )
{
	if( !_b_init ) return false;
	if( idx < 0 || idx >= _woice_num ) return false;
	SAFE_DELETE( _woices[ idx ] );
	_woice_num--;
	for( int32_t i = idx; i < _woice_num; i++ ) _woices[ i ] = _woices[ i + 1 ];
	_woices[ _woice_num ] = NULL;
	return true;
}

bool pxtnService::Woice_Replace( int32_t old_place, int32_t new_place )
{
	if( !_b_init ) return false;

	pxtnWoice* p_w       = _woices[ old_place ];
	int32_t    max_place = _woice_num - 1;

	if( new_place >  max_place ) new_place = max_place;
	if( new_place == old_place ) return true;

	if( old_place < new_place )
	{
		for( int32_t w = old_place; w < new_place; w++ ){ if( _woices[ w ] ) _woices[ w ] = _woices[ w + 1 ]; }
	}
	else
	{
		for( int32_t w = old_place; w > new_place; w-- ){ if( _woices[ w ] ) _woices[ w ] = _woices[ w - 1 ]; }
	}

	_woices[ new_place ] = p_w;
	return true;
}

// ---------------------------
// Unit..
// ---------------------------

int32_t  pxtnService::Unit_Num() const{ return _b_init ? _unit_num : 0; }
int32_t  pxtnService::Unit_Max() const{ return _b_init ? _unit_max : 0; }

const pxtnUnit *pxtnService::Unit_Get( int32_t idx ) const
{
	if( !_b_init ) return NULL;
	if( idx < 0 || idx >= _unit_num ) return NULL;
	return _units[ idx ];
}
pxtnUnit       *pxtnService::Unit_Get_variable( int32_t idx )
{
	if( !_b_init ) return NULL;
	if( idx < 0 || idx >= _unit_num ) return NULL;
	return _units[ idx ];
}

bool pxtnService::Unit_AddNew()
{
	if( _unit_num >= _unit_max ) return false;
	_units[ _unit_num ] = new pxtnUnit( _io_read, _io_write, _io_seek, _io_pos );
	_unit_num++;
	return true;
}

bool pxtnService::Unit_Remove( int32_t idx )
{
	if( !_b_init ) return false;
	if( idx < 0 || idx >= _unit_num ) return false;
	SAFE_DELETE( _units[ idx ] );
	_unit_num--;
	for( int32_t i = idx; i < _unit_num; i++ ) _units[ i ] = _units[ i + 1 ];
	_units[ _unit_num ] = NULL;
	return true;
}

bool pxtnService::Unit_Replace( int32_t old_place, int32_t new_place )
{
	if( !_b_init ) return false;

	pxtnUnit* p_w        = _units[ old_place ];
	int32_t   max_place  = _unit_num - 1;

	if( new_place >  max_place ) new_place = max_place;
	if( new_place == old_place ) return true;

	if( old_place < new_place )
	{
		for( int32_t w = old_place; w < new_place; w++ ){ if( _units[ w ] ) _units[ w ] = _units[ w + 1 ]; }
	}
	else
	{
		for( int32_t w = old_place; w > new_place; w-- ){ if( _units[ w ] ) _units[ w ] = _units[ w - 1 ]; }
	}
	_units[ new_place ] = p_w;
	return true;
}

bool pxtnService::Unit_SetOpratedAll( bool b )
{
	if( !_b_init ) return false;
	for( int32_t u = 0; u < _unit_num; u++ )
	{
		_units[ u ]->set_operated( b );
		if( b ) _units[ u ]->set_played( true );
	}
	return true;
}

bool pxtnService::Unit_Solo( int32_t idx )
{
	if( !_b_init ) return false;
	for( int32_t u = 0; u < _unit_num; u++ )
	{
		if( u == idx ) _units[ u ]->set_played( true  );
		else           _units[ u ]->set_played( false );
	}
	return false;
}


// ---------------------------
// Quality..
// ---------------------------

bool pxtnService::set_destination_quality( int32_t ch_num, int32_t sps )
{
	if( !_b_init ) return false;
	switch( ch_num )
	{
	case  1: break;
	case  2: break;
	default: return false;
	}

	_dst_ch_num = ch_num;
	_dst_sps    = sps   ;
	_dst_byte_per_smp = pxtnBITPERSAMPLE / 8 * ch_num;
	return true;
}

bool pxtnService::get_destination_quality( int32_t *p_ch_num, int32_t *p_sps ) const
{
	if( !_b_init ) return false;
	if( p_ch_num ) *p_ch_num = _dst_ch_num;
	if( p_sps    ) *p_sps    = _dst_sps   ;
	return true;
}

bool pxtnService::set_sampled_callback   ( pxtnSampledCallback proc, void* user )
{
	if( !_b_init ) return false;
	_sampled_proc = proc;
	_sampled_user = user;
	return true;
}


static _enum_Tag _CheckTagCode( const char *p_code )
{
	if(      !memcmp( p_code, _code_antiOPER    , _CODESIZE ) ) return _TAG_antiOPER;
	else if( !memcmp( p_code, _code_x1x_PROJ    , _CODESIZE ) ) return _TAG_x1x_PROJ;
	else if( !memcmp( p_code, _code_x1x_UNIT    , _CODESIZE ) ) return _TAG_x1x_UNIT;
	else if( !memcmp( p_code, _code_x1x_PCM     , _CODESIZE ) ) return _TAG_x1x_PCM;
	else if( !memcmp( p_code, _code_x1x_EVEN    , _CODESIZE ) ) return _TAG_x1x_EVEN;
	else if( !memcmp( p_code, _code_x1x_END     , _CODESIZE ) ) return _TAG_x1x_END;
	else if( !memcmp( p_code, _code_x3x_pxtnUNIT, _CODESIZE ) ) return _TAG_x3x_pxtnUNIT;
	else if( !memcmp( p_code, _code_x4x_evenMAST, _CODESIZE ) ) return _TAG_x4x_evenMAST;
	else if( !memcmp( p_code, _code_x4x_evenUNIT, _CODESIZE ) ) return _TAG_x4x_evenUNIT;
	else if( !memcmp( p_code, _code_num_UNIT    , _CODESIZE ) ) return _TAG_num_UNIT;
	else if( !memcmp( p_code, _code_Event_V5    , _CODESIZE ) ) return _TAG_Event_V5;
	else if( !memcmp( p_code, _code_MasterV5    , _CODESIZE ) ) return _TAG_MasterV5;
	else if( !memcmp( p_code, _code_matePCM     , _CODESIZE ) ) return _TAG_matePCM ;
	else if( !memcmp( p_code, _code_matePTV     , _CODESIZE ) ) return _TAG_matePTV ;
	else if( !memcmp( p_code, _code_matePTN     , _CODESIZE ) ) return _TAG_matePTN ;
	else if( !memcmp( p_code, _code_mateOGGV    , _CODESIZE ) ) return _TAG_mateOGGV;
	else if( !memcmp( p_code, _code_effeDELA    , _CODESIZE ) ) return _TAG_effeDELA;
	else if( !memcmp( p_code, _code_effeOVER    , _CODESIZE ) ) return _TAG_effeOVER;
	else if( !memcmp( p_code, _code_textNAME    , _CODESIZE ) ) return _TAG_textNAME;
	else if( !memcmp( p_code, _code_textCOMM    , _CODESIZE ) ) return _TAG_textCOMM;
	else if( !memcmp( p_code, _code_assiUNIT    , _CODESIZE ) ) return _TAG_assiUNIT;
	else if( !memcmp( p_code, _code_assiWOIC    , _CODESIZE ) ) return _TAG_assiWOIC;
	else if( !memcmp( p_code, _code_pxtoneND    , _CODESIZE ) ) return _TAG_pxtoneND;
	return _TAG_Unknown;
}

bool pxtnService::clear()
{
	if( !_b_init ) return false;

	if( !_b_edit ) _moo_b_valid_data = false;

	if( !text->set_name_buf   ( "", 0 ) ) return false;
	if( !text->set_comment_buf( "", 0 ) ) return false;

	evels->Clear();

	for( int32_t i = 0; i < _delay_num; i++ ) SAFE_DELETE( _delays[ i ] ); _delay_num = 0;
	for( int32_t i = 0; i < _delay_num; i++ ) SAFE_DELETE( _ovdrvs[ i ] ); _ovdrv_num = 0;
	for( int32_t i = 0; i < _woice_num; i++ ) SAFE_DELETE( _woices[ i ] ); _woice_num = 0;
	for( int32_t i = 0; i < _unit_num ; i++ ) SAFE_DELETE( _units [ i ] ); _unit_num  = 0;

	master->Reset();

	if( !_b_edit ) evels->Release();
	else           evels->Clear  ();
	return true;
}

pxtnERR pxtnService::_io_Read_Delay( void* desc )
{
	if( !_b_init ) return pxtnERR_INIT;
	if( !_delays ) return pxtnERR_INIT;
	if( _delay_num >= _delay_max ) return pxtnERR_fmt_unknown;

	pxtnERR    res   = pxtnERR_VOID;
	pxtnDelay* delay = new pxtnDelay( _io_read, _io_write, _io_seek, _io_pos );

	res = delay->Read( desc ); if( res != pxtnOK ) goto term;
	res = pxtnOK;
term:
	if( res == pxtnOK ){ _delays[ _delay_num ] = delay; _delay_num++; }
	else               { SAFE_DELETE( delay ); }
	return res;
}

pxtnERR pxtnService::_io_Read_OverDrive( void* desc )
{
	if( !_b_init ) return pxtnERR_INIT;
	if( !_ovdrvs ) return pxtnERR_INIT;
	if( _ovdrv_num >= _ovdrv_max ) return pxtnERR_fmt_unknown;

	pxtnERR    res   = pxtnERR_VOID;
	pxtnOverDrive *ovdrv = new pxtnOverDrive( _io_read, _io_write, _io_seek, _io_pos );
	res = ovdrv->Read( desc ); if( res != pxtnOK ) goto term;
	res = pxtnOK;
term:
	if( res == pxtnOK ){ _ovdrvs[ _ovdrv_num ] = ovdrv; _ovdrv_num++; }
	else               { SAFE_DELETE( ovdrv ); }

	return res;
}

pxtnERR pxtnService::_io_Read_Woice( void* desc, pxtnWOICETYPE type )
{
	pxtnERR res = pxtnERR_VOID;

	if( !_b_init ) return pxtnERR_INIT;
	if( !_woices ) return pxtnERR_INIT;
	if( _woice_num >= _woice_max ) return pxtnERR_woice_full;

	pxtnWoice *woice = new pxtnWoice( _io_read, _io_write, _io_seek, _io_pos );

	switch( type )
	{
	case pxtnWOICE_PCM : res = woice->io_matePCM_r( desc ); if( res != pxtnOK ) goto term; break;
	case pxtnWOICE_PTV : res = woice->io_matePTV_r( desc ); if( res != pxtnOK ) goto term; break;
	case pxtnWOICE_PTN : res = woice->io_matePTN_r( desc ); if( res != pxtnOK ) goto term; break;
	case pxtnWOICE_OGGV:

#ifdef pxINCLUDE_OGGVORBIS
		res = woice->io_mateOGGV_r( desc );
		if( res != pxtnOK ) goto term;
#else
		res = pxtnERR_ogg_no_supported; goto term;
#endif
		break;

	default: res = pxtnERR_fmt_unknown; goto term;
	}
	_woices[ _woice_num ] = woice; _woice_num++;
	res = pxtnOK;
term:
	if( res != pxtnOK ) SAFE_DELETE( woice );
	return res;
}

pxtnERR pxtnService::_io_Read_OldUnit( void* desc, int32_t ver        )
{
	if( !_b_init               ) return pxtnERR_INIT;
	if( !_units                ) return pxtnERR_INIT;
	if( _unit_num >= _unit_max ) return pxtnERR_fmt_unknown;

	pxtnERR   res   = pxtnERR_VOID  ;
	pxtnUnit* unit  = new pxtnUnit( _io_read, _io_write, _io_seek, _io_pos );
	int32_t   group =              0;

	switch( ver )
	{
	case  1: if( !unit->Read_v1x ( desc, &group )                    ) goto term; break;
	case  3: res = unit->Read_v3x( desc, &group ); if( res != pxtnOK ) goto term; break;
	default: res = pxtnERR_fmt_unknown; goto term;
	}

	if( group >= _group_num ) group = _group_num - 1;

	evels->x4x_Read_Add( 0, (uint8_t)_unit_num, EVENTKIND_GROUPNO, (int32_t)group     );
	evels->x4x_Read_NewKind();
	evels->x4x_Read_Add( 0, (uint8_t)_unit_num, EVENTKIND_VOICENO, (int32_t)_unit_num );
	evels->x4x_Read_NewKind();

	res = pxtnOK;
term:
	if( res == pxtnOK ){ _units[ _unit_num ] = unit; _unit_num++; }
	else               { SAFE_DELETE( unit ); }

	return res;
}

/////////////
// assi woice
/////////////

typedef struct
{
	uint16_t  woice_index;
	uint16_t  rrr;
	char name[ pxtnMAX_TUNEWOICENAME ];
}
_ASSIST_WOICE;

#ifdef px_BIG_ENDIAN
px_FORCE_INLINE void swapEndian( _ASSIST_WOICE &assi)
{
	assi.woice_index = pxtnData::_swap16( assi.woice_index );
	assi.rrr =         pxtnData::_swap16( assi.rrr )        ;
}
#endif

bool pxtnService::_io_assiWOIC_w( void* desc, int32_t idx ) const
{
	if( !_b_init ) return false;

	_ASSIST_WOICE assi = {0};
	int32_t       size;
	int32_t       name_size = 0;
	const char*   p_name = _woices[ idx ]->get_name_buf( &name_size );

	if( name_size > pxtnMAX_TUNEWOICENAME ) return false;

	memcpy( assi.name, p_name, name_size );
	assi.woice_index = pxSwapLE16( (uint16_t)idx );

	size = sizeof( _ASSIST_WOICE );
	if( !_io_write_le32( desc, &size                 ) ) return false;
	if( !_io_write( desc, &assi, size,             1 ) ) return false;

	return true;
}

pxtnERR pxtnService::_io_assiWOIC_r( void* desc )
{
	if( !_b_init ) return pxtnERR_INIT;

	_ASSIST_WOICE assi = {0};
	int32_t       size =  0 ;

	if( !_io_read_le32( desc, &size )    ) return pxtnERR_desc_r     ;
	if( size != sizeof(assi)           ) return pxtnERR_fmt_unknown;
	if( !_io_read( desc, &assi, size, 1 )    ) return pxtnERR_desc_r     ;
	swapEndian( assi );

	if( assi.rrr                       ) return pxtnERR_fmt_unknown;
	if( assi.woice_index >= _woice_num ) return pxtnERR_fmt_unknown;

	if( !_woices[ assi.woice_index ]->set_name_buf( assi.name, pxtnMAX_TUNEWOICENAME ) ) return pxtnERR_FATAL;

	return pxtnOK;
}


// -----
// assi unit.
// -----

typedef struct
{
	uint16_t  unit_index;
	uint16_t  rrr;
	char      name[ pxtnMAX_TUNEUNITNAME ];
}
_ASSIST_UNIT;

#ifdef px_BIG_ENDIAN
px_FORCE_INLINE void swapEndian( _ASSIST_UNIT &unit)
{
	unit.unit_index = pxtnData::_swap16( unit.unit_index );
	unit.rrr =        pxtnData::_swap16( unit.rrr )       ;
}
#endif


bool pxtnService::_io_assiUNIT_w( void* desc, int32_t idx ) const
{
	if( !_b_init ) return false;

	_ASSIST_UNIT assi = {0};
	int32_t      size;
	int32_t      name_size;
	const char*  p_name = _units[ idx ]->get_name_buf( &name_size );

	memcpy( assi.name, p_name, name_size );
	assi.unit_index = pxSwapLE16( (uint16_t)idx );

	size = sizeof(assi);
	if( !_io_write_le32( desc, &size ) ) return false;
	if( !_io_write( desc, &assi, size            , 1 ) ) return false;

	return true;
}

pxtnERR pxtnService::_io_assiUNIT_r( void* desc )
{
	if( !_b_init ) return pxtnERR_INIT;

	_ASSIST_UNIT assi = {0};
	int32_t      size;

	if( !_io_read_le32( desc, &size       ) ) return pxtnERR_desc_r     ;
	if( size != sizeof(assi)                ) return pxtnERR_fmt_unknown;
	if( !_io_read( desc, &assi, sizeof(assi), 1 ) ) return pxtnERR_desc_r     ;
	swapEndian( assi );

	if( assi.rrr                            ) return pxtnERR_fmt_unknown;
	if( assi.unit_index >= _unit_num        ) return pxtnERR_fmt_unknown;

	if( !_units[ assi.unit_index ]->set_name_buf( assi.name, pxtnMAX_TUNEUNITNAME ) ) return pxtnERR_FATAL;

	return pxtnOK;
}

// -----
// unit num
// -----
typedef struct
{
	int16_t num;
	int16_t rrr;
}
_NUM_UNIT;

#ifdef px_BIG_ENDIAN
px_FORCE_INLINE void swapEndian( _NUM_UNIT &data)
{
	data.num = pxtnData::_swap16( data.num );
	data.rrr = pxtnData::_swap16( data.rrr );
}
#endif


bool pxtnService::_io_UNIT_num_w( void* desc ) const
{
	if( !_b_init ) return false;

	_NUM_UNIT data;
	int32_t   size;

	memset( &data, 0, sizeof( _NUM_UNIT ) );

	data.num = pxSwapLE16((int16_t)_unit_num);

	size     = sizeof(_NUM_UNIT);

	if( !_io_write_le32( desc, &size                ) ) return false;
	if( !_io_write( desc, &data, size           , 1 ) ) return false;

	return true;
}

pxtnERR pxtnService::_io_UNIT_num_r    ( void* desc, int32_t* p_num )
{
	if( !_b_init ) return pxtnERR_INIT;

	_NUM_UNIT data = {0};
	int32_t   size =  0 ;

	if( !_io_read_le32( desc, &size              ) ) return pxtnERR_desc_r     ;
	if( size != sizeof( _NUM_UNIT )                ) return pxtnERR_fmt_unknown;
	if( !_io_read( desc, &data, sizeof( _NUM_UNIT ), 1 ) ) return pxtnERR_desc_r     ;
	swapEndian( data );

	if( data.rrr                                   ) return pxtnERR_fmt_unknown;
	if( data.num > _unit_max                       ) return pxtnERR_fmt_new    ;
	if( data.num <         0                       ) return pxtnERR_fmt_unknown;
	*p_num = data.num;

	return pxtnOK;
}

////////////////////////////////////////
// save               //////////////////
////////////////////////////////////////

pxtnERR pxtnService::write( void* desc, bool b_tune, uint16_t exe_ver )
{
	if( !_b_init ) return pxtnERR_INIT;

//	bool     b_ret = false;
	int32_t  rough = b_tune ? 10 : 1;
	uint16_t rrr   =            0;
	pxtnERR  res   = pxtnERR_VOID;

	// format version
	if( b_tune ){ if( !_io_write( desc, _code_tune_v5, 1, _VERSIONSIZE ) ){ res = pxtnERR_desc_w; goto End; } }
	else        { if( !_io_write( desc, _code_proj_v5, 1, _VERSIONSIZE ) ){ res = pxtnERR_desc_w; goto End; } }

	// exe version
	if( !_io_write_le16( desc, &exe_ver )                                ){ res = pxtnERR_desc_w; goto End; }
	if( !_io_write_le16( desc, &rrr )                                    ){ res = pxtnERR_desc_w; goto End; }

	// master
	if( !_io_write( desc, _code_MasterV5    , 1, _CODESIZE ) ){ res = pxtnERR_desc_w; goto End; }
	if( !master->io_w_v5( desc, rough )                      ){ res = pxtnERR_desc_w; goto End; }

	// event
	if( !_io_write( desc, _code_Event_V5,     1, _CODESIZE ) ){ res = pxtnERR_desc_w; goto End; }
	if( !evels->io_Write( desc, rough )                      ){ res = pxtnERR_desc_w; goto End; }

	// name
	if( text->is_name_buf() )
	{
		if( !_io_write( desc, _code_textNAME, 1, _CODESIZE ) ){ res = pxtnERR_desc_w; goto End; }
		if( !text->Name_w( desc )                            ){ res = pxtnERR_desc_w; goto End; }
	}

	// comment
	if( text->is_comment_buf() )
	{
		if( !_io_write( desc, _code_textCOMM, 1, _CODESIZE ) ){ res = pxtnERR_desc_w; goto End; }
		if( !text->Comment_w( desc                         ) ){ res = pxtnERR_desc_w; goto End; }
	}

	// delay
	for( int32_t d = 0; d < _delay_num; d++ )
	{
		if( !_io_write( desc, _code_effeDELA, 1, _CODESIZE ) ){ res = pxtnERR_desc_w; goto End; }
		if( !_delays[ d ]->Write( desc )                     ){ res = pxtnERR_desc_w; goto End; }
	}

	// overdrive
	for( int32_t o = 0; o < _ovdrv_num; o++ )
	{
		if( !_io_write( desc, _code_effeOVER, 1, _CODESIZE ) ){ res = pxtnERR_desc_w; goto End; }
		if( !_ovdrvs[ o ]->Write( desc )                     ){ res = pxtnERR_desc_w; goto End; }
	}

	// woice
	for( int32_t w = 0; w < _woice_num; w++ )
	{
		pxtnWoice * p_w = _woices[ w ];

		switch( p_w->get_type() )
		{
		case pxtnWOICE_PCM:
			if( !_io_write( desc, _code_matePCM , 1, _CODESIZE ) ){ res = pxtnERR_desc_w; goto End; }
			if( !p_w->io_matePCM_w ( desc )                      ){ res = pxtnERR_desc_w; goto End; }
			break;
		case pxtnWOICE_PTV:
			if( !_io_write( desc, _code_matePTV , 1, _CODESIZE ) ){ res = pxtnERR_desc_w; goto End; }
			if( !p_w->io_matePTV_w ( desc                      ) ){ res = pxtnERR_desc_w; goto End; }
			break;
		case pxtnWOICE_PTN:
			if( !_io_write( desc, _code_matePTN , 1, _CODESIZE ) ){ res = pxtnERR_desc_w; goto End; }
			if( !p_w->io_matePTN_w ( desc                      ) ){ res = pxtnERR_desc_w; goto End; }
			break;
		case pxtnWOICE_OGGV:

#ifdef pxINCLUDE_OGGVORBIS
			if( !_io_write( desc, _code_mateOGGV, 1, _CODESIZE ) ){ res = pxtnERR_desc_w; goto End; }
			if( !p_w->io_mateOGGV_w( desc                      ) ){ res = pxtnERR_desc_w; goto End; }
#else
			res = pxtnERR_ogg_no_supported; goto End;
#endif
			break;
        default:
            res = pxtnERR_inv_data; goto End;
		}

        if( !b_tune && p_w->is_name_buf() )
		{
			if( !_io_write( desc, _code_assiWOIC, 1, _CODESIZE ) ){ res = pxtnERR_desc_w; goto End; }
			if( !_io_assiWOIC_w( desc, w )                       ){ res = pxtnERR_desc_w; goto End; }
		}
	}

	// unit
	if( !_io_write( desc, _code_num_UNIT, 1, _CODESIZE ) ){ res = pxtnERR_desc_w; goto End; }
	if( !_io_UNIT_num_w( desc )                          ){ res = pxtnERR_desc_w; goto End; }

	for( int32_t u = 0; u < _unit_num; u++ )
	{
		if( !b_tune && _units[ u ]->is_name_buf() )
		{
			if( !_io_write( desc, _code_assiUNIT, 1, _CODESIZE) ){ res = pxtnERR_desc_w; goto End; }
			if( !_io_assiUNIT_w( desc, u )                      ){ res = pxtnERR_desc_w; goto End; }
		}
	}

	{
		int32_t end_size = 0;
		if( !_io_write( desc, _code_pxtoneND, 1, _CODESIZE ) ){ res = pxtnERR_desc_w; goto End; }
		if( !_io_write( desc, &end_size     , 4,         1 ) ){ res = pxtnERR_desc_w; goto End; }
	}

	res = pxtnOK;
End:

	return res;
}

////////////////////////////////////////
// Read Project //////////////
////////////////////////////////////////

pxtnERR pxtnService::_ReadTuneItems( void* desc )
{
	if( !_b_init ) return pxtnERR_INIT;

	pxtnERR res   = pxtnERR_VOID;
	bool    b_end = false;
	char    code[ _CODESIZE + 1 ] = {'\0'};


	/// must the unit before the voice.
	while( !b_end )
	{
		if( !_io_read( desc, code, 1, _CODESIZE ) ){ res = pxtnERR_desc_r; goto term; }

		_enum_Tag tag = _CheckTagCode( code );
		switch( tag )
		{
		case _TAG_antiOPER    : res = pxtnERR_anti_opreation; goto term;

		// new -------
		case _TAG_num_UNIT    :
			{
				int32_t num = 0;
				res = _io_UNIT_num_r( desc, &num ); if( res != pxtnOK ) goto term;
				for( int32_t i = 0; i < num; i++ ) _units[ i ] = new pxtnUnit( _io_read, _io_write, _io_seek, _io_pos );
				_unit_num = num;
			}
			break;

		case _TAG_MasterV5: res = master->io_r_v5( desc                ); if( res != pxtnOK ) goto term; break;
		case _TAG_Event_V5: res = evels->io_Read ( desc                ); if( res != pxtnOK ) goto term; break;

		case _TAG_matePCM : res = _io_Read_Woice ( desc, pxtnWOICE_PCM ); if( res != pxtnOK ) goto term; break;
		case _TAG_matePTV : res = _io_Read_Woice ( desc, pxtnWOICE_PTV ); if( res != pxtnOK ) goto term; break;
		case _TAG_matePTN : res = _io_Read_Woice ( desc, pxtnWOICE_PTN ); if( res != pxtnOK ) goto term; break;

		case _TAG_mateOGGV:

#ifdef pxINCLUDE_OGGVORBIS
			res = _io_Read_Woice( desc, pxtnWOICE_OGGV );
			if( res != pxtnOK ) goto term;
#else
			res = pxtnERR_ogg_no_supported; goto term;
#endif
			break;

		case _TAG_effeDELA    : res = _io_Read_Delay    ( desc ); if( res != pxtnOK ) goto term; break;
		case _TAG_effeOVER    : res = _io_Read_OverDrive( desc ); if( res != pxtnOK ) goto term; break;
		case _TAG_textNAME    : if( !text->Name_r       ( desc ) ){ res = pxtnERR_desc_r; goto term; } break;
		case _TAG_textCOMM    : if( !text->Comment_r    ( desc ) ){ res = pxtnERR_desc_r; goto term; } break;
		case _TAG_assiWOIC    : res = _io_assiWOIC_r    ( desc ); if( res != pxtnOK ) goto term; break;
		case _TAG_assiUNIT    : res = _io_assiUNIT_r    ( desc ); if( res != pxtnOK ) goto term; break;
		case _TAG_pxtoneND    : b_end = true; break;

		// old -------
		case _TAG_x4x_evenMAST: res = master->io_r_x4x  ( desc                ); if( res != pxtnOK ) goto term; break;
		case _TAG_x4x_evenUNIT: res = evels ->io_Unit_Read_x4x_EVENT( desc, false, true   ); if( res != pxtnOK ) goto term; break;
		case _TAG_x3x_pxtnUNIT: res = _io_Read_OldUnit  ( desc, 3             ); if( res != pxtnOK ) goto term; break;
		case _TAG_x1x_PROJ    : if( !_x1x_Project_Read  ( desc              ) ){ res = pxtnERR_desc_r; goto term; } break;
		case _TAG_x1x_UNIT    : res = _io_Read_OldUnit  ( desc, 1             ); if( res != pxtnOK ) goto term; break;
		case _TAG_x1x_PCM     : res = _io_Read_Woice    ( desc, pxtnWOICE_PCM ); if( res != pxtnOK ) goto term; break;
		case _TAG_x1x_EVEN    : res = evels ->io_Unit_Read_x4x_EVENT( desc, true, false   ); if( res != pxtnOK ) goto term; break;
		case _TAG_x1x_END     : b_end = true; break;

		default: res = pxtnERR_fmt_unknown; goto term;
		}
	}

	res = pxtnOK;
term:

	return res;

}



#define _MAX_FMTVER_x1x_EVENTNUM 10000

pxtnERR pxtnService::_ReadVersion( void* desc, _enum_FMTVER *p_fmt_ver, uint16_t *p_exe_ver )
{
	if( !_b_init    ) return pxtnERR_INIT  ;

	char     version[ _VERSIONSIZE  ] = {'\0'};
	uint16_t dummy;

	if( !_io_read( desc, version, 1, _VERSIONSIZE ) ) return pxtnERR_desc_r;

	// fmt version
	if(      !memcmp( version, _code_proj_x1x , _VERSIONSIZE ) ){ *p_fmt_ver = _enum_FMTVER_x1x; *p_exe_ver = 0; return pxtnOK; }
	else if( !memcmp( version, _code_proj_x2x , _VERSIONSIZE ) ){ *p_fmt_ver = _enum_FMTVER_x2x; *p_exe_ver = 0; return pxtnOK; }
	else if( !memcmp( version, _code_proj_x3x , _VERSIONSIZE ) ){ *p_fmt_ver = _enum_FMTVER_x3x;                                }
	else if( !memcmp( version, _code_proj_x4x , _VERSIONSIZE ) ){ *p_fmt_ver = _enum_FMTVER_x4x;							    }
	else if( !memcmp( version, _code_proj_v5  , _VERSIONSIZE ) ){ *p_fmt_ver = _enum_FMTVER_v5 ;							    }
	else if( !memcmp( version, _code_tune_x2x , _VERSIONSIZE ) ){ *p_fmt_ver = _enum_FMTVER_x2x; *p_exe_ver = 0; return pxtnOK; }
	else if( !memcmp( version, _code_tune_x3x , _VERSIONSIZE ) ){ *p_fmt_ver = _enum_FMTVER_x3x;							    }
	else if( !memcmp( version, _code_tune_x4x , _VERSIONSIZE ) ){ *p_fmt_ver = _enum_FMTVER_x4x;                                }
	else if( !memcmp( version, _code_tune_v5  , _VERSIONSIZE ) ){ *p_fmt_ver = _enum_FMTVER_v5 ;                                }
	else return pxtnERR_fmt_unknown;

	// exe version
	if( !_io_read_le16( desc, p_exe_ver ) ) return pxtnERR_desc_r;
	if( !_io_read_le16( desc, &dummy    ) ) return pxtnERR_desc_r;

	return pxtnOK;
}

// fix old key event
bool pxtnService::_x3x_TuningKeyEvent()
{
	if( !_b_init ) return false;

	if( _unit_num > _woice_num ) return false;

	for( int32_t u = 0; u < _unit_num; u++ )
	{
		if( u >= _woice_num ) return false;

		int32_t change_value = _woices[ u ]->get_x3x_basic_key() - EVENTDEFAULT_BASICKEY;

		if( !evels->get_Count( (uint8_t)u, (uint8_t)EVENTKIND_KEY ) )
		{
			evels->Record_Add_i( 0, (uint8_t)u, EVENTKIND_KEY, (int32_t)0x6000 );
		}
		evels->Record_Value_Change( 0, -1, (uint8_t)u, EVENTKIND_KEY, change_value );
	}
	return true;
}

// fix old tuning (1.0)
bool pxtnService::_x3x_AddTuningEvent()
{
	if( !_b_init ) return false;

	if( _unit_num > _woice_num ) return false;

	for( int32_t u = 0; u < _unit_num; u++ )
	{
		float tuning = _woices[ u ]->get_x3x_tuning();
		if( tuning ) evels->Record_Add_f( 0, (uint8_t)u, EVENTKIND_TUNING, tuning );
	}

	return true;
}

bool pxtnService::_x3x_SetVoiceNames()
{
	if( !_b_init ) return false;

	for( int32_t i = 0; i < _woice_num; i++ )
	{
		char name[ pxtnMAX_TUNEWOICENAME + 1 ];
		sprintf( name, "voice_%02d", (int)i );
		_woices[ i ]->set_name_buf( name, 8 );
	}
	return true;
}

pxtnERR pxtnService::_pre_count_event( void* desc, int32_t* p_count )
{
	if( !_b_init ) return pxtnERR_INIT ;
	if( !p_count ) return pxtnERR_param;

	pxtnERR      res   = pxtnERR_VOID;
	bool         b_end = false;

	int32_t      count = 0;
	int32_t      c     = 0;
	int32_t      size  = 0;
	char         code[ _CODESIZE + 1 ] = {'\0'};

	uint16_t     exe_ver = 0;
	_enum_FMTVER fmt_ver = _enum_FMTVER_unknown;

	res = _ReadVersion( desc, &fmt_ver, &exe_ver ); if( res != pxtnOK ) goto term;

	if( fmt_ver == _enum_FMTVER_x1x )
	{
		count = _MAX_FMTVER_x1x_EVENTNUM;
		res = pxtnOK;
		goto term;
	}

	while( !b_end )
	{
		if( !_io_read( desc, code, 1, _CODESIZE ) ){ res = pxtnERR_desc_r; goto term; }

		switch( _CheckTagCode( code ) )
		{
		case _TAG_Event_V5    : count += evels ->io_Read_EventNum ( desc ); break;
		case _TAG_MasterV5    : count += master->io_r_v5_EventNum ( desc ); break;
		case _TAG_x4x_evenMAST: count += master->io_r_x4x_EventNum( desc ); break;
		case _TAG_x4x_evenUNIT: res = evels ->io_Read_x4x_EventNum( desc, &c ); if( res != pxtnOK ) goto term; count += c; break;
		case _TAG_pxtoneND    : b_end = true; break;

		// skip
		case _TAG_antiOPER    :
		case _TAG_num_UNIT    :
		case _TAG_x3x_pxtnUNIT:
		case _TAG_matePCM     :
		case _TAG_matePTV     :
		case _TAG_matePTN     :
		case _TAG_mateOGGV    :
		case _TAG_effeDELA    :
		case _TAG_effeOVER    :
		case _TAG_textNAME    :
		case _TAG_textCOMM    :
		case _TAG_assiUNIT    :
		case _TAG_assiWOIC    :

			if( !_io_read_le32( desc, &size ) ){ res = pxtnERR_desc_r; goto term; }
			if( !_io_seek( desc, SEEK_CUR, size )            ){ res = pxtnERR_desc_r; goto term; }
			break;

		// ignore
		case _TAG_x1x_PROJ    :
		case _TAG_x1x_UNIT    :
		case _TAG_x1x_PCM     :
		case _TAG_x1x_EVEN    :
		case _TAG_x1x_END     : res = pxtnERR_x1x_ignore; goto term;
		default               : res = pxtnERR_FATAL     ; goto term;
		}
	}

	if( fmt_ver <= _enum_FMTVER_x3x ) count += pxtnMAX_TUNEUNITSTRUCT * 4; // voice_no, group_no, key tuning, key event x3x

	res = pxtnOK;
term:

	if( res != pxtnOK ) *p_count =     0;
	else                *p_count = count;

	return res;
}


pxtnERR pxtnService::read( void* desc )
{
	if( !_b_init ) return pxtnERR_INIT;

	pxtnERR      res       = pxtnERR_VOID;
	uint16_t     exe_ver   =            0;
	_enum_FMTVER fmt_ver   = _enum_FMTVER_unknown;
	int32_t      event_num =            0;

	clear();

	res = _pre_count_event( desc, &event_num );
	if( res != pxtnOK ) goto term;
	_io_seek( desc, SEEK_SET, 0 );

	if( _b_fix_evels_num )
	{
		if( event_num > evels->get_Num_Max() ){ res = pxtnERR_too_much_event; goto term; }
	}
	else
	{
		if( !evels->Allocate( event_num )    ){ res = pxtnERR_memory        ; goto term; }
	}

	res = _ReadVersion( desc, &fmt_ver, &exe_ver ); if( res != pxtnOK ) goto term;

	if( fmt_ver >= _enum_FMTVER_v5 ) evels->Linear_Start  ();
	else                             evels->x4x_Read_Start();

	res = _ReadTuneItems( desc ); if( res != pxtnOK ) goto term;

	if( fmt_ver >= _enum_FMTVER_v5  ) evels->Linear_End( true );

	if( fmt_ver <= _enum_FMTVER_x3x )
	{
		if( !_x3x_TuningKeyEvent() ){ res = pxtnERR_x3x_key       ; goto term; }
		if( !_x3x_AddTuningEvent() ){ res = pxtnERR_x3x_add_tuning; goto term; }
		_x3x_SetVoiceNames();
	}

	if( _b_edit && master->get_beat_clock() != EVENTDEFAULT_BEATCLOCK ){ res = pxtnERR_deny_beatclock; goto term; }

	{
		int32_t clock1 = evels ->get_Max_Clock ();
		int32_t clock2 = master->get_last_clock();

		if( clock1 > clock2 ) master->AdjustMeasNum( clock1 );
		else                  master->AdjustMeasNum( clock2 );
	}

	_moo_b_valid_data = true;
	res = pxtnOK;
term:

	if( res != pxtnOK ) clear();

	return res;
}


// x1x project..------------------


#define _MAX_PROJECTNAME_x1x 16

// project (36byte) ================
typedef struct
{
	char  x1x_name[_MAX_PROJECTNAME_x1x];

	float x1x_beat_tempo;
	uint16_t   x1x_beat_clock;
	uint16_t   x1x_beat_num;
	uint16_t   x1x_beat_note;
	uint16_t   x1x_meas_num;
	uint16_t   x1x_channel_num;
	uint16_t   x1x_bps;
	uint32_t   x1x_sps;
}
_x1x_PROJECT;

#ifdef px_BIG_ENDIAN
px_FORCE_INLINE void swapEndian( _x1x_PROJECT &prjc)
{
	prjc.x1x_beat_tempo =  pxtnData::_swap_float( prjc.x1x_beat_tempo );
	prjc.x1x_beat_clock =  pxtnData::_swap16( prjc.x1x_beat_clock )    ;
	prjc.x1x_beat_num =    pxtnData::_swap16( prjc.x1x_beat_num )      ;
	prjc.x1x_beat_note =   pxtnData::_swap16( prjc.x1x_beat_note )     ;
	prjc.x1x_meas_num =    pxtnData::_swap16( prjc.x1x_meas_num )      ;
	prjc.x1x_channel_num = pxtnData::_swap16( prjc.x1x_channel_num )   ;
	prjc.x1x_bps =         pxtnData::_swap16( prjc.x1x_bps )           ;
	prjc.x1x_sps =         pxtnData::_swap16( prjc.x1x_sps )           ;
}
#endif

bool pxtnService::_x1x_Project_Read( void* desc )
{
	if( !_b_init ) return false;

	_x1x_PROJECT prjc = {0};
	int32_t  beat_num, beat_clock;
	int32_t  size;
	float    beat_tempo;

	if( !_io_read_le32( desc, &size                       ) ) return false;
	if( !_io_read( desc, &prjc, sizeof( _x1x_PROJECT ), 1 ) ) return false;
	swapEndian( prjc );

	beat_num   = prjc.x1x_beat_num  ;
	beat_tempo = prjc.x1x_beat_tempo;
	beat_clock = prjc.x1x_beat_clock;

	int32_t  ns = 0;
	for( ; ns <  _MAX_PROJECTNAME_x1x; ns++ ){ if( !prjc.x1x_name[ ns ] ) break; }

	text->set_name_buf( prjc.x1x_name, ns );
	master->Set( beat_num, beat_tempo, beat_clock );

	return true;
}
