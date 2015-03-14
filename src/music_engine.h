#pragma once
#include "micro-f.h"

class mf_MusicEngine
{
public:
	enum Melody
	{
		MelodyAviatorsMarch,
		MelodyValkyriesFlight,
		LastMelody
	};

	mf_MusicEngine();
	~mf_MusicEngine();

	void Play( Melody melody );
	void Stop();
	void Pause();
	void Continue();

private:
	bool is_playing_;
	char tmp_file_name_[512];
	MCI_OPEN_PARMS mci_open_params_;

};