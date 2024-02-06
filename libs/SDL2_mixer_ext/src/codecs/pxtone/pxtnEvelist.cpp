
#include "./pxtnEvelist.h"

void pxtnEvelist::Release()
{
	if( _eves ) free( _eves );
	_eves              = NULL;
	_start             = NULL;
	_eve_allocated_num =    0;
}

pxtnEvelist::pxtnEvelist( pxtnIO_r io_read, pxtnIO_w io_write, pxtnIO_seek io_seek, pxtnIO_pos io_pos )
{
	_set_io_funcs( io_read, io_write, io_seek, io_pos );

	_eves              = NULL;
	_start             = NULL;
	_eve_allocated_num =    0;
	_linear            =    0;
	_p_x4x_rec         =    0;
}

pxtnEvelist::~pxtnEvelist()
{
	pxtnEvelist::Release();
}

void pxtnEvelist::Clear()
{
	if( _eves ) memset( _eves, 0, sizeof(EVERECORD) * _eve_allocated_num );
	_start   = NULL;
}


bool pxtnEvelist::Allocate( int32_t max_event_num )
{
	pxtnEvelist::Release();
	if( !(  _eves = (EVERECORD*)malloc( sizeof(EVERECORD) * max_event_num ) ) ) return false;
	memset( _eves, 0,                   sizeof(EVERECORD) * max_event_num );
	_eve_allocated_num = max_event_num;
	return true;
}

int32_t  pxtnEvelist::get_Num_Max() const
{
	if( !_eves ) return 0;
	return _eve_allocated_num;
}

int32_t  pxtnEvelist::get_Max_Clock() const
{
	int32_t max_clock = 0;
	int32_t clock;

	for( EVERECORD* p = _start; p; p = p->next )
	{
		if( Evelist_Kind_IsTail( p->kind ) ) clock = p->clock + p->value;
		else                                 clock = p->clock           ;
		if( clock > max_clock ) max_clock = clock;
	}

	return max_clock;

}

int32_t  pxtnEvelist::get_Count() const
{
	if( !_eves || !_start ) return 0;

	int32_t    count = 0;
	for( EVERECORD* p = _start; p; p = p->next ) count++;
	return count;
}

int32_t  pxtnEvelist::get_Count( uint8_t kind, int32_t value ) const
{
	if( !_eves ) return 0;

	int32_t count = 0;
	for( EVERECORD* p = _start; p; p = p->next ){ if( p->kind == kind && p->value == value ) count++; }
	return count;
}

int32_t  pxtnEvelist::get_Count( uint8_t unit_no ) const
{
	if( !_eves ) return 0;

	int32_t count = 0;
	for( EVERECORD* p = _start; p; p = p->next ){ if( p->unit_no == unit_no ) count++; }
	return count;
}

int32_t  pxtnEvelist::get_Count( uint8_t unit_no, uint8_t kind ) const
{
	if( !_eves ) return 0;

	int32_t count = 0;
	for( EVERECORD* p = _start; p; p = p->next ){ if( p->unit_no == unit_no && p->kind == kind ) count++; }
	return count;
}

int32_t  pxtnEvelist::get_Count( int32_t clock1, int32_t clock2, uint8_t unit_no ) const
{
	if( !_eves ) return 0;

	EVERECORD* p;
	for( p = _start; p; p = p->next )
	{
		if( p->unit_no == unit_no )
		{
			if(                                   p->clock            >= clock1 ) break;
			if( Evelist_Kind_IsTail( p->kind ) && p->clock + p->value >  clock1 ) break;
		}
	}

	int32_t count = 0;
	for(           ; p; p = p->next )
	{
		if( p->clock != clock1 && p->clock >= clock2 ) break;
		if( p->unit_no == unit_no ) count++;
	}
	return count;
}


static int32_t _DefaultKindValue( uint8_t kind )
{
	switch( kind )
	{
	case EVENTKIND_KEY       : return EVENTDEFAULT_KEY      ;
	case EVENTKIND_PAN_VOLUME: return EVENTDEFAULT_PAN_VOLUME  ;
	case EVENTKIND_VELOCITY  : return EVENTDEFAULT_VELOCITY ;
	case EVENTKIND_VOLUME    : return EVENTDEFAULT_VOLUME   ;
	case EVENTKIND_PORTAMENT : return EVENTDEFAULT_PORTAMENT;
	case EVENTKIND_BEATCLOCK : return EVENTDEFAULT_BEATCLOCK;
	case EVENTKIND_BEATTEMPO : return EVENTDEFAULT_BEATTEMPO;
	case EVENTKIND_BEATNUM   : return EVENTDEFAULT_BEATNUM  ;
	case EVENTKIND_VOICENO   : return EVENTDEFAULT_VOICENO  ;
	case EVENTKIND_GROUPNO   : return EVENTDEFAULT_GROUPNO  ;
	case EVENTKIND_TUNING    :
		return pxtnData::cast_to_int( EVENTDEFAULT_TUNING ); //*( (int32_t*)&tuning );
	case EVENTKIND_PAN_TIME  : return EVENTDEFAULT_PAN_TIME ;
	}
	return 0;
}

int32_t pxtnEvelist::get_Value( int32_t clock, uint8_t unit_no, uint8_t kind ) const
{
	if( !_eves ) return 0;

	EVERECORD* p;
	int32_t val = _DefaultKindValue( kind );

	for( p = _start; p; p = p->next )
	{
		if( p->clock > clock ) break;
		if( p->unit_no == unit_no && p->kind == kind ) val = p->value;
	}

	return val;
}

const EVERECORD* pxtnEvelist::get_Records() const
{
	if( !_eves ) return NULL;
	return _start;
}


void pxtnEvelist::_rec_set( EVERECORD* p_rec, EVERECORD* prev, EVERECORD* next, int32_t clock, uint8_t unit_no, uint8_t kind, int32_t value )
{
	if( prev ) prev->next = p_rec;
	else       _start     = p_rec;
	if( next ) next->prev = p_rec;

	p_rec->next    = next   ;
	p_rec->prev    = prev   ;
	p_rec->clock   = clock  ;
	p_rec->kind    = kind   ;
	p_rec->unit_no = unit_no;
	p_rec->value   = value  ;
}

static int32_t _ComparePriority( uint8_t kind1, uint8_t kind2 )
{
	static const int32_t priority_table[ EVENTKIND_NUM ] =
	{
		  0, // EVENTKIND_NULL  = 0
		 50, // EVENTKIND_ON
		 40, // EVENTKIND_KEY
		 60, // EVENTKIND_PAN_VOLUME
		 70, // EVENTKIND_VELOCITY
		 80, // EVENTKIND_VOLUME
		 30, // EVENTKIND_PORTAMENT
		  0, // EVENTKIND_BEATCLOCK
		  0, // EVENTKIND_BEATTEMPO
		  0, // EVENTKIND_BEATNUM
		  0, // EVENTKIND_REPEAT
		255, // EVENTKIND_LAST
		 10, // EVENTKIND_VOICENO
		 20, // EVENTKIND_GROUPNO
		 90, // EVENTKIND_TUNING
		100, // EVENTKIND_PAN_TIME
	};

	return priority_table[ kind1 ] - priority_table[ kind2 ];
}

void pxtnEvelist::_rec_cut( EVERECORD* p_rec )
{
	if( p_rec->prev ) p_rec->prev->next = p_rec->next;
	else              _start            = p_rec->next;
	if( p_rec->next ) p_rec->next->prev = p_rec->prev;
	p_rec->kind = EVENTKIND_NULL;
}

bool pxtnEvelist::Record_Add_f( int32_t clock, uint8_t unit_no, uint8_t kind, float value_f )
{
	int32_t value = cast_to_int( value_f );
	return Record_Add_i( clock, unit_no, kind, value );
}

bool pxtnEvelist::Record_Add_i( int32_t clock, uint8_t unit_no, uint8_t kind, int32_t value )
{
	if( !_eves ) return false;

	EVERECORD* p_new  = NULL;
	EVERECORD* p_prev = NULL;
	EVERECORD* p_next = NULL;

	// 空き検索
	for( int32_t r = 0; r < _eve_allocated_num; r++ )
	{
		if( _eves[ r ].kind == EVENTKIND_NULL ){ p_new = &_eves[ r ]; break; }
	}
	if( !p_new ) return false;

	// first.
	if( !_start )
	{
	}
	// top.
	else if( clock < _start->clock )
	{
		p_next = _start;
	}
	else
	{

		for( EVERECORD* p = _start; p; p = p->next )
		{
			if( p->clock == clock ) // 同時
			{
				for( ; true; p = p->next )
				{
					if( p->clock != clock                        ){ p_prev = p->prev; p_next = p; break; }
					if( unit_no == p->unit_no && kind == p->kind ){ p_prev = p->prev; p_next = p->next; p->kind = EVENTKIND_NULL; break; } // 置き換え
					if( _ComparePriority( kind, p->kind ) < 0    ){ p_prev = p->prev; p_next = p; break; }// プライオリティを検査
					if( !p->next                                 ){ p_prev = p; break; }// 末端
				}
				break;
			}
			else if( p->clock > clock ){ p_prev = p->prev; p_next = p      ; break; } // 追い越した
			else if( !p->next         ){ p_prev = p; break; }// 末端
		}
	}

	_rec_set( p_new, p_prev, p_next, clock, unit_no, kind, value );

	// cut prev tail
	if( Evelist_Kind_IsTail( kind ) )
	{
		for( EVERECORD* p = p_new->prev; p; p = p->prev )
		{
			if( p->unit_no == unit_no && p->kind == kind )
			{
				if( clock < p->clock + p->value ) p->value = clock - p->clock;
				break;
			}
		}
	}

	// delete next
	if( Evelist_Kind_IsTail( kind ) )
	{
		for( EVERECORD* p = p_new->next; p && p->clock < clock + value; p = p->next )
		{
			if( p->unit_no == unit_no && p->kind == kind )
			{
				_rec_cut( p );
			}
		}
	}

	return true;
}

int32_t pxtnEvelist::Record_Delete( int32_t clock1, int32_t clock2, uint8_t unit_no, uint8_t kind )
{
	if( !_eves  ) return 0;

	int32_t count = 0;

	for( EVERECORD* p = _start; p; p = p->next )
	{
		if( p->clock != clock1 && p->clock >= clock2 ) break;
		if( p->clock >= clock1 && p->unit_no == unit_no && p->kind == kind ){ _rec_cut( p ); count++; }
	}

	if( Evelist_Kind_IsTail( kind ) )
	{
		for( EVERECORD* p = _start; p; p = p->next )
		{
			if( p->clock >= clock1 ) break;
			if( p->unit_no == unit_no && p->kind == kind && p->clock + p->value > clock1 )
			{
				p->value = clock1 - p->clock;
				count++;
			}
		}
	}

	return count;
}

int32_t pxtnEvelist::Record_Delete( int32_t clock1, int32_t clock2, uint8_t unit_no )
{
	if( !_eves  ) return 0;

	int32_t count = 0;

	for( EVERECORD* p = _start; p; p = p->next )
	{
		if( p->clock != clock1 && p->clock >= clock2 ) break;
		if( p->clock >= clock1 && p->unit_no == unit_no ){ _rec_cut( p ); count++; }
	}

	for( EVERECORD* p = _start; p; p = p->next )
	{
		if( p->clock >= clock1 ) break;
		if( p->unit_no == unit_no && Evelist_Kind_IsTail( p->kind ) && p->clock + p->value > clock1 )
		{
			p->value = clock1 - p->clock;
			count++;
		}
	}

	return count;
}


int32_t pxtnEvelist::Record_UnitNo_Miss( uint8_t unit_no )
{
	if( !_eves  ) return 0;

	int32_t count = 0;

	for( EVERECORD* p = _start; p; p = p->next )
	{
		if(      p->unit_no == unit_no ){ _rec_cut( p ); count++; }
		else if( p->unit_no >  unit_no ){ p->unit_no--;    count++; }
	}
	return count;
}

int32_t pxtnEvelist::Record_UnitNo_Set( uint8_t unit_no )
{
	if( !_eves  ) return 0;

	int32_t count = 0;
	for( EVERECORD* p = _start; p; p = p->next ){ p->unit_no = unit_no; count++; }
	return count;
}

int32_t pxtnEvelist::Record_UnitNo_Replace( uint8_t old_u, uint8_t new_u )
{
	if( !_eves  ) return 0;

	int32_t count = 0;

	if( old_u == new_u ) return 0;
	if( old_u <  new_u )
	{
		for( EVERECORD* p = _start; p; p = p->next )
		{
			if(      p->unit_no == old_u                        ){ p->unit_no = new_u; count++; }
			else if( p->unit_no >  old_u && p->unit_no <= new_u ){ p->unit_no--;       count++; }
		}
	}
	else
	{
		for( EVERECORD* p = _start; p; p = p->next )
		{
			if(      p->unit_no == old_u                        ){ p->unit_no = new_u; count++; }
			else if( p->unit_no <  old_u && p->unit_no >= new_u ){ p->unit_no++;       count++; }
		}
	}

	return count;
}


int32_t pxtnEvelist::Record_Value_Set( int32_t clock1, int32_t clock2, uint8_t unit_no, uint8_t kind, int32_t value )
{
	if( !_eves  ) return 0;

	int32_t count = 0;

	for( EVERECORD* p = _start; p; p = p->next )
	{
		if( p->unit_no == unit_no && p->kind == kind && p->clock >= clock1 && p->clock < clock2 )
		{
			p->value = value;
			count++;
		}
	}

	return count;
}

int32_t  pxtnEvelist::BeatClockOperation( int32_t rate )
{
	if( !_eves  ) return 0;

	int32_t count = 0;

	for( EVERECORD* p = _start; p; p = p->next )
	{
		p->clock *= rate;
		if( Evelist_Kind_IsTail( p->kind ) ) p->value *= rate;
		count++;
	}

	return count;
}


int32_t pxtnEvelist::Record_Value_Change( int32_t clock1, int32_t clock2, uint8_t unit_no, uint8_t kind, int32_t value )
{
	if( !_eves  ) return 0;

	int32_t count = 0;

	int32_t max, min;

	switch( kind )
	{
	case EVENTKIND_NULL      : max =      0; min =   0; break;
	case EVENTKIND_ON        : max =    120; min = 120; break;
	case EVENTKIND_KEY       : max = 0xbfff; min =   0; break;
	case EVENTKIND_PAN_VOLUME: max =   0x80; min =   0; break;
	case EVENTKIND_PAN_TIME  : max =   0x80; min =   0; break;
	case EVENTKIND_VELOCITY  : max =   0x80; min =   0; break;
	case EVENTKIND_VOLUME    : max =   0x80; min =   0; break;
	default: max = 0; min = 0;
	}

	for( EVERECORD* p = _start; p; p = p->next )
	{
		if( p->unit_no == unit_no && p->kind == kind && p->clock >= clock1 )
		{
			if( clock2 == -1 || p->clock < clock2 )
			{
				p->value += value;
				if( p->value < min ) p->value = min;
				if( p->value > max ) p->value = max;
				count++;
			}
		}
	}

	return count;
}

int32_t pxtnEvelist::Record_Value_Omit( uint8_t kind, int32_t value )
{
	if( !_eves  ) return 0;

	int32_t count = 0;

	for( EVERECORD* p = _start; p; p = p->next )
	{
		if( p->kind == kind )
		{
			if(      p->value == value ){ _rec_cut( p ); count++; }
			else if( p->value >  value ){ p->value--;      count++; }
		}
	}
	return count;
}


int32_t pxtnEvelist::Record_Value_Replace( uint8_t kind, int32_t old_value, int32_t new_value )
{
	if( !_eves  ) return 0;

	int32_t count = 0;

	if( old_value == new_value ) return 0;
	if( old_value <  new_value )
	{
		for( EVERECORD* p = _start; p; p = p->next )
		{
			if( p->kind == kind )
			{
				if(      p->value == old_value                          ){ p->value = new_value; count++; }
				else if( p->value >  old_value && p->value <= new_value ){ p->value--;           count++; }
			}
		}
	}
	else
	{
		for( EVERECORD* p = _start; p; p = p->next )
		{
			if( p->kind == kind )
			{
				if(      p->value == old_value                          ){ p->value = new_value; count++; }
				else if( p->value <  old_value && p->value >= new_value ){ p->value++;           count++; }
			}
		}
	}

	return count;
}


int32_t pxtnEvelist::Record_Clock_Shift( int32_t clock, int32_t shift, uint8_t unit_no )
{
	if( !_eves  ) return 0;
	if( !_start ) return 0;
	if( !shift  ) return 0;

	int32_t          count = 0;
	int32_t          c;
	uint8_t           k;
	int32_t          v;
	EVERECORD*   p_next;
	EVERECORD*   p_prev;
	EVERECORD*   p = _start;


	if( shift < 0 )
	{
		for( ; p; p = p->next ){ if( p->clock >= clock ) break; }
		while( p )
		{
			if( p->unit_no == unit_no )
			{
				c      = p->clock + shift;
				k      = p->kind         ;
				v      = p->value        ;
				p_next = p->next;

				_rec_cut( p );
				if( c >= 0 ) Record_Add_i( c, unit_no, k, v );
				count++;

				p = p_next;
			}
			else
			{
				p = p->next;
			}
		}
	}
	else if( shift > 0 )
	{
		while( p->next ) p = p->next;
		while( p )
		{
			if( p->clock < clock ) break;

			if( p->unit_no == unit_no )
			{
				c      = p->clock + shift;
				k      = p->kind         ;
				v      = p->value        ;
				p_prev = p->prev;

				_rec_cut( p );
				Record_Add_i( c, unit_no, k, v );
				count++;

				p = p_prev;
			}
			else
			{
				p = p->prev;
			}
		}
	}
	return count;
}

/////////////////////
// linear
/////////////////////

bool pxtnEvelist::Linear_Start()
{
	if( !_eves ) return false;
	Clear(); _linear = 0;
	return true;
}


void pxtnEvelist::Linear_Add_i(  int32_t clock, uint8_t unit_no, uint8_t kind, int32_t value )
{
	EVERECORD* p = &_eves[ _linear ];

	p->clock      = clock  ;
	p->unit_no    = unit_no;
	p->kind       = kind   ;
	p->value      = value  ;

	_linear++;
}

void pxtnEvelist::Linear_Add_f( int32_t clock, uint8_t unit_no, uint8_t kind, float value_f )
{
	int32_t value = cast_to_int( value_f );
	Linear_Add_i( clock, unit_no, kind, value );
}

void pxtnEvelist::Linear_End( bool b_connect )
{
	if( _eves[ 0 ].kind != EVENTKIND_NULL ) _start = &_eves[ 0 ];

	if( b_connect )
	{
		for( int32_t r = 1; r < _eve_allocated_num; r++ )
		{
			if( _eves[ r ].kind == EVENTKIND_NULL ) break;
			_eves[ r     ].prev = &_eves[ r - 1 ];
			_eves[ r - 1 ].next = &_eves[ r     ];
		}
	}
}




bool pxtnEvelist::x4x_Read_Start()
{
	if( !_eves ) return false;
	Clear();
	_linear    =    0;
	_p_x4x_rec = NULL;
	return true;
}

void pxtnEvelist::x4x_Read_NewKind()
{
	_p_x4x_rec = NULL;
}

void pxtnEvelist::x4x_Read_Add( int32_t clock, uint8_t unit_no, uint8_t kind, int32_t value )
{
	EVERECORD* p_new  = NULL;
	EVERECORD* p_prev = NULL;
	EVERECORD* p_next = NULL;

	p_new = &_eves[ _linear++ ];

	// first.
	if( !_start )
	{
	}
	// top
	else if( clock < _start->clock )
	{
		p_next = _start;
	}
	else
	{
		EVERECORD* p;

		if( _p_x4x_rec ) p = _p_x4x_rec;
		else             p = _start    ;

		for( ; p; p = p->next )
		{
			if( p->clock == clock ) // 同時
			{
				for( ; true; p = p->next )
				{
					if( p->clock != clock                        ){ p_prev = p->prev; p_next = p; break; }
					if( unit_no == p->unit_no && kind == p->kind ){ p_prev = p->prev; p_next = p->next; p->kind = EVENTKIND_NULL; break; } // 置き換え
					if( _ComparePriority( kind, p->kind ) < 0    ){ p_prev = p->prev; p_next = p; break; }// プライオリティを検査
					if( !p->next                                 ){ p_prev = p; break; }// 末端
				}
				break;
			}
			else if( p->clock > clock ){ p_prev = p->prev; p_next = p; break; } // 追い越した
			else if( !p->next         ){ p_prev = p; break; }// 末端
		}
	}
	_rec_set( p_new, p_prev, p_next, clock, unit_no, kind, value );

	_p_x4x_rec = p_new;
}



// ------------
// io
// ------------


bool pxtnEvelist::io_Write( void* desc, int32_t rough ) const
{
	int32_t eve_num        = get_Count();
	int32_t ralatived_size = 0;
	int32_t absolute       = 0;
	int32_t clock;
	int32_t value;

	for( const EVERECORD* p = get_Records(); p; p = p->next )
	{
		clock    = p->clock - absolute;

		ralatived_size += _data_check_v_size( p->clock );
		ralatived_size += 1;
		ralatived_size += 1;
		ralatived_size += _data_check_v_size( p->value );

		absolute = p->clock;
	}

	int32_t size = sizeof(int32_t) + ralatived_size;
	if( !_io_write_le32( desc, &size    ) ) return false;
	if( !_io_write_le32( desc, &eve_num ) ) return false;

	absolute = 0;

	for( const EVERECORD* p = get_Records(); p; p = p->next )
	{
		clock    = p->clock - absolute;

		if( Evelist_Kind_IsTail( p->kind ) ) value = p->value / rough;
		else                                 value = p->value        ;

		if( !_data_w_v( desc, clock / rough, NULL ) ) return false;
		if( !_io_write( desc, &p->unit_no  , sizeof(uint8_t), 1 ) ) return false;
		if( !_io_write( desc, &p->kind     , sizeof(uint8_t), 1 ) ) return false;
		if( !_data_w_v( desc, value        , NULL ) ) return false;

		absolute = p->clock;
	}

	return true;
}

pxtnERR pxtnEvelist::io_Read( void* desc )
{
	int32_t size     = 0;
	int32_t eve_num  = 0;

	if( !_io_read_le32( desc, &size    ) ) return pxtnERR_desc_r;
	if( !_io_read_le32( desc, &eve_num ) ) return pxtnERR_desc_r;

	int32_t clock    = 0;
	int32_t absolute = 0;
	uint8_t unit_no  = 0;
	uint8_t kind     = 0;
	int32_t value    = 0;

	for( int32_t e = 0; e < eve_num; e++ )
	{
		if( !_data_r_v( desc,&clock          ) ) return pxtnERR_desc_r;
		if( !_io_read ( desc, &unit_no, 1, 1 ) ) return pxtnERR_desc_r;
		if( !_io_read ( desc, &kind   , 1, 1 ) ) return pxtnERR_desc_r;
		if( !_data_r_v( desc,&value          ) ) return pxtnERR_desc_r;
		absolute += clock;
		clock     = absolute;
		Linear_Add_i( clock, unit_no, kind, value );
	}

	return pxtnOK;
}

int32_t pxtnEvelist::io_Read_EventNum( void* desc ) const
{
	int32_t size    = 0;
	int32_t eve_num = 0;

	if( !_io_read_le32( desc, &size    ) ) return 0;
	if( !_io_read_le32( desc, &eve_num ) ) return 0;

	int32_t count   = 0;
	int32_t clock   = 0;
	uint8_t unit_no = 0;
	uint8_t kind    = 0;
	int32_t value   = 0;

	for( int32_t e = 0; e < eve_num; e++ )
	{
		if( !_data_r_v( desc,&clock          ) ) return 0;
		if( !_io_read ( desc, &unit_no, 1, 1 ) ) return 0;
		if( !_io_read ( desc, &kind   , 1, 1 ) ) return 0;
		if( !_data_r_v( desc,&value          ) ) return 0;
		count++;
	}
	if( count != eve_num ) return 0;

	return eve_num;
}


// event struct(12byte) =================
typedef struct
{
	uint16_t unit_index;
	uint16_t event_kind;
	uint16_t data_num;        // １イベントのデータ数。現在は 2 ( clock / volume ）
	uint16_t rrr;
	uint32_t  event_num;
}
_x4x_EVENTSTRUCT;

#ifdef px_BIG_ENDIAN
px_FORCE_INLINE void swapEndian( _x4x_EVENTSTRUCT &evnt)
{
	evnt.unit_index = pxtnData::_swap16( evnt.unit_index );
	evnt.event_kind = pxtnData::_swap16( evnt.event_kind );
	evnt.data_num =   pxtnData::_swap16( evnt.data_num )  ;
	evnt.rrr =        pxtnData::_swap16( evnt.rrr )       ;
	evnt.event_num =  pxtnData::_swap32( evnt.event_num ) ;
}
#endif

// write event.
pxtnERR pxtnEvelist::io_Unit_Read_x4x_EVENT( void* desc, bool bTailAbsolute, bool bCheckRRR )
{
	_x4x_EVENTSTRUCT evnt     ={0};
	int32_t          clock    = 0;
	int32_t          value    = 0;
	int32_t          absolute = 0;
	int32_t          e        = 0;
	int32_t          size     = 0;

	if( !_io_read_le32( desc, &size                           ) ) return pxtnERR_desc_r;
	if( !_io_read( desc, &evnt, sizeof( _x4x_EVENTSTRUCT ), 1 ) ) return pxtnERR_desc_r;
	swapEndian( evnt );

	if( evnt.data_num != 2               ) return pxtnERR_fmt_unknown;
	if( evnt.event_kind >= EVENTKIND_NUM ) return pxtnERR_fmt_unknown;
	if( bCheckRRR && evnt.rrr            ) return pxtnERR_fmt_unknown;

	absolute = 0;
	for( e = 0; e < (int32_t)evnt.event_num; e++ )
	{
		if( !_data_r_v( desc,&clock ) ) break;
		if( !_data_r_v( desc,&value ) ) break;
		absolute += clock;
		clock     = absolute;
		x4x_Read_Add( clock, (uint8_t)evnt.unit_index, (uint8_t)evnt.event_kind, value );
		if( bTailAbsolute && Evelist_Kind_IsTail( evnt.event_kind ) ) absolute += value;
	}
	if( e != (int32_t)evnt.event_num ) return pxtnERR_desc_broken;

	x4x_Read_NewKind();

	return pxtnOK;
}

pxtnERR pxtnEvelist::io_Read_x4x_EventNum( void* desc, int32_t* p_num ) const
{
	if( !desc || !p_num ) return pxtnERR_param;

	_x4x_EVENTSTRUCT evnt = {0};
	int32_t          work =  0 ;
	int32_t          e    =  0 ;
	int32_t          size =  0 ;

	if( !_io_read_le32( desc, &size                           ) ) return pxtnERR_desc_r;
	if( !_io_read( desc, &evnt, sizeof( _x4x_EVENTSTRUCT ), 1 ) ) return pxtnERR_desc_r;
	swapEndian( evnt );

	// support only 2
	if( evnt.data_num != 2 ) return pxtnERR_fmt_unknown;

	for( e = 0; e < (int32_t)evnt.event_num; e++ )
	{
		if( !_data_r_v( desc,&work ) ) break;
		if( !_data_r_v( desc,&work ) ) break;
	}
	if( e != (int32_t)evnt.event_num ) return pxtnERR_desc_broken;

	*p_num = evnt.event_num;

	return pxtnOK;
}


///////////////////////
// global
///////////////////////

bool Evelist_Kind_IsTail( int32_t kind )
{
	if( kind == EVENTKIND_ON || kind == EVENTKIND_PORTAMENT ) return true;
	return false;
}
