#ifndef FX_COMMON_HPP
#define FX_COMMON_HPP

#include <stdint.h>
#include "fx_format.h"

#ifndef INT8_MIN
#define INT8_MIN    (-0x7f - 1)
#endif
#ifndef INT8_MAX
#define INT8_MAX    0x7f
#endif

#ifndef UINT8_MAX
#define UINT8_MAX   0xff
#endif

#ifndef INT16_MIN
#define INT16_MIN   (-0x7fff - 1)
#endif
#ifndef INT16_MAX
#define INT16_MAX   0x7fff
#endif

#ifndef UINT16_MAX
#define UINT16_MAX  0xffff
#endif

#ifndef INT32_MIN
#define INT32_MIN   (-0x7fffffff - 1)
#endif

#ifndef INT32_MAX
#define INT32_MAX   0x7fffffff
#endif

#define MAX_CHANNELS    10

// Float32-LE
static float getFloatLSBSample(uint8_t *raw, int c)
{
    union
    {
        float f;
        uint32_t r;
    } o;
    raw += (c * sizeof(float));
    o.r = (((uint32_t)raw[0] <<  0) & 0x000000FF) |
          (((uint32_t)raw[1] <<  8) & 0x0000FF00) |
          (((uint32_t)raw[2] << 16) & 0x00FF0000) |
          (((uint32_t)raw[3] << 24) & 0xFF000000);
    return o.f;
}
static void setFloatLSBSample(uint8_t **raw, float ov)
{
    void *t = &ov;
    uint32_t r = *(uint32_t*)t;
    *(*raw)++ = (uint8_t)((r >> 0) & 0xFF);
    *(*raw)++ = (uint8_t)((r >> 8) & 0xFF);
    *(*raw)++ = (uint8_t)((r >> 16) & 0xFF);
    *(*raw)++ = (uint8_t)((r >> 24) & 0xFF);
}

// Float32-BE
static float getFloatMSBSample(uint8_t *raw, int c)
{
    union
    {
        float f;
        uint32_t r;
    } o;
    raw += (c * sizeof(float));
    o.r = (((uint32_t)raw[3] <<  0) & 0x000000FF) |
          (((uint32_t)raw[2] <<  8) & 0x0000FF00) |
          (((uint32_t)raw[1] << 16) & 0x00FF0000) |
          (((uint32_t)raw[0] << 24) & 0xFF000000);
    return o.f;
}
static void setFloatMSBSample(uint8_t **raw, float ov)
{
    void *t = &ov;
    uint32_t r = *(uint32_t*)t;
    *(*raw)++ = (uint8_t)((r >> 24) & 0xFF);
    *(*raw)++ = (uint8_t)((r >> 16) & 0xFF);
    *(*raw)++ = (uint8_t)((r >> 8) & 0xFF);
    *(*raw)++ = (uint8_t)((r >> 0) & 0xFF);
}

// int32_t-LE
static float getInt32LSB(uint8_t *raw, int c)
{
    uint32_t r;
    int32_t f;
    raw += (c * sizeof(int32_t));
    r = (((uint32_t)raw[0] <<  0) & 0x000000FF) |
        (((uint32_t)raw[1] <<  8) & 0x0000FF00) |
        (((uint32_t)raw[2] << 16) & 0x00FF0000) |
        (((uint32_t)raw[3] << 24) & 0xFF000000);
    f = *(int32_t*)(&r);
    return (float)((double)f / INT32_MAX);
}
static void setInt32LSB(uint8_t **raw, float ov)
{
    int32_t f = ((int32_t)(double(ov) * INT32_MAX));
    uint32_t r = *(uint32_t*)(&f);
    *(*raw)++ = (uint8_t)((r >> 0) & 0xFF);
    *(*raw)++ = (uint8_t)((r >> 8) & 0xFF);
    *(*raw)++ = (uint8_t)((r >> 16) & 0xFF);
    *(*raw)++ = (uint8_t)((r >> 24) & 0xFF);
}

// int32_t-BE
static float getInt32MSB(uint8_t *raw, int c)
{
    uint32_t r;
    int32_t f;
    raw += (c * sizeof(int32_t));
    r = (((uint32_t)raw[3] <<  0) & 0x000000FF) |
        (((uint32_t)raw[2] <<  8) & 0x0000FF00) |
        (((uint32_t)raw[1] << 16) & 0x00FF0000) |
        (((uint32_t)raw[0] << 24) & 0xFF000000);
    f = *(int32_t*)(&r);
    return (float)((double)f / INT32_MAX);
}
static void setInt32MSB(uint8_t **raw, float ov)
{
    int32_t f = (int32_t(double(ov) * INT32_MAX));
    uint32_t r = *(uint32_t*)(&f);
    *(*raw)++ = (uint8_t)((r >> 24) & 0xFF);
    *(*raw)++ = (uint8_t)((r >> 16) & 0xFF);
    *(*raw)++ = (uint8_t)((r >> 8) & 0xFF);
    *(*raw)++ = (uint8_t)((r >> 0) & 0xFF);
}

// int16_t-LE
static float getInt16LSB(uint8_t *raw, int c)
{
    uint16_t r;
    int16_t f;
    raw += (c * sizeof(int16_t));
    r = (((uint16_t)raw[0] <<  0) & 0x00FF) |
        (((uint16_t)raw[1] <<  8) & 0xFF00);
    f = *(int16_t*)(&r);
    return (float)f / INT16_MAX;
}
static void setInt16LSB(uint8_t **raw, float ov)
{
    int16_t f = int16_t(ov * INT16_MAX);
    uint16_t r = *(uint16_t*)(&f);
    *(*raw)++ = (uint8_t)((r >> 0) & 0xFF);
    *(*raw)++ = (uint8_t)((r >> 8) & 0xFF);
}

// int16_t-BE
static float getInt16MSB(uint8_t *raw, int c)
{
    uint16_t r;
    int16_t f;
    raw += (c * sizeof(int16_t));
    r = (((uint16_t)raw[1] <<  0) & 0x00FF) |
        (((uint16_t)raw[0] <<  8) & 0xFF00);
    f = *(int16_t*)(&r);
    return (float)f / INT16_MIN;
}
static void setInt16MSB(uint8_t **raw, float ov)
{
    int16_t f = int16_t(ov * INT16_MAX);
    uint16_t r = *(uint16_t*)(&f);
    *(*raw)++ = (uint8_t)((r >> 8) & 0xFF);
    *(*raw)++ = (uint8_t)((r >> 0) & 0xFF);
}

// uint16_t-LE
static float getuint16_tLSB(uint8_t *raw, int c)
{
    uint16_t r;
    float f;
    raw += (c * sizeof(uint16_t));
    r = (((uint16_t)raw[0] <<  0) & 0x00FF) |
        (((uint16_t)raw[1] <<  8) & 0xFF00);
    f = ((float)r + INT16_MIN) / INT16_MAX;
    return f;
}
static void setuint16_tLSB(uint8_t **raw, float ov)
{
    int16_t f = int16_t((ov * INT16_MAX) - INT16_MIN);
    uint16_t r = *(uint16_t*)(&f);
    *(*raw)++ = (uint8_t)((r >> 0) & 0xFF);
    *(*raw)++ = (uint8_t)((r >> 8) & 0xFF);
}

// uint16_t-BE
static float getuint16_tMSB(uint8_t *raw, int c)
{
    uint16_t r;
    float f;
    raw += (c * sizeof(uint16_t));
    r = (((uint16_t)raw[1] <<  0) & 0x00FF) |
        (((uint16_t)raw[0] <<  8) & 0xFF00);
    f = ((float)r + INT16_MIN) / INT16_MAX;
    return f;
}
static void setuint16_tMSB(uint8_t **raw, float ov)
{
    int16_t f = int16_t((ov * INT16_MAX) - INT16_MIN);
    uint16_t r = *(uint16_t*)(&f);
    *(*raw)++ = (uint8_t)((r >> 8) & 0xFF);
    *(*raw)++ = (uint8_t)((r >> 0) & 0xFF);
}

// int8_t
static float getInt8(uint8_t *raw, int c)
{
    float f;
    raw += (c * sizeof(int8_t));
    f = (float)(int8_t)(*raw) / INT8_MAX;
    return f;
}
static void setInt8(uint8_t **raw, float ov)
{
    *(*raw)++ = (uint8_t)(ov * INT8_MAX);
}

// uint8_t
static float getuint8_t(uint8_t *raw, int c)
{
    float f;
    raw += (c * sizeof(int8_t));
    f = (float)((int)*raw + INT8_MIN) / INT8_MAX;
    return f;
}
static void setuint8_t(uint8_t **raw, float ov)
{
    *(*raw)++ = (uint8_t)((ov * INT8_MAX) - INT8_MIN);
}


typedef float (*ReadSampleCB)(uint8_t* raw, int channel);
typedef  void (*WriteSampleCB)(uint8_t** raw, float sample);

static bool initFormat(ReadSampleCB &readSample,
                       WriteSampleCB &writeSample,
                       int &sample_size, uint16_t format)
{
    switch(format)
    {
    case AUDIO_U8:
        readSample = getuint8_t;
        writeSample = setuint8_t;
        sample_size = sizeof(uint8_t);
        break;

    case AUDIO_S8:
        readSample = getInt8;
        writeSample = setInt8;
        sample_size = sizeof(int8_t);
        break;

    case AUDIO_S16LSB:
        readSample = getInt16LSB;
        writeSample = setInt16LSB;
        sample_size = sizeof(int16_t);
        break;

    case AUDIO_S16MSB:
        readSample = getInt16MSB;
        writeSample = setInt16MSB;
        sample_size = sizeof(int16_t);
        break;

    case AUDIO_U16LSB:
        readSample = getuint16_tLSB;
        writeSample = setuint16_tLSB;
        sample_size = sizeof(uint16_t);
        break;

    case AUDIO_U16MSB:
        readSample = getuint16_tMSB;
        writeSample = setuint16_tMSB;
        sample_size = sizeof(uint16_t);
        break;

    case AUDIO_S32LSB:
        readSample = getInt32LSB;
        writeSample = setInt32LSB;
        sample_size = sizeof(int32_t);
        break;

    case AUDIO_S32MSB:
        readSample = getInt32MSB;
        writeSample = setInt32MSB;
        sample_size = sizeof(int32_t);
        break;

    case AUDIO_F32LSB:
        readSample = getFloatLSBSample;
        writeSample = setFloatLSBSample;
        sample_size = sizeof(float);
        break;

    case AUDIO_F32MSB:
        readSample = getFloatMSBSample;
        writeSample = setFloatMSBSample;
        sample_size = sizeof(float);
        break;

    default:
        return false; /* Unsupported format */
    }

    return true;
}


#endif // FX_COMMON_HPP
