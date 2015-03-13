#include "micro-f.h"
#include "main_loop.h"

#include <mmsystem.h>

void PlayMidi()
{
	const char* file_name= "sounds/marsh_aviatorov.mid";
	FILE* f= fopen( file_name, "rb" );

	fseek( f, 0, SEEK_END );
	unsigned int file_size= ftell(f);
	fseek( f, 0, SEEK_SET );

	char* file_data= new char[ file_size ];
	fread( &file_data, file_size, 1, f );
	fclose(f);

	MCI_OPEN_PARMS midi_params;
	midi_params.lpstrDeviceType= (LPCSTR)MCI_DEVTYPE_SEQUENCER;
	midi_params.dwCallback= NULL;
	midi_params.lpstrElementName= file_name;
	mciSendCommand(0, MCI_OPEN, MCI_WAIT|MCI_OPEN_ELEMENT|MCI_OPEN_TYPE|MCI_OPEN_TYPE_ID, (DWORD)(LPVOID)&midi_params );

	mciSendCommand( midi_params.wDeviceID, MCI_PLAY, MCI_NOTIFY, (DWORD)&midi_params );
	Sleep(2000);
	mciSendCommand( midi_params.wDeviceID, MCI_STOP, MCI_NOTIFY, (DWORD)&midi_params );
	mciSendCommand( midi_params.wDeviceID, MCI_CLOSE, 0, (DWORD)(LPVOID)&midi_params );
	Sleep(1000);
	mciSendCommand(0, MCI_OPEN, MCI_WAIT|MCI_OPEN_ELEMENT|MCI_OPEN_TYPE|MCI_OPEN_TYPE_ID, (DWORD)(LPVOID)&midi_params );
	mciSendCommand( midi_params.wDeviceID, MCI_PLAY, MCI_NOTIFY, (DWORD)&midi_params );
}

const char* const intro_sting=
"\"Micro-F\" - simple 96k game. (c) 2015 Artom \"Panzerschrek\" Kunz\n";

const char* const help_string=
"options:\n"
"-fs             run game in fullscreen mode (using current screen resolution)\n"
"-no-vs          no vertical synchronisation\n"
"-imy            invert mouse Y axis\n"
"-ms-x [number]  set mouse x speed (1.0 is default)\n"
"-ms-y [number]  set mouse y speed (0.7 is default)\n"
"-h              show this help\n";

const char* const expected_number_after=
"warning, expected number, after \"%s\"";

float clampf( float min, float max, float value )
{
	float r= value;
	if( r < min ) r= min;
	else if( r > max ) r= max;
	return r;
}

int main( int argc, char* argv[] )
{PlayMidi();
	printf( "%s", intro_sting );

	mf_RenderingConfig cfg;
	cfg.fullscreen= false;
	cfg.vsync= true;
	cfg.invert_mouse_y= false;
	cfg.mouse_speed_x= 1.0f;
	cfg.mouse_speed_y= 0.7f;

	for( int i= 1; i< argc; i++ )
	{
		if( strcmp( argv[i], "-fs" ) == 0 )
			cfg.fullscreen= true;
		else if( strcmp( argv[i], "-no-vs" ) == 0 )
			cfg.vsync= false;
		else if( strcmp( argv[i], "-i-my" ) == 0 )
			cfg.invert_mouse_y= true;
		else if( strcmp( argv[i], "-ms-x" ) == 0 )
		{
			if( i != argc-1)
				cfg.mouse_speed_x= clampf( 0.2f, 3.0f, (float) atof( argv[++i] ) );
			else
				printf( expected_number_after, "-ms-x" );
		}
		else if( strcmp( argv[i], "-ms-y" ) == 0 )
		{
			if( i != argc-1)
				cfg.mouse_speed_y= (float) clampf( 0.2f, 3.0f, (float) atof( argv[++i] ) );
			else
				printf( expected_number_after, "-ms-y" );
		}
		else if( strcmp( argv[i], "-h" ) == 0 )
		{
			printf( "%s", help_string );
			return 0;
		}
	} // for arguments

	mf_MainLoop::SetRenderingConfig( &cfg );

	mf_MainLoop::Instance()->Loop();
	mf_MainLoop::DeleteInstance();

	return 0;
}