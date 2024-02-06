// '16/12/16 pxtnError.

#ifndef pxtnError_H
#define pxtnError_H

enum pxtnERR
{
	pxtnOK               = 0,
	pxtnERR_VOID            ,
	pxtnERR_INIT            ,
	pxtnERR_FATAL           ,

	pxtnERR_anti_opreation  ,

	pxtnERR_deny_beatclock  ,

	pxtnERR_desc_w          ,
	pxtnERR_desc_r          ,
	pxtnERR_desc_broken     ,

	pxtnERR_fmt_new         ,
	pxtnERR_fmt_unknown     ,

	pxtnERR_inv_code        ,
	pxtnERR_inv_data        ,

	pxtnERR_memory          ,

	pxtnERR_moo_init        ,

	pxtnERR_ogg             ,
	pxtnERR_ogg_no_supported,

	pxtnERR_param           ,

	pxtnERR_pcm_convert     ,
	pxtnERR_pcm_unknown     ,

	pxtnERR_ptn_build       ,
	pxtnERR_ptn_init        ,

	pxtnERR_ptv_no_supported,

	pxtnERR_too_much_event  ,

	pxtnERR_woice_full      ,

	pxtnERR_x1x_ignore      ,

	pxtnERR_x3x_add_tuning  ,
	pxtnERR_x3x_key         ,

	pxtnERR_num
};

const char* pxtnError_get_string( pxtnERR err_code );

#endif
