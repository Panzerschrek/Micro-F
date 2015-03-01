#pragma once
#include "micro-f.h"

#include <dsound.h>

class mf_SoundEngine
{
public:
	mf_SoundEngine();
	~mf_SoundEngine();
private:
	LPDIRECTSOUND8 direct_sound_p_;
	LPDIRECTSOUNDBUFFER primary_sound_buffer_p_;
	LPDIRECTSOUND3DLISTENER listener_p_;
};