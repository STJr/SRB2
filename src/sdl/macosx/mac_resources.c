#include "mac_resources.h"
#include <string.h>

#include <CoreFoundation/CoreFoundation.h>

void OSX_GetResourcesPath(char * buffer)
{
    CFBundleRef mainBundle;
    mainBundle = CFBundleGetMainBundle();
    if (mainBundle)
    {
        const int BUF_SIZE = 256; // because we somehow always know that

        CFURLRef appUrlRef = CFBundleCopyBundleURL(mainBundle);
        CFStringRef macPath;
        if (appUrlRef != NULL)
            macPath = CFURLCopyFileSystemPath(appUrlRef, kCFURLPOSIXPathStyle);
        else
            macPath = NULL;

        const char* rawPath;

        if (macPath != NULL)
            rawPath = CFStringGetCStringPtr(macPath, kCFStringEncodingASCII);
        else
            rawPath = NULL;

        if (rawPath != NULL && (CFStringGetLength(macPath) + strlen("/Contents/Resources") < BUF_SIZE))
        {
            strcpy(buffer, rawPath);
            strcat(buffer, "/Contents/Resources");
        }

        CFRelease(macPath);
        CFRelease(appUrlRef);
    }
}
