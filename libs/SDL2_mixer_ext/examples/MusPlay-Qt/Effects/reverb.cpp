/*
 * Simple Reverb sound effect
 *
 * Copyright (c) 2022-2023 Vitaly Novichkov <admin@wohlnet.ru>
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <vector>
#include <deque>
#include <cmath>
#include <tgmath.h>
#include "reverb.h"
#include "fx_common.hpp"


// Code was taken from FreeVerb: https://github.com/sinshu/freeverb (Public Domain)

// Written by Jezar at Dreampoint, June 2000
// http://www.dreampoint.co.uk
// This code is public domain

const int   numcombs        = 8;
const int   numallpasses    = 4;
const float muted           = 0;
const float fixedgain       = 0.015f;
const float scalewet        = 3;
const float scaledry        = 2;
const float scaledamp       = 0.4f;
const float scaleroom       = 0.28f;
const float offsetroom      = 0.7f;
const float initialroom     = 0.5f;
const float initialdamp     = 0.5f;
const float initialwet      = 1 / scalewet;
const float initialdry      = 0;
const float initialwidth    = 1;
const float initialmode     = 0;
const float freezemode      = 0.5f;
const int   stereospread    = 23;

// These values assume 44.1KHz sample rate
// they will probably be OK for 48KHz sample rate
// but would need scaling for 96KHz (or other) sample rates.
// The values were obtained by listening tests.
const int combtuningL1      = 1116;
const int combtuningR1      = 1116 + stereospread;
const int combtuningL2      = 1188;
const int combtuningR2      = 1188 + stereospread;
const int combtuningL3      = 1277;
const int combtuningR3      = 1277 + stereospread;
const int combtuningL4      = 1356;
const int combtuningR4      = 1356 + stereospread;
const int combtuningL5      = 1422;
const int combtuningR5      = 1422 + stereospread;
const int combtuningL6      = 1491;
const int combtuningR6      = 1491 + stereospread;
const int combtuningL7      = 1557;
const int combtuningR7      = 1557 + stereospread;
const int combtuningL8      = 1617;
const int combtuningR8      = 1617 + stereospread;
const int allpasstuningL1   = 556;
const int allpasstuningR1   = 556 + stereospread;
const int allpasstuningL2   = 441;
const int allpasstuningR2   = 441 + stereospread;
const int allpasstuningL3   = 341;
const int allpasstuningR3   = 341 + stereospread;
const int allpasstuningL4   = 225;
const int allpasstuningR4   = 225 + stereospread;


static inline void undenormalise(float &sample)
{
    uint32_t i = *reinterpret_cast<uint32_t*>(&sample);
    if((i & 0x7f800000) == 0)
        sample = 0.0f;
}


class comb
{
public:
    comb()
    {
        filterstore = 0;
        bufidx = 0;
    }

    void setbuffer(float* buf, int size)
    {
        buffer = buf;
        bufsize = size;
    }

    // Big to inline - but crucial for speed
    inline float process(float input)
    {
        float output;

        output = buffer[bufidx];
        undenormalise(output);

        filterstore = (output * damp2) + (filterstore * damp1);
        undenormalise(filterstore);

        buffer[bufidx] = input + (filterstore * feedback);

        if(++bufidx >= bufsize) bufidx = 0;

        return output;
    }

    void mute()
    {
        for(int i = 0; i < bufsize; i++)
            buffer[i] = 0;
    }

    void setdamp(float val)
    {
        damp1 = val;
        damp2 = 1 - val;
    }

    float getdamp()
    {
        return damp1;
    }

    void setfeedback(float val)
    {
        feedback = val;
    }

    float getfeedback()
    {
        return feedback;
    }

private:
    float   feedback = 0.f;
    float   filterstore = 0.f;
    float   damp1 = 0.f;
    float   damp2 = 0.f;
    float*  buffer = nullptr;
    int     bufsize = 0;
    int     bufidx = 0;
};


class allpass
{
public:
    allpass()
    {
        bufidx = 0;
    }

    void setbuffer(float* buf, int size)
    {
        buffer = buf;
        bufsize = size;
    }

    // Big to inline - but crucial for speed
    inline float process(float input)
    {
        float output;
        float bufout;

        bufout = buffer[bufidx];
        undenormalise(bufout);

        output = -input + bufout;
        buffer[bufidx] = input + (bufout * feedback);

        if(++bufidx >= bufsize)
            bufidx = 0;

        return output;
    }

    void mute()
    {
        for(int i = 0; i < bufsize; i++)
            buffer[i] = 0;
    }

    void setfeedback(float val)
    {
        feedback = val;
    }

    float getfeedback()
    {
        return feedback;
    }

    // private:
    float   feedback = 0.f;
    float*  buffer = nullptr;
    int     bufsize = 0;
    int     bufidx = 0;
};


class revmodel
{
    double rateScale = 1.0;

public:
    void setSampleRate(int rate)
    {
        rateScale = rate / 44100.0;
        // Prepare buffers
        bufcombL1.resize(static_cast<size_t>(combtuningL1 * rateScale));
        bufcombR1.resize(static_cast<size_t>(combtuningR1 * rateScale));
        bufcombL2.resize(static_cast<size_t>(combtuningL2 * rateScale));
        bufcombR2.resize(static_cast<size_t>(combtuningR2 * rateScale));
        bufcombL3.resize(static_cast<size_t>(combtuningL3 * rateScale));
        bufcombR3.resize(static_cast<size_t>(combtuningR3 * rateScale));
        bufcombL4.resize(static_cast<size_t>(combtuningL4 * rateScale));
        bufcombR4.resize(static_cast<size_t>(combtuningR4 * rateScale));
        bufcombL5.resize(static_cast<size_t>(combtuningL5 * rateScale));
        bufcombR5.resize(static_cast<size_t>(combtuningR5 * rateScale));
        bufcombL6.resize(static_cast<size_t>(combtuningL6 * rateScale));
        bufcombR6.resize(static_cast<size_t>(combtuningR6 * rateScale));
        bufcombL7.resize(static_cast<size_t>(combtuningL7 * rateScale));
        bufcombR7.resize(static_cast<size_t>(combtuningR7 * rateScale));
        bufcombL8.resize(static_cast<size_t>(combtuningL8 * rateScale));
        bufcombR8.resize(static_cast<size_t>(combtuningR8 * rateScale));
        bufallpassL1.resize(static_cast<size_t>(allpasstuningL1 * rateScale));
        bufallpassR1.resize(static_cast<size_t>(allpasstuningR1 * rateScale));
        bufallpassL2.resize(static_cast<size_t>(allpasstuningL2 * rateScale));
        bufallpassR2.resize(static_cast<size_t>(allpasstuningR2 * rateScale));
        bufallpassL3.resize(static_cast<size_t>(allpasstuningL3 * rateScale));
        bufallpassR3.resize(static_cast<size_t>(allpasstuningR3 * rateScale));
        bufallpassL4.resize(static_cast<size_t>(allpasstuningL4 * rateScale));
        bufallpassR4.resize(static_cast<size_t>(allpasstuningR4 * rateScale));

        // Tie the components to their buffers
        combL[0].setbuffer(bufcombL1.data(), static_cast<int>(bufcombL1.size()));
        combR[0].setbuffer(bufcombR1.data(), static_cast<int>(bufcombR1.size()));
        combL[1].setbuffer(bufcombL2.data(), static_cast<int>(bufcombL2.size()));
        combR[1].setbuffer(bufcombR2.data(), static_cast<int>(bufcombR2.size()));
        combL[2].setbuffer(bufcombL3.data(), static_cast<int>(bufcombL3.size()));
        combR[2].setbuffer(bufcombR3.data(), static_cast<int>(bufcombR3.size()));
        combL[3].setbuffer(bufcombL4.data(), static_cast<int>(bufcombL4.size()));
        combR[3].setbuffer(bufcombR4.data(), static_cast<int>(bufcombR4.size()));
        combL[4].setbuffer(bufcombL5.data(), static_cast<int>(bufcombL5.size()));
        combR[4].setbuffer(bufcombR5.data(), static_cast<int>(bufcombR5.size()));
        combL[5].setbuffer(bufcombL6.data(), static_cast<int>(bufcombL6.size()));
        combR[5].setbuffer(bufcombR6.data(), static_cast<int>(bufcombR6.size()));
        combL[6].setbuffer(bufcombL7.data(), static_cast<int>(bufcombL7.size()));
        combR[6].setbuffer(bufcombR7.data(), static_cast<int>(bufcombR7.size()));
        combL[7].setbuffer(bufcombL8.data(), static_cast<int>(bufcombL8.size()));
        combR[7].setbuffer(bufcombR8.data(), static_cast<int>(bufcombR8.size()));
        allpassL[0].setbuffer(bufallpassL1.data(), static_cast<int>(bufallpassL1.size()));
        allpassR[0].setbuffer(bufallpassR1.data(), static_cast<int>(bufallpassR1.size()));
        allpassL[1].setbuffer(bufallpassL2.data(), static_cast<int>(bufallpassL2.size()));
        allpassR[1].setbuffer(bufallpassR2.data(), static_cast<int>(bufallpassR2.size()));
        allpassL[2].setbuffer(bufallpassL3.data(), static_cast<int>(bufallpassL3.size()));
        allpassR[2].setbuffer(bufallpassR3.data(), static_cast<int>(bufallpassR3.size()));
        allpassL[3].setbuffer(bufallpassL4.data(), static_cast<int>(bufallpassL4.size()));
        allpassR[3].setbuffer(bufallpassR4.data(), static_cast<int>(bufallpassR4.size()));
    }

    revmodel()
    {
        // Set default values
        allpassL[0].setfeedback(0.5f);
        allpassR[0].setfeedback(0.5f);
        allpassL[1].setfeedback(0.5f);
        allpassR[1].setfeedback(0.5f);
        allpassL[2].setfeedback(0.5f);
        allpassR[2].setfeedback(0.5f);
        allpassL[3].setfeedback(0.5f);
        allpassR[3].setfeedback(0.5f);
        setwet(initialwet);
        setroomsize(initialroom);
        setdry(initialdry);
        setdamp(initialdamp);
        setwidth(initialwidth);
        setmode(initialmode);

        // Buffer will be full of rubbish - so we MUST mute them
        mute();
    }

    void mute()
    {
        if(getmode() >= freezemode)
            return;

        for(int i = 0; i < numcombs; i++)
        {
            combL[i].mute();
            combR[i].mute();
        }

        for(int i = 0; i < numallpasses; i++)
        {
            allpassL[i].mute();
            allpassR[i].mute();
        }
    }

    void processmix(float* inputL, float* inputR, float* outputL, float* outputR, long numsamples, int skip)
    {
        float outL, outR, input;

        while(numsamples-- > 0)
        {
            outL = outR = 0;
            input = (*inputL + *inputR) * gain;

            // Accumulate comb filters in parallel
            for(int i = 0; i < numcombs; i++)
            {
                outL += combL[i].process(input);
                outR += combR[i].process(input);
            }

            // Feed through allpasses in series
            for(int i = 0; i < numallpasses; i++)
            {
                outL = allpassL[i].process(outL);
                outR = allpassR[i].process(outR);
            }

            // Calculate output MIXING with anything already there
            *outputL += outL * wet1 + outR * wet2 + *inputL * dry;
            *outputR += outR * wet1 + outL * wet2 + *inputR * dry;

            // Increment sample pointers, allowing for interleave (if any)
            inputL += skip;
            inputR += skip;
            outputL += skip;
            outputR += skip;
        }
    }

    void processreplace(float* inputL, float* inputR, float* outputL, float* outputR, long numsamples, int skip)
    {
        float outL, outR, input;

        while(numsamples-- > 0)
        {
            outL = outR = 0;
            input = (*inputL + *inputR) * gain;

            // Accumulate comb filters in parallel
            for(int i = 0; i < numcombs; i++)
            {
                outL += combL[i].process(input);
                outR += combR[i].process(input);
            }

            // Feed through allpasses in series
            for(int i = 0; i < numallpasses; i++)
            {
                outL = allpassL[i].process(outL);
                outR = allpassR[i].process(outR);
            }

            // Calculate output REPLACING anything already there
            *outputL = outL * wet1 + outR * wet2 + *inputL * dry;
            *outputR = outR * wet1 + outL * wet2 + *inputR * dry;

            // Increment sample pointers, allowing for interleave (if any)
            inputL += skip;
            inputR += skip;
            outputL += skip;
            outputR += skip;
        }
    }


    // The following get/set functions are not inlined, because
    // speed is never an issue when calling them, and also
    // because as you develop the reverb model, you may
    // wish to take dynamic action when they are called.

    void setroomsize(float value)
    {
        roomsize = (value * scaleroom) + offsetroom;
        update();
    }

    float getroomsize()
    {
        return (roomsize - offsetroom) / scaleroom;
    }

    void setdamp(float value)
    {
        damp = value * scaledamp;
        update();
    }

    float getdamp()
    {
        return damp / scaledamp;
    }

    void setwet(float value)
    {
        wet = value * scalewet;
        update();
    }

    float getwet()
    {
        return wet / scalewet;
    }

    void setdry(float value)
    {
        dry = value * scaledry;
    }

    float getdry()
    {
        return dry / scaledry;
    }

    void setwidth(float value)
    {
        width = value;
        update();
    }

    float getwidth()
    {
        return width;
    }

    void setmode(float value)
    {
        mode = value;
        update();
    }

    float getmode()
    {
        if(mode >= freezemode)
            return 1;
        else
            return 0;
    }

private:
    void update()
    {
        // Recalculate internal values after parameter change

        int i;

        wet1 = wet * (width / 2 + 0.5f);
        wet2 = wet * ((1 - width) / 2);

        if(mode >= freezemode)
        {
            roomsize1 = 1;
            damp1 = 0;
            gain = muted;
        }
        else
        {
            roomsize1 = roomsize;
            damp1 = damp;
            gain = fixedgain;
        }

        for(i = 0; i < numcombs; i++)
        {
            combL[i].setfeedback(roomsize1);
            combR[i].setfeedback(roomsize1);
        }

        for(i = 0; i < numcombs; i++)
        {
            combL[i].setdamp(damp1);
            combR[i].setdamp(damp1);
        }
    }

private:
    float   gain = 0.0f;
    float   roomsize = 0.0f, roomsize1 = 0.0f;
    float   damp = 0.0f, damp1 = 0.0f;
    float   wet = 0.0f, wet1 = 0.0f, wet2 = 0.0f;
    float   dry = 0.0f;
    float   width = 0.0f;
    float   mode = 0.0f;

    // The following are all declared inline
    // to remove the need for dynamic allocation
    // with its subsequent error-checking messiness

    // Comb filters
    comb    combL[numcombs];
    comb    combR[numcombs];

    // Allpass filters
    allpass allpassL[numallpasses];
    allpass allpassR[numallpasses];

    // Buffers for the combs
    std::vector<float>  bufcombL1;
    std::vector<float>  bufcombR1;
    std::vector<float>  bufcombL2;
    std::vector<float>  bufcombR2;
    std::vector<float>  bufcombL3;
    std::vector<float>  bufcombR3;
    std::vector<float>  bufcombL4;
    std::vector<float>  bufcombR4;
    std::vector<float>  bufcombL5;
    std::vector<float>  bufcombR5;
    std::vector<float>  bufcombL6;
    std::vector<float>  bufcombR6;
    std::vector<float>  bufcombL7;
    std::vector<float>  bufcombR7;
    std::vector<float>  bufcombL8;
    std::vector<float>  bufcombR8;

    // Buffers for the allpasses
    std::vector<float> bufallpassL1;
    std::vector<float> bufallpassR1;
    std::vector<float> bufallpassL2;
    std::vector<float> bufallpassR2;
    std::vector<float> bufallpassL3;
    std::vector<float> bufallpassR3;
    std::vector<float> bufallpassL4;
    std::vector<float> bufallpassR4;
};


struct FxReverb
{
    int         channels = 0;
    int         sampleRate = 0;
    uint16_t    format = AUDIO_F32LSB;
    bool        isValid = false;
    ReverbSetup m_setup;

    revmodel    rev[MAX_CHANNELS / 2];

    std::vector<std::vector<float>> inBuffer;
    std::vector<std::vector<float>> outBuffer;
    int                             lastBufferSize = 0;

    ReadSampleCB    readSample = nullptr;
    WriteSampleCB   writeSample = nullptr;
    int             sample_size = 2;

    int init(int i_rate, uint16_t i_format, int i_channels)
    {
        isValid = false;

        if(channels > MAX_CHANNELS)
            return -1;

        format = i_format;
        sampleRate = i_rate;
        channels = i_channels;

        if(!initFormat(readSample, writeSample, sample_size, format))
            return -1;

        for(int i = 0; i < channels; i += 2)
        {
            auto &c = rev[i / 2];
            c.setSampleRate(sampleRate);
        }

        setSettings(m_setup);

        inBuffer.clear();
        outBuffer.clear();
        inBuffer.resize(channels + (channels % 2));
        outBuffer.resize(channels + (channels % 2));
        lastBufferSize = 0;

        isValid = true;
        return 0;
    }

    void updateSetup(const ReverbSetup& setup)
    {
        m_setup = setup;
    }

    void getSetup(ReverbSetup& setup)
    {
        setup = m_setup;
    }

    void setSettings(const ReverbSetup& setup)
    {
        for(int i = 0; i < channels; i += 2)
        {
            auto &c = rev[i / 2];
            c.setroomsize(setup.roomSize);
            c.setdamp(setup.damping);
            c.setmode(setup.mode);
            c.setdry(setup.dryLevel);
            c.setwet(setup.wetLevel);
            c.setwidth(setup.width);
        }
    }

    void setMode(float val)
    {
        m_setup.mode = val;
        for(int i = 0; i < channels; i += 2)
        {
            auto &c = rev[i / 2];
            c.setmode(val);
        }
    }

    void setRoomSize(float val)
    {
        m_setup.roomSize = val;
        for(int i = 0; i < channels; i += 2)
        {
            auto &c = rev[i / 2];
            c.setroomsize(val);
        }
    }

    void setDamping(float val)
    {
        m_setup.damping = val;
        for(int i = 0; i < channels; i += 2)
        {
            auto &c = rev[i / 2];
            c.setdamp(val);
        }
    }

    void setWetLevel(float val)
    {
        m_setup.wetLevel = val;
        for(int i = 0; i < channels; i += 2)
        {
            auto &c = rev[i / 2];
            c.setwet(val);
        }
    }

    void setDryLevel(float val)
    {
        m_setup.dryLevel = val;
        for(int i = 0; i < channels; i += 2)
        {
            auto &c = rev[i / 2];
            c.setdry(val);
        }
    }

    void setWidth(float val)
    {
        m_setup.width = val;
        for(int i = 0; i < channels; i += 2)
        {
            auto &c = rev[i / 2];
            c.setwidth(val);
        }
    }

    void close()
    {
        isValid = false;
    }

    void process(uint8_t* stream, int len)
    {
        if(!isValid)
            return; // Do nothing

        int frames = len / (sample_size * channels);
        uint8_t* in_stream = stream;
        uint8_t* out_stream = stream;

        for(int i = 0; i < channels; i += 2)
        {
            if(inBuffer[i].size() < size_t(frames))
                inBuffer[i].resize(frames);
            if(inBuffer[i + 1].size() < size_t(frames))
                inBuffer[i + 1].resize(frames);
            if(outBuffer[i].size() < size_t(frames))
                outBuffer[i].resize(frames);
            if(outBuffer[i + 1].size() < size_t(frames))
                outBuffer[i + 1].resize(frames);
        }

        for(int i = 0; i < frames; ++i)
        {
            for(int c = 0; c < channels; ++c)
            {
                inBuffer[c][i] = readSample(in_stream, c);
                if(channels % 2 == 1 && c == channels - 1) // Mono to Stereo
                    inBuffer[c + 1][i] = inBuffer[c][i];
            }

            in_stream += sample_size * channels;
        }

        for(int i = 0; i < channels; i += 2)
        {
            auto &c = rev[i / 2];
            c.processreplace(inBuffer[i].data(), inBuffer[i + 1].data(),
                             outBuffer[i].data(), outBuffer[i + 1].data(), frames, 1);
        }

        for(int p = 0; p < frames; ++p)
        {
            for(int w = 0; w < channels; ++w)
            {
                if(channels % 2 == 1 && w == channels - 1) // Stereo to Mono
                    outBuffer[w][p] = (outBuffer[w][p] + outBuffer[w + 1][p]) / 2.0f;

                writeSample(&out_stream, outBuffer[w][p]);
            }
        }
    }
};


FxReverb* reverbEffectInit(int rate, uint16_t format, int channels)
{
    FxReverb* out = new FxReverb();
    out->init(rate, format, channels);
    return out;
}

void reverbEffectFree(FxReverb* context)
{
    if(context)
    {
        context->close();
        delete context;
    }
}

void reverbEffect(int, void* stream, int len, void* context)
{
    FxReverb* out = reinterpret_cast<FxReverb*>(context);

    if(!out)
        return; // Effect doesn't working

    out->process((uint8_t*)stream, len);
}

void reverbUpdateSetup(FxReverb* context, const ReverbSetup& setup)
{
    if(context)
    {
        context->setSettings(setup);
        context->updateSetup(setup);
    }
}

void reverbGetSetup(FxReverb *context, ReverbSetup &setup)
{
    if(context)
        context->getSetup(setup);
}

void reverbUpdateMode(FxReverb *context, float mode)
{
    if(context)
        context->setMode(mode);
}

void reverbUpdateRoomSize(FxReverb *context, float roomSize)
{
    if(context)
        context->setRoomSize(roomSize);
}

void reverbUpdateDamping(FxReverb *context, float damping)
{
    if(context)
        context->setDamping(damping);
}

void reverbUpdateWetLevel(FxReverb *context, float wet)
{
    if(context)
        context->setWetLevel(wet);
}

void reverbUpdateDryLevel(FxReverb *context, float dry)
{
    if(context)
        context->setDryLevel(dry);
}

void reverbUpdateWidth(FxReverb *context, float width)
{
    if(context)
        context->setWidth(width);
}
