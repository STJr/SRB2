#ifndef pxtnPulse_NoiseBuilder_H
#define pxtnPulse_NoiseBuilder_H

#include "./pxtnData.h"

#include "./pxtnPulse_Noise.h"

class pxtnPulse_NoiseBuilder: public pxtnData
{
private:
	void operator =       (const pxtnPulse_NoiseBuilder& src){}
	pxtnPulse_NoiseBuilder(const pxtnPulse_NoiseBuilder& src){}

	bool    _b_init;
	short*  _p_tables[ pxWAVETYPE_num ];
	int32_t _rand_buf [ 2 ];

	void  _random_reset();
	short _random_get  ();

	pxtnPulse_Frequency* _freq;

public :

	 pxtnPulse_NoiseBuilder( pxtnIO_r io_read, pxtnIO_w io_write, pxtnIO_seek io_seek, pxtnIO_pos io_pos );
	~pxtnPulse_NoiseBuilder();

	bool Init();

	pxtnPulse_PCM *BuildNoise( pxtnPulse_Noise *p_noise, int32_t ch, int32_t sps, int32_t bps ) const;
};

#endif
