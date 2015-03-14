#include "sounds_generation.h"

#include "mf_math.h"

static short AmplitudeFloatToShort( float a )
{
	int a_i= (int)(a * 32767.0f );
	if( a_i >  32767 ) return 32767;
	if( a_i < -32767 ) return -32767;
	return (short) a_i;
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