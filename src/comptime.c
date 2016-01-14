/*
 * Compiles date and time.
 *
 * Kalaron: Can't this be somewhere else instead of in an extra c file?
 * Alam: Code::Block, XCode and the Makefile touch this file to update
 *  the timestamp
 *
 */

#if (defined(CMAKECONFIG))
#include "config.h"
const char *compbranch = SRB2_COMP_BRANCH;
const char *comprevision = SRB2_COMP_REVISION;

#elif (defined(COMPVERSION))
#include "comptime.h"

#else
const char *compbranch = "Unknown";
const char *comprevision = "illegal";

#endif

const char *compdate = __DATE__;
const char *comptime = __TIME__;
