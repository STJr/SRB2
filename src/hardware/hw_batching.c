// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2020 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file hw_batching.c
/// \brief Draw call batching and related things.

#ifdef HWRENDER
#include "hw_glob.h"
#include "hw_batching.h"
#include "../i_system.h"

// The texture for the next polygon given to HWR_ProcessPolygon.
// Set with HWR_SetCurrentTexture.
GLMipmap_t *current_texture = NULL;

boolean currently_batching = false;

FOutVector* finalVertexArray = NULL;// contains subset of sorted vertices and texture coordinates to be sent to gpu
UINT32* finalVertexIndexArray = NULL;// contains indexes for glDrawElements, taking into account fan->triangles conversion
//     NOTE have this alloced as 3x finalVertexArray size
int finalVertexArrayAllocSize = 65536;
//GLubyte* colorArray = NULL;// contains color data to be sent to gpu, if needed
//int colorArrayAllocSize = 65536;
// not gonna use this for now, just sort by color and change state when it changes
// later maybe when using vertex attributes if it's needed

PolygonArrayEntry* polygonArray = NULL;// contains the polygon data from DrawPolygon, waiting to be processed
int polygonArraySize = 0;
UINT32* polygonIndexArray = NULL;// contains sorting pointers for polygonArray
int polygonArrayAllocSize = 65536;

FOutVector* unsortedVertexArray = NULL;// contains unsorted vertices and texture coordinates from DrawPolygon
int unsortedVertexArraySize = 0;
int unsortedVertexArrayAllocSize = 65536;

// Enables batching mode. HWR_ProcessPolygon will collect polygons instead of passing them directly to the rendering backend.
// Call HWR_RenderBatches to render all the collected geometry.
void HWR_StartBatching(void)
{
    if (currently_batching)
        I_Error("Repeat call to HWR_StartBatching without HWR_RenderBatches");

    // init arrays if that has not been done yet
	if (!finalVertexArray)
	{
		finalVertexArray = malloc(finalVertexArrayAllocSize * sizeof(FOutVector));
		finalVertexIndexArray = malloc(finalVertexArrayAllocSize * 3 * sizeof(UINT32));
		polygonArray = malloc(polygonArrayAllocSize * sizeof(PolygonArrayEntry));
		polygonIndexArray = malloc(polygonArrayAllocSize * sizeof(UINT32));
		unsortedVertexArray = malloc(unsortedVertexArrayAllocSize * sizeof(FOutVector));
	}

    currently_batching = true;
}

// This replaces the direct calls to pfnSetTexture in cases where batching is available.
// The texture selection is saved for the next HWR_ProcessPolygon call.
// Doing this was easier than getting a texture pointer to HWR_ProcessPolygon.
void HWR_SetCurrentTexture(GLMipmap_t *texture)
{
    if (currently_batching)
    {
        current_texture = texture;
    }
    else
    {
        HWD.pfnSetTexture(texture);
    }
}

// If batching is enabled, this function collects the polygon data and the chosen texture
// for later use in HWR_RenderBatches. Otherwise the rendering backend is used to
// render the polygon immediately.
void HWR_ProcessPolygon(FSurfaceInfo *pSurf, FOutVector *pOutVerts, FUINT iNumPts, FBITFIELD PolyFlags, int shader, boolean horizonSpecial)
{
    if (currently_batching)
	{
		if (!pSurf)
			I_Error("Got a null FSurfaceInfo in batching");// nulls should not come in the stuff that batching currently applies to
		if (polygonArraySize == polygonArrayAllocSize)
		{
			PolygonArrayEntry* new_array;
			// ran out of space, make new array double the size
			polygonArrayAllocSize *= 2;
			new_array = malloc(polygonArrayAllocSize * sizeof(PolygonArrayEntry));
			memcpy(new_array, polygonArray, polygonArraySize * sizeof(PolygonArrayEntry));
			free(polygonArray);
			polygonArray = new_array;
			// also need to redo the index array, dont need to copy it though
			free(polygonIndexArray);
			polygonIndexArray = malloc(polygonArrayAllocSize * sizeof(UINT32));
		}

		while (unsortedVertexArraySize + (int)iNumPts > unsortedVertexArrayAllocSize)
		{
			FOutVector* new_array;
			// need more space for vertices in unsortedVertexArray
			unsortedVertexArrayAllocSize *= 2;
			new_array = malloc(unsortedVertexArrayAllocSize * sizeof(FOutVector));
			memcpy(new_array, unsortedVertexArray, unsortedVertexArraySize * sizeof(FOutVector));
			free(unsortedVertexArray);
			unsortedVertexArray = new_array;
		}

		// add the polygon data to the arrays

		polygonArray[polygonArraySize].surf = *pSurf;
		polygonArray[polygonArraySize].vertsIndex = unsortedVertexArraySize;
		polygonArray[polygonArraySize].numVerts = iNumPts;
		polygonArray[polygonArraySize].polyFlags = PolyFlags;
		polygonArray[polygonArraySize].texture = current_texture;
		polygonArray[polygonArraySize].shader = shader;
		polygonArray[polygonArraySize].horizonSpecial = horizonSpecial;
		polygonArraySize++;

		memcpy(&unsortedVertexArray[unsortedVertexArraySize], pOutVerts, iNumPts * sizeof(FOutVector));
		unsortedVertexArraySize += iNumPts;
	}
	else
	{
        if (shader)
            HWD.pfnSetShader(shader);
        HWD.pfnDrawPolygon(pSurf, pOutVerts, iNumPts, PolyFlags);
    }
}

static int comparePolygons(const void *p1, const void *p2)
{
	unsigned int index1 = *(const unsigned int*)p1;
	unsigned int index2 = *(const unsigned int*)p2;
	PolygonArrayEntry* poly1 = &polygonArray[index1];
	PolygonArrayEntry* poly2 = &polygonArray[index2];
	int diff;
	INT64 diff64;
	UINT32 downloaded1 = 0;
	UINT32 downloaded2 = 0;

	int shader1 = poly1->shader;
	int shader2 = poly2->shader;
	// make skywalls and horizon lines first in order
	if (poly1->polyFlags & PF_NoTexture || poly1->horizonSpecial)
		shader1 = -1;
	if (poly2->polyFlags & PF_NoTexture || poly2->horizonSpecial)
		shader2 = -1;
	diff = shader1 - shader2;
	if (diff != 0) return diff;

	// skywalls and horizon lines must retain their order for horizon lines to work
	if (shader1 == -1 && shader2 == -1)
		return index1 - index2;

	if (poly1->texture)
		downloaded1 = poly1->texture->downloaded; // there should be a opengl texture name here, usable for comparisons
	if (poly2->texture)
		downloaded2 = poly2->texture->downloaded;
	diff64 = downloaded1 - downloaded2;
	if (diff64 != 0) return diff64;

	diff = poly1->polyFlags - poly2->polyFlags;
	if (diff != 0) return diff;

	diff64 = poly1->surf.PolyColor.rgba - poly2->surf.PolyColor.rgba;
	if (diff64 < 0) return -1; else if (diff64 > 0) return 1;
	diff64 = poly1->surf.TintColor.rgba - poly2->surf.TintColor.rgba;
	if (diff64 < 0) return -1; else if (diff64 > 0) return 1;
	diff64 = poly1->surf.FadeColor.rgba - poly2->surf.FadeColor.rgba;
	if (diff64 < 0) return -1; else if (diff64 > 0) return 1;

	diff = poly1->surf.LightInfo.light_level - poly2->surf.LightInfo.light_level;
	if (diff != 0) return diff;
	diff = poly1->surf.LightInfo.fade_start - poly2->surf.LightInfo.fade_start;
	if (diff != 0) return diff;
	diff = poly1->surf.LightInfo.fade_end - poly2->surf.LightInfo.fade_end;
	return diff;
}

static int comparePolygonsNoShaders(const void *p1, const void *p2)
{
	unsigned int index1 = *(const unsigned int*)p1;
	unsigned int index2 = *(const unsigned int*)p2;
	PolygonArrayEntry* poly1 = &polygonArray[index1];
	PolygonArrayEntry* poly2 = &polygonArray[index2];
	int diff;
	INT64 diff64;

	GLMipmap_t *texture1 = poly1->texture;
	GLMipmap_t *texture2 = poly2->texture;
	UINT32 downloaded1 = 0;
	UINT32 downloaded2 = 0;
	if (poly1->polyFlags & PF_NoTexture || poly1->horizonSpecial)
		texture1 = NULL;
	if (poly2->polyFlags & PF_NoTexture || poly2->horizonSpecial)
		texture2 = NULL;
	if (texture1)
		downloaded1 = texture1->downloaded; // there should be a opengl texture name here, usable for comparisons
	if (texture2)
		downloaded2 = texture2->downloaded;
	// skywalls and horizon lines must retain their order for horizon lines to work
	if (!texture1 && !texture2)
		return index1 - index2;
	diff64 = downloaded1 - downloaded2;
	if (diff64 != 0) return diff64;

	diff = poly1->polyFlags - poly2->polyFlags;
	if (diff != 0) return diff;

	diff64 = poly1->surf.PolyColor.rgba - poly2->surf.PolyColor.rgba;
	if (diff64 < 0) return -1; else if (diff64 > 0) return 1;

	return 0;
}

// This function organizes the geometry collected by HWR_ProcessPolygon calls into batches and uses
// the rendering backend to draw them.
void HWR_RenderBatches(void)
{
    int finalVertexWritePos = 0;// position in finalVertexArray
	int finalIndexWritePos = 0;// position in finalVertexIndexArray

	int polygonReadPos = 0;// position in polygonIndexArray

	int currentShader;
	int nextShader = 0;
	GLMipmap_t *currentTexture;
	GLMipmap_t *nextTexture = NULL;
	FBITFIELD currentPolyFlags = 0;
	FBITFIELD nextPolyFlags = 0;
	FSurfaceInfo currentSurfaceInfo;
	FSurfaceInfo nextSurfaceInfo;

	int i;

    if (!currently_batching)
		I_Error("HWR_RenderBatches called without starting batching");

	nextSurfaceInfo.LightInfo.fade_end = 0;
	nextSurfaceInfo.LightInfo.fade_start = 0;
	nextSurfaceInfo.LightInfo.light_level = 0;

	currently_batching = false;// no longer collecting batches
	if (!polygonArraySize)
	{
		ps_hw_numpolys = ps_hw_numcalls = ps_hw_numshaders = ps_hw_numtextures = ps_hw_numpolyflags = ps_hw_numcolors = 0;
		return;// nothing to draw
	}
	// init stats vars
	ps_hw_numpolys = polygonArraySize;
	ps_hw_numcalls = ps_hw_numverts = 0;
	ps_hw_numshaders = ps_hw_numtextures = ps_hw_numpolyflags = ps_hw_numcolors = 1;
	// init polygonIndexArray
	for (i = 0; i < polygonArraySize; i++)
	{
		polygonIndexArray[i] = i;
	}

	// sort polygons
	ps_hw_batchsorttime = I_GetPreciseTime();
	if (cv_glshaders.value && gl_shadersavailable)
		qsort(polygonIndexArray, polygonArraySize, sizeof(unsigned int), comparePolygons);
	else
		qsort(polygonIndexArray, polygonArraySize, sizeof(unsigned int), comparePolygonsNoShaders);
	ps_hw_batchsorttime = I_GetPreciseTime() - ps_hw_batchsorttime;
	// sort order
	// 1. shader
	// 2. texture
	// 3. polyflags
	// 4. colors + light level
	// not sure about what order of the last 2 should be, or if it even matters

	ps_hw_batchdrawtime = I_GetPreciseTime();

	currentShader = polygonArray[polygonIndexArray[0]].shader;
	currentTexture = polygonArray[polygonIndexArray[0]].texture;
	currentPolyFlags = polygonArray[polygonIndexArray[0]].polyFlags;
	currentSurfaceInfo = polygonArray[polygonIndexArray[0]].surf;
	// For now, will sort and track the colors. Vertex attributes could be used instead of uniforms
	// and a color array could replace the color calls.

	// set state for first batch

	if (cv_glshaders.value && gl_shadersavailable)
	{
		HWD.pfnSetShader(currentShader);
	}

	if (currentPolyFlags & PF_NoTexture)
		currentTexture = NULL;
    else
	    HWD.pfnSetTexture(currentTexture);

	while (1)// note: remember handling notexture polyflag as having texture number 0 (also in comparePolygons)
	{
		int firstIndex;
		int lastIndex;

		boolean stopFlag = false;
		boolean changeState = false;
		boolean changeShader = false;
		boolean changeTexture = false;
		boolean changePolyFlags = false;
		boolean changeSurfaceInfo = false;

		// steps:
		// write vertices
		// check for changes or end, otherwise go back to writing
			// changes will affect the next vars and the change bools
			// end could set flag for stopping
		// execute draw call
		// could check ending flag here
		// change states according to next vars and change bools, updating the current vars and reseting the bools
		// reset write pos
		// repeat loop

		int index = polygonIndexArray[polygonReadPos++];
		int numVerts = polygonArray[index].numVerts;
		// before writing, check if there is enough room
		// using 'while' instead of 'if' here makes sure that there will *always* be enough room.
		// probably never will this loop run more than once though
		while (finalVertexWritePos + numVerts > finalVertexArrayAllocSize)
		{
			FOutVector* new_array;
			unsigned int* new_index_array;
			finalVertexArrayAllocSize *= 2;
			new_array = malloc(finalVertexArrayAllocSize * sizeof(FOutVector));
			memcpy(new_array, finalVertexArray, finalVertexWritePos * sizeof(FOutVector));
			free(finalVertexArray);
			finalVertexArray = new_array;
			// also increase size of index array, 3x of vertex array since
			// going from fans to triangles increases vertex count to 3x
			new_index_array = malloc(finalVertexArrayAllocSize * 3 * sizeof(UINT32));
			memcpy(new_index_array, finalVertexIndexArray, finalIndexWritePos * sizeof(UINT32));
			free(finalVertexIndexArray);
			finalVertexIndexArray = new_index_array;
		}
		// write the vertices of the polygon
		memcpy(&finalVertexArray[finalVertexWritePos], &unsortedVertexArray[polygonArray[index].vertsIndex],
			numVerts * sizeof(FOutVector));
		// write the indexes, pointing to the fan vertexes but in triangles format
		firstIndex = finalVertexWritePos;
		lastIndex = finalVertexWritePos + numVerts;
		finalVertexWritePos += 2;
		while (finalVertexWritePos < lastIndex)
		{
			finalVertexIndexArray[finalIndexWritePos++] = firstIndex;
			finalVertexIndexArray[finalIndexWritePos++] = finalVertexWritePos - 1;
			finalVertexIndexArray[finalIndexWritePos++] = finalVertexWritePos++;
		}

		if (polygonReadPos >= polygonArraySize)
		{
			stopFlag = true;
		}
		else
		{
			// check if a state change is required, set the change bools and next vars
			int nextIndex = polygonIndexArray[polygonReadPos];
			nextShader = polygonArray[nextIndex].shader;
			nextTexture = polygonArray[nextIndex].texture;
			nextPolyFlags = polygonArray[nextIndex].polyFlags;
			nextSurfaceInfo = polygonArray[nextIndex].surf;
			if (nextPolyFlags & PF_NoTexture)
				nextTexture = 0;
			if (currentShader != nextShader && cv_glshaders.value && gl_shadersavailable)
			{
				changeState = true;
				changeShader = true;
			}
			if (currentTexture != nextTexture)
			{
				changeState = true;
				changeTexture = true;
			}
			if (currentPolyFlags != nextPolyFlags)
			{
				changeState = true;
				changePolyFlags = true;
			}
			if (cv_glshaders.value && gl_shadersavailable)
			{
				if (currentSurfaceInfo.PolyColor.rgba != nextSurfaceInfo.PolyColor.rgba ||
					currentSurfaceInfo.TintColor.rgba != nextSurfaceInfo.TintColor.rgba ||
					currentSurfaceInfo.FadeColor.rgba != nextSurfaceInfo.FadeColor.rgba ||
					currentSurfaceInfo.LightInfo.light_level != nextSurfaceInfo.LightInfo.light_level ||
					currentSurfaceInfo.LightInfo.fade_start != nextSurfaceInfo.LightInfo.fade_start ||
					currentSurfaceInfo.LightInfo.fade_end != nextSurfaceInfo.LightInfo.fade_end)
				{
					changeState = true;
					changeSurfaceInfo = true;
				}
			}
			else
			{
				if (currentSurfaceInfo.PolyColor.rgba != nextSurfaceInfo.PolyColor.rgba)
				{
					changeState = true;
					changeSurfaceInfo = true;
				}
			}
		}

		if (changeState || stopFlag)
		{
			// execute draw call
            HWD.pfnDrawIndexedTriangles(&currentSurfaceInfo, finalVertexArray, finalIndexWritePos, currentPolyFlags, finalVertexIndexArray);
			// update stats
			ps_hw_numcalls++;
			ps_hw_numverts += finalIndexWritePos;
			// reset write positions
			finalVertexWritePos = 0;
			finalIndexWritePos = 0;
		}
		else continue;

		// if we're here then either its time to stop or time to change state
		if (stopFlag) break;

		// change state according to change bools and next vars, update current vars and reset bools
		if (changeShader)
		{
			HWD.pfnSetShader(nextShader);
			currentShader = nextShader;
			changeShader = false;

			ps_hw_numshaders++;
		}
		if (changeTexture)
		{
			// texture should be already ready for use from calls to SetTexture during batch collection
		    HWD.pfnSetTexture(nextTexture);
			currentTexture = nextTexture;
			changeTexture = false;

			ps_hw_numtextures++;
		}
		if (changePolyFlags)
		{
			currentPolyFlags = nextPolyFlags;
			changePolyFlags = false;

			ps_hw_numpolyflags++;
		}
		if (changeSurfaceInfo)
		{
			currentSurfaceInfo = nextSurfaceInfo;
			changeSurfaceInfo = false;

			ps_hw_numcolors++;
		}
		// and that should be it?
	}
	// reset the arrays (set sizes to 0)
	polygonArraySize = 0;
	unsortedVertexArraySize = 0;

	ps_hw_batchdrawtime = I_GetPreciseTime() - ps_hw_batchdrawtime;
}


#endif // HWRENDER
