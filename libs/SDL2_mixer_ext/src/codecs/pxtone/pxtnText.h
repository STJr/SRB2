// '12/03/03

#ifndef pxtnText_H
#define pxtnText_H

#include "./pxtnData.h"

class pxtnText: public pxtnData
{
private:
	void operator = (const pxtnText& src){}
	pxtnText        (const pxtnText& src){}

	char*   _p_comment_buf;
	int32_t _comment_size ;

	char*   _p_name_buf   ;
	int32_t _name_size    ;

	bool _read4_malloc(       char** pp, int32_t* p_buf_size, void* desc );
	bool _write4      ( const char*  p , int32_t    buf_size, void* desc ) const;

public :
	 pxtnText( pxtnIO_r io_read, pxtnIO_w io_write, pxtnIO_seek io_seek, pxtnIO_pos io_pos );
	~pxtnText();

	bool        set_comment_buf( const char *p_comment, int32_t    buf_size );
	const char* get_comment_buf(                        int32_t* p_buf_size ) const;
	bool        is_comment_buf () const;

	bool        set_name_buf   ( const char *p_name   , int32_t    buf_size );
	const char* get_name_buf   (                        int32_t* p_buf_size ) const;
	bool        is_name_buf    () const;

	bool Comment_r( void* desc );
	bool Comment_w( void* desc );
	bool Name_r   ( void* desc );
	bool Name_w   ( void* desc );
};

#endif
