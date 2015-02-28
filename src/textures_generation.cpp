#include "micro-f.h"

#include "textures_generation.h"
#include "texture.h"
#include "mf_math.h"


static const float g_dirt_color[]= { 0.588f, 0.349f, 0.211f, 0.0f };

void GenF1949Texture( mf_Texture* tex )
{
	const unsigned int tex_scaler= 1<< ( tex->SizeXLog2() - 8 );

	static const float plane_color[]= { 0.2f, 0.4f, 0.1f, 0.0f };
	tex->Fill( plane_color );
	static const float turbine_back_color[]= { 0.3f, 0.1f, 0.7f, 0.0f };
	tex->FillRect( 1 * tex_scaler, 1 * tex_scaler, 34 * tex_scaler, 35 * tex_scaler, turbine_back_color );
	static const float turbine_front_color[]= { 0.7f, 0.1f, 0.3f, 0.0f };
	tex->FillRect( 27 * tex_scaler, 36 * tex_scaler, 71 * tex_scaler, 81 * tex_scaler, turbine_front_color );
	static const float window_color[]= {0.5f, 0.5f, 0.9f, 1.0f };
	tex->FillRect( 207 * tex_scaler, 1 * tex_scaler, 49 * tex_scaler, 88 * tex_scaler, window_color );

	mf_Texture tex2( tex->SizeXLog2(), tex->SizeYLog2() );
	tex2.Noise();
	static const float add_color[]= { 1.5f, 1.5f, 1.5f, 1.5f };
	static const float mul_color[]= { 0.4f, 0.4f, 0.4f, 0.0f };
	tex2.Add( add_color );
	tex2.Mul( mul_color );
	tex->Mul( &tex2 );

	static const float text_color[]= { 1.0f, 1.0f, 1.0f, 0.0f };
	tex->DrawText( 147 * tex_scaler, 64 * tex_scaler, tex_scaler, text_color, "F-1949" );
	tex->LinearNormalization(1.0f);
}

void GenSunTexture( mf_Texture* tex )
{
	unsigned int size_x= 1 << tex->SizeXLog2();
	unsigned int size_y= 1 << tex->SizeYLog2();
	static const float sun_color0[]= { 1.0f, 0.95f, 0.9f, 1.0f };
	static const float sun_color1[]= { 1.0f, 0.9f, 0.85f, 0.0f };
	tex->RadialGradient( size_x / 2, size_y / 2, size_x/2, sun_color0, sun_color1 );

	tex->Pow( 0.7f );
}

void GenDirtTexture( mf_Texture* tex )
{
	tex->Noise();
	tex->Pow(0.3f);
	tex->Mul( g_dirt_color );
}

void GenDirtWithGrassTexture( mf_Texture* tex )
{
	tex->Noise( 8 );
	mf_Texture noise_sub_tex( tex->SizeXLog2(), tex->SizeYLog2() );
	noise_sub_tex.Noise( 2 );
	tex->Sub( &noise_sub_tex );
	static const float after_second_tex_sub_mul_value[]= { 2.0f, 2.0f, 2.0f, 2.0f };
	tex->Mul( after_second_tex_sub_mul_value );

	static const float befor_noise_add_value[]= { 0.45f, 0.45f, 0.45f, 0.45f };
	static const float after_noise_mul_value[]= { 1.0f / 1.5f, 1.0f / 1.5f, 1.0f / 1.5f, 1.0f / 1.5f };
	tex->Add(befor_noise_add_value);
	tex->Pow(32.0f);
	tex->Mul(after_noise_mul_value);

	tex->SinWaveDeformX( 7.0f, 1.0f / 64.0f, MF_PI3 );
	tex->SinWaveDeformY( 9.0f, 1.0f / 64.0f, MF_PI4 );

	static const float grass_color[]= { 0.325f*0.6f, 0.65f*0.6f, 0.2f*0.6f, 0.0f*0.6f };
	static const float one[]= { 1.0f, 1.0f, 1.0f, 1.0f };
	tex->Mix( grass_color, g_dirt_color, one );

	noise_sub_tex.Noise();
	static const float second_noise_mul[]= { 2.0f, 2.0f, 2.0f, 2.0f };
	noise_sub_tex.Mul( second_noise_mul );
	tex->Mul( &noise_sub_tex );
}

void GenSandTexture( mf_Texture* tex )
{
	tex->Noise( 8 );
	mf_Texture tex2( tex->SizeXLog2(), tex->SizeYLog2() );
	tex2.Noise( 5 );

	tex->Sub( &tex2 );
	static const float mul_color[]= { 6.0f, 6.0f, 6.0f, 6.0f };
	tex->Mul( mul_color );
	static const float add_color[]= { 0.4f, 0.4f, 0.4f, 0.4f };
	tex->Add( add_color );

	static const float sand_color[]= { 0.88f, 0.81f, 0.32f, 0.0f };
	tex->Mul( sand_color );
}

void GenRockTexture( mf_Texture* tex )
{
	tex->Noise(7);
	tex->Pow( 0.7f );

	static const float add_color[]= { 0.3f, 0.3f, 0.3f, 0.3f };
	tex->Add( add_color );
	static const float mul_color[]= { 1.0f / 1.3f, 1.0f / 1.3f, 1.0f / 1.3f, 0.0f };
	tex->Mul( mul_color );

	static const float rock_color[]= { 0.6f, 0.5f, 0.55f, 0.0f };
	tex->Mul( rock_color );

	tex->Shift( 111, 57 );
}

void (* const terrain_texture_gen_func[])(mf_Texture* t)=
{
	GenDirtTexture,
	GenDirtWithGrassTexture,
	GenSandTexture,
	GenRockTexture
};