#include "micro-f.h"
#include "sound_engine.h"
#include "main_loop.h"

#include "mf_math.h"
#include "sounds_generation.h"

#define MF_SND_PRIORITY 0
#define MF_SOUND_MAX_DISTANCE 768.0f

void mf_SoundSource::Play()
{
	source_buffer_->Play( 0, MF_SND_PRIORITY, DSBPLAY_LOOPING );
}

void mf_SoundSource::Pause()
{
	source_buffer_->Stop();
}

void mf_SoundSource::Stop()
{
	source_buffer_->Stop();
	source_buffer_->SetCurrentPosition(0);
}

void mf_SoundSource::SetOrientation( const float* pos, const float* vel )
{
	source_buffer_3d_->SetPosition( pos[0], pos[1], pos[2], DS3D_IMMEDIATE );
	source_buffer_3d_->SetVelocity( vel[0], vel[1], vel[2], DS3D_IMMEDIATE );
}

void mf_SoundSource::SetPitch( float pitch )
{
	source_buffer_->SetFrequency( (unsigned int)(float(samples_per_second_) * pitch) );
}

void mf_SoundSource::SetVolume( float volume )
{
	source_buffer_3d_->SetMinDistance( mf_Math::sqrt(volume), DS3D_IMMEDIATE );
}


mf_SoundEngine* mf_SoundEngine::instance_= NULL;

mf_SoundEngine::mf_SoundEngine(HWND hwnd)
	: sound_source_count_(0), single_sounds_count_(0)
{
	DirectSoundCreate8( &DSDEVID_DefaultPlayback, &direct_sound_p_, NULL );

	direct_sound_p_->SetCooperativeLevel( hwnd, DSSCL_PRIORITY );

	DSBUFFERDESC primary_buffer_info;
	ZeroMemory( &primary_buffer_info, sizeof(DSBUFFERDESC) );
	primary_buffer_info.dwSize= sizeof(DSBUFFERDESC);
	primary_buffer_info.dwBufferBytes= 0;
	primary_buffer_info.guid3DAlgorithm= GUID_NULL;
	primary_buffer_info.lpwfxFormat= NULL;
	primary_buffer_info.dwFlags= DSBCAPS_PRIMARYBUFFER | DSBCAPS_CTRL3D;

	direct_sound_p_->CreateSoundBuffer( &primary_buffer_info, &primary_sound_buffer_p_, NULL );

	WAVEFORMATEX format;
	DWORD written;
	primary_sound_buffer_p_->GetFormat( &format, sizeof(format), &written );
	samples_per_second_= format.nSamplesPerSec;

	primary_sound_buffer_p_->QueryInterface( IID_IDirectSound3DListener8, (LPVOID*) &listener_p_ );
	primary_sound_buffer_p_->Play( 0, 0, DSBPLAY_LOOPING );

	GenSounds();

	MF_ASSERT( instance_ == NULL );
	instance_= this;
}

mf_SoundEngine::~mf_SoundEngine()
{
	listener_p_->Release();
	primary_sound_buffer_p_->Release();
	direct_sound_p_->Release();
	instance_= NULL;
}

void mf_SoundEngine::Tick()
{
	float current_time= mf_MainLoop::Instance()->CurrentTime();
	for( unsigned int i= 0; i< single_sounds_count_; )
	{
		if(single_sounds_[i].death_time <= current_time )
		{
			DestroySoundSource( single_sounds_[i].source );
			if( i != single_sounds_count_ - 1 )
				single_sounds_[i]= single_sounds_[ single_sounds_count_ - 1 ];
			single_sounds_count_--;
			continue;
		}
		i++;
	}
}

void mf_SoundEngine::SetListenerOrinetation( const float* pos, const float* angle, const float* vel )
{
	static const float init_front_vec[]= { 0.0f, 1.0f, 1.0f };
	static const float init_top_vec[]= { 0.0f, 0.0f, -1.0f };
	float front_vec[3];
	float top_vec[3];

	float mat[16];
	float rot_x_mat[16];
	float rot_y_mat[16];
	float rot_z_mat[16];
	//y * x * z
	Mat4RotateX( rot_x_mat, angle[0] );
	Mat4RotateY( rot_y_mat, angle[1] );
	Mat4RotateZ( rot_z_mat, angle[2] );

	Mat4Mul( rot_y_mat, rot_x_mat, mat );
	Mat4Mul( mat, rot_z_mat );
	Vec3Mat4Mul( init_front_vec, mat, front_vec );
	Vec3Mat4Mul( init_top_vec, mat, top_vec );

	listener_p_->SetOrientation( front_vec[0], front_vec[1], front_vec[2], top_vec[0], top_vec[1], top_vec[2], DS3D_DEFERRED );
	listener_p_->SetPosition( pos[0], pos[1], pos[2], DS3D_DEFERRED );
	listener_p_->SetVelocity( vel[0], vel[1], vel[2], DS3D_DEFERRED );
	listener_p_->CommitDeferredSettings();
}

mf_SoundSource* mf_SoundEngine::CreateSoundSource( mf_SoundType sound_type )
{
	if ( sound_source_count_ == MF_MAX_PARALLEL_SOUNDS )
		return NULL;

	mf_SoundSource* src= new mf_SoundSource;
	sound_sources_[ sound_source_count_++ ]= src;

	direct_sound_p_->DuplicateSoundBuffer( sound_buffers_[ sound_type ].buffer, &src->source_buffer_ );
	src->source_buffer_->QueryInterface( IID_IDirectSound3DBuffer8, (LPVOID*) &src->source_buffer_3d_ );

	src->source_buffer_3d_->SetMaxDistance( MF_SOUND_MAX_DISTANCE, DS3D_IMMEDIATE );

	src->samples_per_second_= samples_per_second_;
	return src;
}

void mf_SoundEngine::DestroySoundSource( mf_SoundSource* source )
{
	source->source_buffer_->Stop();
	source->source_buffer_->Release();

	for( unsigned int i= 0; i< sound_source_count_-1; i++ )
		if( source == sound_sources_[i] )
			sound_sources_[i]= sound_sources_[ sound_source_count_ - 1];

	delete source;
	sound_source_count_--;
}

void mf_SoundEngine::AddSingleSound( mf_SoundType sound_type, float volume, float pitch, const float* opt_pos, const float* opt_speed )
{
	static const float c_zero_vel[3]= { 0.0f, 0.0f, 0.0f };

	mf_SoundSource* sound= CreateSoundSource( sound_type );
	if( opt_pos != NULL )
		sound->SetOrientation( opt_pos, opt_speed ? opt_speed : c_zero_vel );
	else
		sound->source_buffer_3d_->SetMode( DS3DMODE_HEADRELATIVE, DS3D_IMMEDIATE );
	sound->SetVolume( volume );
	sound->SetPitch( pitch );
	sound->source_buffer_->Play( 0, MF_SND_PRIORITY, 0 );

	single_sounds_[ single_sounds_count_ ].source= sound;
	single_sounds_[ single_sounds_count_ ].death_time= mf_MainLoop::Instance()->CurrentTime() + sound_buffers_[sound_type].length / pitch + 0.01f;
	single_sounds_count_++;
}

void mf_SoundEngine::GenSounds()
{
	for( unsigned int i= 0; i< LastSound; i++ )
	{
		unsigned int sample_count;
		short* data= sound_gen_func[i]( samples_per_second_, &sample_count );;

		WAVEFORMATEX sound_format;
		ZeroMemory( &sound_format, sizeof(WAVEFORMATEX) );
		sound_format.nSamplesPerSec= samples_per_second_;
		sound_format.wFormatTag= WAVE_FORMAT_PCM;
		sound_format.nChannels= 1;
		sound_format.nAvgBytesPerSec= samples_per_second_ * sizeof(short);
		sound_format.wBitsPerSample= 16;
		sound_format.nBlockAlign= 2;

		DSBUFFERDESC secondary_buffer_info;
		ZeroMemory( &secondary_buffer_info, sizeof(DSBUFFERDESC) );
		secondary_buffer_info.dwSize= sizeof(DSBUFFERDESC);
		secondary_buffer_info.dwFlags= DSBCAPS_CTRL3D | DSBCAPS_CTRLFREQUENCY;
		secondary_buffer_info.dwBufferBytes= sample_count * sizeof(short);
		secondary_buffer_info.lpwfxFormat= &sound_format;

		direct_sound_p_->CreateSoundBuffer( &secondary_buffer_info, &sound_buffers_[i].buffer, 0 );

		char* buff_data1, *buff_data2= NULL;
		unsigned int buff_size1, buff_size2= 0;
		sound_buffers_[i].buffer->Lock(
			0, sample_count * sizeof(short),
			(LPVOID*)&buff_data1,
			(LPDWORD)&buff_size1,
			(LPVOID*)&buff_data2,
			(LPDWORD)&buff_size2,
			0 );

		memcpy( buff_data1, data, sample_count * sizeof(short) );

		sound_buffers_[i].buffer->Unlock( buff_data1, buff_size1, NULL, 0 );
		sound_buffers_[i].length= float(sample_count) / float(samples_per_second_);

		delete[] data;
	}
}