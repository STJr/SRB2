// '12/03/03

#ifndef pxtnDelay_H
#define pxtnDelay_H

#include "./pxtnData.h"

#include "./pxtnMax.h"

enum DELAYUNIT
{
	DELAYUNIT_Beat = 0,
	DELAYUNIT_Meas    ,
	DELAYUNIT_Second  ,
	DELAYUNIT_num
};

class pxtnDelay: public pxtnData
{
private:

	void operator = (const pxtnDelay& src){}
	pxtnDelay       (const pxtnDelay& src){}

	bool      _b_played  ;
	DELAYUNIT _unit      ;
	int32_t   _group     ;
	float     _rate      ;
	float     _freq      ;

	int32_t   _smp_num   ;
	int32_t   _offset    ;
	int32_t*  _bufs[ pxtnMAX_CHANNEL ];
	int32_t   _rate_s32  ;

public :

	 pxtnDelay( pxtnIO_r io_read, pxtnIO_w io_write, pxtnIO_seek io_seek, pxtnIO_pos io_pos );
	~pxtnDelay();

	pxtnERR Tone_Ready    ( int32_t beat_num, float beat_tempo, int32_t sps );
	void    Tone_Supple   ( int32_t ch_num  , int32_t *group_smps );
	void    Tone_Increment();
	void    Tone_Release  ();
	void    Tone_Clear    ();

	bool Add_New    ( DELAYUNIT scale, float freq, float rate, int32_t group );

	bool      Write( void* desc ) const;
	pxtnERR   Read ( void* desc );


	DELAYUNIT get_unit ()const;
	float     get_freq ()const;
	float     get_rate ()const;
	int32_t   get_group()const;

	void      Set( DELAYUNIT unit, float freq, float rate, int32_t group );

	bool      get_played()const;
	void      set_played( bool b );
	bool      switch_played();
};



#endif
