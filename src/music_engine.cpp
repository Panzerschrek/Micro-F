#include "micro-f.h"

#include "music_engine.h"

static const unsigned char aviators_march[]=
#include "../sounds/aviators_march.h"
;

static const unsigned char valkyries_flight[]=
#include "../sounds/valkyries_flight.h"
;

static const unsigned char* melodies_data[ mf_MusicEngine::LastMelody ]=
{
	aviators_march,
	valkyries_flight
};

static const unsigned int melodies_size[ mf_MusicEngine::LastMelody ]=
{
	sizeof(aviators_march),
	sizeof(valkyries_flight)
};

mf_MusicEngine::mf_MusicEngine()
	: is_playing_(false)
{
	char tmp[512]= "";
	GetEnvironmentVariable( "TEMP", tmp, sizeof(tmp) );
	sprintf( tmp_file_name_, "%s%s%s", tmp, "/", "sound_tmp.midi" );
}

mf_MusicEngine::~mf_MusicEngine()
{
}

void mf_MusicEngine::Play( Melody melody )
{
	FILE* f= fopen( tmp_file_name_, "wb" );
	if( f == NULL ) return;
	fwrite( melodies_data[melody], melodies_size[melody], 1, f );
	fclose(f);

	mci_open_params_.lpstrDeviceType= (LPCSTR)MCI_DEVTYPE_SEQUENCER;
	mci_open_params_.dwCallback= NULL;
	mci_open_params_.lpstrElementName= tmp_file_name_;
	mciSendCommand(0, MCI_OPEN, MCI_WAIT|MCI_OPEN_ELEMENT|MCI_OPEN_TYPE|MCI_OPEN_TYPE_ID, (DWORD)(LPVOID)&mci_open_params_ );
	mciSendCommand( mci_open_params_.wDeviceID, MCI_PLAY, MCI_NOTIFY, (DWORD)&mci_open_params_ );

	is_playing_= true;
}

void mf_MusicEngine::Stop()
{
	if( !is_playing_ ) return;
	mciSendCommand( mci_open_params_.wDeviceID, MCI_STOP, MCI_NOTIFY, (DWORD)&mci_open_params_ );
	mciSendCommand( mci_open_params_.wDeviceID, MCI_CLOSE, 0, (DWORD)(LPVOID)&mci_open_params_ );

	DeleteFile( tmp_file_name_ );
	is_playing_= false;
}

void mf_MusicEngine::Pause()
{
	if( !is_playing_ ) return;
	mciSendCommand( mci_open_params_.wDeviceID, MCI_PAUSE, 0, (DWORD)(LPVOID)&mci_open_params_ );
}

void mf_MusicEngine::Continue()
{
	if( !is_playing_ ) return;
	mciSendCommand( mci_open_params_.wDeviceID, MCI_PLAY, 0, (DWORD)(LPVOID)&mci_open_params_ );
}