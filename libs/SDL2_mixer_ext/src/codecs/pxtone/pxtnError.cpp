#include "./pxtnError.h"

static const char* _err_msg_tbl[ pxtnERR_num + 1 ] =
{
	"OK"              ,
	"VOID"            ,
	"INIT"            ,
	"FATAL"           ,

	"anti operation"  ,

	"deny beatclock"  ,
	"desc w"          ,
	"desc r"          ,
	"desc broken "    ,

	"fmt new "        ,
	"fmt unknown "    ,

	"inv code"        ,
	"inv data"        ,

	"memory"          ,
	"moo init"        ,

	"ogg "            ,
	"ogg no supported",

	"param "          ,
	"pcm convert "    ,
	"pcm unknown "    ,
	"ptn build "      ,
	"ptn init"        ,
	"ptv no supported",

	"woice full"      ,

	"x1x ignore"      ,
	"x3x add tuning"  ,
	"x3x key "        ,

	"?",
};

const char* pxtnError_get_string( pxtnERR err_code )
{
	if( err_code < 0 || err_code >= pxtnERR_num ) return _err_msg_tbl[ pxtnERR_num ];
	return                                               _err_msg_tbl[ err_code    ];
}
