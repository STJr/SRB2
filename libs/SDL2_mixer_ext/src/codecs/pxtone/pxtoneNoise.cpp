// '12/03/29

#include "./pxtn.h"

#include "./pxtnService.h"

#include "./pxtoneNoise.h"

pxtoneNoise::pxtoneNoise( pxtnIO_r io_read, pxtnIO_w io_write, pxtnIO_seek io_seek, pxtnIO_pos io_pos )
{
	_set_io_funcs( io_read, io_write, io_seek, io_pos );

	_bldr   = NULL ;
	_sps    = 44100;
	_ch_num =     2;
	_bps    =    16;
}

pxtoneNoise::~pxtoneNoise()
{
	if( _bldr ) { delete (pxtnPulse_NoiseBuilder*)_bldr; } _bldr = NULL;
}

bool pxtoneNoise::init()
{
	pxtnPulse_NoiseBuilder *bldr = new pxtnPulse_NoiseBuilder( _io_read, _io_write, _io_seek, _io_pos );
	if( !bldr->Init() ){ delete bldr; return false; }
	_bldr = bldr;
	return true;
}

bool pxtoneNoise::quality_set( int32_t ch_num, int32_t sps, int32_t bps )
{
	switch( ch_num )
	{
	case 1: case 2: break;
	default: return false;
	}

	switch( sps )
	{
	case 48000: case 44100: case 22050: case 11025: break;
	default: return false;
	}

	switch( bps )
	{
	case 8: case 16: break;
	default: return false;
	}

	_ch_num = ch_num;
	_bps    = bps   ;
	_sps    = sps   ;

	return false;
}

void pxtoneNoise::quality_get( int32_t *p_ch_num, int32_t *p_sps, int32_t *p_bps ) const
{
	if( p_ch_num ) *p_ch_num = _ch_num;
	if( p_sps    ) *p_sps    = _sps   ;
	if( p_bps    ) *p_bps    = _bps   ;
}


bool pxtoneNoise::generate( void* desc, void **pp_buf, int32_t *p_size ) const
{
	bool                   b_ret  = false;
	pxtnPulse_NoiseBuilder *bldr  = (pxtnPulse_NoiseBuilder*)_bldr;
	pxtnPulse_Noise        *noise = new pxtnPulse_Noise( _io_read, _io_write, _io_seek, _io_pos );
	pxtnPulse_PCM          *pcm   = NULL;

	if( noise->read( desc ) != pxtnOK ) goto End;
	if( !( pcm = bldr->BuildNoise( noise, _ch_num, _sps, _bps ) ) ) goto End;

	*p_size = pcm->get_buf_size();
	*pp_buf = pcm->Devolve_SamplingBuffer();

	b_ret = true;
End:
	if( noise ) delete noise;
	if( pcm   ) delete pcm  ;

	return b_ret;
}
