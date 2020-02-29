/*
Copyright 2019-2020, James R.
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

#include <stdlib.h>
#include <string.h>

#include "apng.h"

#define APNG_INFO_acTL 0x20000U

#define APNG_WROTE_acTL 0x10000U

struct apng_info_def
{
	png_uint_32 mode;
	png_uint_32 valid;

	png_uint_32 num_frames;
	png_uint_32 num_plays;

	long start_acTL;/* acTL is written here */

	png_flush_ptr output_flush_fn;
	apng_seek_ptr output_seek_fn;
	apng_tell_ptr output_tell_fn;

	apng_set_acTL_ptr set_acTL_fn;
};

/* PROTOS (FUCK COMPILER) */
void   apng_seek  (png_structp, apng_const_infop, size_t);
size_t apng_tell  (png_structp, apng_const_infop);
#ifdef PNG_WRITE_FLUSH_SUPPORTED
void   apng_flush (png_structp, apng_infop);
#ifdef PNG_STDIO_SUPPORTED
void   apng_default_flush (png_structp);
#endif/* PNG_STDIO_SUPPORTED */
#endif/* PNG_WRITE_FLUSH_SUPPORTED */
#ifdef PNG_STDIO_SUPPORTED
void   apng_default_seek  (png_structp, size_t);
size_t apng_default_tell  (png_structp);
#endif/* PNG_STDIO_SUPPORTED */
void   apng_write_IEND (png_structp);
void   apng_write_acTL (png_structp, png_uint_32, png_uint_32);
#ifndef PNG_WRITE_APNG_SUPPORTED
png_uint_32 apng_set_acTL_dummy (png_structp, png_infop,
		png_uint_32, png_uint_32);
#endif/* PNG_WRITE_APNG_SUPPORTED */

apng_infop
apng_create_info_struct (png_structp pngp)
{
	apng_infop ainfop;
	(void)pngp;
	if (( ainfop = calloc(sizeof (apng_info),1) ))
	{
		apng_set_write_fn(pngp, ainfop, 0, 0, 0, 0, 0);
		apng_set_set_acTL_fn(pngp, ainfop, 0);
	}
	return ainfop;
}

void
apng_destroy_info_struct (png_structp pngp, apng_infopp ainfopp)
{
	(void)pngp;
	if (!( pngp && ainfopp ))
		return;

	free((*ainfopp));
}

void
apng_seek (png_structp pngp, apng_const_infop ainfop, size_t l)
{
	(*(ainfop->output_seek_fn))(pngp, l);
}

size_t
apng_tell (png_structp pngp, apng_const_infop ainfop)
{
	return (*(ainfop->output_tell_fn))(pngp);
}

#ifdef PNG_WRITE_FLUSH_SUPPORTED
void
apng_flush (png_structp pngp, apng_infop ainfop)
{
	if (ainfop->output_flush_fn)
		(*(ainfop->output_flush_fn))(pngp);
}

#ifdef PNG_STDIO_SUPPORTED
void
apng_default_flush (png_structp pngp)
{
	if (!( pngp ))
		return;

	fflush((png_FILE_p)png_get_io_ptr);
}
#endif/* PNG_STDIO_SUPPORTED */
#endif/* PNG_WRITE_FLUSH_SUPPORTED */

#ifdef PNG_STDIO_SUPPORTED
void
apng_default_seek (png_structp pngp, size_t l)
{
	if (!( pngp ))
		return;

	if (fseek((png_FILE_p)png_get_io_ptr(pngp), (long)l, SEEK_SET) == -1)
		png_error(pngp, "Seek Error");
}

size_t
apng_default_tell (png_structp pngp)
{
	long l;

	if (!( pngp ))
	{
		png_error(pngp, "Call to apng_default_tell with NULL pngp failed");
	}

	if (( l = ftell((png_FILE_p)png_get_io_ptr(pngp)) ) == -1)
		png_error(pngp, "Tell Error");

	return (size_t)l;
}
#endif/* PNG_STDIO_SUPPORTED */

void
apng_set_write_fn (png_structp pngp, apng_infop ainfop, png_voidp iop,
		png_rw_ptr write_f, png_flush_ptr flush_f,
		apng_seek_ptr seek_f, apng_tell_ptr tell_f)
{
	if (!( pngp && ainfop ))
		return;

	png_set_write_fn(pngp, iop, write_f, flush_f);

#ifdef PNG_WRITE_FLUSH_SUPPORTED
#ifdef PNG_STDIO_SUPPORTED
	if (!flush_f)
		ainfop->output_flush_fn = &apng_default_flush;
	else
#endif/* PNG_STDIO_SUPPORTED */
		ainfop->output_flush_fn = flush_f;
#endif/* PNG_WRITE_FLUSH_SUPPORTED */
#ifdef PNG_STDIO_SUPPORTED
	if (!seek_f)
		ainfop->output_seek_fn = &apng_default_seek;
	else
#endif/* PNG_STDIO_SUPPORTED */
		ainfop->output_seek_fn  = seek_f;
#ifdef PNG_STDIO_SUPPORTED
	if (!seek_f)
		ainfop->output_tell_fn  = apng_default_tell;
	else
#endif/* PNG_STDIO_SUPPORTED */
		ainfop->output_tell_fn  = tell_f;
}

void
apng_write_IEND (png_structp pngp)
{
	png_byte chunkc[] = "IEND";
	png_write_chunk(pngp, chunkc, 0, 0);
}

void
apng_write_acTL (png_structp pngp, png_uint_32 frames, png_uint_32 plays)
{
	png_byte chunkc[] = "acTL";
	png_byte buf[8];
	png_save_uint_32(buf, frames);
	png_save_uint_32(buf + 4, plays);
	png_write_chunk(pngp, chunkc, buf, 8);
}

png_uint_32
apng_set_acTL (png_structp pngp, png_infop infop, apng_infop ainfop,
		png_uint_32 frames, png_uint_32 plays)
{
	(void)pngp;
	(void)infop;
	if (!( pngp && infop && ainfop ))
		return 0;

	ainfop->num_frames = frames;
	ainfop->num_plays  = plays;

	ainfop->valid |= APNG_INFO_acTL;

	return 1;
}

void
apng_write_info_before_PLTE (png_structp pngp, png_infop infop,
		apng_infop ainfop)
{
	if (!( pngp && infop && ainfop ))
		return;

	png_write_info_before_PLTE(pngp, infop);

	if (( ainfop->valid & APNG_INFO_acTL )&&!( ainfop->mode & APNG_WROTE_acTL ))
	{
		ainfop->start_acTL = apng_tell(pngp, ainfop);

		apng_write_acTL(pngp, 0, 0);
		/* modified for runtime dynamic linking */
		(*(ainfop->set_acTL_fn))(pngp, infop, PNG_UINT_31_MAX, 0);

		ainfop->mode |= APNG_WROTE_acTL;
	}
}

void
apng_write_info (png_structp pngp, png_infop infop,
		apng_infop ainfop)
{
	apng_write_info_before_PLTE(pngp, infop, ainfop);
	png_write_info(pngp, infop);
}

void
apng_write_end (png_structp pngp, png_infop infop, apng_infop ainfop)
{
	(void)infop;
	apng_write_IEND(pngp);
	apng_seek(pngp, ainfop, ainfop->start_acTL);
	apng_write_acTL(pngp, ainfop->num_frames, ainfop->num_plays);

#ifdef PNG_WRITE_FLUSH_SUPPORTED
#ifdef PNG_WRITE_FLUSH_AFTER_IEND_SUPPORTED
	apng_flush(pngp, infop);
#endif/* PNG_WRITE_FLUSH_SUPPORTED */
#endif/* PNG_WRITE_FLUSH_AFTER_IEND_SUPPORTED */
}

#ifndef PNG_WRITE_APNG_SUPPORTED
png_uint_32
apng_set_acTL_dummy (png_structp pngp, png_infop infop,
		png_uint_32 frames, png_uint_32 plays)
{
	(void)pngp;
	(void)infop;
	(void)frames;
	(void)plays;
	return 0;
}
#endif/* PNG_WRITE_APNG_SUPPORTED */

/* Dynamic runtime linking capable! (Hopefully.) */
void
apng_set_set_acTL_fn (png_structp pngp, apng_infop ainfop,
		apng_set_acTL_ptr set_acTL_f)
{
	(void)pngp;
	if (!ainfop->set_acTL_fn)
#ifndef PNG_WRITE_APNG_SUPPORTED
		ainfop->set_acTL_fn = &apng_set_acTL_dummy;
#else
		ainfop->set_acTL_fn = &png_set_acTL;
#endif/* PNG_WRITE_APNG_SUPPORTED */
	else
		ainfop->set_acTL_fn = set_acTL_f;
}
