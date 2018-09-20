///
/// Dynamic Library Loading
///

///
/// OpenMPT Loading
///

#ifdef HAVE_OPENMPT

#include "libopenmpt/libopenmpt.h"

// Dynamic loading inspired by SDL Mixer
// Why: It's hard to compile for Windows without MSVC dependency, see https://trac.videolan.org/vlc/ticket/13055
// So let's not force that on the user, and they can download it if they want.
//
// ADD FUNCTIONS HERE AS YOU USE THEM!!!!!
typedef struct {
	int loaded;
	void *handle;

	// errors
	int (*module_error_get_last) ( openmpt_module * mod );
	const char *(*error_string) ( int error );
	const char *(*get_string) ( const char * key );

	// module loading
	void (*module_destroy) ( openmpt_module * mod );
	openmpt_module *(*module_create_from_memory2) ( const void * filedata, size_t filesize, openmpt_log_func logfunc, void * loguser, openmpt_error_func errfunc, void * erruser, int * error, const char * * error_message, const openmpt_module_initial_ctl * ctls );

	// audio callback
	size_t (*module_read_interleaved_stereo) ( openmpt_module * mod, int32_t samplerate, size_t count, int16_t * interleaved_stereo );

	// playback settings
	int (*module_set_render_param) ( openmpt_module * mod, int param, int32_t value );
	int (*module_set_repeat_count) ( openmpt_module * mod, int32_t repeat_count );
	int (*module_ctl_set) ( openmpt_module * mod, const char * ctl, const char * value );

	// positioning
	double (*module_get_duration_seconds) ( openmpt_module * mod );
	double (*module_get_position_seconds) ( openmpt_module * mod );
	double (*module_set_position_seconds) ( openmpt_module * mod, double seconds );
	int32_t (*module_get_num_subsongs) ( openmpt_module * mod );
	int (*module_select_subsong) ( openmpt_module * mod, int32_t subsong );
} openmpt_loader;

extern openmpt_loader openmpt;

void load_openmpt(void);
void unload_openmpt(void);

#endif
