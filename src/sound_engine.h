#pragma once
#include "micro-f.h"

#include <dsound.h>

#define MF_MAX_PARALLEL_SOUNDS 16


enum mf_SoundType
{
	SoundTurbojetEngine,
	LastSound
};

class mf_SoundSource
{
public:
	void Play();
	void Pause();
	void Stop();

	void SetOrientation( const float* pos, const float* vel );
	void SetPitch( float pitch );
	void SetVolume( float volume ); // 1.0f means original volume in distance 1n, 4.0f in distance 2m, etc. square root law.

private:
	friend class mf_SoundEngine;

	LPDIRECTSOUNDBUFFER source_buffer_;
	LPDIRECTSOUND3DBUFFER source_buffer_3d_;
	unsigned int samples_per_second_;
};

class mf_SoundEngine
{
public:
	mf_SoundEngine(HWND hwnd);
	~mf_SoundEngine();

	// input - xyz of position, vec3 of angles, vec3 of velocity
	void SetListenerOrinetation( const float* pos, const float* angle, const float* vel );

	mf_SoundSource* CreateSoundSource( mf_SoundType sound_type );
	void DestroySoundSource( mf_SoundSource* source );

private:
	void GenSounds();

private:
	LPDIRECTSOUND8 direct_sound_p_;
	LPDIRECTSOUNDBUFFER primary_sound_buffer_p_;
	LPDIRECTSOUND3DLISTENER listener_p_;
	unsigned int samples_per_second_;

	struct
	{
		LPDIRECTSOUNDBUFFER buffer;
		float length; // in seconds
	} sound_buffers_[LastSound];

	mf_SoundSource sound_sources_[ MF_MAX_PARALLEL_SOUNDS ];
	unsigned int sound_source_count_;
};