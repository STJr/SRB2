#ifndef pxtnPulse_Frequency_H
#define pxtnPulse_Frequency_H

#include "./pxtnData.h"

class pxtnPulse_Frequency: public pxtnData
{
private:
	void operator =    (const pxtnPulse_Frequency& src){}
	pxtnPulse_Frequency(const pxtnPulse_Frequency& src){}

	float* _freq_table;
	double _GetDivideOctaveRate( int32_t divi );

public:

	 pxtnPulse_Frequency( pxtnIO_r io_read, pxtnIO_w io_write, pxtnIO_seek io_seek, pxtnIO_pos io_pos );
	~pxtnPulse_Frequency();

	bool Init();

	float        Get      ( int32_t key     );
	float        Get2     ( int32_t key     );
	const float* GetDirect( int32_t *p_size );
};

#endif
