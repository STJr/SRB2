#ifndef pxtnPulse_Noise_H
#define pxtnPulse_Noise_H

#include "./pxtnData.h"

#include "./pxtnPulse_Frequency.h"
#include "./pxtnPulse_Oscillator.h"
#include "./pxtnPulse_PCM.h"

enum pxWAVETYPE
{
	pxWAVETYPE_None = 0,
	pxWAVETYPE_Sine,
	pxWAVETYPE_Saw,
	pxWAVETYPE_Rect,
	pxWAVETYPE_Random,
	pxWAVETYPE_Saw2,
	pxWAVETYPE_Rect2,

	pxWAVETYPE_Tri    ,
	pxWAVETYPE_Random2,
	pxWAVETYPE_Rect3  ,
	pxWAVETYPE_Rect4  ,
	pxWAVETYPE_Rect8  ,
	pxWAVETYPE_Rect16 ,
	pxWAVETYPE_Saw3   ,
	pxWAVETYPE_Saw4   ,
	pxWAVETYPE_Saw6   ,
	pxWAVETYPE_Saw8   ,

	pxWAVETYPE_num
};

typedef struct
{
	pxWAVETYPE type  ;
	float      freq  ;
	float      volume;
	float      offset;
	bool       b_rev ;
}
pxNOISEDESIGN_OSCILLATOR;

typedef struct
{
	bool                     bEnable ;
	int32_t                  enve_num;
	pxtnPOINT*               enves  ;
	int32_t                  pan     ;
	pxNOISEDESIGN_OSCILLATOR main    ;
	pxNOISEDESIGN_OSCILLATOR freq    ;
	pxNOISEDESIGN_OSCILLATOR volu    ;
}
pxNOISEDESIGN_UNIT;


class pxtnPulse_Noise: public pxtnData
{
private:
	void operator = (const pxtnPulse_Noise& src){}
	pxtnPulse_Noise (const pxtnPulse_Noise& src){}

	int32_t             _smp_num_44k;
	int32_t             _unit_num   ;
	pxNOISEDESIGN_UNIT* _units      ;

	bool     _WriteOscillator( const pxNOISEDESIGN_OSCILLATOR *p_osc, void* desc, int32_t *p_add ) const;
	pxtnERR  _ReadOscillator ( pxNOISEDESIGN_OSCILLATOR *p_osc, void* desc );
	uint32_t _MakeFlags      ( const pxNOISEDESIGN_UNIT *pU ) const;

public:
	 pxtnPulse_Noise    ( pxtnIO_r io_read, pxtnIO_w io_write, pxtnIO_seek io_seek, pxtnIO_pos io_pos );
	~pxtnPulse_Noise    ();

	bool    write       ( void* desc, int32_t *p_add ) const;
	pxtnERR read        ( void* desc );

	void Release        ( );
	bool Allocate       ( int32_t unit_num, int32_t envelope_num );
	bool copy_from      ( const pxtnPulse_Noise *src );
	int32_t Compare     ( const pxtnPulse_Noise *src ) const;
	void Fix();

	void set_smp_num_44k( int32_t num );

	int32_t get_unit_num   () const;
	int32_t get_smp_num_44k() const;
	float   get_sec        () const;
	pxNOISEDESIGN_UNIT *get_unit( int32_t u );
};

#endif
