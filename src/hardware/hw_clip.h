/*
 *  hw_clip.h
 *  SRB2CB
 *
 *  PrBoom's OpenGL clipping
 *
 *
 */

// OpenGL BSP clipping
#include "../doomdef.h"
#include "../tables.h"
#include "../doomtype.h"

//#define HAVE_SPHEREFRUSTRUM // enable if you want HWR_SphereInFrustum and related code

boolean HWR_clipper_SafeCheckRange(angle_t startAngle, angle_t endAngle);
void HWR_clipper_SafeAddClipRange(angle_t startangle, angle_t endangle);
void HWR_clipper_Clear(void);
angle_t HWR_FrustumAngle(void);
#ifdef HAVE_SPHEREFRUSTRUM
void HWR_FrustrumSetup(void);
boolean HWR_SphereInFrustum(float x, float y, float z, float radius);
#endif
