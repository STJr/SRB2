#ifndef pxtnPulse_Oscillator_H
#define pxtnPulse_Oscillator_H

#include "./pxtnData.h"

class pxtnPulse_Oscillator: public pxtnData
{
private:
	void operator =     (const pxtnPulse_Oscillator& src){}
	pxtnPulse_Oscillator(const pxtnPulse_Oscillator& src){}

	pxtnPOINT *_p_point  ;
	int32_t   _point_num ;
	int32_t   _point_reso;
	int32_t   _volume    ;
	int32_t   _sample_num;

public:

	pxtnPulse_Oscillator( pxtnIO_r io_read, pxtnIO_w io_write, pxtnIO_seek io_seek, pxtnIO_pos io_pos );

	void   ReadyGetSample( pxtnPOINT *p_point, int32_t point_num, int32_t volume, int32_t sample_num, int32_t point_reso );
	double GetOneSample_Overtone ( int32_t index );
	double GetOneSample_Coodinate( int32_t index );
};

#endif
