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

#ifndef REVERB_H
#define REVERB_H

#include <stdint.h>
#include "fx_format.h"

struct FxReverb;

struct ReverbSetup
{
    float mode         = 0.0f; // Normal (0) or Freeze (>0.5)
    float roomSize     = 0.7f;
    float damping      = 0.5f; // 0.0...1.0
    float wetLevel     = 0.2f;
    float dryLevel     = 0.4f;
    float width        = 1.0f; // 0.0...1.0
};

extern FxReverb *reverbEffectInit(int rate, uint16_t format, int channels);
extern void reverbEffectFree(FxReverb *context);

extern void reverbEffect(int chan, void *stream, int len, void *context);

// Update all setup at once
extern void reverbUpdateSetup(FxReverb *context, const ReverbSetup &setup);
extern void reverbGetSetup(FxReverb *context, ReverbSetup &setup);

// Update every single setting
extern void reverbUpdateMode(FxReverb *context, float mode);
extern void reverbUpdateRoomSize(FxReverb *context, float roomSize);
extern void reverbUpdateDamping(FxReverb *context, float damping);
extern void reverbUpdateWetLevel(FxReverb *context, float wet);
extern void reverbUpdateDryLevel(FxReverb *context, float dry);
extern void reverbUpdateWidth(FxReverb *context, float width);

#endif // REVERB_H
