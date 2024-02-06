/* WAVE sound file writer for recording 16-bit output during program development */

#pragma once
#ifndef WAVE_WRITER_H
#define WAVE_WRITER_H

#ifdef __cplusplus
extern "C" {
#endif

#define WAVE_FORMAT_PCM         0x0001
#define WAVE_FORMAT_IEEE_FLOAT  0x0003

void *ctx_wave_open(int chans_count,
                    long sample_rate,
                    int sample_size,
                    int sample_format,
                    int has_sign,
                    int is_big_endian,
                    const char *filename);

void ctx_wave_write(void *ctx, const unsigned char *in, long count);
long ctx_wave_sample_count(void *ctx);
void ctx_wave_close(void *ctx);

#ifdef __cplusplus
}
#endif

#endif
