#include "mac_resources.h"
#include <string.h>

#include <CoreFoundation/CoreFoundation.h>

void OSX_GetResourcesPath(char * buffer)
{
    CFBundleRef mainBundle;
    mainBundle = CFBundleGetMainBundle();
    if (mainBundle)
    {
        CFURLRef appUrlRef = CFBundleCopyBundleURL(mainBundle);
        CFStringRef macPath = CFURLCopyFileSystemPath(appUrlRef, kCFURLPOSIXPathStyle);
        CFStringRef resources = CFStringCreateWithCString(kCFAllocatorMalloc, "/Contents/Resources", kCFStringEncodingASCII);
        const void* rawarray[2] = {macPath, resources};
        CFArrayRef array = CFArrayCreate(kCFAllocatorMalloc, rawarray, 2, NULL);
        CFStringRef separator = CFStringCreateWithCString(kCFAllocatorMalloc, "", kCFStringEncodingASCII);
        CFStringRef fullPath = CFStringCreateByCombiningStrings(kCFAllocatorMalloc, array, separator);
        const char * path = CFStringGetCStringPtr(fullPath, kCFStringEncodingASCII);
        strcpy(buffer, path);
        CFRelease(fullPath);
        path = NULL;
        CFRelease(array);
        CFRelease(resources);
        CFRelease(macPath);
        CFRelease(appUrlRef);
        //CFRelease(mainBundle);
        CFRelease(separator);
    }

}