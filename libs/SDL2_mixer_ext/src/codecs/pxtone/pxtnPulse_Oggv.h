#ifndef pxtnPulse_Oggv_H
#define pxtnPulse_Oggv_H

#ifdef  pxINCLUDE_OGGVORBIS

#include "./pxtnData.h"

#include "./pxtnPulse_PCM.h"

class pxtnPulse_Oggv: public pxtnData
{
private:
	void operator = (const pxtnPulse_Oggv& src){}
	pxtnPulse_Oggv  (const pxtnPulse_Oggv& src){}

	int32_t _ch     ;
	int32_t _sps2   ;
	int32_t _smp_num;
	int32_t _size   ;
	char*   _p_data ;

	bool _SetInformation();

public :

	 pxtnPulse_Oggv( pxtnIO_r io_read, pxtnIO_w io_write, pxtnIO_seek io_seek, pxtnIO_pos io_pos );
	~pxtnPulse_Oggv();

	pxtnERR Decode ( pxtnPulse_PCM *p_pcm ) const;
	void    Release();
	bool    GetInfo( int32_t *p_ch, int32_t *p_sps, int32_t *p_smp_num );
	int32_t GetSize() const;

	bool    ogg_write ( void* desc ) const;
	pxtnERR ogg_read  ( void* desc );
	bool    pxtn_write( void* desc ) const;
	bool    pxtn_read ( void* desc );

	bool    copy_from ( const pxtnPulse_Oggv* src );
};
#endif
#endif
