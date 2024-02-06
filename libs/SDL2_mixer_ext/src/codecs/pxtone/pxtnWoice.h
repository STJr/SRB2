// '12/03/03 pxtnWoice.

#ifndef pxtnWoice_H
#define pxtnWoice_H

#include "./pxtnData.h"

#include "./pxtnPulse_Noise.h"
#include "./pxtnPulse_NoiseBuilder.h"
#include "./pxtnPulse_PCM.h"
#include "./pxtnPulse_Oggv.h"

#define pxtnMAX_TUNEWOICENAME      16 // fixture.

#define pxtnMAX_UNITCONTROLVOICE    2 // max-woice per unit

#define pxtnBUFSIZE_TIMEPAN      0x40
#define pxtnBITPERSAMPLE           16

#define PTV_VOICEFLAG_WAVELOOP   0x00000001
#define PTV_VOICEFLAG_SMOOTH     0x00000002
#define PTV_VOICEFLAG_BEATFIT    0x00000004
#define PTV_VOICEFLAG_UNCOVERED  0xfffffff8

#define PTV_DATAFLAG_WAVE        0x00000001
#define PTV_DATAFLAG_ENVELOPE    0x00000002
#define PTV_DATAFLAG_UNCOVERED   0xfffffffc

enum pxtnWOICETYPE
{
	pxtnWOICE_None = 0,
	pxtnWOICE_PCM ,
	pxtnWOICE_PTV ,
	pxtnWOICE_PTN ,
	pxtnWOICE_OGGV
};

enum pxtnVOICETYPE
{
	pxtnVOICE_Coodinate = 0,
	pxtnVOICE_Overtone ,
	pxtnVOICE_Noise    ,
	pxtnVOICE_Sampling ,
	pxtnVOICE_OggVorbis
};

typedef struct
{
	int32_t  smp_head_w ;
	int32_t  smp_body_w ;
	int32_t  smp_tail_w ;
	uint8_t* p_smp_w    ;

	uint8_t* p_env      ;
	int32_t  env_size   ;
	int32_t  env_release;

	bool     b_sine_over;
}
pxtnVOICEINSTANCE;

typedef struct
{
	int32_t    fps     ;
	int32_t    head_num;
	int32_t    body_num;
	int32_t    tail_num;
	pxtnPOINT* points  ;
}
pxtnVOICEENVELOPE;

typedef struct
{
	int32_t    num    ;
	int32_t    reso   ; // COODINATERESOLUTION
	pxtnPOINT *points;
}
pxtnVOICEWAVE;

typedef struct
{
	int32_t           basic_key  ;
	int32_t           volume     ;
	int32_t           pan        ;
	float             tuning     ;
	uint32_t          voice_flags;
	uint32_t          data_flags ;

	pxtnVOICETYPE     type       ;
	pxtnPulse_PCM     *p_pcm     ;
	pxtnPulse_Noise   *p_ptn     ;
#ifdef  pxINCLUDE_OGGVORBIS
	pxtnPulse_Oggv    *p_oggv    ;
#endif

	pxtnVOICEWAVE     wave       ;
	pxtnVOICEENVELOPE envelope   ;
}
pxtnVOICEUNIT;

typedef struct
{
	double  smp_pos    ;
	float   offset_freq;
	int32_t env_volume ;
	int32_t life_count ;
	int32_t on_count   ;

	int32_t smp_count  ;
	int32_t env_start  ;
	int32_t env_pos    ;
	int32_t env_release_clock;

	int32_t smooth_volume;
}
pxtnVOICETONE;


class pxtnWoice: public pxtnData
{
private:
	void operator = (const pxtnWoice& src){}
	pxtnWoice       (const pxtnWoice& src){}

	int32_t            _voice_num;

	char               _name_buf[ pxtnMAX_TUNEWOICENAME + 1 ];
	int32_t            _name_size    ;

	pxtnWOICETYPE      _type         ;
	pxtnVOICEUNIT*     _voices       ;
	pxtnVOICEINSTANCE* _voinsts      ;

	float              _x3x_tuning   ;
	int32_t            _x3x_basic_key; // tuning old-fmt when key-event



	bool    _Write_Wave    ( void* desc, const pxtnVOICEUNIT *p_vc, int32_t *p_total ) const;
	bool    _Write_Envelope( void* desc, const pxtnVOICEUNIT *p_vc, int32_t *p_total ) const;
	pxtnERR _Read_Wave     ( void* desc, pxtnVOICEUNIT *p_vc );
	pxtnERR _Read_Envelope ( void* desc, pxtnVOICEUNIT *p_vc );

	void    _UpdateWavePTV( pxtnVOICEUNIT* p_vc, pxtnVOICEINSTANCE* p_vi, int32_t  ch, int32_t  sps, int32_t  bps );


public :
	 pxtnWoice( pxtnIO_r io_read, pxtnIO_w io_write, pxtnIO_seek io_seek, pxtnIO_pos io_pos );
	~pxtnWoice();

	int32_t              get_voice_num     () const;
	float                get_x3x_tuning    () const;
	int32_t              get_x3x_basic_key () const;
	pxtnWOICETYPE        get_type          () const;
	const pxtnVOICEUNIT* get_voice         ( int32_t idx ) const;
	pxtnVOICEUNIT*       get_voice_variable( int32_t idx );

	const pxtnVOICEINSTANCE* get_instance  ( int32_t idx ) const;

	bool        set_name_buf( const char *name_buf, int32_t    buf_size );
	const char* get_name_buf(                       int32_t* p_buf_size ) const;
	bool        is_name_buf () const;

	bool Voice_Allocate( int32_t voice_num );
	void Voice_Release ();
	bool Copy( pxtnWoice *p_dst ) const;
	void Slim();

	pxtnERR read  ( void* desc, pxtnWOICETYPE type );

	bool    PTV_Write    ( void* desc, int32_t *p_total ) const;
	pxtnERR PTV_Read     ( void* desc                   );

	bool    io_matePCM_w ( void* desc ) const;
	pxtnERR io_matePCM_r ( void* desc );

	bool    io_matePTN_w ( void* desc ) const;
	pxtnERR io_matePTN_r ( void* desc );

	bool    io_matePTV_w ( void* desc ) const;
	pxtnERR io_matePTV_r ( void* desc );

#ifdef  pxINCLUDE_OGGVORBIS
	bool    io_mateOGGV_w( void* desc ) const;
	pxtnERR io_mateOGGV_r( void* desc );
#endif

	pxtnERR Tone_Ready_sample  ( const pxtnPulse_NoiseBuilder *ptn_bldr  );
	pxtnERR Tone_Ready_envelope( int32_t sps );
	pxtnERR Tone_Ready         ( const pxtnPulse_NoiseBuilder *ptn_bldr, int32_t sps );
};

#endif
