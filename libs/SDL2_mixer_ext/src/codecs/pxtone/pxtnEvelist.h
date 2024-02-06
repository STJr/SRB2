#ifndef pxtnEvelist_H
#define pxtnEvelist_H

#include "./pxtnData.h"

enum
{
	EVENTKIND_NULL  = 0 ,//  0

	EVENTKIND_ON        ,//  1
	EVENTKIND_KEY       ,//  2
	EVENTKIND_PAN_VOLUME,//  3
	EVENTKIND_VELOCITY  ,//  4
	EVENTKIND_VOLUME    ,//  5
	EVENTKIND_PORTAMENT ,//  6
	EVENTKIND_BEATCLOCK ,//  7
	EVENTKIND_BEATTEMPO ,//  8
	EVENTKIND_BEATNUM   ,//  9
	EVENTKIND_REPEAT    ,// 10
	EVENTKIND_LAST      ,// 11
	EVENTKIND_VOICENO   ,// 12
	EVENTKIND_GROUPNO   ,// 13
	EVENTKIND_TUNING    ,// 14
	EVENTKIND_PAN_TIME  ,// 15

	EVENTKIND_NUM       // 16
};

#define EVENTDEFAULT_VOLUME       104
#define EVENTDEFAULT_VELOCITY     104
#define EVENTDEFAULT_PAN_VOLUME    64
#define EVENTDEFAULT_PAN_TIME      64
#define EVENTDEFAULT_PORTAMENT      0
#define EVENTDEFAULT_VOICENO        0
#define EVENTDEFAULT_GROUPNO        0
#define EVENTDEFAULT_KEY       0x6000
#define EVENTDEFAULT_BASICKEY  0x4500 // 4A(440Hz?)
#define EVENTDEFAULT_TUNING      1.0f

#define EVENTDEFAULT_BEATNUM        4
#define EVENTDEFAULT_BEATTEMPO    120
#define EVENTDEFAULT_BEATCLOCK    480

typedef struct EVERECORD
{
	uint8_t    kind    ;
	uint8_t    unit_no ;
	uint8_t    reserve1;
	uint8_t    reserve2;
	int32_t    value   ;
	int32_t    clock   ;
	EVERECORD* prev    ;
	EVERECORD* next    ;
}
EVERECORD;

//--------------------------------

class pxtnEvelist: public pxtnData
{

private:

	pxtnEvelist(              const pxtnEvelist &src  ){               } // copy
	pxtnEvelist & operator = (const pxtnEvelist &right){ return *this; } // substitution

	int32_t    _eve_allocated_num;
	EVERECORD* _eves     ;
	EVERECORD* _start    ;
	int32_t    _linear   ;

	EVERECORD* _p_x4x_rec;

	void _rec_set( EVERECORD* p_rec, EVERECORD* prev, EVERECORD* next, int32_t clock, uint8_t unit_no, uint8_t kind, int32_t value );
	void _rec_cut( EVERECORD* p_rec );

public:

	void Release();
	void Clear  ();

	 pxtnEvelist( pxtnIO_r io_read, pxtnIO_w io_write, pxtnIO_seek io_seek, pxtnIO_pos io_pos );
	~pxtnEvelist();

	bool Allocate( int32_t max_event_num );

	int32_t  get_Num_Max  () const;
	int32_t  get_Max_Clock() const;
	int32_t  get_Count    () const;
	int32_t  get_Count    (                                                  uint8_t kind, int32_t value ) const;
	int32_t  get_Count    (                                 uint8_t unit_no                              ) const;
	int32_t  get_Count    (                                 uint8_t unit_no, uint8_t kind                ) const;
	int32_t  get_Count    ( int32_t clock1, int32_t clock2, uint8_t unit_no                              ) const;
	int32_t  get_Value    ( int32_t clock ,                 uint8_t unit_no, uint8_t kind                ) const;

	const EVERECORD* get_Records() const;

	bool    Record_Add_i         ( int32_t clock,             uint8_t unit_no, uint8_t kind, int32_t   value   );
	bool    Record_Add_f         ( int32_t clock,             uint8_t unit_no, uint8_t kind, float value_f     );

	bool    Linear_Start         ();
	void    Linear_Add_i         ( int32_t clock,             uint8_t unit_no, uint8_t kind, int32_t   value   );
	void    Linear_Add_f         ( int32_t clock,             uint8_t unit_no, uint8_t kind, float value_f     );
	void    Linear_End           ( bool b_connect );

	int32_t Record_Clock_Shift   ( int32_t clock , int32_t shift , uint8_t unit_no  ); // can't be under 0.
	int32_t Record_Value_Set     ( int32_t clock1, int32_t clock2, uint8_t unit_no, uint8_t kind, int32_t value );
	int32_t Record_Value_Change  ( int32_t clock1, int32_t clock2, uint8_t unit_no, uint8_t kind, int32_t value );
	int32_t Record_Value_Omit    (                                                  uint8_t kind, int32_t value );
	int32_t Record_Value_Replace (                                                  uint8_t kind, int32_t old_value, int32_t new_value );
	int32_t Record_Delete        ( int32_t clock1, int32_t clock2, uint8_t unit_no, uint8_t kind                );
	int32_t Record_Delete        ( int32_t clock1, int32_t clock2, uint8_t unit_no                              );

	int32_t Record_UnitNo_Miss   (                                 uint8_t unit_no              ); // delete event has the unit-no
	int32_t Record_UnitNo_Set    (                                 uint8_t unit_no              ); // set the unit-no
	int32_t Record_UnitNo_Replace(                                 uint8_t old_u, uint8_t new_u ); // exchange unit

	int32_t BeatClockOperation( int32_t rate );

	bool    io_Write( void* desc, int32_t rough    ) const;
	pxtnERR io_Read ( void* desc );

	int32_t io_Read_EventNum( void* desc ) const;

	bool x4x_Read_Start  ();
	void x4x_Read_NewKind();
	void x4x_Read_Add    ( int32_t clock, uint8_t unit_no, uint8_t kind, int32_t value );

	pxtnERR io_Unit_Read_x4x_EVENT( void* desc, bool bTailAbsolute, bool bCheckRRR );
	pxtnERR io_Read_x4x_EventNum  ( void* desc, int32_t* p_num ) const;
};

bool Evelist_Kind_IsTail( int32_t kind );

#endif
