#include "sounds_generation.h"
#include "sound_engine.h"

#include "mf_math.h"

static short AmplitudeFloatToShort( float a )
{
	int a_i= (int)(a * 32767.0f );
	if( a_i >  32767 ) return 32767;
	if( a_i < -32767 ) return -32767;
	return (short) a_i;
}

static int Noise1( int x ) // returns noise value in range [0;2^31]
{
	int n= x;
	n= (n << 13) ^ n;
	return ((n * (n * n * 15731 + 789221) + 1376312589) & 0x7fffffff);
}

static float Noise1Interpolated( float x ) // output - value in range [0;6535)
{
	int X= int(x);
	float dx= x - float(X);

	int noise[2]= { Noise1(X)>>15, Noise1(X+1)>>15 };
	return float(noise[1]) * dx + (1.0f - dx) * float(noise[0]);
}

static float Noise1Final( float t, int octaves ) // input - time in seconds, output - value in  range [-1;1]
{
	float f=0.0;
	float m= 0.5f;
	float im= 1.0f;
	for( int i= 0; i< octaves; i++, m*= 0.5f,im*= 2.0f )
		f+= Noise1Interpolated(t*im) * m;
	return f * (2.0f / 65536.0f) - 1.0f;
}

// function cut border_size samples for sound. output sample count = samples_count - border_size
static void SmoothSoundEdges( short* data, unsigned int samples_count, unsigned int border_size )
{
	for( unsigned int i= 0, j= samples_count - border_size; i< border_size; i++, j++ )
	{
		int k= data[i] * i / border_size ;
		k+= data[j] * (samples_count - j) / border_size ;
		data[i]= short(k);
	}
}

short* GenPulsejetSound( unsigned int sample_rate, unsigned int* out_samples_count )
{
	const float freq= 100.0f;

	unsigned int sample_count= (unsigned int) mf_Math::round( float(sample_rate) / freq );
	short* data= new short[ sample_count ];

	float sample_rate_f= float(sample_rate);
	for( unsigned int i= 0; i< sample_count; i++ )
	{
		float int_part;
		float sawtooth_a= 2.0f * modf( freq * float(i) / sample_rate_f, &int_part ) - 1.0f;

		float sin_arg= MF_2PI * freq * float(i) / sample_rate_f;
		float sin_a2= mf_Math::sin( 2.0f * sin_arg );
		float sin_a3= mf_Math::sin( 3.0f * sin_arg );
		float sin_a5= mf_Math::sin( 5.0f * sin_arg );

		float result= 
			sawtooth_a * 0.6f +
			sin_a2 * 0.1f +
			sin_a3 * 0.1f +
			sin_a5 * 0.2f;
		data[i]= AmplitudeFloatToShort( result );
	}

	*out_samples_count= sample_count;
	return data;
}

short* GenPlasmajetSound( unsigned int sample_rate, unsigned int* out_samples_count )
{
	const float c_length= 1.0f;

	unsigned int sample_count= (unsigned int) ( float(sample_rate) * c_length );
	short* data= new short[ sample_count ];

	float sample_rate_f= float(sample_rate);
	for( unsigned int i= 0; i< sample_count; i++ )
	{
		float t= float(i) / sample_rate_f;
		float a= Noise1Final( t * 512.0f, 5 );
		a+= 0.25f * mf_Math::sin( t * 3096.0f );
		data[i]= AmplitudeFloatToShort( a );
	}

	SmoothSoundEdges( data, sample_count, 512 );
	*out_samples_count= sample_count - 512;
	return data;
}

short* GenPowerupPickupSound( unsigned int sample_rate, unsigned int* out_samples_count )
{
	const float c_base_freq= 400.0f;
	const float c_length= 1.0f;

	unsigned int sample_count= (unsigned int) ( float(sample_rate) * c_length );
	short* data= new short[ sample_count ];

	float sample_rate_f= float(sample_rate);
	for( unsigned int i= 0; i< sample_count; i++ )
	{
		float t= float(i) / sample_rate_f;
		float a[4];
		a[3]= (c_length - t) / c_length;
		a[2]= a[3] * a[3];
		a[1]= a[2] * a[2];
		a[0]= a[2] * a[1];
		t*= c_base_freq * MF_2PI;
		float s=
			 0.5f * a[0] * mf_Math::sin(t) +
			0.25f * a[1] * mf_Math::sin(t*2.0f) +
			0.25f * a[2] * mf_Math::sin(t*3.0f) +
			0.25f * a[3] * mf_Math::sin(t*4.0f);
		data[i]= AmplitudeFloatToShort( s );
	}

	*out_samples_count= sample_count;
	return data;
}

short* GenMachinegunShotSound( unsigned int sample_rate, unsigned int* out_samples_count )
{
	const float c_length= 0.1f;

	unsigned int sample_count= (unsigned int) ( float(sample_rate) * c_length );
	short* data= new short[ sample_count ];

	float sample_rate_f= float(sample_rate);
	for( unsigned int i= 0; i< sample_count; i++ )
	{
		float t= float(i) / sample_rate_f;
		float a= Noise1Final( t * 1536.0f - t * t * 512.0f, 4 ) * 2.0f * mf_Math::exp(-t*12.0f);

		if( t > c_length * 0.5f )
		{
			const float c_base_freq= 1834.0f;
			float sin_sum=
				0.5f * mf_Math::sin( c_base_freq * MF_2PI * t ) +
				0.1f * mf_Math::sin( c_base_freq* MF_2PI * t * 2.0f ) +
				0.05f * mf_Math::sin( c_base_freq * MF_2PI * t * 3.0f ) +
				0.05f * mf_Math::sin( c_base_freq * MF_2PI * t * 4.0f );
			a+= 0.25f * sin_sum * mf_Math::exp( -15.0f * ( t - c_length * 0.5f ) );
		}
		data[i]= AmplitudeFloatToShort( a );
	}

	*out_samples_count= sample_count;
	return data;
}

short* (* const sound_gen_func[LastSound])(unsigned int sample_rate, unsigned int* out_samples_count)=
{
	GenPulsejetSound,
	GenPlasmajetSound,
	GenPowerupPickupSound,
	GenMachinegunShotSound
};