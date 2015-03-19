#pragma once
#include "micro-f.h"

void mfMonochromeImageTo8Bit( const unsigned char* in_data, unsigned char* out_data, unsigned int out_data_size );

class mf_Texture
{
public:
	mf_Texture( unsigned int size_x_log2, unsigned int size_y_log2 );
	~mf_Texture();

	const float* GetData() const;
	float* GetData();
	const unsigned char* GetNormalizedData() const;
	unsigned int SizeX() const;
	unsigned int SizeY() const;
	unsigned int SizeXLog2() const;
	unsigned int SizeYLog2() const;

	// All input colors - 4float vectors

	void Noise( unsigned int octave_count= 8 );


	// Generates cellular texture, based on Poisson disk points.
	// R - distance to neraest point
	// G - distance to nearest cell border
	// B - distance to second nearest point
	// A - zero, but one in output points
	// param min_distanse_div_sqrt2 - minimal distance between points / sqrt(2)
	void PoissonDiskPoints( unsigned int min_distanse_div_sqrt2 );

	void GenNormalMap(); // takes heightmap in alpha channel and writes d/dx in r-channel and d/dy in g-channel
	void Gradient( unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1, const float* color0, const float* color1 );
	void RadialGradient( int center_x, int center_y, int radius, const float* color0, const float* color1 );
	void Fill( const float* color );
	void FillRect( unsigned int x, unsigned int y, unsigned int width, unsigned int height, const float* color );
	void FillEllipse( int center_x, int center_y, int radius, const float* color, float scale_x= 1.0f, float scale_y= 1.0f );
	void DrawLine( unsigned int x0, unsigned int y0, unsigned int x1, unsigned int y1, const float* color );
	void Grayscale();
	void Abs();
	void SinWaveDeformX( float amplitude, float freq, float phase );
	void SinWaveDeformY( float amplitude, float freq, float phase );
	void DeformX( float dx_dy );
	void DeformY( float dy_dx );

	void Copy( const mf_Texture* t );
	void Rotate( float deg );
	void Shift( unsigned int dx, unsigned int dy );
	void Invert( const float* add_color );
	void Add( const mf_Texture* t );
	void Sub( const mf_Texture* t );
	void Mul( const mf_Texture* t );
	void Max( const mf_Texture* t );
	void Min( const mf_Texture* t );

	void Add( const float* color );
	void Sub( const float* color );
	void Mul( const float* color );
	void Max( const float* color );
	void Min( const float* color );
	void Pow( float p );

	void Mix( const float* color0, const float* color1, const float* sub_color );

	void DrawText( unsigned int x, unsigned int y, unsigned int size, const float* color, const char* text );

	void LinearNormalization( float k );
	void ExpNormalization( float k );

private:

	unsigned short Noise2( int x, int y, unsigned int mask );
	unsigned short InterpolatedNoise( unsigned int x, unsigned int y, unsigned int k );
	unsigned short FinalNoise( unsigned int x, unsigned int y, unsigned int octave_count );

	void AllocateNormalizedData();

private:
	float* data_;
	unsigned char* normalized_data_;
	unsigned int size_[2];
	unsigned int size_log2_[2];
};

inline const float* mf_Texture::GetData() const
{
	return data_;
}

inline float* mf_Texture::GetData()
{
	return data_;
}

inline const unsigned char* mf_Texture::GetNormalizedData() const
{
	return normalized_data_;
}

inline unsigned int mf_Texture::SizeX() const
{
	return size_[0];
}

inline unsigned int mf_Texture::SizeY() const
{
	return size_[1];
}

inline unsigned int mf_Texture::SizeXLog2() const
{
	return size_log2_[0];
}

inline unsigned int mf_Texture::SizeYLog2() const
{
	return size_log2_[1];
}