#include "micro-f.h"
#include "sound_engine_sdl.h"
#include "main_loop.h"

#include "mf_math.h"
#include "sounds_generation.h"

#define MF_SND_PRIORITY 0
#define MF_SOUND_MAX_DISTANCE 768.0f

void mf_SoundSource::Play()
{
}

void mf_SoundSource::Pause()
{
}

void mf_SoundSource::Stop()
{
}

void mf_SoundSource::SetOrientation( const float* pos, const float* vel )
{
}

void mf_SoundSource::SetPitch( float pitch )
{
}

void mf_SoundSource::SetVolume( float volume )
{
}

mf_SoundEngine* mf_SoundEngine::instance_= NULL;

mf_SoundEngine::mf_SoundEngine()
{
	MF_ASSERT( instance_ == NULL );
	instance_= this;
}

mf_SoundEngine::~mf_SoundEngine()
{
}

void mf_SoundEngine::Tick()
{
}

void mf_SoundEngine::SetListenerOrinetation( const float* pos, const float* angle, const float* vel )
{
}

mf_SoundSource* mf_SoundEngine::CreateSoundSource( mf_SoundType sound_type )
{
	mf_SoundSource* src= new mf_SoundSource;
	//sound_sources_[ sound_source_count_++ ]= src;
	return src;
}

void mf_SoundEngine::DestroySoundSource( mf_SoundSource* source )
{
}

void mf_SoundEngine::AddSingleSound( mf_SoundType sound_type, float volume, float pitch, const float* opt_pos, const float* opt_speed )
{
}

void mf_SoundEngine::GenSounds()
{
}
