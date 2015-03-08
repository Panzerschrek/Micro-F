#include "micro-f.h"

#include "textures_generation.h"
#include "texture.h"
#include "mf_math.h"
#include "aircraft.h"

static const float g_dirt_color[]= { 0.588f, 0.349f, 0.211f, 0.0f };


void GenNaviballTexture( mf_Texture* tex )
{
	static const float sky_color[]= { 0.125f, 0.62f, 0.96f, 0.0f };
	static const float ground_color[]= { 0.7f, 0.45f, 0.07f, 0.0f };
	static const float line_color[]= { 0.8f, 0.8f, .8f, 0.8f };

	tex->FillRect( 0, 0, tex->SizeX(), tex->SizeY() / 2, ground_color );
	tex->FillRect( 0, tex->SizeY() / 2, tex->SizeX(), tex->SizeY() / 2, sky_color );

	unsigned int deg= 0;
	unsigned int d_deg= 45;
	for( unsigned int i= 0; i< 8; i++, deg+= d_deg )
	{
		unsigned int dy= i * tex->SizeY() / 8;
		unsigned int dx= i * tex->SizeX() / 8;
		tex->DrawLine( 0, dy, tex->SizeX(), dy, line_color );
		if( i == 4 )
		{
			tex->DrawLine( 0, dy-1, tex->SizeX(), dy-1, line_color );
			tex->DrawLine( 0, dy+1, tex->SizeX(), dy+1, line_color );
		}
		tex->DrawLine( dx, 0, dx, tex->SizeY(), line_color );
		if( (i&1) == 0 )
			tex->DrawLine( dx-1, 0, dx-1, tex->SizeY(), line_color );

		char str[16];
		sprintf( str, "%d", deg );
		tex->DrawText( dx + 4, tex->SizeY() / 2 + 4, 1, line_color, str );
	}
}

void GenF1949Texture( mf_Texture* tex )
{
	const unsigned int tex_scaler= 1<< ( tex->SizeXLog2() - 8 );

	static const float plane_color[]= { 0.2f, 0.4f, 0.1f, 0.0f };
	tex->Fill( plane_color );
	static const float turbine_back_color[]= { 0.7f, 0.1f, 0.3f, 0.0f };
	tex->FillRect( 1 * tex_scaler, 1 * tex_scaler, 34 * tex_scaler, 35 * tex_scaler, turbine_back_color );
	static const float turbine_front_color[]= { 0.3f, 0.1f, 0.7f, 0.0f };
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

void GenF2XXXTexture( mf_Texture* tex )
{
	// source texture has size 512x512
	const unsigned int tex_scaler= 1<< ( tex->SizeXLog2() - 9 );

	static const float main_color[]= { 0.7f, 0.7f, 0.7f, 0.0f };
	tex->Fill( main_color );

	static const float outer_field_color[]= { 0.27f, 0.239f, 0.521f, 0.0f };
	tex->FillRect( 1, 1, tex_scaler * 187, tex_scaler * 222, outer_field_color );

	static const float inner_field_color[]= { 0.4f, 0.4f, 0.5f, 0.0f };
	tex->FillRect( tex_scaler * 189, 1, tex_scaler * 195, tex_scaler * 222, inner_field_color );

	static const float engine_color[]= { 1.0f, 0.7f, 0.65f, 0.0f };
	tex->FillEllipse( 45 * tex_scaler, 294 * tex_scaler, 27 * tex_scaler, engine_color );

	static const float antenna_color[]= { 0.2f, 0.2f, 0.2f,0.0f };
	tex->FillRect( 137 * tex_scaler, 274 * tex_scaler, 53 * tex_scaler, 53 * tex_scaler, antenna_color );

	//static const float antenna_ending_color[]= { 0.8f, 0.2f, 0.2f,0.0f };
	//tex->FillEllipse( 159 * tex_scaler, 290 * tex_scaler, 5 * tex_scaler, antenna_ending_color );
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

void GenMoonTexture( mf_Texture* tex )
{
	static const float black_color[]= { 0.0f, 0.0f, 0.0f, 0.0f };
	tex->Fill( black_color );

	unsigned int size_x= 1 << tex->SizeXLog2();
	unsigned int size_y= 1 << tex->SizeYLog2();
	static const float moon_color[]= { 1.0f, 1.0f, 1.0f, 1.0f };
	tex->FillEllipse( size_x / 2, size_y / 2, size_x/2, moon_color );


	mf_Texture noise_texture( tex->SizeXLog2(), tex->SizeYLog2() );
	noise_texture.Noise();
	noise_texture.Rotate( 37.0f );
	static const float noise_mul_color[]= { 0.33f, 0.33f, 0.33f, 0.33f };
	noise_texture.Mul( noise_mul_color );
	static const float noise_add_color[]= { 0.66f, 0.66f, 0.66f, 0.66f };
	noise_texture.Add( noise_add_color );

	tex->Mul( &noise_texture );
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

void (* const aircraft_texture_gen_func[mf_Aircraft::LastType])(mf_Texture* t)=
{
	GenF1949Texture,
	GenF2XXXTexture
};