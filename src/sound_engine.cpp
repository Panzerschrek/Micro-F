#include "micro-f.h"
#include "sound_engine.h"

#include "mf_math.h"

mf_SoundSource::mf_SoundSource()
{
}

mf_SoundSource::~mf_SoundSource()
{
}

mf_SoundEngine::mf_SoundEngine(HWND hwnd)
	: sound_source_count_(0)
{
	HRESULT ok= 0;

	DirectSoundCreate8( &DSDEVID_DefaultPlayback, &direct_sound_p_, NULL );

	direct_sound_p_->SetCooperativeLevel( hwnd, DSSCL_PRIORITY );

	DSBUFFERDESC primary_buffer_info;
	ZeroMemory( &primary_buffer_info, sizeof(DSBUFFERDESC) );
	primary_buffer_info.dwSize= sizeof(DSBUFFERDESC);
	primary_buffer_info.dwBufferBytes= 0;
	primary_buffer_info.guid3DAlgorithm= GUID_NULL;
	primary_buffer_info.lpwfxFormat= NULL;
	primary_buffer_info.dwFlags= DSBCAPS_PRIMARYBUFFER | /*DSBCAPS_CTRLVOLUME*/DSBCAPS_CTRL3D;

	ok= direct_sound_p_->CreateSoundBuffer( &primary_buffer_info, &primary_sound_buffer_p_, NULL );

	ok= primary_sound_buffer_p_->QueryInterface( IID_IDirectSound3DListener8, (LPVOID*) &listener_p_ );
	ok= primary_sound_buffer_p_->Play( 0, 0, DSBPLAY_LOOPING );

	GenSounds();
}

mf_SoundEngine::~mf_SoundEngine()
{
	listener_p_->Release();
	primary_sound_buffer_p_->Release();
	direct_sound_p_->Release();
}

void mf_SoundEngine::SetListenerOrinetation( const float* pos, const float* angle )
{
	(void)angle;
	listener_p_->SetPosition( pos[0], pos[1], pos[2], DS3D_IMMEDIATE );
}

mf_SoundSource* mf_SoundEngine::CreateSoundSource( mf_SoundType sound_type )
{
	mf_SoundSource* src= &sound_sources_[ sound_source_count_++ ];

	direct_sound_p_->DuplicateSoundBuffer( sound_buffers_[ sound_type ].buffer, &src->source_buffer_ );
	src->source_buffer_->QueryInterface( IID_IDirectSound3DBuffer8, (LPVOID*) &src->source_buffer_3d_ );

	src->source_buffer_3d_->SetMaxDistance( 200.0f, DS3D_IMMEDIATE );
	src->source_buffer_3d_->SetMinDistance( 20.0f, DS3D_IMMEDIATE );
	src->source_buffer_3d_->SetPosition( 0.0f, 5.0f, 0.0f, DS3D_IMMEDIATE );
	src->source_buffer_->Play( 0, 0, DSBPLAY_LOOPING );

	return src;
}

void mf_SoundEngine::DestroySoundSource( mf_SoundSource* source )
{
	(void)source;
}

void mf_SoundEngine::GenSounds()
{
	const float sound_length= 0.5f;
	const unsigned int buffer_freq= 22050;

	unsigned int sample_count= (unsigned int)( float(buffer_freq) * sound_length );

	short* data= new short[ sample_count ];

	const float note_freq= 523.25f;
	for( unsigned int i= 0; i< sample_count; i++ )
	{
		float s= mf_Math::sin( MF_2PI * note_freq * float(i) / float(buffer_freq) );
		data[i]= (short)( s * 32766.9f );
	}

	WAVEFORMATEX sound_format;
	ZeroMemory( &sound_format, sizeof(WAVEFORMATEX) );
	sound_format.nSamplesPerSec= buffer_freq;
	sound_format.wFormatTag= WAVE_FORMAT_PCM;
	sound_format.nChannels= 1;
	sound_format.nAvgBytesPerSec= buffer_freq * sizeof(short);
	sound_format.wBitsPerSample= 16;
	sound_format.nBlockAlign= 2;

	DSBUFFERDESC secondary_buffer_info;
	ZeroMemory( &secondary_buffer_info, sizeof(DSBUFFERDESC) );
	secondary_buffer_info.dwSize= sizeof(DSBUFFERDESC);
	secondary_buffer_info.dwFlags= /*DSBCAPS_CTRLFREQUENCY | DSBCAPS_CTRLVOLUME | DSBCAPS_CTRLPAN |*/ DSBCAPS_CTRL3D;
	secondary_buffer_info.dwBufferBytes= sample_count * sizeof(short);
	secondary_buffer_info.lpwfxFormat= &sound_format;

	direct_sound_p_->CreateSoundBuffer( &secondary_buffer_info, &sound_buffers_[0].buffer, 0 );

	char* buff_data1, *buff_data2= NULL;
	unsigned int buff_size1, buff_size2= 0;
	sound_buffers_[0].buffer->Lock(
		0, sample_count * sizeof(short),
		(LPVOID*)&buff_data1,
		(LPDWORD)&buff_size1,
		(LPVOID*)&buff_data2,
		(LPDWORD)&buff_size2,
		0 );

	memcpy( buff_data1, data, sample_count * sizeof(short) );

	sound_buffers_[0].buffer->Unlock( buff_data1, buff_size1, NULL, 0 );

	sound_buffers_[0].length= sound_length;

	delete[] data;
}