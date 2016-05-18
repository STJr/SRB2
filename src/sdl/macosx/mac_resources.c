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
        CFStringRef macPath = CFURLCopyFileSystemPath(appUrlRef, kCFURLPOSIXPathStyle);

        const char* rawPath = CFStringGetCStringPtr(macPath, kCFStringEncodingASCII);
 
        if (CFStringGetLength(macPath) + strlen("/Contents/Resources") < BUF_SIZE)
        {
            strcpy(buffer, rawPath);
            strcat(buffer, "/Contents/Resources");
        }

        CFRelease(macPath);
        CFRelease(appUrlRef);
    }
    CFRelease(mainBundle);
}
