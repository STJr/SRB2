/*
	From the 'Wizard2' engine by Spaddlewit Inc. ( http://www.spaddlewit.com )
	An experimental work-in-progress.

	Donated to Sonic Team Junior and adapted to work with
	Sonic Robo Blast 2. The license of this code matches whatever
	the licensing is for Sonic Robo Blast 2.
*/

#include "../z_zone.h"
#include "../doomdef.h"
#include "hw_model.h"
#include "hw_md2load.h"
#include "hw_md3load.h"
#include "u_list.h"
#include <string.h>

static float PI = (3.1415926535897932384626433832795f);
static float U_Deg2Rad(float deg)
{
	return deg * ((float)PI / 180.0f);
}

vector_t vectorXaxis = { 1.0f, 0.0f, 0.0f };
vector_t vectorYaxis = { 0.0f, 1.0f, 0.0f };
vector_t vectorZaxis = { 0.0f, 0.0f, 1.0f };

void VectorRotate(vector_t *rotVec, const vector_t *axisVec, float angle)
{
	angle = U_Deg2Rad(angle);

	// Rotate the point (x,y,z) around the vector (u,v,w)
	float ux = axisVec->x * rotVec->x;
	float uy = axisVec->x * rotVec->y;
	float uz = axisVec->x * rotVec->z;
	float vx = axisVec->y * rotVec->x;
	float vy = axisVec->y * rotVec->y;
	float vz = axisVec->y * rotVec->z;
	float wx = axisVec->z * rotVec->x;
	float wy = axisVec->z * rotVec->y;
	float wz = axisVec->z * rotVec->z;
	float sa = sinf(angle);
	float ca = cosf(angle);

	rotVec->x = axisVec->x*(ux + vy + wz) + (rotVec->x*(axisVec->y*axisVec->y + axisVec->z*axisVec->z) - axisVec->x*(vy + wz))*ca + (-wy + vz)*sa;
	rotVec->y = axisVec->y*(ux + vy + wz) + (rotVec->y*(axisVec->x*axisVec->x + axisVec->z*axisVec->z) - axisVec->y*(ux + wz))*ca + (wx - uz)*sa;
	rotVec->z = axisVec->z*(ux + vy + wz) + (rotVec->z*(axisVec->x*axisVec->x + axisVec->y*axisVec->y) - axisVec->z*(ux + vy))*ca + (-vx + uy)*sa;
}

void CreateVBOTiny(mesh_t *mesh, tinyframe_t *frame)
{
	(void)mesh;
	(void)frame;
    return;
/*	int bufferSize = sizeof(VBO::vbotiny_t)*mesh->numTriangles*3;
	VBO::vbotiny_t *buffer = (VBO::vbotiny_t*)Z_Malloc(bufferSize, PU_STATIC, 0);
	VBO::vbotiny_t *bufPtr = buffer;

	short *vertPtr = frame->vertices;
	char *normPtr = frame->normals;
	float *uvPtr = mesh->uvs;
	char *tanPtr = frame->tangents;

	int i;
	for (i = 0; i < mesh->numTriangles*3; i++)
	{
		bufPtr->x = *vertPtr++;
		bufPtr->y = *vertPtr++;
		bufPtr->z = *vertPtr++;

		bufPtr->nx = (*normPtr++) * 127;
		bufPtr->ny = (*normPtr++) * 127;
		bufPtr->nz = (*normPtr++) * 127;

		bufPtr->s0 = *uvPtr++;
		bufPtr->t0 = *uvPtr++;

		if (tanPtr)
		{
			bufPtr->tanx = *tanPtr++;
			bufPtr->tany = *tanPtr++;
			bufPtr->tanz = *tanPtr++;
		}

		bufPtr++;
	}

	bglGenBuffers(1, &frame->vboID);
	bglBindBuffer(BGL_ARRAY_BUFFER, frame->vboID);
	bglBufferData(BGL_ARRAY_BUFFER, bufferSize, buffer, BGL_STATIC_DRAW);
	Z_Free(buffer);*/
}

void CreateVBO(mesh_t *mesh, mdlframe_t *frame)
{
	(void)mesh;
	(void)frame;
    return;
/*	int bufferSize = sizeof(VBO::vbo64_t)*mesh->numTriangles*3;
	VBO::vbo64_t *buffer = (VBO::vbo64_t*)Z_Malloc(bufferSize, PU_STATIC, 0);
	VBO::vbo64_t *bufPtr = buffer;

	float *vertPtr = frame->vertices;
	float *normPtr = frame->normals;
	float *tanPtr = frame->tangents;
	float *uvPtr = mesh->uvs;
	float *lightPtr = mesh->lightuvs;
	byte *colorPtr = frame->colors;

	int i;
	for (i = 0; i < mesh->numTriangles*3; i++)
	{
		bufPtr->x = *vertPtr++;
		bufPtr->y = *vertPtr++;
		bufPtr->z = *vertPtr++;

		bufPtr->nx = *normPtr++;
		bufPtr->ny = *normPtr++;
		bufPtr->nz = *normPtr++;

		bufPtr->s0 = *uvPtr++;
		bufPtr->t0 = *uvPtr++;

		if (tanPtr != NULL)
		{
			bufPtr->tan0 = *tanPtr++;
			bufPtr->tan1 = *tanPtr++;
			bufPtr->tan2 = *tanPtr++;
		}

		if (lightPtr != NULL)
		{
			bufPtr->s1 = *lightPtr++;
			bufPtr->t1 = *lightPtr++;
		}

		if (colorPtr)
		{
			bufPtr->r = *colorPtr++;
			bufPtr->g = *colorPtr++;
			bufPtr->b = *colorPtr++;
			bufPtr->a = *colorPtr++;
		}
		else
		{
			bufPtr->r = 255;
			bufPtr->g = 255;
			bufPtr->b = 255;
			bufPtr->a = 255;
		}

		bufPtr++;
	}

	bglGenBuffers(1, &frame->vboID);
	bglBindBuffer(BGL_ARRAY_BUFFER, frame->vboID);
	bglBufferData(BGL_ARRAY_BUFFER, bufferSize, buffer, BGL_STATIC_DRAW);
	Z_Free(buffer);*/
}

void UnloadModel(model_t *model)
{
	// Wouldn't it be great if C just had destructors?
	int i;
	for (i = 0; i < model->numMeshes; i++)
	{
		mesh_t *mesh = &model->meshes[i];

		if (mesh->frames)
		{
			int j;
			for (j = 0; j < mesh->numFrames; j++)
			{
				if (mesh->frames[j].normals)
					Z_Free(mesh->frames[j].normals);

				if (mesh->frames[j].tangents)
					Z_Free(mesh->frames[j].tangents);

				if (mesh->frames[j].vertices)
					Z_Free(mesh->frames[j].vertices);

				if (mesh->frames[j].colors)
					Z_Free(mesh->frames[j].colors);
			}

			Z_Free(mesh->frames);
		}
		else if (mesh->tinyframes)
		{
			int j;
			for (j = 0; j < mesh->numFrames; j++)
			{
				if (mesh->tinyframes[j].normals)
					Z_Free(mesh->tinyframes[j].normals);

				if (mesh->tinyframes[j].tangents)
					Z_Free(mesh->tinyframes[j].tangents);

				if (mesh->tinyframes[j].vertices)
					Z_Free(mesh->tinyframes[j].vertices);
			}

			if (mesh->indices)
				Z_Free(mesh->indices);

			Z_Free(mesh->tinyframes);
		}

		if (mesh->uvs)
			Z_Free(mesh->uvs);

		if (mesh->lightuvs)
			Z_Free(mesh->lightuvs);
	}

	if (model->meshes)
		Z_Free(model->meshes);

	if (model->tags)
		Z_Free(model->tags);

	if (model->materials)
		Z_Free(model->materials);

	DeleteVBOs(model);
	Z_Free(model);
}

tag_t *GetTagByName(model_t *model, char *name, int frame)
{
	if (frame < model->maxNumFrames)
	{
		tag_t *iterator = &model->tags[frame * model->numTags];

		int i;
		for (i = 0; i < model->numTags; i++)
		{
			if (!stricmp(iterator[i].name, name))
				return &iterator[i];
		}
	}

	return NULL;
}

//
// LoadModel
//
// Load a model and
// convert it to the
// internal format.
//
model_t *LoadModel(const char *filename, int ztag)
{
	model_t *model;

	// What type of file?
	const char *extension = NULL;
	int i;
	for (i = (int)strlen(filename)-1; i >= 0; i--)
	{
		if (filename[i] != '.')
			continue;

		extension = &filename[i];
		break;
	}

	if (!extension)
	{
		CONS_Printf("Model %s is lacking a file extension, unable to determine type!\n", filename);
		return NULL;
	}

	if (!strcmp(extension, ".md3"))
	{
		if (!(model = MD3_LoadModel(filename, ztag, false)))
			return NULL;
	}
	else if (!strcmp(extension, ".md3s")) // MD3 that will be converted in memory to use full floats
	{
		if (!(model = MD3_LoadModel(filename, ztag, true)))
			return NULL;
	}
	else if (!strcmp(extension, ".md2"))
	{
		if (!(model = MD2_LoadModel(filename, ztag, false)))
			return NULL;
	}
	else if (!strcmp(extension, ".md2s"))
	{
		if (!(model = MD2_LoadModel(filename, ztag, true)))
			return NULL;
	}
	else
	{
		CONS_Printf("Unknown model format: %s\n", extension);
		return NULL;
	}

	model->mdlFilename = (char*)Z_Malloc(strlen(filename)+1, ztag, 0);
	strcpy(model->mdlFilename, filename);

	Optimize(model);
	GeneratePolygonNormals(model, ztag);

	// Default material properties
	for (i = 0 ; i < model->numMaterials; i++)
	{
		material_t *material = &model->materials[i];
		material->ambient[0] = 0.7686f;
		material->ambient[1] = 0.7686f;
		material->ambient[2] = 0.7686f;
		material->ambient[3] = 1.0f;
		material->diffuse[0] = 0.5863f;
		material->diffuse[1] = 0.5863f;
		material->diffuse[2] = 0.5863f;
		material->diffuse[3] = 1.0f;
		material->specular[0] = 0.4902f;
		material->specular[1] = 0.4902f;
		material->specular[2] = 0.4902f;
		material->specular[3] = 1.0f;
		material->shininess = 25.0f;
	}

//	CONS_Printf("Generating VBOs for %s\n", filename);
	for (i = 0; i < model->numMeshes; i++)
	{
		mesh_t *mesh = &model->meshes[i];

		if (mesh->frames)
		{
			int j;
			for (j = 0; j < model->meshes[i].numFrames; j++)
			{
				mdlframe_t *frame = &mesh->frames[j];
				frame->vboID = 0;
				CreateVBO(mesh, frame);
			}
		}
		else if (mesh->tinyframes)
		{
			int j;
			for (j = 0; j < model->meshes[i].numFrames; j++)
			{
				tinyframe_t *frame = &mesh->tinyframes[j];
				frame->vboID = 0;
				CreateVBOTiny(mesh, frame);
			}
		}
	}

	return model;
}

//
// GenerateVertexNormals
//
// Creates a new normal for a vertex using the average of all of the polygons it belongs to.
//
void GenerateVertexNormals(model_t *model)
{
	int i;
	for (i = 0; i < model->numMeshes; i++)
	{
		mesh_t *mesh = &model->meshes[i];

		if (!mesh->frames)
			continue;

		int j;
		for (j = 0; j < mesh->numFrames; j++)
		{
			mdlframe_t *frame = &mesh->frames[j];
			int memTag = PU_STATIC;
			float *newNormals = (float*)Z_Malloc(sizeof(float)*3*mesh->numTriangles*3, memTag, 0);
			M_Memcpy(newNormals, frame->normals, sizeof(float)*3*mesh->numTriangles*3);

/*			if (!systemSucks)
			{
				memTag = Z_GetTag(frame->tangents);
				float *newTangents = (float*)Z_Malloc(sizeof(float)*3*mesh->numTriangles*3, memTag);
				M_Memcpy(newTangents, frame->tangents, sizeof(float)*3*mesh->numTriangles*3);
			}*/

			int k;
			float *vertPtr = frame->vertices;
			for (k = 0; k < mesh->numVertices; k++)
			{
				float x, y, z;
				x = *vertPtr++;
				y = *vertPtr++;
				z = *vertPtr++;

				int vCount = 0;
				vector_t normal;
				normal.x = normal.y = normal.z = 0;
				int l;
				float *testPtr = frame->vertices;
				for (l = 0; l < mesh->numVertices; l++)
				{
					float testX, testY, testZ;
					testX = *testPtr++;
					testY = *testPtr++;
					testZ = *testPtr++;

					if (x != testX || y != testY || z != testZ)
						continue;

					// Found a vertex match! Add it...
					normal.x += frame->normals[3 * l + 0];
					normal.y += frame->normals[3 * l + 1];
					normal.z += frame->normals[3 * l + 2];
					vCount++;
				}

				if (vCount > 1)
				{
//					Vector::Normalize(&normal);
					newNormals[3 * k + 0] = (float)normal.x;
					newNormals[3 * k + 1] = (float)normal.y;
					newNormals[3 * k + 2] = (float)normal.z;

/*					if (!systemSucks)
					{
						Vector::vector_t tangent;
						Vector::Tangent(&normal, &tangent);
						newTangents[3 * k + 0] = tangent.x;
						newTangents[3 * k + 1] = tangent.y;
						newTangents[3 * k + 2] = tangent.z;
					}*/
				}
			}

			float *oldNormals = frame->normals;
			frame->normals = newNormals;
			Z_Free(oldNormals);

/*			if (!systemSucks)
			{
				float *oldTangents = frame->tangents;
				frame->tangents = newTangents;
				Z_Free(oldTangents);
			}*/
		}
	}
}

typedef struct materiallist_s
{
	struct materiallist_s *next;
	struct materiallist_s *prev;
	material_t *material;
} materiallist_t;

static boolean AddMaterialToList(materiallist_t **head, material_t *material)
{
	materiallist_t *node;
	for (node = *head; node; node = node->next)
	{
		if (node->material == material)
			return false;
	}

	// Didn't find it, so add to the list
	materiallist_t *newMatNode = (materiallist_t*)Z_Malloc(sizeof(materiallist_t), PU_CACHE, 0);
	newMatNode->material = material;
	ListAdd(newMatNode, (listitem_t**)head);
	return true;
}

//
// Optimize
//
// Groups triangles from meshes in the model
// Only works for models with 1 frame
//
void Optimize(model_t *model)
{
	if (model->numMeshes <= 1)
		return; // No need

	int numMeshes = 0;
	int i;
	materiallist_t *matListHead = NULL;
	for (i = 0; i < model->numMeshes; i++)
	{
		mesh_t *curMesh = &model->meshes[i];

		if (curMesh->numFrames > 1)
			return; // Can't optimize models with > 1 frame

		if (!curMesh->frames)
			return; // Don't optimize tinyframe models (no need)

		// We are condensing to 1 mesh per material, so
		// the # of materials we use will be the new
		// # of meshes
		if (AddMaterialToList(&matListHead, curMesh->frames[0].material))
			numMeshes++;
	}

	int memTag = PU_STATIC;
	mesh_t *newMeshes = (mesh_t*)Z_Calloc(sizeof(mesh_t) * numMeshes, memTag, 0);

	i = 0;
	materiallist_t *node;
	for (node = matListHead; node; node = node->next)
	{
		material_t *curMat = node->material;
		mesh_t *newMesh = &newMeshes[i];

		// Find all triangles with this material and count them
		int numTriangles = 0;
		int j;
		for (j = 0; j < model->numMeshes; j++)
		{
			mesh_t *curMesh = &model->meshes[j];

			if (curMesh->frames[0].material == curMat)
				numTriangles += curMesh->numTriangles;
		}

		newMesh->numFrames = 1;
		newMesh->numTriangles = numTriangles;
		newMesh->numVertices = numTriangles * 3;
		newMesh->uvs = (float*)Z_Malloc(sizeof(float)*2*numTriangles*3, memTag, 0);
//		if (node->material->lightmap)
//			newMesh->lightuvs = (float*)Z_Malloc(sizeof(float)*2*numTriangles*3, memTag, 0);
		newMesh->frames = (mdlframe_t*)Z_Calloc(sizeof(mdlframe_t), memTag, 0);
		mdlframe_t *curFrame = &newMesh->frames[0];

		curFrame->material = curMat;
		curFrame->normals = (float*)Z_Malloc(sizeof(float)*3*numTriangles*3, memTag, 0);
//		if (!systemSucks)
//			curFrame->tangents = (float*)Z_Malloc(sizeof(float)*3*numTriangles*3, memTag, 0);
		curFrame->vertices = (float*)Z_Malloc(sizeof(float)*3*numTriangles*3, memTag, 0);
		curFrame->colors = (byte*)Z_Malloc(sizeof(byte)*4*numTriangles*3, memTag, 0);

		// Now traverse the meshes of the model, adding in
		// vertices/normals/uvs that match the current material
		int uvCount = 0;
		int vertCount = 0;
		int colorCount = 0;
		for (j = 0; j < model->numMeshes; j++)
		{
			mesh_t *curMesh = &model->meshes[j];

			if (curMesh->frames[0].material == curMat)
			{
				float *dest;
				float *src;

				M_Memcpy(&newMesh->uvs[uvCount],
					curMesh->uvs,
					sizeof(float)*2*curMesh->numTriangles*3);

/*				if (node->material->lightmap)
				{
					M_Memcpy(&newMesh->lightuvs[uvCount],
						curMesh->lightuvs,
						sizeof(float)*2*curMesh->numTriangles*3);
				}*/
				uvCount += 2*curMesh->numTriangles*3;

				dest = (float*)newMesh->frames[0].vertices;
				src = (float*)curMesh->frames[0].vertices;
				M_Memcpy(&dest[vertCount],
					src,
					sizeof(float)*3*curMesh->numTriangles*3);

				dest = (float*)newMesh->frames[0].normals;
				src = (float*)curMesh->frames[0].normals;
				M_Memcpy(&dest[vertCount],
					src,
					sizeof(float)*3*curMesh->numTriangles*3);

/*				if (!systemSucks)
				{
					dest = (float*)newMesh->frames[0].tangents;
					src = (float*)curMesh->frames[0].tangents;
					M_Memcpy(&dest[vertCount],
						src,
						sizeof(float)*3*curMesh->numTriangles*3);
				}*/

				vertCount += 3 * curMesh->numTriangles * 3;

				byte *destByte;
				byte *srcByte;
				destByte = (byte*)newMesh->frames[0].colors;
				srcByte = (byte*)curMesh->frames[0].colors;

				if (srcByte)
				{
					M_Memcpy(&destByte[colorCount],
						srcByte,
						sizeof(byte)*4*curMesh->numTriangles*3);
				}
				else
				{
					memset(&destByte[colorCount],
						255,
						sizeof(byte)*4*curMesh->numTriangles*3);
				}

				colorCount += 4 * curMesh->numTriangles * 3;
			}
		}

		i++;
	}

	CONS_Printf("Model::Optimize(): Model reduced from %d to %d meshes.\n", model->numMeshes, numMeshes);
	model->meshes = newMeshes;
	model->numMeshes = numMeshes;
}

void GeneratePolygonNormals(model_t *model, int ztag)
{
	int i;
	for (i = 0; i < model->numMeshes; i++)
	{
		mesh_t *mesh = &model->meshes[i];

		if (!mesh->frames)
			continue;

		int j;
		for (j = 0; j < mesh->numFrames; j++)
		{
			mdlframe_t *frame = &mesh->frames[j];

			frame->polyNormals = (vector_t*)Z_Malloc(sizeof(vector_t) * mesh->numTriangles, ztag, 0);

			const float *vertices = frame->vertices;
			vector_t *polyNormals = frame->polyNormals;
			int k;
			for (k = 0; k < mesh->numTriangles; k++)
			{
//				Vector::Normal(vertices, polyNormals);
				vertices += 3 * 3;
				polyNormals++;
			}
		}
	}
}

//
// Reload
//
// Reload VBOs
//
#if 0
static void Reload(void)
{
/*	model_t *node;
	for (node = modelHead; node; node = node->next)
	{
		int i;
		for (i = 0; i < node->numMeshes; i++)
		{
			mesh_t *mesh = &node->meshes[i];

			if (mesh->frames)
			{
				int j;
				for (j = 0; j < mesh->numFrames; j++)
					CreateVBO(mesh, &mesh->frames[j]);
			}
			else if (mesh->tinyframes)
			{
				int j;
				for (j = 0; j < mesh->numFrames; j++)
					CreateVBO(mesh, &mesh->tinyframes[j]);
			}
		}
	}*/
}
#endif

void DeleteVBOs(model_t *model)
{
	(void)model;
/*	for (int i = 0; i < model->numMeshes; i++)
	{
		mesh_t *mesh = &model->meshes[i];

		if (mesh->frames)
		{
			for (int j = 0; j < mesh->numFrames; j++)
			{
				mdlframe_t *frame = &mesh->frames[j];
				if (!frame->vboID)
					continue;
				bglDeleteBuffers(1, &frame->vboID);
				frame->vboID = 0;
			}
		}
		else if (mesh->tinyframes)
		{
			for (int j = 0; j < mesh->numFrames; j++)
			{
				tinyframe_t *frame = &mesh->tinyframes[j];
				if (!frame->vboID)
					continue;
				bglDeleteBuffers(1, &frame->vboID);
				frame->vboID = 0;
			}
		}
	}*/
}
