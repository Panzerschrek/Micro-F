#include "micro-f.h"
#include "sound_engine.h"

mf_SoundEngine::mf_SoundEngine()
{
	DirectSoundCreate8( &DSDEVID_DefaultPlayback, &direct_sound_p_, NULL );

	DSBUFFERDESC primary_buffer_info;
	ZeroMemory( &primary_buffer_info, sizeof(DSBUFFERDESC) );
	primary_buffer_info.dwSize= sizeof(DSBUFFERDESC);
	primary_buffer_info.dwBufferBytes= 0;
	primary_buffer_info.guid3DAlgorithm= GUID_NULL;
	primary_buffer_info.lpwfxFormat= NULL;
	primary_buffer_info.dwFlags= DSBCAPS_PRIMARYBUFFER | /*DSBCAPS_CTRLVOLUME*/DSBCAPS_CTRL3D;

	direct_sound_p_->CreateSoundBuffer( &primary_buffer_info, &primary_sound_buffer_p_, NULL );

	primary_sound_buffer_p_->QueryInterface( IID_IDirectSound3DListener8, (LPVOID*) &listener_p_ );
	primary_sound_buffer_p_->Play( 0, 0, DSBPLAY_LOOPING );
}

mf_SoundEngine::~mf_SoundEngine()
{
	listener_p_->Release();
	primary_sound_buffer_p_->Release();
	direct_sound_p_->Release();
}