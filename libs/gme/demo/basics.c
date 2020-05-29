/* C example that opens a game music file and records 10 seconds to "out.wav" */

#include "gme/gme.h"

#include "Wave_Writer.h" /* wave_ functions for writing sound file */
#include <stdlib.h>
#include <stdio.h>

void handle_error( const char* str );

int main(int argc, char *argv[])
{
	const char *filename = "test.nsf"; /* Default file to open */
	if ( argc >= 2 )
		filename = argv[1];

	long sample_rate = 44100; /* number of samples per second */
	/* index of track to play (0 = first) */
	int track = argc >= 3 ? atoi(argv[2]) : 0;
	
	/* Open music file in new emulator */
	Music_Emu* emu;
	handle_error( gme_open_file( filename, &emu, sample_rate ) );
	
	/* Start track */
	handle_error( gme_start_track( emu, track ) );
	
	/* Begin writing to wave file */
	wave_open( sample_rate, "out.wav" );
	wave_enable_stereo();
	
	/* Record 10 seconds of track */
	while ( gme_tell( emu ) < 10 * 1000L )
	{
		/* Sample buffer */
		#define buf_size 1024 /* can be any multiple of 2 */
		short buf [buf_size];
		
		/* Fill sample buffer */
		handle_error( gme_play( emu, buf_size, buf ) );
		
		/* Write samples to wave file */
		wave_write( buf, buf_size );
	}
	
	/* Cleanup */
	gme_delete( emu );
	wave_close();
	
	return 0;
}

void handle_error( const char* str )
{
	if ( str )
	{
		printf( "Error: %s\n", str ); getchar();
		exit( EXIT_FAILURE );
	}
}
