/*
 * SPC Echo sound effect
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

#ifndef SPC_ECHO_HHHH
#define SPC_ECHO_HHHH

#include <stdint.h>
#include "fx_format.h"

struct SpcEcho;

extern SpcEcho *echoEffectInit(int rate, uint16_t format, int channels);
extern void echoEffectFree(SpcEcho *context);

enum EchoSetup
{
    ECHO_EON = 0,
    ECHO_EDL,
    ECHO_EFB,

    ECHO_MVOLL,
    ECHO_MVOLR,
    ECHO_EVOLL,
    ECHO_EVOLR,

    ECHO_FIR0,
    ECHO_FIR1,
    ECHO_FIR2,
    ECHO_FIR3,
    ECHO_FIR4,
    ECHO_FIR5,
    ECHO_FIR6,
    ECHO_FIR7
};

extern void spcEchoEffect(int chan, void *stream, int len, void *context);

extern void echoEffectResetFir(SpcEcho *out);
extern void echoEffectResetDefaults(SpcEcho *out);

extern void echoEffectSetReg(SpcEcho *out, EchoSetup key, int val);
extern int  echoEffectGetReg(SpcEcho *out, EchoSetup key);

#endif // SPC_ECHO_HHHH
