#include <cstring>
#include "micro-f.h"
#include "main_loop.h"
#include "mf_math.h"

#define MF_MIN_WINDOW_WIDTH 800
#define MF_MIN_WINDOW_HEIGHT 600
#define MF_MAX_WINDOW_WIDTH 4096
#define MF_MAX_WINDOW_HEIGHT (1024+2048)

const char* const intro_sting=
"\"Micro-F\" - simple 96k game. (c) 2015 Artom \"Panzerschrek\" Kunz\n";

const char* const help_string=
"options:\n"
"-ws [num] [num] set window size\n"
"-fs             run game in fullscreen mode (using current screen resolution)\n"
"-no-vs          no vertical synchronisation\n"
"-imy            invert mouse Y axis\n"
"-ms-x [num]     set mouse x speed (1.0 is default)\n"
"-ms-y [num]     set mouse y speed (0.7 is default)\n"
"-h              show this help\n";

const char* const expected_number_after=
"warning, expected number, after \"%s\"";

int main( int argc, char* argv[] )
{
	bool fullscreen= false;
	bool vsync= true;
	bool invert_mouse_y= false;
	float mouse_speed_x= 1.0f;
	float mouse_speed_y= 0.7f;
	unsigned int width= MF_MIN_WINDOW_WIDTH;
	unsigned int height= MF_MIN_WINDOW_HEIGHT;

	printf( "%s", intro_sting );

	for( int i= 1; i< argc; i++ )
	{
		if( strcmp( argv[i], "-fs" ) == 0 )
			fullscreen= true;
		else if( strcmp( argv[i], "-no-vs" ) == 0 )
			vsync= false;
		else if( strcmp( argv[i], "-i-my" ) == 0 )
			invert_mouse_y= true;
		else if( strcmp( argv[i], "-ms-x" ) == 0 )
		{
			if( i != argc-1 )
				mouse_speed_x= mf_Math::clamp( 0.2f, 3.0f, (float) atof( argv[++i] ) );
			else
				printf( expected_number_after, "-ms-x" );
		}
		else if( strcmp( argv[i], "-ms-y" ) == 0 )
		{
			if( i != argc-1 )
				mouse_speed_y= mf_Math::clamp( 0.2f, 3.0f, (float) atof( argv[++i] ) );
			else
				printf( expected_number_after, "-ms-y" );
		}
		else if( strcmp( argv[i], "-ws" ) == 0 )
		{
			if( i < argc-2 )
			{
				width= atoi( argv[++i] );
				height= atoi( argv[++i] );
				if( width > MF_MAX_WINDOW_WIDTH ) width= MF_MAX_WINDOW_WIDTH;
				else if( width < MF_MIN_WINDOW_WIDTH ) width= MF_MIN_WINDOW_WIDTH;
				if( height > MF_MAX_WINDOW_HEIGHT ) height= MF_MAX_WINDOW_HEIGHT;
				else if( height < MF_MIN_WINDOW_HEIGHT ) height= MF_MIN_WINDOW_HEIGHT;
			}
			else
				printf( "expected 2 numbers, after \"%s\"", "-ws" );
		}
		else if( strcmp( argv[i], "-h" ) == 0 )
		{
			printf( "%s", help_string );
			return 0;
		}
	} // for arguments

	mf_MainLoop::CreateInstance(
		width, height,
		fullscreen, vsync,
		invert_mouse_y,
		mouse_speed_x, mouse_speed_y );

	mf_MainLoop::Instance()->Loop();
	mf_MainLoop::DeleteInstance();

	return 0;
}
