// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2021 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file hw_shaders.c
/// \brief Handles the shaders used by the game.

#ifdef HWRENDER

#include "hw_glob.h"
#include "hw_drv.h"
#include "hw_shaders.h"
#include "../z_zone.h"

// ================
//  Shader sources
// ================

static struct {
	const char *vertex;
	const char *fragment;
} const gl_shadersources[] = {
	// Floor shader
	{GLSL_DEFAULT_VERTEX_SHADER, GLSL_FLOOR_FRAGMENT_SHADER},

	// Wall shader
	{GLSL_DEFAULT_VERTEX_SHADER, GLSL_WALL_FRAGMENT_SHADER},

	// Sprite shader
	{GLSL_DEFAULT_VERTEX_SHADER, GLSL_WALL_FRAGMENT_SHADER},

	// Model shader
	{GLSL_MODEL_VERTEX_SHADER, GLSL_MODEL_FRAGMENT_SHADER},

	// Water shader
	{GLSL_DEFAULT_VERTEX_SHADER, GLSL_WATER_FRAGMENT_SHADER},

	// Fog shader
	{GLSL_DEFAULT_VERTEX_SHADER, GLSL_FOG_FRAGMENT_SHADER},

	// Sky shader
	{GLSL_DEFAULT_VERTEX_SHADER, GLSL_SKY_FRAGMENT_SHADER},

	// Palette postprocess shader
	{GLSL_DEFAULT_VERTEX_SHADER, GLSL_PALETTE_POSTPROCESS_FRAGMENT_SHADER},

	// UI colormap fade shader
	{GLSL_DEFAULT_VERTEX_SHADER, GLSL_UI_COLORMAP_FADE_FRAGMENT_SHADER},

	// UI tinted wipe shader
	{GLSL_DEFAULT_VERTEX_SHADER, GLSL_UI_TINTED_WIPE_FRAGMENT_SHADER},

	{NULL, NULL},
};

typedef struct
{
	int base_shader; // index of base shader_t
	int custom_shader; // index of custom shader_t
} shadertarget_t;

typedef struct
{
	char *vertex;
	char *fragment;
	boolean compiled;
} shader_t; // these are in an array and accessed by indices

// the array has NUMSHADERTARGETS entries for base shaders and for custom shaders
// the array could be expanded in the future to fit "dynamic" custom shaders that
// aren't fixed to shader targets
static shader_t gl_shaders[NUMSHADERTARGETS*2];

static shadertarget_t gl_shadertargets[NUMSHADERTARGETS];

#define WHITESPACE_CHARS " \t"

#define MODEL_LIGHTING_DEFINE "#define SRB2_MODEL_LIGHTING"
#define PALETTE_RENDERING_DEFINE "#define SRB2_PALETTE_RENDERING"

// Initialize shader variables and the backend's shader system. Load the base shaders.
// Returns false if shaders cannot be used.
boolean HWR_InitShaders(void)
{
	int i;

	if (!HWD.pfnInitShaders())
		return false;

	for (i = 0; i < NUMSHADERTARGETS; i++)
	{
		// set up string pointers for base shaders
		gl_shaders[i].vertex = Z_StrDup(gl_shadersources[i].vertex);
		gl_shaders[i].fragment = Z_StrDup(gl_shadersources[i].fragment);
		// set shader target indices to correct values
		gl_shadertargets[i].base_shader = i;
		gl_shadertargets[i].custom_shader = -1;
	}

	HWR_CompileShaders();

	return true;
}

// helper function: strstr but returns an int with the substring position
// returns INT32_MAX if not found
static INT32 strstr_int(const char *str1, const char *str2)
{
	char *location = strstr(str1, str2);
	if (location)
		return location - str1;
	else
		return INT32_MAX;
}

// Creates a preprocessed copy of the shader according to the current graphics settings
// Returns a pointer to the results on success and NULL on failure.
// Remember memory management of the returned string.
static char *HWR_PreprocessShader(char *original)
{
	const char *line_ending = "\n";
	int line_ending_len;
	char *read_pos = original;
	int original_len = strlen(original);
	int distance_to_end = original_len;
	int new_len;
	char *new_shader;
	char *write_pos;
	char shader_glsl_version[3];
	int version_pos = -1;
	int version_len = 0;

	if (strstr(original, "\r\n"))
	{
		line_ending = "\r\n";
		// check if all line endings are same
		while ((read_pos = strchr(read_pos, '\n')))
		{
			read_pos--;
			if (*read_pos != '\r')
			{
				// this file contains mixed CRLF and LF line endings.
				// treating it as a LF file during parsing should keep
				// the results sane enough as long as the gpu driver is fine
				// with these kinds of weirdly formatted shader sources.
				line_ending = "\n";
				break;
			}
			read_pos += 2;
		}
		read_pos = original;
	}

	line_ending_len = strlen(line_ending);

	// Find the #version directive, if it exists. Also don't get fooled if it's
	// inside a comment. Copy the version digits so they can be used in the preamble.
	// Time for some string parsing :D

#define STARTSWITH(str, with_what) !strncmp(str, with_what, sizeof(with_what)-1)
#define ADVANCE(amount) read_pos += (amount); distance_to_end -= (amount);
	while (true)
	{
		// we're at the start of a line or at the end of a block comment.
		// first get any possible whitespace out of the way
		int whitespace_len = strspn(read_pos, WHITESPACE_CHARS);
		if (whitespace_len == distance_to_end)
			break; // we got to the end
		ADVANCE(whitespace_len)

		if (STARTSWITH(read_pos, "#version"))
		{
			// found a version directive (and it's not inside a comment)
			// now locate, verify and read the version number
			int version_number_len;
			version_pos = read_pos - original;
			ADVANCE(sizeof("#version") - 1)
			whitespace_len = strspn(read_pos, WHITESPACE_CHARS);
			if (!whitespace_len)
			{
				CONS_Alert(CONS_ERROR, "HWR_PreprocessShader: Syntax error in #version. Expected space after #version, but got other text.\n");
				return NULL;
			}
			else if (whitespace_len == distance_to_end)
			{
				CONS_Alert(CONS_ERROR, "HWR_PreprocessShader: Syntax error in #version. Expected version number, but got end of file.\n");
				return NULL;
			}
			ADVANCE(whitespace_len)
			version_number_len = strspn(read_pos, "0123456789");
			if (!version_number_len)
			{
				CONS_Alert(CONS_ERROR, "HWR_PreprocessShader: Syntax error in #version. Expected version number, but got other text.\n");
				return NULL;
			}
			else if (version_number_len != 3)
			{
				CONS_Alert(CONS_ERROR, "HWR_PreprocessShader: Syntax error in #version. Expected version with 3 digits, but got %d digits.\n", version_number_len);
				return NULL;
			}
			M_Memcpy(shader_glsl_version, read_pos, 3);
			ADVANCE(version_number_len)
			version_len = (read_pos - original) - version_pos;
			whitespace_len = strspn(read_pos, WHITESPACE_CHARS);
			ADVANCE(whitespace_len)
			if (STARTSWITH(read_pos, "es"))
			{
				CONS_Alert(CONS_ERROR, "HWR_PreprocessShader: Support for ES shaders is not implemented.\n");
				return NULL;
			}
			break;
		}
		else
		{
			// go to next newline or end of next block comment if it starts before the newline
			// and is not inside a line comment
			INT32 newline_pos = strstr_int(read_pos, line_ending);
			INT32 line_comment_pos;
			INT32 block_comment_pos;
			// optimization: temporarily put a null at the line ending, so strstr does not needlessly
			// look past it since we're only interested in the current line
			if (newline_pos != INT32_MAX)
				read_pos[newline_pos] = '\0';
			line_comment_pos = strstr_int(read_pos, "//");
			block_comment_pos = strstr_int(read_pos, "/*");
			// restore the line ending, remove the null we just put there
			if (newline_pos != INT32_MAX)
				read_pos[newline_pos] = line_ending[0];
			if (line_comment_pos < block_comment_pos)
			{
				// line comment found, skip rest of the line
				if (newline_pos != INT32_MAX)
				{
					ADVANCE(newline_pos + line_ending_len)
				}
				else
				{
					// we got to the end
					break;
				}
			}
			else if (block_comment_pos < line_comment_pos)
			{
				// block comment found, skip past it
				INT32 block_comment_end;
				ADVANCE(block_comment_pos + 2)
				block_comment_end = strstr_int(read_pos, "*/");
				if (block_comment_end == INT32_MAX)
				{
					// could also leave insertion_pos at 0 and let the GLSL compiler
					// output an error message for this broken comment
					CONS_Alert(CONS_ERROR, "HWR_PreprocessShader: Encountered unclosed block comment in shader.\n");
					return NULL;
				}
				ADVANCE(block_comment_end + 2)
			}
			else if (newline_pos == INT32_MAX)
			{
				// we got to the end
				break;
			}
			else
			{
				// nothing special on this line, move to the next one
				ADVANCE(newline_pos + line_ending_len)
			}
		}
	}
#undef STARTSWITH
#undef ADVANCE

#define ADD_TO_LEN(def) new_len += sizeof(def) - 1 + line_ending_len;

	// Calculate length of modified shader.
	new_len = original_len;
	if (cv_glmodellighting.value)
		ADD_TO_LEN(MODEL_LIGHTING_DEFINE)
	if (cv_glpaletterendering.value)
		ADD_TO_LEN(PALETTE_RENDERING_DEFINE)

#undef ADD_TO_LEN

#define VERSION_PART "#version "

	if (new_len != original_len)
	{
		if (version_pos != -1)
			new_len += sizeof(VERSION_PART) - 1 + 3 + line_ending_len;
		new_len += sizeof("#line 0") - 1 + line_ending_len;
	}

	// Allocate memory for modified shader.
	new_shader = Z_Malloc(new_len + 1, PU_STATIC, NULL);

	read_pos = original;
	write_pos = new_shader;

	if (new_len != original_len && version_pos != -1)
	{
		strcpy(write_pos, VERSION_PART);
		write_pos += sizeof(VERSION_PART) - 1;
		M_Memcpy(write_pos, shader_glsl_version, 3);
		write_pos += 3;
		strcpy(write_pos, line_ending);
		write_pos += line_ending_len;
	}

#undef VERSION_PART

#define WRITE_DEFINE(define) \
	{ \
		strcpy(write_pos, define); \
		write_pos += sizeof(define) - 1; \
		strcpy(write_pos, line_ending); \
		write_pos += line_ending_len; \
	}

	// Write the defines.
	if (cv_glmodellighting.value)
		WRITE_DEFINE(MODEL_LIGHTING_DEFINE)
	if (cv_glpaletterendering.value)
		WRITE_DEFINE(PALETTE_RENDERING_DEFINE)

#undef WRITE_DEFINE

	// Write a #line directive, so compiler errors will report line numbers from the
	// original shader without our preamble lines.
	if (new_len != original_len)
	{
		// line numbering in the #line directive is different for versions 110-150
		if (version_pos == -1 || shader_glsl_version[0] == '1')
			strcpy(write_pos, "#line 0");
		else
			strcpy(write_pos, "#line 1");
		write_pos += sizeof("#line 0") - 1;
		strcpy(write_pos, line_ending);
		write_pos += line_ending_len;
	}

	// Copy the original shader.
	M_Memcpy(write_pos, read_pos, original_len);

	// Erase the original #version directive, if it exists and was copied.
	if (new_len != original_len && version_pos != -1)
		memset(write_pos + version_pos, ' ', version_len);

	// Terminate the new string.
	new_shader[new_len] = '\0';

	return new_shader;
}

// preprocess and compile shader at gl_shaders[index]
static void HWR_CompileShader(int index)
{
	char *vertex_source = gl_shaders[index].vertex;
	char *fragment_source = gl_shaders[index].fragment;

	if (vertex_source)
	{
		char *preprocessed = HWR_PreprocessShader(vertex_source);
		if (!preprocessed) return;
		HWD.pfnLoadShader(index, preprocessed, HWD_SHADERSTAGE_VERTEX);
	}
	if (fragment_source)
	{
		char *preprocessed = HWR_PreprocessShader(fragment_source);
		if (!preprocessed) return;
		HWD.pfnLoadShader(index, preprocessed, HWD_SHADERSTAGE_FRAGMENT);
	}

	gl_shaders[index].compiled = HWD.pfnCompileShader(index);
}

// compile or recompile shaders
void HWR_CompileShaders(void)
{
	int i;

	for (i = 0; i < NUMSHADERTARGETS; i++)
	{
		int custom_index = gl_shadertargets[i].custom_shader;
		HWR_CompileShader(i);
		if (!gl_shaders[i].compiled)
			CONS_Alert(CONS_ERROR, "HWR_CompileShaders: Compilation failed for base %s shader!\n", shaderxlat[i].type);
		if (custom_index != -1)
		{
			HWR_CompileShader(custom_index);
			if (!gl_shaders[custom_index].compiled)
				CONS_Alert(CONS_ERROR, "HWR_CompileShaders: Recompilation failed for the custom %s shader! See the console messages above for more information.\n", shaderxlat[i].type);
		}
	}
}

int HWR_GetShaderFromTarget(int shader_target)
{
	int custom_shader = gl_shadertargets[shader_target].custom_shader;
	// use custom shader if following are true
	// - custom shader exists
	// - custom shader has been compiled successfully
	// - custom shaders are enabled
	// - custom shaders are allowed by the server
	if (custom_shader != -1 && gl_shaders[custom_shader].compiled &&
		cv_glshaders.value == 1 && cv_glallowshaders.value)
		return custom_shader;
	else
		return gl_shadertargets[shader_target].base_shader;
}

static inline UINT16 HWR_FindShaderDefs(UINT16 wadnum)
{
	UINT16 i;
	lumpinfo_t *lump_p;

	lump_p = wadfiles[wadnum]->lumpinfo;
	for (i = 0; i < wadfiles[wadnum]->numlumps; i++, lump_p++)
		if (memcmp(lump_p->name, "SHADERS", 7) == 0)
			return i;

	return INT16_MAX;
}

customshaderxlat_t shaderxlat[] =
{
	{"Flat", SHADER_FLOOR},
	{"WallTexture", SHADER_WALL},
	{"Sprite", SHADER_SPRITE},
	{"Model", SHADER_MODEL},
	{"WaterRipple", SHADER_WATER},
	{"Fog", SHADER_FOG},
	{"Sky", SHADER_SKY},
	{"PalettePostprocess", SHADER_PALETTE_POSTPROCESS},
	{"UIColormapFade", SHADER_UI_COLORMAP_FADE},
	{"UITintedWipe", SHADER_UI_TINTED_WIPE},
	{NULL, 0},
};

void HWR_LoadAllCustomShaders(void)
{
	INT32 i;

	// read every custom shader
	for (i = 0; i < numwadfiles; i++)
		HWR_LoadCustomShadersFromFile(i, W_FileHasFolders(wadfiles[i]));
}

static const char version_directives[][14] = {
	"#version 330\n",
	"#version 150\n",
	"#version 140\n",
	"#version 130\n",
	"#version 120\n",
	"#version 110\n",
};

static boolean HWR_VersionDirectiveExists(const char* source)
{
    return strncmp(source, "#version", 8) == 0;
}

static char* HWR_PrependVersionDirective(const char* source, UINT32 version_index)
{
	const UINT32 version_len = sizeof(version_directives[version_index]) - 1;
	const UINT32 source_len = strlen(source);

	char* result = Z_Malloc(source_len + version_len + 1, PU_STATIC, NULL);
	strcpy(result, version_directives[version_index]);
	strcpy(result + version_len, source);

	return result;
}

static void HWR_ReplaceVersionInplace(char* shader, UINT32 version_index)
{
	shader[9] = version_directives[version_index][9];
	shader[10] = version_directives[version_index][10];
	shader[11] = version_directives[version_index][11];
}

static boolean HWR_CheckVersionDirectives(const char* vert, const char* frag)
{
	return HWR_VersionDirectiveExists(vert) && HWR_VersionDirectiveExists(frag);
}

static void HWR_TryToCompileShaderWithImplicitVersion(INT32 shader_index, INT32 shaderxlat_id)
{
	char* vert_shader = gl_shaders[shader_index].vertex;
	char* frag_shader = gl_shaders[shader_index].fragment;

	boolean vert_shader_version_exists = HWR_VersionDirectiveExists(vert_shader);
	boolean frag_shader_version_exists = HWR_VersionDirectiveExists(frag_shader);

	if(!vert_shader_version_exists) {
		CONS_Alert(CONS_WARNING, "HWR_LoadCustomShadersFromFile: vertex shader '%s' is missing a #version directive\n", HWR_GetShaderName(shaderxlat_id));
	}

	if(!frag_shader_version_exists) {
		CONS_Alert(CONS_WARNING, "HWR_LoadCustomShadersFromFile: fragment shader '%s' is missing a #version directive\n", HWR_GetShaderName(shaderxlat_id));
	}

	// try to compile as is
	HWR_CompileShader(shader_index);
	if (gl_shaders[shader_index].compiled)
		return;

	// try each version directive
	for(UINT32 i = 0; i < sizeof(version_directives) / sizeof(version_directives[0]); ++i) {
		CONS_Alert(CONS_NOTICE, "HWR_TryToCompileShaderWithImplicitVersion: Trying %s\n", version_directives[i]);

		if(!vert_shader_version_exists) {
			// first time reallocation would have to be made

			if(i == 0) {
				void* old = (void*)gl_shaders[shader_index].vertex;
				vert_shader = gl_shaders[shader_index].vertex = HWR_PrependVersionDirective(vert_shader, i);
				Z_Free(old);
			} else {
				HWR_ReplaceVersionInplace(vert_shader, i);
			}
		}

		if(!frag_shader_version_exists) {
			if(i == 0) {
				void* old = (void*)gl_shaders[shader_index].fragment;
				frag_shader = gl_shaders[shader_index].fragment = HWR_PrependVersionDirective(frag_shader, i);
				Z_Free(old);
			} else {
				HWR_ReplaceVersionInplace(frag_shader, i);
			}
		}

		HWR_CompileShader(shader_index);
		if (gl_shaders[shader_index].compiled) {
			CONS_Alert(CONS_NOTICE, "HWR_TryToCompileShaderWithImplicitVersion: Compiled with %s\n",
					   version_directives[i]);
			CONS_Alert(CONS_WARNING, "Implicit GLSL version is used. Correct behavior is not guaranteed\n");
			return;
		}
	}
}

void HWR_LoadCustomShadersFromFile(UINT16 wadnum, boolean PK3)
{
	UINT16 lump;
	char *shaderdef, *line;
	char *stoken;
	char *value;
	size_t size;
	int linenum = 1;
	int shadertype = 0;
	int i;
	boolean modified_shaders[NUMSHADERTARGETS] = {0};

	if (!gl_shadersavailable)
		return;

	lump = HWR_FindShaderDefs(wadnum);
	if (lump == INT16_MAX)
		return;

	shaderdef = W_CacheLumpNumPwad(wadnum, lump, PU_CACHE);
	size = W_LumpLengthPwad(wadnum, lump);

	line = Z_Malloc(size+1, PU_STATIC, NULL);
	M_Memcpy(line, shaderdef, size);
	line[size] = '\0';

	stoken = strtok(line, "\r\n ");
	while (stoken)
	{
		if ((stoken[0] == '/' && stoken[1] == '/')
			|| (stoken[0] == '#'))// skip comments
		{
			stoken = strtok(NULL, "\r\n");
			goto skip_field;
		}

		if (!stricmp(stoken, "GLSL"))
		{
			value = strtok(NULL, "\r\n ");
			if (!value)
			{
				CONS_Alert(CONS_WARNING, "HWR_LoadCustomShadersFromFile: Missing shader type (file %s, line %d)\n", wadfiles[wadnum]->filename, linenum);
				stoken = strtok(NULL, "\r\n"); // skip end of line
				goto skip_lump;
			}

			if (!stricmp(value, "VERTEX"))
				shadertype = 1;
			else if (!stricmp(value, "FRAGMENT"))
				shadertype = 2;

skip_lump:
			stoken = strtok(NULL, "\r\n ");
			linenum++;
		}
		else
		{
			value = strtok(NULL, "\r\n= ");
			if (!value)
			{
				CONS_Alert(CONS_WARNING, "HWR_LoadCustomShadersFromFile: Missing shader target (file %s, line %d)\n", wadfiles[wadnum]->filename, linenum);
				stoken = strtok(NULL, "\r\n"); // skip end of line
				goto skip_field;
			}

			if (!shadertype)
			{
				CONS_Alert(CONS_ERROR, "HWR_LoadCustomShadersFromFile: Missing shader type (file %s, line %d)\n", wadfiles[wadnum]->filename, linenum);
				Z_Free(line);
				return;
			}

			for (i = 0; shaderxlat[i].type; i++)
			{
				if (!stricmp(shaderxlat[i].type, stoken))
				{
					size_t shader_string_length;
					char *shader_source;
					char *shader_lumpname;
					UINT16 shader_lumpnum;
					int shader_index; // index in gl_shaders

					if (PK3)
					{
						shader_lumpname = Z_Malloc(strlen(value) + 12, PU_STATIC, NULL);
						strcpy(shader_lumpname, "Shaders/sh_");
						strcat(shader_lumpname, value);
						shader_lumpnum = W_CheckNumForFullNamePK3(shader_lumpname, wadnum, 0);
					}
					else
					{
						shader_lumpname = Z_Malloc(strlen(value) + 4, PU_STATIC, NULL);
						strcpy(shader_lumpname, "SH_");
						strcat(shader_lumpname, value);
						shader_lumpnum = W_CheckNumForNamePwad(shader_lumpname, wadnum, 0);
					}

					if (shader_lumpnum == INT16_MAX)
					{
						CONS_Alert(CONS_ERROR, "HWR_LoadCustomShadersFromFile: Missing shader source %s (file %s, line %d)\n", shader_lumpname, wadfiles[wadnum]->filename, linenum);
						Z_Free(shader_lumpname);
						continue;
					}

					shader_string_length = W_LumpLengthPwad(wadnum, shader_lumpnum) + 1;
					shader_source = Z_Malloc(shader_string_length, PU_STATIC, NULL);
					W_ReadLumpPwad(wadnum, shader_lumpnum, shader_source);
					shader_source[shader_string_length-1] = '\0';

					shader_index = shaderxlat[i].id + NUMSHADERTARGETS;
					if (!modified_shaders[shaderxlat[i].id])
					{
						// this will clear any old custom shaders from previously loaded files
						// Z_Free checks if the pointer is NULL!
						Z_Free(gl_shaders[shader_index].vertex);
						gl_shaders[shader_index].vertex = NULL;
						Z_Free(gl_shaders[shader_index].fragment);
						gl_shaders[shader_index].fragment = NULL;
					}
					modified_shaders[shaderxlat[i].id] = true;

					if (shadertype == 1)
					{
						if (gl_shaders[shader_index].vertex)
						{
							CONS_Alert(CONS_WARNING, "HWR_LoadCustomShadersFromFile: %s is overwriting another %s vertex shader from the same addon! (file %s, line %d)\n", shader_lumpname, shaderxlat[i].type, wadfiles[wadnum]->filename, linenum);
							Z_Free(gl_shaders[shader_index].vertex);
						}
						gl_shaders[shader_index].vertex = shader_source;
					}
					else
					{
						if (gl_shaders[shader_index].fragment)
						{
							CONS_Alert(CONS_WARNING, "HWR_LoadCustomShadersFromFile: %s is overwriting another %s fragment shader from the same addon! (file %s, line %d)\n", shader_lumpname, shaderxlat[i].type, wadfiles[wadnum]->filename, linenum);
							Z_Free(gl_shaders[shader_index].fragment);
						}
						gl_shaders[shader_index].fragment = shader_source;
					}

					Z_Free(shader_lumpname);
				}
			}

skip_field:
			stoken = strtok(NULL, "\r\n= ");
			linenum++;
		}
	}

	for (i = 0; i < NUMSHADERTARGETS; i++)
	{
		if (modified_shaders[i])
		{
			int shader_index = i + NUMSHADERTARGETS; // index to gl_shaders
			gl_shadertargets[i].custom_shader = shader_index;
			// if only one stage (vertex/fragment) is defined, the other one
			// is copied from the base shaders.
			if (!gl_shaders[shader_index].fragment)
				gl_shaders[shader_index].fragment = Z_StrDup(gl_shadersources[i].fragment);
			if (!gl_shaders[shader_index].vertex)
				gl_shaders[shader_index].vertex = Z_StrDup(gl_shadersources[i].vertex);

			if(!HWR_CheckVersionDirectives(gl_shaders[shader_index].vertex, gl_shaders[shader_index].fragment)) {
				HWR_TryToCompileShaderWithImplicitVersion(shader_index, i);
			} else {
				HWR_CompileShader(shader_index);
			}

			if (!gl_shaders[shader_index].compiled)
				CONS_Alert(CONS_ERROR, "HWR_LoadCustomShadersFromFile: A compilation error occured for the %s shader in file %s. See the console messages above for more information.\n", shaderxlat[i].type, wadfiles[wadnum]->filename);
		}
	}

	Z_Free(line);
	return;
}

const char *HWR_GetShaderName(INT32 shader)
{
	INT32 i;

	for (i = 0; shaderxlat[i].type; i++)
	{
		if (shaderxlat[i].id == shader)
			return shaderxlat[i].type;
	}

	return "Unknown";
}

#endif // HWRENDER
