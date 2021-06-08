/*
Copyright 2019-2021, James R.
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef APNG_H
#define APNG_H

#ifndef _MSC_VER
#ifndef _LARGEFILE64_SOURCE
#define _LARGEFILE64_SOURCE
#endif
#endif

#ifndef _LFS64_LARGEFILE
#define _LFS64_LARGEFILE
#endif

#ifndef _FILE_OFFSET_BITS
#define _FILE_OFFSET_BITS 0
#endif

#include <png.h>

typedef struct apng_info_def apng_info;
typedef apng_info * apng_infop;
typedef const apng_info * apng_const_infop;
typedef apng_info * * apng_infopp;

typedef void   (*apng_seek_ptr)(png_structp, size_t);
typedef size_t (*apng_tell_ptr)(png_structp);

typedef png_uint_32 (*apng_set_acTL_ptr)(png_structp, png_infop,
		png_uint_32, png_uint_32);

apng_infop apng_create_info_struct (png_structp png_ptr);

void apng_destroy_info_struct (png_structp png_ptr,
		apng_infopp info_ptr_ptr);

/* Call the following functions in place of the libpng counterparts. */

png_uint_32 apng_set_acTL (png_structp png_ptr, png_infop info_ptr,
		apng_infop ainfo_ptr,
		png_uint_32 num_frames, png_uint_32 num_plays);

void apng_write_info_before_PLTE (png_structp png_ptr, png_infop info_ptr,
		apng_infop ainfo_ptr);
void apng_write_info (png_structp png_ptr, png_infop info_ptr,
		apng_infop ainfo_ptr);

void apng_write_end (png_structp png_ptr, png_infop info_ptr,
		apng_infop ainfo_ptr);

void apng_set_write_fn (png_structp png_ptr, apng_infop ainfo_ptr,
		png_voidp io_ptr,
		png_rw_ptr write_data_fn, png_flush_ptr output_flush_fn,
		apng_seek_ptr output_seek_fn, apng_tell_ptr output_tell_fn);

void apng_set_set_acTL_fn (png_structp png_ptr, apng_infop ainfo_ptr,
		apng_set_acTL_ptr set_acTL_fn);

#endif/* APNG_H */
