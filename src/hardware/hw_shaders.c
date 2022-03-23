// SONIC ROBO BLAST 2
//-----------------------------------------------------------------------------
// Copyright (C) 2021 by Sonic Team Junior.
//
// This program is free software distributed under the
// terms of the GNU General Public License, version 2.
// See the 'LICENSE' file for more details.
//-----------------------------------------------------------------------------
/// \file hw_shaders.h
/// \brief Handles the shaders used by the game.

#ifdef HWRENDER

#include "hw_glob.h"
#include "hw_drv.h"
#include "../z_zone.h"

// ================
//  Vertex shaders
// ================

//
// Generic vertex shader
//

#define GLSL_DEFAULT_VERTEX_SHADER \
	"void main()\n" \
	"{\n" \
		"gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * gl_Vertex;\n" \
		"gl_FrontColor = gl_Color;\n" \
		"gl_TexCoord[0].xy = gl_MultiTexCoord0.xy;\n" \
		"gl_ClipVertex = gl_ModelViewMatrix * gl_Vertex;\n" \
	"}\0"

// replicates the way fixed function lighting is used by the model lighting option,
// stores the lighting result to gl_Color
// (ambient lighting of 0.75 and diffuse lighting from above)
#define GLSL_MODEL_VERTEX_SHADER \
	"void main()\n" \
	"{\n" \
		"#ifdef SRB2_MODEL_LIGHTING\n" \
		"float nDotVP = dot(gl_Normal, vec3(0, 1, 0));\n" \
		"float light = min(0.75 + max(nDotVP, 0.0), 1.0);\n" \
		"gl_FrontColor = vec4(light, light, light, 1.0);\n" \
		"#else\n" \
		"gl_FrontColor = gl_Color;\n" \
		"#endif\n" \
		"gl_Position = gl_ProjectionMatrix * gl_ModelViewMatrix * gl_Vertex;\n" \
		"gl_TexCoord[0].xy = gl_MultiTexCoord0.xy;\n" \
		"gl_ClipVertex = gl_ModelViewMatrix * gl_Vertex;\n" \
	"}\0"

// ==================
//  Fragment shaders
// ==================

//
// Generic fragment shader
//

#define GLSL_DEFAULT_FRAGMENT_SHADER \
	"uniform sampler2D tex;\n" \
	"uniform vec4 poly_color;\n" \
	"void main(void) {\n" \
		"gl_FragColor = texture2D(tex, gl_TexCoord[0].st) * poly_color;\n" \
	"}\0"

//
// Software fragment shader
//

// Include GLSL_FLOOR_FUDGES or GLSL_WALL_FUDGES or define the fudges in shaders that use this macro.
#define GLSL_DOOM_COLORMAP \
	"float R_DoomColormap(float light, float z)\n" \
	"{\n" \
		"float lightnum = clamp(light / 17.0, 0.0, 15.0);\n" \
		"float lightz = clamp(z / 16.0, 0.0, 127.0);\n" \
		"float startmap = (15.0 - lightnum) * 4.0;\n" \
		"float scale = 160.0 / (lightz + 1.0);\n" \
		"float cap = (155.0 - light) * 0.26;\n" \
		"return max(startmap * STARTMAP_FUDGE - scale * 0.5 * SCALE_FUDGE, cap);\n" \
	"}\n"
// lighting cap adjustment:
// first num (155.0), increase to make it start to go dark sooner
// second num (0.26), increase to make it go dark faster

#define GLSL_DOOM_LIGHT_EQUATION \
	"float R_DoomLightingEquation(float light)\n" \
	"{\n" \
		"float z = gl_FragCoord.z / gl_FragCoord.w;\n" \
		"float colormap = floor(R_DoomColormap(light, z)) + 0.5;\n" \
		"return clamp(colormap, 0.0, 31.0) / 32.0;\n" \
	"}\n"

#define GLSL_SOFTWARE_TINT_EQUATION \
	"if (tint_color.a > 0.0) {\n" \
		"float color_bright = sqrt((base_color.r * base_color.r) + (base_color.g * base_color.g) + (base_color.b * base_color.b));\n" \
		"float strength = sqrt(9.0 * tint_color.a);\n" \
		"final_color.r = clamp((color_bright * (tint_color.r * strength)) + (base_color.r * (1.0 - strength)), 0.0, 1.0);\n" \
		"final_color.g = clamp((color_bright * (tint_color.g * strength)) + (base_color.g * (1.0 - strength)), 0.0, 1.0);\n" \
		"final_color.b = clamp((color_bright * (tint_color.b * strength)) + (base_color.b * (1.0 - strength)), 0.0, 1.0);\n" \
	"}\n"

#define GLSL_SOFTWARE_FADE_EQUATION \
	"float darkness = R_DoomLightingEquation(lighting);\n" \
	"if (fade_start != 0.0 || fade_end != 31.0) {\n" \
		"float fs = fade_start / 31.0;\n" \
		"float fe = fade_end / 31.0;\n" \
		"float fd = fe - fs;\n" \
		"darkness = clamp((darkness - fs) * (1.0 / fd), 0.0, 1.0);\n" \
	"}\n" \
	"final_color = mix(final_color, fade_color, darkness);\n"

#define GLSL_PALETTE_RENDERING \
	"float tex_pal_idx = texture3D(palette_lookup_tex, vec3((texel * 63.0 + 0.5) / 64.0))[0] * 255.0;\n" \
	"float z = gl_FragCoord.z / gl_FragCoord.w;\n" \
	"float light_y = clamp(floor(R_DoomColormap(lighting, z)), 0.0, 31.0);\n" \
	"vec2 lighttable_coord = vec2((tex_pal_idx + 0.5) / 256.0, (light_y + 0.5) / 32.0);\n" \
	"vec4 final_color = texture2D(lighttable_tex, lighttable_coord);\n" \
	"final_color.a = texel.a * poly_color.a;\n" \
	"gl_FragColor = final_color;\n" \

#define GLSL_SOFTWARE_FRAGMENT_SHADER \
	"#ifdef SRB2_PALETTE_RENDERING\n" \
	"uniform sampler2D tex;\n" \
	"uniform sampler3D palette_lookup_tex;\n" \
	"uniform sampler2D lighttable_tex;\n" \
	"uniform vec4 poly_color;\n" \
	"uniform float lighting;\n" \
	GLSL_DOOM_COLORMAP \
	"void main(void) {\n" \
		"vec4 texel = texture2D(tex, gl_TexCoord[0].st);\n" \
		GLSL_PALETTE_RENDERING \
	"}\n" \
	"#else\n" \
	"uniform sampler2D tex;\n" \
	"uniform vec4 poly_color;\n" \
	"uniform vec4 tint_color;\n" \
	"uniform vec4 fade_color;\n" \
	"uniform float lighting;\n" \
	"uniform float fade_start;\n" \
	"uniform float fade_end;\n" \
	GLSL_DOOM_COLORMAP \
	GLSL_DOOM_LIGHT_EQUATION \
	"void main(void) {\n" \
		"vec4 texel = texture2D(tex, gl_TexCoord[0].st);\n" \
		"vec4 base_color = texel * poly_color;\n" \
		"vec4 final_color = base_color;\n" \
		GLSL_SOFTWARE_TINT_EQUATION \
		GLSL_SOFTWARE_FADE_EQUATION \
		"final_color.a = texel.a * poly_color.a;\n" \
		"gl_FragColor = final_color;\n" \
	"}\n" \
	"#endif\0"

// hand tuned adjustments for light level calculation
#define GLSL_FLOOR_FUDGES \
	"#define STARTMAP_FUDGE 1.06\n" \
	"#define SCALE_FUDGE 1.15\n"

#define GLSL_WALL_FUDGES \
	"#define STARTMAP_FUDGE 1.05\n" \
	"#define SCALE_FUDGE 2.2\n"

#define GLSL_FLOOR_FRAGMENT_SHADER \
	GLSL_FLOOR_FUDGES \
	GLSL_SOFTWARE_FRAGMENT_SHADER

#define GLSL_WALL_FRAGMENT_SHADER \
	GLSL_WALL_FUDGES \
	GLSL_SOFTWARE_FRAGMENT_SHADER

// same as above but multiplies results with the lighting value from the
// accompanying vertex shader (stored in gl_Color) if model lighting is enabled
#define GLSL_MODEL_FRAGMENT_SHADER \
	GLSL_WALL_FUDGES \
	"#ifdef SRB2_PALETTE_RENDERING\n" \
	"uniform sampler2D tex;\n" \
	"uniform sampler3D palette_lookup_tex;\n" \
	"uniform sampler2D lighttable_tex;\n" \
	"uniform vec4 poly_color;\n" \
	"uniform float lighting;\n" \
	GLSL_DOOM_COLORMAP \
	"void main(void) {\n" \
		"vec4 texel = texture2D(tex, gl_TexCoord[0].st);\n" \
		"#ifdef SRB2_MODEL_LIGHTING\n" \
		"texel *= gl_Color;\n" \
		"#endif\n" \
		GLSL_PALETTE_RENDERING \
	"}\n" \
	"#else\n" \
	"uniform sampler2D tex;\n" \
	"uniform vec4 poly_color;\n" \
	"uniform vec4 tint_color;\n" \
	"uniform vec4 fade_color;\n" \
	"uniform float lighting;\n" \
	"uniform float fade_start;\n" \
	"uniform float fade_end;\n" \
	GLSL_DOOM_COLORMAP \
	GLSL_DOOM_LIGHT_EQUATION \
	"void main(void) {\n" \
		"vec4 texel = texture2D(tex, gl_TexCoord[0].st);\n" \
		"vec4 base_color = texel * poly_color;\n" \
		"vec4 final_color = base_color;\n" \
		GLSL_SOFTWARE_TINT_EQUATION \
		GLSL_SOFTWARE_FADE_EQUATION \
		"#ifdef SRB2_MODEL_LIGHTING\n" \
		"final_color *= gl_Color;\n" \
		"#endif\n" \
		"final_color.a = texel.a * poly_color.a;\n" \
		"gl_FragColor = final_color;\n" \
	"}\n" \
	"#endif\0"

//
// Water surface shader
//
// Mostly guesstimated, rather than the rest being built off Software science.
// Still needs to distort things underneath/around the water...
//

#define GLSL_WATER_TEXEL \
	"float water_z = (gl_FragCoord.z / gl_FragCoord.w) / 2.0;\n" \
	"float a = -pi * (water_z * freq) + (leveltime * speed);\n" \
	"float sdistort = sin(a) * amp;\n" \
	"float cdistort = cos(a) * amp;\n" \
	"vec4 texel = texture2D(tex, vec2(gl_TexCoord[0].s - sdistort, gl_TexCoord[0].t - cdistort));\n"

#define GLSL_WATER_FRAGMENT_SHADER \
	GLSL_FLOOR_FUDGES \
	"const float freq = 0.025;\n" \
	"const float amp = 0.025;\n" \
	"const float speed = 2.0;\n" \
	"const float pi = 3.14159;\n" \
	"#ifdef SRB2_PALETTE_RENDERING\n" \
	"uniform sampler2D tex;\n" \
	"uniform sampler3D palette_lookup_tex;\n" \
	"uniform sampler2D lighttable_tex;\n" \
	"uniform vec4 poly_color;\n" \
	"uniform float lighting;\n" \
	"uniform float leveltime;\n" \
	GLSL_DOOM_COLORMAP \
	"void main(void) {\n" \
		GLSL_WATER_TEXEL \
		GLSL_PALETTE_RENDERING \
	"}\n" \
	"#else\n" \
	"uniform sampler2D tex;\n" \
	"uniform vec4 poly_color;\n" \
	"uniform vec4 tint_color;\n" \
	"uniform vec4 fade_color;\n" \
	"uniform float lighting;\n" \
	"uniform float fade_start;\n" \
	"uniform float fade_end;\n" \
	"uniform float leveltime;\n" \
	GLSL_DOOM_COLORMAP \
	GLSL_DOOM_LIGHT_EQUATION \
	"void main(void) {\n" \
		GLSL_WATER_TEXEL \
		"vec4 base_color = texel * poly_color;\n" \
		"vec4 final_color = base_color;\n" \
		GLSL_SOFTWARE_TINT_EQUATION \
		GLSL_SOFTWARE_FADE_EQUATION \
		"final_color.a = texel.a * poly_color.a;\n" \
		"gl_FragColor = final_color;\n" \
	"}\n" \
	"#endif\0"

//
// Fog block shader
//
// Alpha of the planes themselves are still slightly off -- see HWR_FogBlockAlpha
//

// The floor fudges are used, but should the wall fudges be used instead? or something inbetween?
// or separate values for floors and walls? (need to change more than this shader for that)
#define GLSL_FOG_FRAGMENT_SHADER \
	GLSL_FLOOR_FUDGES \
	"uniform vec4 tint_color;\n" \
	"uniform vec4 fade_color;\n" \
	"uniform float lighting;\n" \
	"uniform float fade_start;\n" \
	"uniform float fade_end;\n" \
	GLSL_DOOM_COLORMAP \
	GLSL_DOOM_LIGHT_EQUATION \
	"void main(void) {\n" \
		"vec4 base_color = gl_Color;\n" \
		"vec4 final_color = base_color;\n" \
		GLSL_SOFTWARE_TINT_EQUATION \
		GLSL_SOFTWARE_FADE_EQUATION \
		"gl_FragColor = final_color;\n" \
	"}\0"

//
// Sky fragment shader
// Modulates poly_color with gl_Color
//
#define GLSL_SKY_FRAGMENT_SHADER \
	"uniform sampler2D tex;\n" \
	"uniform vec4 poly_color;\n" \
	"void main(void) {\n" \
		"gl_FragColor = texture2D(tex, gl_TexCoord[0].st) * gl_Color * poly_color;\n" \
	"}\0"

// Shader for the palette rendering postprocess step
#define GLSL_PALETTE_POSTPROCESS_FRAGMENT_SHADER \
	"uniform sampler2D tex;\n" \
	"uniform sampler3D palette_lookup_tex;\n" \
	"uniform sampler1D palette_tex;\n" \
	"void main(void) {\n" \
		"vec4 texel = texture2D(tex, gl_TexCoord[0].st);\n" \
		"float tex_pal_idx = texture3D(palette_lookup_tex, vec3((texel * 63.0 + 0.5) / 64.0))[0] * 255.0;\n" \
		"float palette_coord = (tex_pal_idx + 0.5) / 256.0;\n" \
		"vec4 final_color = texture1D(palette_tex, palette_coord);\n" \
		"gl_FragColor = final_color;\n" \
	"}\0"

// Applies a palettized colormap fade to tex
#define GLSL_UI_COLORMAP_FADE_FRAGMENT_SHADER \
	"uniform sampler2D tex;\n" \
	"uniform float lighting;\n" \
	"uniform sampler3D palette_lookup_tex;\n" \
	"uniform sampler2D lighttable_tex;\n" \
	"void main(void) {\n" \
		"vec4 texel = texture2D(tex, gl_TexCoord[0].st);\n" \
		"float tex_pal_idx = texture3D(palette_lookup_tex, vec3((texel * 63.0 + 0.5) / 64.0))[0] * 255.0;\n" \
		"vec2 lighttable_coord = vec2((tex_pal_idx + 0.5) / 256.0, (lighting + 0.5) / 32.0);\n" \
		"gl_FragColor = texture2D(lighttable_tex, lighttable_coord);\n" \
	"}\0"

// For wipes that use additive and subtractive blending.
// alpha_factor = 31 * 8 / 10 = 24.8
// Calculated based on the use of the "fade" variable from the GETCOLOR macro
// in r_data.c:R_CreateFadeColormaps.
// However this value created some ugliness in fades to white (special stage entry)
// while palette rendering is enabled, so I raised the value just a bit.
#define GLSL_UI_TINTED_WIPE_FRAGMENT_SHADER \
	"uniform sampler2D tex;\n" \
	"uniform vec4 poly_color;\n" \
	"const float alpha_factor = 24.875;\n" \
	"void main(void) {\n" \
		"vec4 texel = texture2D(tex, gl_TexCoord[0].st);\n" \
		"vec4 final_color = poly_color;\n" \
		"float alpha = texel.a;\n" \
		"if (final_color.a >= 0.5)\n" \
			"alpha = 1.0 - alpha;\n" \
		"alpha *= alpha_factor;\n" \
		"final_color *= alpha;\n" \
		"final_color.a = 1.0;\n" \
		"gl_FragColor = final_color;\n" \
	"}\0"

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
			HWR_CompileShader(shader_index);
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
