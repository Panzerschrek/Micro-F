#pragma once
#include "micro-f.h"

#define MF_MAX_PARALLEL_SOUNDS 64


enum mf_SoundType
{
	SoundPulsejetEngine,
	SoundPlasmajetEngine,
	SoundPowerupPickup,
	SoundMachinegunShot,
	SoundAutomaticCannonShot,
	SoundPlasmagunShot,
	SoundBlast,
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

	unsigned int samples_per_second_;
};

class mf_SoundEngine
{
public:
	mf_SoundEngine();
	~mf_SoundEngine();

	static mf_SoundEngine* Instance();
	void Tick();

	// input - xyz of position, vec3 of angles, vec3 of velocity
	void SetListenerOrinetation( const float* pos, const float* angle, const float* vel );

	mf_SoundSource* CreateSoundSource( mf_SoundType sound_type );
	void DestroySoundSource( mf_SoundSource* source );

	// if position is NULL - sound is player relative
	void AddSingleSound( mf_SoundType sound_type, float volume, float pitch, const float* opt_pos= NULL, const float* opt_speed= NULL );

private:
	void GenSounds();

private:
	static mf_SoundEngine* instance_;
};

inline mf_SoundEngine* mf_SoundEngine::Instance()
{
	return instance_;
}
