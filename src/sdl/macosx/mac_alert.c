// Emacs style mode select   -*- C++ -*-
//-----------------------------------------------------------------------------
//
// Copyright (C) 1993-1996 by id Software, Inc.
// Portions Copyright (C) 1998-2000 by DooM Legacy Team.
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//-----------------------------------------------------------------------------
/// \file
/// \brief Graphical Alerts for MacOSX
///
///	Shows alerts, since we can't just print these to the screen when
///	launched graphically on a mac.

#ifdef __APPLE_CC__

#include "mac_alert.h"
#include <CoreFoundation/CoreFoundation.h>

#define CFSTRINGIFY(x) CFStringCreateWithCString(NULL, x, kCFStringEncodingASCII)

int MacShowAlert(const char *title, const char *message, const char *button1, const char *button2, const char *button3)
{
	CFOptionFlags results;

        CFStringRef cf_title   = CFSTRINGIFY(title);
        CFStringRef cf_message = CFSTRINGIFY(message);
        CFStringRef cf_button1 = NULL;
        CFStringRef cf_button2 = NULL;
        CFStringRef cf_button3 = NULL;

        if (button1 != NULL)
            cf_button1 = CFSTRINGIFY(button1);
        if (button2 != NULL)
            cf_button2 = CFSTRINGIFY(button2);
        if (button3 != NULL)
            cf_button3 = CFSTRINGIFY(button3);

        CFOptionFlags alert_flags = kCFUserNotificationStopAlertLevel | kCFUserNotificationNoDefaultButtonFlag;

	CFUserNotificationDisplayAlert(0, alert_flags, NULL, NULL, NULL, cf_title, cf_message,
                                       cf_button1, cf_button2, cf_button3, &results);

        if (cf_button1 != NULL)
           CFRelease(cf_button1);
        if (cf_button2 != NULL)
           CFRelease(cf_button2);
        if (cf_button3 != NULL)
           CFRelease(cf_button3);
        CFRelease(cf_message);
        CFRelease(cf_title);

	return (int)results;
}

#endif
