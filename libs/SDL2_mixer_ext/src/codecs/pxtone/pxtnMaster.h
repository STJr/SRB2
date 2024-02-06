// '12/03/03

#ifndef pxtnMaster_H
#define pxtnMaster_H

#include "./pxtnData.h"

class pxtnMaster: public pxtnData
{
private:
	void operator = (const pxtnMaster& src){}
	pxtnMaster      (const pxtnMaster& src){}

	int32_t _beat_num   ;
	float   _beat_tempo ;
	int32_t _beat_clock ;
	int32_t _meas_num   ;
	int32_t _repeat_meas;
	int32_t _last_meas  ;
	int32_t _volume_    ;

public :
	 pxtnMaster( pxtnIO_r io_read, pxtnIO_w io_write, pxtnIO_seek io_seek, pxtnIO_pos io_pos );
	~pxtnMaster();

	void  Reset();

	void  Set( int32_t    beat_num, float    beat_tempo, int32_t    beat_clock );
	void  Get( int32_t *p_beat_num, float *p_beat_tempo, int32_t *p_beat_clock, int32_t* p_meas_num ) const;

	int32_t get_beat_num   ()const;
	float   get_beat_tempo ()const;
	int32_t get_beat_clock ()const;
	int32_t get_meas_num   ()const;
	int32_t get_repeat_meas()const;
	int32_t get_last_meas  ()const;
	int32_t get_last_clock ()const;
	int32_t get_play_meas  ()const;

	void    set_meas_num   ( int32_t meas_num )  ;
	void    set_repeat_meas( int32_t meas       );
	void    set_last_meas  ( int32_t meas       );
	void    set_beat_clock ( int32_t beat_clock );

	void    AdjustMeasNum  ( int32_t clock    );

	int32_t get_this_clock( int32_t meas, int32_t beat, int32_t clock ) const;

	bool    io_w_v5          ( void* desc, int32_t rough ) const;
	pxtnERR io_r_v5          ( void* desc );
	int32_t io_r_v5_EventNum ( void* desc );

	pxtnERR io_r_x4x         ( void* desc );
	int32_t io_r_x4x_EventNum( void* desc );
};

#endif
