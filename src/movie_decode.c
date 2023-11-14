// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2023 by LJ Sonic
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file  movie_decode.c
/// \brief Movie decoding using FFmpeg

#include "movie_decode.h"
#include "byteptr.h"
#include "doomtype.h"
#include "libavutil/channel_layout.h"
#include "libavutil/imgutils.h"
#include "s_sound.h"
#include "v_video.h"
#include "w_wad.h"

#if defined(__GNUC__) || defined(__clang__)
	// Because of swr_convert not using const on the input buffer's pointer
    #pragma GCC diagnostic ignored "-Wcast-qual"
#endif

#define IO_BUFFER_SIZE (8 * 1024)
#define STREAM_BUFFER_TIME (4 * TICRATE)

static void InitialiseBuffer(moviebuffer_t *buffer, INT32 capacity, INT32 slotsize)
{
	buffer->capacity = capacity;
	buffer->slotsize = slotsize;
	buffer->start = 0;
	buffer->size = 0;

	buffer->data = calloc(capacity, slotsize);
	if (!buffer->data)
		I_Error("FFmpeg: cannot allocate buffer");
}

static void UninitialiseBuffer(moviebuffer_t *buffer)
{
	free(buffer->data);
}

static void CloneBuffer(moviebuffer_t *dst, moviebuffer_t *src)
{
	memcpy(dst, src, sizeof(*dst));

	dst->data = calloc(dst->capacity, dst->slotsize);
	if (!dst->data)
		I_Error("FFmpeg: cannot allocate buffer");

	memcpy(dst->data, src->data, dst->capacity * dst->slotsize);
}

static void *GetBufferSlot(const moviebuffer_t *buffer, INT32 index)
{
	if (index < 0)
		return NULL;

	index = (buffer->start + index) % buffer->capacity;
	return &((UINT8*)buffer->data)[index * buffer->slotsize];
}

static void *PeekBuffer(const moviebuffer_t *buffer)
{
	return GetBufferSlot(buffer, 0);
}

static void *EnqueueBuffer(moviebuffer_t *buffer)
{
	buffer->size++;
	return GetBufferSlot(buffer, buffer->size - 1);
}

static void *DequeueBuffer(moviebuffer_t *buffer)
{
	void *slot = PeekBuffer(buffer);
	buffer->start = (buffer->start + 1) % buffer->capacity;
	buffer->size--;
	return slot;
}

static void *DequeueBufferIntoBuffer(moviebuffer_t *dst, moviebuffer_t *src)
{
	void *srcslot = DequeueBuffer(src);
	void *dstslot = EnqueueBuffer(dst);
	memcpy(dstslot, srcslot, src->slotsize);
	return dstslot;
}

static INT64 TicsToMS(tic_t tics)
{
	AVRational oldtb = { 1, 35 };
	AVRational newtb = { 1, 1000 };

	return av_rescale_q(tics, oldtb, newtb);
}

static INT64 TicsToVideoPTS(movie_t *movie, tic_t tics)
{
	AVRational oldtb = { 1, TICRATE };
	AVRational newtb = movie->videostream.stream->time_base;

	return av_rescale_q(tics, oldtb, newtb);
}

static INT64 TicsToAudioPTS(movie_t *movie, tic_t tics)
{
	AVRational oldtb = { 1, TICRATE };
	AVRational newtb = movie->audiostream.stream->time_base;

	return av_rescale_q(tics, oldtb, newtb);
}

static INT64 PTSToSamples(movie_t *movie, INT64 pts)
{
	AVRational oldtb = movie->audiostream.stream->time_base;
	AVRational newtb = { 1, 44100 };

	return av_rescale_q(pts, oldtb, newtb);
}

static INT64 SamplesToPTS(movie_t *movie, INT64 numsamples)
{
	AVRational oldtb = { 1, 44100 };
	AVRational newtb = movie->audiostream.stream->time_base;

	return av_rescale_q(numsamples, oldtb, newtb);
}

static INT64 AudioPTSToMS(movie_t *movie, INT64 pts)
{
	AVRational oldtb = movie->audiostream.stream->time_base;
	AVRational newtb = { 1, 1000 };

	return av_rescale_q(pts, oldtb, newtb);
}

static INT64 VideoPTSToMS(movie_t *movie, INT64 pts)
{
	AVRational oldtb = movie->videostream.stream->time_base;
	AVRational newtb = { 1, 1000 };

	return av_rescale_q(pts, oldtb, newtb);
}

static INT64 PTSToTics(INT64 pts)
{
	AVRational newtb = { 1, 35 };

	return av_rescale_q(pts, AV_TIME_BASE_Q, newtb);
}

static INT64 GetAudioFrameEndPTS(movie_t *movie, movieaudioframe_t *frame)
{
	AVRational oldtb = { 1, 44100 };
	AVRational newtb = movie->audiostream.stream->time_base;

	return frame->pts + av_rescale_q(frame->numsamples, oldtb, newtb);
}

static INT32 FindVideoBufferIndexForPosition(movie_t *movie, INT64 pts)
{
	INT32 i;

	for (i = movie->videostream.buffer.size - 1; i >= 0; i--)
	{
		movievideoframe_t *frame = GetBufferSlot(&movie->videostream.buffer, i);
		if (frame->pts <= pts)
			return i;
	}

	return -1;
}

static INT32 FindAudioBufferIndexForPosition(movie_t *movie, INT64 pts)
{
	INT32 i;

	for (i = 0; i < movie->audiostream.buffer.size; i++)
	{
		movieaudioframe_t *frame = GetBufferSlot(&movie->audiostream.buffer, i);
		if (frame->pts <= pts && pts < GetAudioFrameEndPTS(movie, frame))
			return i;
	}

	return -1;
}

static void ClearOldestFrame(movie_t *movie, moviestream_t *stream)
{
	DequeueBufferIntoBuffer(&stream->framepool, &stream->buffer);
	I_wake_one_cond(&movie->cond);
}

static void ClearOldVideoFrames(movie_t *movie)
{
	moviebuffer_t *buffer = &movie->videostream.buffer;
	INT64 limit = TicsToVideoPTS(movie, movie->position) - TicsToVideoPTS(movie, STREAM_BUFFER_TIME / 2);

	while (buffer->size > 0 && ((movievideoframe_t*)PeekBuffer(buffer))->pts < limit)
		ClearOldestFrame(movie, &movie->videostream);
}

static void ClearOldAudioFrames(movie_t *movie)
{
	moviebuffer_t *buffer = &movie->audiostream.buffer;
	INT64 limit = max(movie->audioposition - TicsToAudioPTS(movie, STREAM_BUFFER_TIME / 2), 0);

	while (buffer->size > 0 && GetAudioFrameEndPTS(movie, PeekBuffer(buffer)) < limit)
		ClearOldestFrame(movie, &movie->audiostream);
}

static void ClearAllFrames(movie_t *movie)
{
	while (movie->videostream.buffer.size != 0)
		ClearOldestFrame(movie, &movie->videostream);

	while (movie->audiostream.buffer.size != 0)
		ClearOldestFrame(movie, &movie->audiostream);
}

static lumpnum_t FindMovieLumpNum(const char *name)
{
	INT32 wadnum;

	for (wadnum = numwadfiles - 1; wadnum >= 0; wadnum--)
	{
		char fullname[256];
		UINT16 lumpnum;

		snprintf(fullname, sizeof(fullname), "Movies/%s", name);
		lumpnum = W_CheckNumForFullNamePK3(fullname, wadnum, 0);
		if (lumpnum != INT16_MAX)
			return (wadnum << 16) + lumpnum;
	}

	return LUMPERROR;
}

static void CacheMovieLump(movie_t *movie, const char *name)
{
	lumpnum_t lumpnum;

	lumpnum = FindMovieLumpNum(name);
	if (lumpnum == LUMPERROR)
		I_Error("FFmpeg: cannot find movie lump");

	movie->lumpsize = W_LumpLength(lumpnum);
	movie->lumpdata = malloc(movie->lumpsize);
	if (!movie->lumpdata)
		I_Error("FFmpeg: cannot allocate lump data");
	W_ReadLump(lumpnum, movie->lumpdata);
}

static int ReadStream(void *owner, uint8_t *buffer, int buffersize)
{
	movie_t *movie = owner;
	size_t bs = buffersize;
	buffersize = min(bs, movie->lumpsize - movie->lumpposition);
	memcpy(buffer, &movie->lumpdata[movie->lumpposition], buffersize);
	movie->lumpposition += buffersize;
	return buffersize;
}

static INT64 SeekStream(void *owner, int64_t offset, int whence)
{
	movie_t *movie = owner;

	if (whence == SEEK_CUR)
		offset += movie->lumpposition;
	else if (whence == SEEK_END)
		offset += movie->lumpsize;
	else if (whence == AVSEEK_SIZE)
		return movie->lumpsize;

	movie->lumpposition = offset;

	return 0;
}

static void InitialiseDemuxing(movie_t *movie)
{
	movie->formatcontext = avformat_alloc_context();
	if (!movie->formatcontext)
		I_Error("FFmpeg: cannot allocate format context");

	UINT8 *streambuffer = av_malloc(IO_BUFFER_SIZE);
	if (!streambuffer)
		I_Error("FFmpeg: cannot allocate stream buffer");
	movie->lumpposition = 0;

	movie->formatcontext->pb = avio_alloc_context(streambuffer, IO_BUFFER_SIZE, 0, movie, ReadStream, NULL, SeekStream);
	if (!movie->formatcontext->pb)
		I_Error("FFmpeg: cannot allocate I/O context");

	if (avformat_open_input(&movie->formatcontext, NULL, NULL, NULL) != 0)
		I_Error("FFmpeg: cannot open format context");

	if (avformat_find_stream_info(movie->formatcontext, NULL) < 0)
		I_Error("FFmpeg: cannot find stream information");

	movie->videostream.index = av_find_best_stream(movie->formatcontext, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
	if (movie->videostream.index < 0)
		I_Error("FFmpeg: cannot find video stream");
	movie->videostream.stream = movie->formatcontext->streams[movie->videostream.index];

	movie->audiostream.index = av_find_best_stream(movie->formatcontext, AVMEDIA_TYPE_AUDIO, -1, -1, NULL, 0);
	if (movie->audiostream.index >= 0)
		movie->audiostream.stream = movie->formatcontext->streams[movie->audiostream.index];
}

static AVCodecContext *InitialiseDecoding(AVStream *stream)
{
	const AVCodec *codec;
	AVCodecContext *codeccontext;

	if (!stream)
		return NULL;

	codec = avcodec_find_decoder(stream->codecpar->codec_id);
	if (!codec)
		I_Error("FFmpeg: cannot find codec");

	codeccontext = avcodec_alloc_context3(codec);
	if (!codeccontext)
		I_Error("FFmpeg: cannot allocate codec context");

	if (avcodec_parameters_to_context(codeccontext, stream->codecpar) < 0)
		I_Error("FFmpeg: cannot copy parameters to codec context");

	if (avcodec_open2(codeccontext, codec, NULL) < 0)
		I_Error("FFmpeg: cannot open codec");

	return codeccontext;
}

static void InitialiseVideoBuffer(movie_t *movie)
{
	moviestream_t *stream = &movie->videostream;
	AVCodecContext *context = stream->codeccontext;

	stream->numplanes = 1;

	AVRational *fps = &stream->stream->avg_frame_rate;
	InitialiseBuffer(
		&stream->buffer,
		(INT64)STREAM_BUFFER_TIME / TICRATE * fps->num / fps->den,
		sizeof(movievideoframe_t)
	);

	CloneBuffer(&stream->framequeue, &stream->buffer);
	CloneBuffer(&stream->framepool, &stream->buffer);

	for (INT32 i = 0; i < stream->framepool.capacity; i++)
	{
		movievideoframe_t *frame = EnqueueBuffer(&stream->framepool);

		frame->imagedatasize = av_image_alloc(
			frame->imagedata, frame->imagelinesize,
			context->width, context->height,
			AV_PIX_FMT_RGBA, 1
		);
		if (frame->imagedatasize < 0)
			I_Error("FFmpeg: cannot allocate image");

		INT32 size = context->width * (4 + Movie_GetBytesPerPatchColumn(movie));
		frame->patchdata = malloc(size);
		if (!frame->patchdata)
			I_Error("FFmpeg: cannot allocate patch data");
	}
}

static void InitialiseAudioBuffer(movie_t *movie)
{
	moviestream_t *stream = &movie->audiostream;

	if (!stream->stream)
		return;

	stream->numplanes = av_sample_fmt_is_planar(AV_SAMPLE_FMT_S16) ? stream->codeccontext->channels : 1;

	InitialiseBuffer(
		&stream->buffer,
		STREAM_BUFFER_TIME / TICRATE * stream->codeccontext->sample_rate / movie->frame->nb_samples,
		sizeof(movieaudioframe_t)
	);

	CloneBuffer(&stream->framequeue, &stream->buffer);
	CloneBuffer(&stream->framepool, &stream->buffer);

	for (INT32 i = 0; i < stream->framepool.capacity; i++)
	{
		movieaudioframe_t *frame = EnqueueBuffer(&stream->framepool);

		if (!av_samples_alloc(
			frame->samples, NULL,
			movie->frame->channels, movie->frame->nb_samples,
			AV_SAMPLE_FMT_S16, 1
		))
			I_Error("FFmpeg: cannot allocate samples");
	}
}

static void UninitialiseMovieStream(movie_t *movie, moviestream_t *stream)
{
	while (stream->buffer.size > 0)
		DequeueBufferIntoBuffer(&stream->framepool, &stream->buffer);
	while (stream->framequeue.size > 0)
		DequeueBufferIntoBuffer(&stream->framepool, &stream->framequeue);

	while (stream->framepool.size > 0)
	{
		if (stream == &movie->videostream)
		{
			movievideoframe_t *frame = DequeueBuffer(&stream->framepool);
			av_freep(&frame->imagedata[0]);
			free(frame->patchdata);
		}
		else if (stream == &movie->audiostream)
		{
			movieaudioframe_t *frame = DequeueBuffer(&stream->framepool);
			av_freep(&frame->samples[0]);
		}
	}

	UninitialiseBuffer(&stream->buffer);
	UninitialiseBuffer(&stream->framepool);
	UninitialiseBuffer(&stream->framequeue);

	avcodec_free_context(&stream->codeccontext);
}

static void SendPacket(movie_t *movie)
{
	moviestream_t *stream;
	AVPacket *packet;

	if (movie->packetqueue.size == 0)
		return;

	AVPacket **packetslot = DequeueBufferIntoBuffer(&movie->packetpool, &movie->packetqueue);
	packet = *packetslot;

	if (packet->stream_index == movie->videostream.index)
		stream = &movie->videostream;
	else if (packet->stream_index == movie->audiostream.index)
		stream = &movie->audiostream;
	else
		I_Error("FFmpeg: unexpected packet");

	if (avcodec_send_packet(stream->codeccontext, packet) < 0)
		I_Error("FFmpeg: cannot send packet to the decoder");

	av_packet_unref(packet);
}

static boolean ReceiveFrame(movie_t *movie, moviestream_t *stream)
{
	int error = avcodec_receive_frame(stream->codeccontext, movie->frame);

	if (error == 0) // Frame received successfully
		return true;
	else if (error == AVERROR_EOF) // End of movie reached
		return false;
	else if (error == AVERROR(EAGAIN)) // More packets needed
		return false;
	else // Error
		I_Error("FFmpeg: cannot receive frame");
}

static void ConvertRGBAToPatch(movie_t *movie, movievideoframe_t *frame)
{
	INT32 width = movie->frame->width;
	INT32 height = movie->frame->height;
	UINT8 * restrict src = frame->imagedata[0];
	UINT8 * restrict dst = frame->patchdata;
	UINT16 *lut = movie->colorlut.table;
	INT32 stride = 4 * width;

	// Write column offsets
	INT32 bytespercolumn = Movie_GetBytesPerPatchColumn(movie);
	for (INT32 x = 0; x < width; x++)
		WRITEUINT32(dst, width * 4 + x * bytespercolumn + 3);

	for (INT32 x = 0; x < width; x++)
	{
		INT32 y = 0;
		UINT8 *srcptr = &src[4 * x];

		// Write posts
		while (y < height)
		{
			INT32 postend = min(y + 254, height);

			// Header
			WRITEUINT8(dst, y ? 254 : 0); // Top delta
			WRITEUINT8(dst, postend - y); // Length
			WRITEUINT8(dst, 0); // Unused

			// Pixel data
			while (y < postend)
			{
				UINT8 r = srcptr[0];
				UINT8 g = srcptr[1];
				UINT8 b = srcptr[2];
				UINT8 i = lut[CLUTINDEX(r, g, b)];
				WRITEUINT8(dst, i);

				srcptr += stride;
				y++;
			}

			// Unused trail byte
			WRITEUINT8(dst, 0);
		}

		// Terminate column
		WRITEUINT8(dst, 0xFF);
	}
}

static void ParseVideoFrame(movie_t *movie)
{
	movievideoframe_t *frame = DequeueBufferIntoBuffer(&movie->videostream.framequeue, &movie->videostream.framepool);

	static UINT64 nextid = 1;
	frame->id = nextid;
	nextid++;

	frame->pts = movie->frame->pts;

	sws_scale(
		movie->scalingcontext,
		(UINT8 const * const *)movie->frame->data,
		movie->frame->linesize,
		0,
		movie->frame->height,
		frame->imagedata,
		frame->imagelinesize
	);

	if (movie->usepatches)
		ConvertRGBAToPatch(movie, frame);
}

static void ParseAudioFrame(movie_t *movie)
{
	movieaudioframe_t *frame;

	if (!movie->audiostream.framequeue.data)
		InitialiseAudioBuffer(movie);

	frame = DequeueBufferIntoBuffer(&movie->audiostream.framequeue, &movie->audiostream.framepool);

	frame->pts = movie->frame->pts;
	frame->numsamples = movie->frame->nb_samples;

	if (swr_convert(
		movie->resamplingcontext,
		frame->samples,
		movie->frame->nb_samples,
		(UINT8 const **)movie->frame->data,
		movie->frame->nb_samples
	) < 0)
		I_Error("FFmpeg: cannot convert audio frames");
}

static void FlushStream(movie_t *movie, moviestream_t *stream)
{
	// Flush the decoder
	if (avcodec_send_packet(stream->codeccontext, NULL) < 0)
		I_Error("FFmpeg: cannot flush decoder");

	while (true) {
		int error = avcodec_receive_frame(stream->codeccontext, movie->frame);
		if (error == 0)
			continue;
		else if (error == AVERROR_EOF)
			break;
		else
			I_Error("FFmpeg: cannot receive frame");
	}

	avcodec_flush_buffers(stream->codeccontext);
}

static void FlushDecoding(movie_t *movie)
{
	FlushStream(movie, &movie->videostream);
	FlushStream(movie, &movie->audiostream);

	I_lock_mutex(&movie->mutex);
	movie->flushing = false;
	I_unlock_mutex(movie->mutex);
}

static void DecoderThread(movie_t *movie)
{
	I_lock_mutex(&movie->condmutex);

	while (true)
	{
		boolean stopping;
		INT64 flushing;
		boolean queuesfull;

		I_lock_mutex(&movie->mutex);
		{
			stopping = movie->stopping;
			flushing = movie->flushing;

			INT32 vsize = movie->videostream.framepool.size;
			INT32 asize = movie->audiostream.framepool.size;
			queuesfull = (vsize == 0 || (movie->audiostream.framequeue.data && asize == 0));
		}
		I_unlock_mutex(movie->mutex);

		if (stopping)
			break;
		if (flushing)
			FlushDecoding(movie);
		if (queuesfull)
		{
			I_hold_cond(&movie->cond, movie->condmutex);
			continue;
		}

		if (ReceiveFrame(movie, &movie->videostream))
		{
			I_lock_mutex(&movie->mutex);
			ParseVideoFrame(movie);
			I_unlock_mutex(movie->mutex);
		}
		else if (ReceiveFrame(movie, &movie->audiostream))
		{
			I_lock_mutex(&movie->mutex);
			ParseAudioFrame(movie);
			I_unlock_mutex(movie->mutex);
		}
		else
		{
			boolean sent = false;

			I_lock_mutex(&movie->mutex);
			{
				if (movie->packetqueue.size > 0)
				{
					SendPacket(movie);
					sent = true;
				}
			}
			I_unlock_mutex(movie->mutex);

			if (!sent)
				I_hold_cond(&movie->cond, movie->condmutex);
		}
	}

	I_lock_mutex(&movie->mutex);
	movie->stopping = false;
	I_unlock_mutex(movie->mutex);
}

static boolean ShouldSeek(movie_t *movie)
{
	moviebuffer_t *buffer = &movie->videostream.buffer;

	if (movie->flushing || buffer->size == 0)
		return false;

	movievideoframe_t *firstframe = GetBufferSlot(buffer, 0);
	movievideoframe_t *lastframe = GetBufferSlot(buffer, buffer->size - 1);
	INT64 ahead = VideoPTSToMS(movie, firstframe->pts) - TicsToMS(movie->position);
	INT64 behind = TicsToMS(movie->position) - VideoPTSToMS(movie, lastframe->pts);
	return (behind > 1000 || ahead > 1000);
}

static void PollFrameQueue(movie_t *movie, moviestream_t *stream)
{
	while (stream->framequeue.size != 0)
	{
		DequeueBufferIntoBuffer(&stream->buffer, &stream->framequeue);
		I_wake_one_cond(&movie->cond);
	}
}

static void UpdateSeeking(movie_t *movie)
{
	I_lock_mutex(&movie->mutex);
	{
		if (ShouldSeek(movie))
		{
			movie->flushing = true;

			ClearAllFrames(movie);

			while (movie->packetqueue.size != 0)
				DequeueBufferIntoBuffer(&movie->packetpool, &movie->packetqueue);
			while (movie->videostream.framequeue.size != 0)
				DequeueBufferIntoBuffer(&movie->videostream.framepool, &movie->videostream.framequeue);
			while (movie->audiostream.framequeue.size != 0)
				DequeueBufferIntoBuffer(&movie->audiostream.framepool, &movie->audiostream.framequeue);

			avformat_seek_file(
				movie->formatcontext,
				movie->videostream.index,
				TicsToVideoPTS(movie, movie->position - 3 * TICRATE),
				TicsToVideoPTS(movie, movie->position),
				TicsToVideoPTS(movie, movie->position + TICRATE / 2),
				0
			);

			I_wake_one_cond(&movie->cond);
		}

		if (llabs(AudioPTSToMS(movie, movie->audioposition) - TicsToMS(movie->position)) > 200)
			movie->audioposition = TicsToAudioPTS(movie, movie->position);
	}
	I_unlock_mutex(movie->mutex);
}

static boolean ReadPacket(movie_t *movie)
{
	AVPacket **packetslot = PeekBuffer(&movie->packetpool);
	AVPacket *packet = *packetslot;

	int error = av_read_frame(movie->formatcontext, packet);

	if (error == AVERROR_EOF)
		return false;
	else if (error < 0)
		I_Error("FFmpeg: cannot read packet");
	else if (packet->stream_index == movie->videostream.index
		|| packet->stream_index == movie->audiostream.index)
	{
		DequeueBufferIntoBuffer(&movie->packetqueue, &movie->packetpool);
		I_wake_one_cond(&movie->cond);
	}
	else
		av_packet_unref(packet);

	return true;
}

movie_t *MovieDecode_Play(const char *name, boolean usepatches)
{
	movie_t *movie;

	movie = calloc(1, sizeof(*movie));
	if (!movie)
		I_Error("FFmpeg: cannot allocate movie object");

	CacheMovieLump(movie, name);
	InitialiseDemuxing(movie);
	movie->videostream.codeccontext = InitialiseDecoding(movie->videostream.stream);
	movie->audiostream.codeccontext = InitialiseDecoding(movie->audiostream.stream);
	InitialiseVideoBuffer(movie);

	InitialiseBuffer(&movie->packetqueue, 32, sizeof(AVPacket*));
	CloneBuffer(&movie->packetpool, &movie->packetqueue);
	for (INT32 i = 0; i < movie->packetpool.capacity; i++)
	{
		AVPacket *packet = av_packet_alloc();
		if (!packet)
			I_Error("FFmpeg: cannot allocate packet");

		AVPacket **packetslot = EnqueueBuffer(&movie->packetpool);
		*packetslot = packet;
	}

	movie->frame = av_frame_alloc();
	if (!movie->frame)
		I_Error("FFmpeg: cannot allocate frame");

	int width = movie->videostream.codeccontext->width;
	int height = movie->videostream.codeccontext->height;
	movie->scalingcontext = sws_getContext(
		width, height, movie->videostream.codeccontext->pix_fmt,
		width, height, AV_PIX_FMT_RGBA,
		SWS_BILINEAR,
		NULL,
		NULL,
		NULL
	);
	if (!movie->scalingcontext)
		I_Error("FFmpeg: cannot create scaling context");

	AVCodecContext *audiocodeccontext = movie->audiostream.codeccontext;
	movie->resamplingcontext = swr_alloc_set_opts(
		NULL,
		audiocodeccontext->channel_layout, AV_SAMPLE_FMT_S16, 44100,
		audiocodeccontext->channel_layout, audiocodeccontext->sample_fmt, audiocodeccontext->sample_rate,
		0, NULL
	);
	if (!movie->resamplingcontext)
		I_Error("FFmpeg: cannot allocate resampling context");
	if (swr_init(movie->resamplingcontext))
		I_Error("FFmpeg: cannot initialise resampling context");

	InitColorLUT(&movie->colorlut, pMasterPalette, true);

	movie->lastvideoframeusedid = 0;
	movie->position = 0;
	movie->audioposition = 0;
	movie->usepatches = usepatches;

	I_spawn_thread("decode-movie", (I_thread_fn)DecoderThread, movie);

	return movie;
}

void MovieDecode_Stop(movie_t **movieptr)
{
	movie_t *movie = *movieptr;

	if (!movie)
		return;

	I_lock_mutex(&movie->mutex);
	movie->stopping = true;
	I_unlock_mutex(movie->mutex);

	I_wake_one_cond(&movie->cond);

	boolean stopping;
	do
	{
		I_lock_mutex(&movie->mutex);
		stopping = movie->stopping;
		I_unlock_mutex(movie->mutex);
	} while (stopping);

	S_StopMusic();

	UninitialiseMovieStream(movie, &movie->videostream);
	UninitialiseMovieStream(movie, &movie->audiostream);

	av_freep(&movie->formatcontext->pb->buffer);
	avio_context_free(&movie->formatcontext->pb);
	free(movie->lumpdata);

	avformat_close_input(&movie->formatcontext);

	sws_freeContext(movie->scalingcontext);
	swr_free(&movie->resamplingcontext);

	while (movie->packetqueue.size > 0)
		DequeueBufferIntoBuffer(&movie->packetpool, &movie->packetqueue);
	while (movie->packetpool.size > 0)
		av_packet_free(DequeueBuffer(&movie->packetpool));
	UninitialiseBuffer(&movie->packetpool);
	UninitialiseBuffer(&movie->packetqueue);

	av_frame_free(&movie->frame);

	free(movie);
	*movieptr = NULL;
}

void MovieDecode_Seek(movie_t *movie, tic_t tic)
{
	movie->position = tic;
}

void MovieDecode_Update(movie_t *movie)
{
	I_lock_mutex(&movie->mutex);
	{
		while (movie->packetpool.size > 0 && ReadPacket(movie))
			;

		if (!movie->flushing)
		{
			PollFrameQueue(movie, &movie->videostream);
			PollFrameQueue(movie, &movie->audiostream);
		}
	}
	I_unlock_mutex(movie->mutex);

	UpdateSeeking(movie);

	if (movie->videostream.buffer.size > 0)
	{
		ClearOldVideoFrames(movie);
		ClearOldAudioFrames(movie);
	}
}

tic_t MovieDecode_GetDuration(movie_t *movie)
{
	return PTSToTics(movie->formatcontext->duration);
}

void MovieDecode_GetDimensions(movie_t *movie, INT32 *width, INT32 *height)
{
	AVCodecContext *context = movie->videostream.codeccontext;
	*width = context->width;
	*height = context->height;
}

UINT8 *MovieDecode_GetImage(movie_t *movie)
{
	INT32 bufferindex = FindVideoBufferIndexForPosition(movie, TicsToVideoPTS(movie, movie->position));
	movievideoframe_t *frame = GetBufferSlot(&movie->videostream.buffer, bufferindex);

	if (frame && movie->lastvideoframeusedid != frame->id)
	{
		movie->lastvideoframeusedid = frame->id;
		return movie->usepatches ? frame->patchdata : frame->imagedata[0];
	}
	else
	{
		return NULL;
	}
}

INT32 Movie_GetBytesPerPatchColumn(movie_t *movie)
{
	INT32 height = movie->videostream.codeccontext->height;
	INT32 numpostspercolumn = (height + 253) / 254;
	return height + numpostspercolumn * 4 + 1;
}

void MovieDecode_CopyAudioSamples(movie_t *movie, void *mem, size_t size)
{
	moviestream_t *stream = &movie->audiostream;
	moviebuffer_t *buffer = &stream->buffer;
	AVCodecContext *codeccontext = stream->codeccontext;
	UINT8 *membytes = mem;

	// Here, if using packed audio, the sample size includes both channels
	INT32 samplesize = av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
	if (!av_sample_fmt_is_planar(AV_SAMPLE_FMT_S16))
		samplesize *= codeccontext->channels;
	INT64 numsamples = size / samplesize;

	INT32 startbufferindex = FindAudioBufferIndexForPosition(movie, movie->audioposition);
	INT32 endbufferindex = FindAudioBufferIndexForPosition(movie, movie->audioposition + SamplesToPTS(movie, numsamples));
	size_t mempos = 0;

	if (startbufferindex != -1 && endbufferindex != -1)
	{
		INT32 i;
		for (i = startbufferindex; i <= endbufferindex; i++)
		{
			movieaudioframe_t *frame = GetBufferSlot(buffer, i);
			INT32 startsample = PTSToSamples(movie, max(movie->audioposition - frame->pts, 0));
			INT32 sizeforframe = min(size, (frame->numsamples - startsample) * samplesize);
			void *ptr = &frame->samples[0][startsample * samplesize];

			memcpy(&membytes[mempos], ptr, sizeforframe);
			mempos += sizeforframe;
			size -= sizeforframe;
		}
	}

	movie->audioposition += SamplesToPTS(movie, numsamples);
}
