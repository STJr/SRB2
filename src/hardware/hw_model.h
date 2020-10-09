/*
	From the 'Wizard2' engine by Spaddlewit Inc. ( http://www.spaddlewit.com )
	An experimental work-in-progress.

	Donated to Sonic Team Junior and adapted to work with
	Sonic Robo Blast 2. The license of this code matches whatever
	the licensing is for Sonic Robo Blast 2.
*/

#ifndef _HW_MODEL_H_
#define _HW_MODEL_H_

#include "../doomtype.h"

typedef struct
{
	float x, y, z;
} vector_t;

extern vector_t vectorXaxis;
extern vector_t vectorYaxis;
extern vector_t vectorZaxis;

void VectorRotate(vector_t *rotVec, const vector_t *axisVec, float angle);

typedef struct
{
	float ambient[4], diffuse[4], specular[4], emissive[4];
	float shininess;
	boolean spheremap;
//	Texture::texture_t *texture;
//	Texture::texture_t *lightmap;
} material_t;

typedef struct
{
	material_t *material; // Pointer to the allocated 'materials' list in model_t
	float *vertices;
	float *normals;
	float *tangents;
	char *colors;
	unsigned int vboID;
	vector_t *polyNormals;
} mdlframe_t;

typedef struct
{
	material_t *material;
	short *vertices;
	char *normals;
	char *tangents;
	unsigned int vboID;
} tinyframe_t;

// Equivalent to MD3's many 'surfaces'
typedef struct mesh_s
{
	int numVertices;
	int numTriangles;

	float *uvs;
	// if uv adjustment is needed, uvs is changed to point to adjusted ones and
	// this one retains the originals
	// note: this member has been added with the assumption that models are never freed.
	// (UnloadModel is called by nobody at the time of writing.)
	float *originaluvs;
	float *lightuvs;

	int numFrames;
	mdlframe_t *frames;
	tinyframe_t *tinyframes;
	unsigned short *indices;
} mesh_t;

typedef struct tag_s
{
	char name[64];
//	matrix_t transform;
} tag_t;

#define MODEL_INTERPOLATION_FLAG "+i"

typedef struct
{
	INT32 frames[256];
	UINT8 numframes;
	boolean interpolate;
} modelspr2frames_t;

typedef struct model_s
{
	int maxNumFrames;

	int numMaterials;
	material_t *materials;
	int numMeshes;
	mesh_t *meshes;
	int numTags;
	tag_t *tags;

	char *mdlFilename;
	boolean unloaded;

	char *framenames;
	boolean interpolate[256];
	modelspr2frames_t *spr2frames;

	// the max_s and max_t values that the uvs are currently adjusted to
	// (if a sprite is used as a texture)
	float max_s;
	float max_t;
	// These are the values that the uvs in the VBO have been adjusted to.
	// If they are not same as max_s and max_t, then the VBO won't be used.
	float vbo_max_s;
	float vbo_max_t;
} model_t;

extern int numModels;
extern model_t *modelHead;

void HWR_ReloadModels(void);

tag_t *GetTagByName(model_t *model, char *name, int frame);
model_t *LoadModel(const char *filename, int ztag);
void UnloadModel(model_t *model);
void Optimize(model_t *model);
void LoadModelInterpolationSettings(model_t *model);
void LoadModelSprite2(model_t *model);
void GenerateVertexNormals(model_t *model);
void GeneratePolygonNormals(model_t *model, int ztag);
void CreateVBOTiny(mesh_t *mesh, tinyframe_t *frame);
void CreateVBO(mesh_t *mesh, mdlframe_t *frame);
void DeleteVBOs(model_t *model);

#endif
