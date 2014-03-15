/*
 * Compiles date and time.
 *
 * Kalaron: Can't this be somewhere else instead of in an extra c file?
 * Alam: Code::Block, XCode and the Makefile touch this file to update
 *  the timestamp
 *
 */

#ifdef COMPVERSION
#include "comptime.h"
#else
const char *comprevision = "illegal";
#endif

const char *compdate = __DATE__;
const char *comptime = __TIME__;
