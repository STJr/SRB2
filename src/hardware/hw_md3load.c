/*
	From the 'Wizard2' engine by Spaddlewit Inc. ( http://www.spaddlewit.com )
	An experimental work-in-progress.

	Donated to Sonic Team Junior and adapted to work with
	Sonic Robo Blast 2. The license of this code matches whatever
	the licensing is for Sonic Robo Blast 2.
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "../doomdef.h"
#include "hw_md3load.h"
#include "hw_model.h"
#include "../z_zone.h"

typedef struct
{
	int ident;			// A "magic number" that's used to identify the .md3 file
	int version;		// The version of the file, always 15
	char name[64];
	int flags;
	int numFrames;		// Number of frames
	int numTags;
	int numSurfaces;
	int numSkins;		// Number of skins with the model
	int offsetFrames;
	int offsetTags;
	int offsetSurfaces;
	int offsetEnd;		// Offset, in bytes from the start of the file, to the end of the file (filesize)
} md3modelHeader;

typedef struct
{
	float minBounds[3];		// First corner of the bounding box
	float maxBounds[3];		// Second corner of the bounding box
	float localOrigin[3];	// Local origin, usually (0, 0, 0)
	float radius;			// Radius of bounding sphere
	char name[16];			// Name of frame
} md3Frame;

typedef struct
{
	char name[64];		// Name of tag
	float origin[3];	// Coordinates of tag
	float axis[9];		// Orientation of tag object
} md3Tag;

typedef struct
{
	int ident;
	char name[64];			// Name of this surface
	int flags;
	int numFrames;			// # of keyframes
	int numShaders;			// # of shaders
	int numVerts;			// # of vertices
	int numTriangles;		// # of triangles
	int offsetTriangles;	// Relative offset from start of this struct to where the list of Triangles start
	int offsetShaders;		// Relative offset from start of this struct to where the list of Shaders start
	int offsetST;			// Relative offset from start of this struct to where the list of tex coords start
	int offsetXYZNormal;	// Relative offset from start of this struct to where the list of vertices start
	int offsetEnd;			// Relative offset from start of this struct to where this surface ends
} md3Surface;

typedef struct
{
	char name[64]; // Name of this shader
	int shaderIndex; // Shader index number
} md3Shader;

typedef struct
{
	int index[3]; // List of offset values into the list of Vertex objects that constitute the corners of the Triangle object.
} md3Triangle;

typedef struct
{
	float st[2];
} md3TexCoord;

typedef struct
{
	short x, y, z, n;
} md3Vertex;

static float latlnglookup[256][256][3];

static void GetNormalFromLatLong(short latlng, float *out)
{
	float *lookup = latlnglookup[(unsigned char)(latlng >> 8)][(unsigned char)(latlng & 255)];

	out[0] = *lookup++;
	out[1] = *lookup++;
	out[2] = *lookup++;
}

#if 0
static void NormalToLatLng(float *n, short *out)
{
	// Special cases
	if (0.0f == n[0] && 0.0f == n[1])
	{
		if (n[2] > 0.0f)
			*out = 0;
		else
			*out = 128;
	}
	else
	{
		char x, y;

		x = (char)(57.2957795f * (atan2(n[1], n[0])) * (255.0f / 360.0f));
		y = (char)(57.2957795f * (acos(n[2])) * (255.0f / 360.0f));

		*out = (x << 8) + y;
	}
}
#endif

static inline void LatLngToNormal(short n, float *out)
{
	const float PI = (3.1415926535897932384626433832795f);
	float lat = (float)(n >> 8);
	float lng = (float)(n & 255);

	lat *= PI / 128.0f;
	lng *= PI / 128.0f;

	out[0] = cosf(lat) * sinf(lng);
	out[1] = sinf(lat) * sinf(lng);
	out[2] = cosf(lng);
}

static void LatLngInit(void)
{
	int i, j;
	for (i = 0; i < 256; i++)
	{
		for (j = 0; j < 256; j++)
			LatLngToNormal((short)((i << 8) + j), latlnglookup[i][j]);
	}
}

static boolean latlnginit = false;

model_t *MD3_LoadModel(const char *fileName, int ztag, boolean useFloat)
{
	const float WUNITS = 1.0f;
	model_t *retModel = NULL;
	md3modelHeader *mdh;
	long fileLen;
	long fileReadLen;
	char *buffer;
	int surfEnd;
	int i, t;
	int matCount;
	FILE *f;

	if (!latlnginit)
	{
		LatLngInit();
		latlnginit = true;
	}

	f = fopen(fileName, "rb");

	if (!f)
		return NULL;

	retModel = (model_t*)Z_Calloc(sizeof(model_t), ztag, 0);

	// find length of file
	fseek(f, 0, SEEK_END);
	fileLen = ftell(f);
	fseek(f, 0, SEEK_SET);

	// read in file
	buffer = malloc(fileLen);
	fileReadLen = fread(buffer, fileLen, 1, f);
	fclose(f);

	(void)fileReadLen; // intentionally ignore return value, per buildbot

	// get pointer to file header
	mdh = (md3modelHeader*)buffer;

	retModel->numMeshes = mdh->numSurfaces;

	retModel->numMaterials = 0;
	surfEnd = 0;
	for (i = 0; i < mdh->numSurfaces; i++)
	{
		md3Surface *mdS = (md3Surface*)&buffer[mdh->offsetSurfaces];
		surfEnd += mdS->offsetEnd;

		retModel->numMaterials += mdS->numShaders;
	}

	// Initialize materials
	if (retModel->numMaterials <= 0) // Always at least one skin, duh
		retModel->numMaterials = 1;

	retModel->materials = (material_t*)Z_Calloc(sizeof(material_t)*retModel->numMaterials, ztag, 0);

	for (t = 0; t < retModel->numMaterials; t++)
	{
		retModel->materials[t].ambient[0] = 0.3686f;
		retModel->materials[t].ambient[1] = 0.3684f;
		retModel->materials[t].ambient[2] = 0.3684f;
		retModel->materials[t].ambient[3] = 1.0f;
		retModel->materials[t].diffuse[0] = 0.8863f;
		retModel->materials[t].diffuse[1] = 0.8850f;
		retModel->materials[t].diffuse[2] = 0.8850f;
		retModel->materials[t].diffuse[3] = 1.0f;
		retModel->materials[t].emissive[0] = 0.0f;
		retModel->materials[t].emissive[1] = 0.0f;
		retModel->materials[t].emissive[2] = 0.0f;
		retModel->materials[t].emissive[3] = 1.0f;
		retModel->materials[t].specular[0] = 0.4902f;
		retModel->materials[t].specular[1] = 0.4887f;
		retModel->materials[t].specular[2] = 0.4887f;
		retModel->materials[t].specular[3] = 1.0f;
		retModel->materials[t].shininess = 25.0f;
		retModel->materials[t].spheremap = false;
	}

	retModel->meshes = (mesh_t*)Z_Calloc(sizeof(mesh_t)*retModel->numMeshes, ztag, 0);

	matCount = 0;
	for (i = 0, surfEnd = 0; i < mdh->numSurfaces; i++)
	{
		int j;
		md3Shader *mdShader;
		md3Surface *mdS = (md3Surface*)&buffer[mdh->offsetSurfaces + surfEnd];
		surfEnd += mdS->offsetEnd;

		mdShader = (md3Shader*)((char*)mdS + mdS->offsetShaders);

		for (j = 0; j < mdS->numShaders; j++, matCount++)
		{
			size_t len = strlen(mdShader[j].name);
			mdShader[j].name[len-1] = 'z';
			mdShader[j].name[len-2] = 'u';
			mdShader[j].name[len-3] = 'b';

			// Load material
/*			retModel->materials[matCount].texture = Texture::ReadTexture(mdShader[j].name, ZT_TEXTURE);

			if (!systemSucks)
			{
				// Check for a normal map...??
				char openfilename[1024];
				char normalMapName[1024];
				strcpy(normalMapName, mdShader[j].name);
				len = strlen(normalMapName);
				char *ptr = &normalMapName[len];
				ptr--; // z
				ptr--; // u
				ptr--; // b
				ptr--; // .
				*ptr++ = '_';
				*ptr++ = 'n';
				*ptr++ = '.';
				*ptr++ = 'b';
				*ptr++ = 'u';
				*ptr++ = 'z';
				*ptr++ = '\0';

				sprintf(openfilename, "%s/%s", "textures", normalMapName);
				// Convert backslashes to forward slashes
				for (int k = 0; k < 1024; k++)
				{
					if (openfilename[k] == '\0')
						break;

					if (openfilename[k] == '\\')
						openfilename[k] = '/';
				}

				Resource::resource_t *res = Resource::Open(openfilename);
				if (res)
				{
					Resource::Close(res);
					retModel->materials[matCount].lightmap = Texture::ReadTexture(normalMapName, ZT_TEXTURE);
				}
			}*/
		}

		retModel->meshes[i].numFrames = mdS->numFrames;
		retModel->meshes[i].numTriangles = mdS->numTriangles;

		if (!useFloat) // 'tinyframe' mode with indices
		{
			float tempNormal[3];
			float *uvptr;
			md3TexCoord *mdST;
			unsigned short *indexptr;
			md3Triangle *mdT;

			retModel->meshes[i].tinyframes = (tinyframe_t*)Z_Calloc(sizeof(tinyframe_t)*mdS->numFrames, ztag, 0);
			retModel->meshes[i].numVertices = mdS->numVerts;
			retModel->meshes[i].uvs = (float*)Z_Malloc(sizeof(float)*2*mdS->numVerts, ztag, 0);
			for (j = 0; j < mdS->numFrames; j++)
			{
				short *vertptr;
				char *normptr;
				// char *tanptr;
				int k;
				md3Vertex *mdV = (md3Vertex*)((char*)mdS + mdS->offsetXYZNormal + (mdS->numVerts*j*sizeof(md3Vertex)));
				retModel->meshes[i].tinyframes[j].vertices = (short*)Z_Malloc(sizeof(short)*3*mdS->numVerts, ztag, 0);
				retModel->meshes[i].tinyframes[j].normals = (char*)Z_Malloc(sizeof(char)*3*mdS->numVerts, ztag, 0);

//				if (retModel->materials[0].lightmap)
//					retModel->meshes[i].tinyframes[j].tangents = (char*)malloc(sizeof(char));//(char*)Z_Malloc(sizeof(char)*3*mdS->numVerts, ztag);
				retModel->meshes[i].indices = (unsigned short*)Z_Malloc(sizeof(unsigned short) * 3 * mdS->numTriangles, ztag, 0);
				vertptr = retModel->meshes[i].tinyframes[j].vertices;
				normptr = retModel->meshes[i].tinyframes[j].normals;

//				tanptr = retModel->meshes[i].tinyframes[j].tangents;
				retModel->meshes[i].tinyframes[j].material = &retModel->materials[i];

				for (k = 0; k < mdS->numVerts; k++)
				{
					// Vertex
					*vertptr = mdV[k].x;
					vertptr++;
					*vertptr = mdV[k].z;
					vertptr++;
					*vertptr = 1.0f - mdV[k].y;
					vertptr++;

					// Normal
					GetNormalFromLatLong(mdV[k].n, tempNormal);
					*normptr = (char)(tempNormal[0] * 127);
					normptr++;
					*normptr = (char)(tempNormal[2] * 127);
					normptr++;
					*normptr = (char)(tempNormal[1] * 127);
					normptr++;
				}
			}

			uvptr = (float*)retModel->meshes[i].uvs;
			mdST = (md3TexCoord*)((char*)mdS + mdS->offsetST);
			for (j = 0; j < mdS->numVerts; j++)
			{
				*uvptr = mdST[j].st[0];
				uvptr++;
				*uvptr = mdST[j].st[1];
				uvptr++;
			}

			indexptr = retModel->meshes[i].indices;
			mdT = (md3Triangle*)((char*)mdS + mdS->offsetTriangles);
			for (j = 0; j < mdS->numTriangles; j++, mdT++)
			{
				// Indices
				*indexptr = (unsigned short)mdT->index[0];
				indexptr++;
				*indexptr = (unsigned short)mdT->index[1];
				indexptr++;
				*indexptr = (unsigned short)mdT->index[2];
				indexptr++;
			}
		}
		else // Traditional full-float loading method
		{
			float dataScale = 0.015624f * WUNITS;
			float tempNormal[3];
			md3TexCoord *mdST;
			md3Triangle *mdT;
			float *uvptr;
			int k;

			retModel->meshes[i].numVertices = mdS->numTriangles * 3;//mdS->numVerts;
			retModel->meshes[i].frames = (mdlframe_t*)Z_Calloc(sizeof(mdlframe_t)*mdS->numFrames, ztag, 0);
			retModel->meshes[i].uvs = (float*)Z_Malloc(sizeof(float)*2*mdS->numTriangles*3, ztag, 0);

			for (j = 0; j < mdS->numFrames; j++)
			{
				float *vertptr;
				float *normptr;
				md3Vertex *mdV = (md3Vertex*)((char*)mdS + mdS->offsetXYZNormal + (mdS->numVerts*j*sizeof(md3Vertex)));
				retModel->meshes[i].frames[j].vertices = (float*)Z_Malloc(sizeof(float)*3*mdS->numTriangles*3, ztag, 0);
				retModel->meshes[i].frames[j].normals = (float*)Z_Malloc(sizeof(float)*3*mdS->numTriangles*3, ztag, 0);
//				if (retModel->materials[i].lightmap)
//					retModel->meshes[i].frames[j].tangents = (float*)malloc(sizeof(float));//(float*)Z_Malloc(sizeof(float)*3*mdS->numTriangles*3, ztag);
				vertptr = retModel->meshes[i].frames[j].vertices;
				normptr = retModel->meshes[i].frames[j].normals;
				retModel->meshes[i].frames[j].material = &retModel->materials[i];

				mdT = (md3Triangle*)((char*)mdS + mdS->offsetTriangles);

				for (k = 0; k < mdS->numTriangles; k++)
				{
					// Vertex 1
					*vertptr = mdV[mdT->index[0]].x * dataScale;
					vertptr++;
					*vertptr = mdV[mdT->index[0]].z * dataScale;
					vertptr++;
					*vertptr = 1.0f - mdV[mdT->index[0]].y * dataScale;
					vertptr++;

					GetNormalFromLatLong(mdV[mdT->index[0]].n, tempNormal);
					*normptr = tempNormal[0];
					normptr++;
					*normptr = tempNormal[2];
					normptr++;
					*normptr = tempNormal[1];
					normptr++;

					// Vertex 2
					*vertptr = mdV[mdT->index[1]].x * dataScale;
					vertptr++;
					*vertptr = mdV[mdT->index[1]].z * dataScale;
					vertptr++;
					*vertptr = 1.0f - mdV[mdT->index[1]].y * dataScale;
					vertptr++;

					GetNormalFromLatLong(mdV[mdT->index[1]].n, tempNormal);
					*normptr = tempNormal[0];
					normptr++;
					*normptr = tempNormal[2];
					normptr++;
					*normptr = tempNormal[1];
					normptr++;

					// Vertex 3
					*vertptr = mdV[mdT->index[2]].x * dataScale;
					vertptr++;
					*vertptr = mdV[mdT->index[2]].z * dataScale;
					vertptr++;
					*vertptr = 1.0f - mdV[mdT->index[2]].y * dataScale;
					vertptr++;

					GetNormalFromLatLong(mdV[mdT->index[2]].n, tempNormal);
					*normptr = tempNormal[0];
					normptr++;
					*normptr = tempNormal[2];
					normptr++;
					*normptr = tempNormal[1];
					normptr++;

					mdT++; // Advance to next triangle
				}
			}

			mdST = (md3TexCoord*)((char*)mdS + mdS->offsetST);
			uvptr = (float*)retModel->meshes[i].uvs;
			mdT = (md3Triangle*)((char*)mdS + mdS->offsetTriangles);

			for (k = 0; k < mdS->numTriangles; k++)
			{
				*uvptr = mdST[mdT->index[0]].st[0];
				uvptr++;
				*uvptr = mdST[mdT->index[0]].st[1];
				uvptr++;

				*uvptr = mdST[mdT->index[1]].st[0];
				uvptr++;
				*uvptr = mdST[mdT->index[1]].st[1];
				uvptr++;

				*uvptr = mdST[mdT->index[2]].st[0];
				uvptr++;
				*uvptr = mdST[mdT->index[2]].st[1];
				uvptr++;

				mdT++; // Advance to next triangle
			}
		}
	}
	/*
	// Tags?
	retModel->numTags = mdh->numTags;
	retModel->maxNumFrames = mdh->numFrames;
	retModel->tags = (tag_t*)Z_Calloc(sizeof(tag_t) * retModel->numTags * mdh->numFrames, ztag);
	md3Tag *mdTag = (md3Tag*)&buffer[mdh->offsetTags];
	tag_t *curTag = retModel->tags;
	for (i = 0; i < mdh->numFrames; i++)
	{
		int j;
		for (j = 0; j < retModel->numTags; j++, mdTag++)
		{
			strcpys(curTag->name, mdTag->name, sizeof(curTag->name) / sizeof(char));
			curTag->transform.m[0][0] = mdTag->axis[0];
			curTag->transform.m[0][1] = mdTag->axis[1];
			curTag->transform.m[0][2] = mdTag->axis[2];
			curTag->transform.m[1][0] = mdTag->axis[3];
			curTag->transform.m[1][1] = mdTag->axis[4];
			curTag->transform.m[1][2] = mdTag->axis[5];
			curTag->transform.m[2][0] = mdTag->axis[6];
			curTag->transform.m[2][1] = mdTag->axis[7];
			curTag->transform.m[2][2] = mdTag->axis[8];
			curTag->transform.m[3][0] = mdTag->origin[0] * WUNITS;
			curTag->transform.m[3][1] = mdTag->origin[1] * WUNITS;
			curTag->transform.m[3][2] = mdTag->origin[2] * WUNITS;
			curTag->transform.m[3][3] = 1.0f;

			Matrix::Rotate(&curTag->transform, 90.0f, &Vector::Xaxis);
			curTag++;
		}
	}*/


	free(buffer);

	return retModel;
}
