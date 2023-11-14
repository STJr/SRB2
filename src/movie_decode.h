// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2023 by LJ Sonic
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  movie_decode.h
/// \brief Movie decoding using FFmpeg

#ifndef __MOVIE_DECODE__
#define __MOVIE_DECODE__

#include "doomtype.h"
#include "i_threads.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/frame.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"
#include "v_video.h"

typedef struct {
	INT32 datasize;
	UINT8 *data[4];
	INT32 linesize[4];
} avimage_t;

typedef struct
{
	UINT64 id;
	INT64 pts;

	union {
		avimage_t rgba;
		UINT8 *patch;
	} image;
} movievideoframe_t;

typedef struct
{
	INT64 pts;
	UINT8 *samples[2];
	size_t numsamples;
} movieaudioframe_t;

typedef struct
{
	void *data;
	INT32 capacity;
	INT32 slotsize;
	INT32 start;
	INT32 size;
} moviebuffer_t;

typedef struct
{
	AVStream *stream;
	int index;
	AVCodecContext *codeccontext;
	moviebuffer_t buffer;
	moviebuffer_t framequeue;
	moviebuffer_t framepool;
	INT32 availablequeuesize;
	INT32 numplanes;
} moviestream_t;

typedef struct
{
	UINT64 lastvideoframeusedid;
	colorlookup_t colorlut;
	boolean usepatches;
	avimage_t tmpimage;

	AVFormatContext *formatcontext;
	AVFrame *frame;
	struct SwsContext *scalingcontext;
	SwrContext *resamplingcontext;

	moviestream_t videostream;
	moviestream_t audiostream;

	moviebuffer_t packetpool;
	moviebuffer_t packetqueue;

	UINT8 *lumpdata;
	size_t lumpsize;
	size_t lumpposition;

	tic_t position;
	INT64 audioposition;
	boolean flushing;
	boolean stopping;

	I_mutex mutex;
	I_cond cond;
	I_mutex condmutex;
} movie_t;

movie_t *MovieDecode_Play(const char *name, boolean usepatches);
void MovieDecode_Stop(movie_t **movieptr);
void MovieDecode_Seek(movie_t *movie, tic_t tic);
void MovieDecode_Update(movie_t *movie);
tic_t MovieDecode_GetDuration(movie_t *movie);
void MovieDecode_GetDimensions(movie_t *movie, INT32 *width, INT32 *height);
UINT8 *MovieDecode_GetImage(movie_t *movie);
INT32 MovieDecode_GetBytesPerPatchColumn(movie_t *movie);
void MovieDecode_CopyAudioSamples(movie_t *movie, void *mem, size_t size);

#endif
