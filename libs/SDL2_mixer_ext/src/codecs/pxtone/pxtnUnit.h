// '12/03/03

#ifndef pxtnUnit_H
#define pxtnUnit_H

#include "./pxtnData.h"

#include "./pxtnMax.h"
#include "./pxtnWoice.h"

class pxtnUnit: public pxtnData
{
private:
	void operator = (const pxtnUnit& src){}
	pxtnUnit        (const pxtnUnit& src){}

	bool     _bOperated;
	bool     _bPlayed;
	char     _name_buf[  pxtnMAX_TUNEUNITNAME + 1 ];
	int32_t  _name_size;

	//	TUNEUNITTONESTRUCT
	int32_t  _key_now;
	int32_t  _key_start;
	int32_t  _key_margin;
	int32_t  _portament_sample_pos;
	int32_t  _portament_sample_num;
	int32_t  _pan_vols     [ pxtnMAX_CHANNEL ];
	int32_t  _pan_times    [ pxtnMAX_CHANNEL ];
	int32_t  _pan_time_bufs[ pxtnMAX_CHANNEL ][ pxtnBUFSIZE_TIMEPAN ];
	int32_t  _v_VOLUME  ;
	int32_t  _v_VELOCITY;
	int32_t  _v_GROUPNO ;
	float    _v_TUNING  ;

	const pxtnWoice *_p_woice;

	pxtnVOICETONE _vts[ pxtnMAX_UNITCONTROLVOICE ];

public :
	 pxtnUnit( pxtnIO_r io_read, pxtnIO_w io_write, pxtnIO_seek io_seek, pxtnIO_pos io_pos );
	~pxtnUnit();

	void    Tone_Init ();

	void    Tone_Clear();

	void    Tone_Reset_and_2prm( int32_t voice_idx, int32_t env_rls_clock, float offset_freq );
	void    Tone_Envelope  ();
	void    Tone_KeyOn     ();
	void    Tone_ZeroLives ();
	void    Tone_Key       ( int32_t key );
	void    Tone_Pan_Volume( int32_t ch, int32_t pan );
	void    Tone_Pan_Time  ( int32_t ch, int32_t pan, int32_t sps );

	void    Tone_Velocity  ( int32_t val );
	void    Tone_Volume    ( int32_t val );
	void    Tone_Portament ( int32_t val );
	void    Tone_GroupNo   ( int32_t val );
	void    Tone_Tuning    ( float   val );

	void    Tone_Sample    ( bool b_mute_by_unit, int32_t ch_num, int32_t time_pan_index, int32_t smooth_smp );
	void    Tone_Supple    ( int32_t *group_smps, int32_t ch_num, int32_t time_pan_index ) const;
	int32_t Tone_Increment_Key   ();
	void    Tone_Increment_Sample( float freq );

	bool             set_woice( const pxtnWoice *p_woice );
	const pxtnWoice* get_woice() const;

	bool        set_name_buf( const char *name_buf, int32_t    buf_size );
	const char* get_name_buf(                       int32_t* p_buf_size ) const;
	bool        is_name_buf () const;

	pxtnVOICETONE *get_tone( int32_t voice_idx );

	void set_operated( bool b );
	void set_played  ( bool b );
	bool get_operated() const;
	bool get_played  () const;

	pxtnERR Read_v3x( void* desc, int32_t *p_group );
	bool    Read_v1x( void* desc, int32_t *p_group );
};

#endif
