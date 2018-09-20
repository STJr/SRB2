///
/// Dynamic Library Loading
///

#include "load_libraries.h"

///
/// OpenMPT Loading
///

#ifdef HAVE_OPENMPT

openmpt_loader openmpt = {
	0, NULL,
	NULL, NULL, NULL, // errors
	NULL, NULL, // module loading
	NULL, // audio callback
	NULL, NULL, NULL, // playback settings
	NULL, NULL, NULL, NULL, NULL // positioning
};

#ifdef OPENMPT_DYNAMIC
#define FUNCTION_LOADER(NAME, FUNC, SIG) \
    openmpt.NAME = (SIG) SDL_LoadFunction(openmpt.handle, #FUNC); \
    if (openmpt.NAME == NULL) { SDL_UnloadObject(openmpt.handle); openmpt.handle = NULL; return; }
#else
#define FUNCTION_LOADER(NAME, FUNC, SIG) \
    openmpt.NAME = FUNC;
#endif

void load_openmpt(void)
{
	if (openmpt.loaded)
		return;

#ifdef OPENMPT_DYNAMIC
#if defined(_WIN32) || defined(_WIN64)
	openmpt.handle = SDL_LoadObject("libopenmpt.dll");
#else
	openmpt.handle = SDL_LoadObject("libopenmpt.so");
#endif
	if (openmpt.handle == NULL)
	{
		CONS_Printf("libopenmpt not found, not loading.\n");
		return;
	}
#endif

	// errors
	FUNCTION_LOADER(module_error_get_last, openmpt_module_error_get_last, int (*) ( openmpt_module * mod ))
	FUNCTION_LOADER(error_string, openmpt_error_string, const char *(*) ( int error ))
	FUNCTION_LOADER(get_string, openmpt_get_string, const char *(*) ( const char * key ))

	// module loading
	FUNCTION_LOADER(module_destroy, openmpt_module_destroy, void (*) ( openmpt_module * mod ))
	FUNCTION_LOADER(module_create_from_memory2, openmpt_module_create_from_memory2, openmpt_module *(*) ( const void * filedata, size_t filesize, openmpt_log_func logfunc, void * loguser, openmpt_error_func errfunc, void * erruser, int * error, const char * * error_message, const openmpt_module_initial_ctl * ctls ))

	// audio callback
	FUNCTION_LOADER(module_read_interleaved_stereo, openmpt_module_read_interleaved_stereo, size_t (*) ( openmpt_module * mod, int32_t samplerate, size_t count, int16_t * interleaved_stereo ))

	// playback settings
	FUNCTION_LOADER(module_set_render_param, openmpt_module_set_render_param, int (*) ( openmpt_module * mod, int param, int32_t value ))
	FUNCTION_LOADER(module_set_repeat_count, openmpt_module_set_repeat_count, int (*) ( openmpt_module * mod, int32_t repeat_count ))
	FUNCTION_LOADER(module_ctl_set, openmpt_module_ctl_set, int (*) ( openmpt_module * mod, const char * ctl, const char * value ))

	// positioning
	FUNCTION_LOADER(module_get_duration_seconds, openmpt_module_get_duration_seconds, double (*) ( openmpt_module * mod ))
	FUNCTION_LOADER(module_get_position_seconds, openmpt_module_get_position_seconds, double (*) ( openmpt_module * mod ))
	FUNCTION_LOADER(module_set_position_seconds, openmpt_module_set_position_seconds, double (*) ( openmpt_module * mod, double seconds ))
	FUNCTION_LOADER(module_get_num_subsongs, openmpt_module_get_num_subsongs, int32_t (*) ( openmpt_module * mod ))
	FUNCTION_LOADER(module_select_subsong, openmpt_module_select_subsong, int (*) ( openmpt_module * mod, int32_t subsong ))

#ifdef OPENMPT_DYNAMIC
	// this will be unset if a function failed to load
	if (openmpt.handle == NULL)
	{
		CONS_Printf("libopenmpt found but failed to load.\n");
		return;
	}
#endif

	CONS_Printf("libopenmpt version: %s\n", openmpt.get_string("library_version"));
	CONS_Printf("libopenmpt build date: %s\n", openmpt.get_string("build"));

	openmpt.loaded = 1;
}

void unload_openmpt(void)
{
#ifdef OPENMPT_DYNAMIC
	if (openmpt.loaded)
	{
		SDL_UnloadObject(openmpt.handle);
		openmpt.handle = NULL;
		openmpt.loaded = 0;
	}
#endif
}

#undef FUNCTION_LOADER

#endif
