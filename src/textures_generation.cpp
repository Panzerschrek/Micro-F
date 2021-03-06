#include "micro-f.h"

#include "textures_generation.h"
#include "texture.h"
#include "text.h"
#include "mf_math.h"
#include "aircraft.h"
#include "level.h"

static const float g_dirt_color[]= { 0.588f, 0.349f, 0.211f, 0.0f };
static const float g_indicators_background_color[]= { 0.1f, 0.1f, 0.1f, 1.0f };
static const float g_indicators_lines_color[]= { 0.5f, 0.5f, 0.5f, 1.0f };
static const float g_indicators_border_color[]= { 0.4f, 0.3f, 0.2f, 1.0f };
static const float g_invisible_color[]= { 0.0f, 0.0f, 0.0f, 0.0f };


void SetAlphaToZero( mf_Texture* tex )
{
	static const float min_color[]= { 100500.0f, 100500.0f, 100500.0f, 0.0f };
	tex->Min( min_color );
}

void SetAlphaToOne( mf_Texture* tex )
{
	static const float max_color[]= { 0.0f, 0.0f, 0.0f, 1.0f };
	tex->Max( max_color );
}

void GenNaviballTexture( mf_Texture* tex )
{
	MF_ASSERT(tex->SizeX() == tex->SizeY());
	unsigned int size= tex->SizeX();
	unsigned int size_2= tex->SizeX()/2;

	static const float sky_color[]= { 0.05f, 0.3f, 0.7f, 0.0f };
	static const float ground_color[]= { 0.03f, 0.03f, 0.03f, 0.0f };
	static const float line_color[]= { 0.9f, 0.9f, 0.9f, 1.0f };

	tex->FillRect( 0, 0, size, size_2, ground_color );
	tex->FillRect( 0, size_2, size, size_2, sky_color );

	int deg= 270+180;
	int d_deg= -45;
	for( unsigned int i= 0; i< 8; i++, deg+= d_deg )
	{
		unsigned int dy= i * size / 8;
		unsigned int dx= i * size / 8;
		tex->DrawLine( 0, dy, size, dy, line_color );
		if( i == 4 )
		{
			tex->DrawLine( 0, dy-1, size, dy-1, line_color );
			tex->DrawLine( 0, dy+1, size, dy+1, line_color );
		}
		tex->DrawLine( dx, 0, dx, size, line_color );
		if( (i&1) == 0 )
			tex->DrawLine( dx-1, 0, dx-1, size, line_color );

		char str[16];
		sprintf( str, "%d", deg%360 );
		tex->DrawText( dx + 4, size_2 + 4, 1, line_color, str );
	}
}

void GenControlPanelTexture( mf_Texture* tex )
{
	tex->Noise();
	static const float mul_color[]= { 0.5f, 0.45f, 0.55f, 0.0f };
	tex->Mul( mul_color );
	static const float add_color[]= { 0.0f, 0.0f, 0.0f, 1.0f };
	tex->Add( add_color );

	const unsigned int border_size= 3;

	tex->FillRect( 0, 0, tex->SizeX(), border_size, g_indicators_border_color );
	tex->FillRect( 0, 0, border_size, tex->SizeY(), g_indicators_border_color );
	tex->FillRect( 0, tex->SizeY() - border_size, tex->SizeX(), border_size, g_indicators_border_color );
	tex->FillRect( tex->SizeX() - border_size, 0, border_size, tex->SizeY(), g_indicators_border_color );
}

void GenThrottleBarTexture( mf_Texture* tex )
{
	tex->Fill( g_indicators_background_color );
	for( unsigned int i= 0; i< 10; i++ )
	{
		unsigned int dy= i * tex->SizeY() / 10;
		tex->FillRect( 0, dy, tex->SizeX(), 2, g_indicators_lines_color );
		char str[16];
		sprintf( str, "%d", i*10 );
		tex->DrawText( 0, dy, 1, g_indicators_lines_color, str );
	}
	tex->FillRect( 0, tex->SizeY() - 2, tex->SizeX(), 2, g_indicators_lines_color );
}

void GenThrottleIndicatorTexture( mf_Texture* tex )
{
	static const float color[]= { 0.8f, 0.2f, 0.2f, 1.0f };
	tex->Fill( color );
}

void GenVerticalSpeedIndicatorTexture( mf_Texture* tex )
{
	MF_ASSERT(tex->SizeX() == tex->SizeY());
	unsigned int size_2= tex->SizeX()/2;

	tex->Fill( g_invisible_color );
	tex->FillEllipse( size_2, size_2, size_2 - 1, g_indicators_border_color );
	tex->FillEllipse( size_2, size_2, size_2 - 4, g_indicators_background_color );

	float sxy= float(tex->SizeX() - 10);
	float center_xy= float(size_2);
	for( int angle= -135; angle<= 135; angle+=45 )
	{
		float a= float(angle) * MF_DEG2RAD;
		float dx= mf_Math::cos(a) * sxy;
		float dy= mf_Math::sin(a) * sxy;
		tex->DrawLine( int(center_xy - dx * 0.4f), int(center_xy + 0.4f * dy),
			( tex->SizeX() - int(dx) )/2,
			( tex->SizeX() + int(dy) )/2,
			g_indicators_lines_color );
	}
}

void GenNumbersTexture( mf_Texture* tex )
{
	MF_ASSERT( tex->SizeX() == 16 && tex->SizeY() == 512 );
	tex->Fill( g_indicators_background_color );
	char text[2];
	text[1]= 0;
	static const char numbers_mps[]= "0123456789 m/s";
	for( unsigned int i= 0; i< 14; i++ )
	{
		text[0]= numbers_mps[i];
		tex->DrawText( tex->SizeX() - 1, i * MF_LETTER_HEIGHT * 2, 2, g_indicators_lines_color, text );
	}
}

void GenGuiButtonTexture( mf_Texture* tex )
{
	static const float button_color[]= { 0.2f, 0.7f, 0.1f, 0.8f };
	tex->Fill( button_color );
}

void GenMenuBackgroundTexture( mf_Texture* tex )
{
	tex->PoissonDiskPoints( 34 );
	static const float save_green[4]= { 0.0f, 2.0f, 0.0f, 0.0f };
	tex->Mul( save_green );
	tex->Grayscale();

	static const float invert_color[4]= { 1.0f, 1.0f, 1.0f, 1.0f };
	tex->Invert( invert_color );

	static const float mul_color[4]= { 0.5f, 0.6f, 1.0f, 4.0f };
	tex->Mul( mul_color );

	tex->SinWaveDeformX( 16.0f, 1.0f / 128.0f, 0.0f );
	tex->SinWaveDeformY( 16.0f, 1.0f / 128.0f, 0.0f );

	SetAlphaToOne( tex );
}

void GenTargetAircraftTexture( mf_Texture* tex )
{
	MF_ASSERT( tex->SizeX() == tex->SizeY() );

	static const float c_white_invisible_color[]= { 1.0f, 1.0f, 1.0f, 0.0f };
	static const float c_white_color[]= { 1.0f, 1.0f, 1.0f, 1.0f };

	tex->Fill( c_white_invisible_color );

	float radius= float( tex->SizeX()/2 - 2 );
	for( unsigned int i= 0; i< 3; i++ )
	{
		float vertices[6];
		float angle= float(i) * MF_2PI / 3.0f;
		vertices[0]= vertices[1]= float(tex->SizeX() / 2 );
		vertices[2]= vertices[0] + mf_Math::cos( angle ) * radius;
		vertices[3]= vertices[1] + mf_Math::sin( angle ) * radius;
		angle+= MF_PI3;
		vertices[4]= vertices[0] + mf_Math::cos( angle ) * radius;
		vertices[5]= vertices[1] + mf_Math::sin( angle ) * radius;

		tex->FillTriangle( vertices, c_white_color );
	}

	mf_Texture circle( tex->SizeXLog2(), tex->SizeYLog2() );
	circle.Fill( c_white_invisible_color );
	circle.FillEllipse( tex->SizeX()/2, tex->SizeX()/2, tex->SizeX()/2 - tex->SizeY()/8, c_white_color );
	circle.FillEllipse( tex->SizeX()/2, tex->SizeX()/2, tex->SizeX()/4, c_white_invisible_color );
	tex->Mul( &circle );

	static const float gradient_color0[]= { 1.0f, 1.0f, 1.0f, -0.5f };
	static const float gradient_color1[]= { 1.0f, 1.0f, 1.0f, 0.8f };
	circle.RadialGradient( tex->SizeX()/2, tex->SizeX()/2, tex->SizeX()/2 - tex->SizeY()/8, gradient_color0, gradient_color1 );
	tex->Mul( &circle );
}

void GenNaviballGlassTexture( mf_Texture* tex )
{
	MF_ASSERT(tex->SizeX() == tex->SizeY());
	unsigned int size_2= tex->SizeX()/2;

	static const float c0[]= { 0.5f, 0.5f, 0.5f, 0.4f };
	static const float c1[]= { 0.9f, 0.9f, 0.9f, 0.2f };
	tex->RadialGradient( size_2, size_2, size_2, c0, c1 );	

	const unsigned int border_size= 12;
	mf_Texture tex2( tex->SizeXLog2(), tex->SizeYLog2() );
	tex2.Fill( g_invisible_color );
	static const float white_color[]= { 1.0f, 1.0f, 1.0f, 1.0f };
	tex2.FillEllipse( size_2, size_2, size_2 - border_size, white_color );
	tex->Mul( &tex2 );

	tex2.Fill( g_invisible_color );
	tex2.FillEllipse( size_2, size_2, size_2 - 1, g_indicators_border_color );
	tex2.FillEllipse( size_2, size_2, size_2 - border_size, g_invisible_color );

	tex->Add( &tex2 );

	static const float direction_color[4]= { 0.4f, 0.3f, 0.05f, 1.0f };
	tex->FillRect( size_2 - 32, size_2, 24, 4, direction_color );
	tex->FillRect( size_2 + 8, size_2, 24, 4, direction_color );
	for( unsigned int i= 0; i< 4; i++ )
	{
		tex->DrawLine( size_2 - 9, i + size_2, size_2, i + size_2 - 12, direction_color );
		tex->DrawLine( size_2 + 9, i + size_2, size_2, i + size_2 - 12, direction_color );
	}
	tex->FillRect( size_2 - 2, size_2 - 2, 4, 4, direction_color );
}

void GenF1949Texture( mf_Texture* tex )
{
	const unsigned int tex_scaler= 1<< ( tex->SizeXLog2() - 8 );

	static const float plane_color[]= { 0.2f, 0.4f, 0.1f, 0.2f };
	tex->Fill( plane_color );
	static const float turbine_back_color[]= { 0.7f, 0.1f, 0.3f, 0.0f };
	tex->FillRect( 1 * tex_scaler, 1 * tex_scaler, 34 * tex_scaler, 35 * tex_scaler, turbine_back_color );
	static const float turbine_front_color[]= { 0.3f, 0.1f, 0.7f, 0.0f };
	tex->FillRect( 27 * tex_scaler, 36 * tex_scaler, 71 * tex_scaler, 81 * tex_scaler, turbine_front_color );
	static const float window_color[]= {0.5f, 0.5f, 0.9f, 0.5f };
	tex->FillRect( 207 * tex_scaler, 1 * tex_scaler, 49 * tex_scaler, 88 * tex_scaler, window_color );

	mf_Texture tex2( tex->SizeXLog2(), tex->SizeYLog2() );
	tex2.Noise();
	static const float add_color[]= { 1.5f, 1.5f, 1.5f, 0.0f };
	static const float mul_color[]= { 0.4f, 0.4f, 0.4f, 1.0f };
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
	MF_ASSERT( tex->SizeX() == tex->SizeY() );
	MF_ASSERT( tex->SizeX() >= 512 );
	const unsigned int tex_scaler= 1<< ( tex->SizeXLog2() - 9 );

	static const float main_color[]= { 0.7f, 0.7f, 0.7f, 0.1f };
	tex->Fill( main_color );

	static const float outer_field_color[]= { 0.27f, 0.239f, 0.521f, 0.8f };
	tex->FillRect( 1, 1, tex_scaler * 187, tex_scaler * 222, outer_field_color );

	static const float inner_field_color[]= { 0.4f, 0.4f, 0.5f, 0.3f };
	tex->FillRect( tex_scaler * 189, 1, tex_scaler * 195, tex_scaler * 222, inner_field_color );

	static const float engine_color[]= { 1.0f, 0.7f, 0.65f, 0.0f };
	tex->FillEllipse( 45 * tex_scaler, 294 * tex_scaler, 27 * tex_scaler, engine_color );

	static const float antenna_color[]= { 0.2f, 0.2f, 0.2f,0.0f };
	tex->FillRect( 137 * tex_scaler, 274 * tex_scaler, 53 * tex_scaler, 53 * tex_scaler, antenna_color );

	//static const float antenna_ending_color[]= { 0.8f, 0.2f, 0.2f,0.0f };
	//tex->FillEllipse( 159 * tex_scaler, 290 * tex_scaler, 5 * tex_scaler, antenna_ending_color );

	static const float window_color[]= { 0.5f, 0.5f, 1.0f, 1.0f };
	tex->FillRect( tex_scaler * 140, tex_scaler * 370,
		tex_scaler * 99, tex_scaler * 130, window_color );
}

void GenV1Texture( mf_Texture* tex )
{
	// source texture has size 512x512
	MF_ASSERT( tex->SizeX() == tex->SizeY() );
	MF_ASSERT( tex->SizeX() >= 512 );
	const unsigned int tex_scaler= 1<< ( tex->SizeXLog2() - 9 );

	static const float main_color[]= { 0.6f, 0.6f, 0.6f, 0.3f };
	tex->Fill( main_color );
	{
		mf_Texture noise_tex( tex->SizeXLog2(), tex->SizeYLog2() );
		noise_tex.Noise();
		static const float mul_color[]= { 0.25f, 0.25f, 0.25f, 0.25f };
		static const float add_color[]= { 0.75f, 0.75f, 0.75f, 0.75f };
		noise_tex.Mul( mul_color );
		noise_tex.Add( add_color );
		tex->Mul( &noise_tex );
	}

	static const float engine_front_color[]= { 0.3f, 0.1f, 0.7f, 0.0f };
	static const float engine_back_color[]= { 0.7f, 0.1f, 0.3f, 0.0f };
	tex->FillRect( 110 * tex_scaler, 252 * tex_scaler, 100 * tex_scaler, 100 * tex_scaler, engine_front_color );
	tex->FillRect( 237 * tex_scaler, 243 * tex_scaler, 100 * tex_scaler, 100 * tex_scaler, engine_back_color );

	static const float flag_red_color[]= { 0.8f, 0.05f, 0.05f, 0.0f };
	static const float flag_white_color[]= { 0.9f, 0.9f, 0.9f, 0.0f };
	static const float flag_black_color[]= { 0.1f, 0.1f, 0.1f, 0.0f };

	static const unsigned int swastica_center[]= { 275, 188 };

	tex->FillRect( 249 * tex_scaler, 168 * tex_scaler, 50 * tex_scaler, 40 * tex_scaler, flag_red_color );
	tex->FillEllipse( swastica_center[0] * tex_scaler, swastica_center[1] * tex_scaler, 19 * tex_scaler, flag_white_color );

	for( int i= -2; i<= 2; i++ )
	{
		tex->DrawLine( i+ swastica_center[0] * tex_scaler,  swastica_center[1] * tex_scaler,
			i+ tex_scaler * (swastica_center[0] + 8),  tex_scaler * (swastica_center[1] + 8), flag_black_color );
		tex->DrawLine( i+ swastica_center[0] * tex_scaler,  swastica_center[1] * tex_scaler,
			i+ tex_scaler * (swastica_center[0] + 8),  tex_scaler * (swastica_center[1] - 8), flag_black_color );
		tex->DrawLine( i+ swastica_center[0] * tex_scaler,  swastica_center[1] * tex_scaler,
			i+ tex_scaler * (swastica_center[0] - 8),  tex_scaler * (swastica_center[1] + 8), flag_black_color );
		tex->DrawLine( i+ swastica_center[0] * tex_scaler, swastica_center[1] * tex_scaler,
			i+ tex_scaler * (swastica_center[0] - 8), tex_scaler * (swastica_center[1] - 8), flag_black_color );

		tex->DrawLine( i+ tex_scaler * (swastica_center[0] + 8), tex_scaler * (swastica_center[1] + 8),
			i+ tex_scaler * (swastica_center[0] + 16), tex_scaler * swastica_center[1], flag_black_color );
		tex->DrawLine( i+ tex_scaler * (swastica_center[0] + 8), tex_scaler * (swastica_center[1] - 8),
			i+ tex_scaler * swastica_center[0], tex_scaler * (swastica_center[1] - 16), flag_black_color );
		tex->DrawLine( i+ tex_scaler * (swastica_center[0] - 8), tex_scaler * (swastica_center[1] + 8),
			i+ tex_scaler * swastica_center[0], tex_scaler * (swastica_center[1] + 16), flag_black_color );
		tex->DrawLine( i+ tex_scaler * (swastica_center[0] - 8), tex_scaler * (swastica_center[1] - 8),
			i+ tex_scaler * (swastica_center[0] - 16), tex_scaler * swastica_center[1], flag_black_color );
	}
}

void GenSunTexture( mf_Texture* tex )
{
	MF_ASSERT( tex->SizeXLog2() == tex->SizeYLog2() );

	static const float sun_color0[]= { 1.0f, 0.95f, 0.9f, 1.0f };
	static const float sun_color1[]= { 1.0f, 0.9f, 0.85f, 0.0f };
	tex->RadialGradient( tex->SizeX() / 2, tex->SizeX() / 2, tex->SizeX() / 2 - 2, sun_color0, sun_color1 );

	tex->Pow( 0.7f );
}

void GenMoonTexture( mf_Texture* tex )
{
	static const unsigned int tex_size_log2= 6;
	static const unsigned int tex_size= 64;
	static const unsigned char initial_tex_data[ tex_size * tex_size / 8 ]=
#include "../textures/moon.h"
	;

	MF_ASSERT( tex->SizeX() == tex->SizeY() && tex->SizeXLog2() >= tex_size_log2 );

	unsigned char decompressed_data[ tex_size * tex_size ];
	mfMonochromeImageTo8Bit( initial_tex_data, decompressed_data, tex_size * tex_size );


	float* tex_data= tex->GetData();
	unsigned int ts_shift_k= tex->SizeXLog2() - tex_size_log2;
	unsigned int tc_cell_size= 1 << ts_shift_k;
	unsigned int tc_mask= ( 1 << ts_shift_k ) - 1;
	for( unsigned int y= 0; y< tex->SizeY(); y++ )
	{
		unsigned int ys= y >> ts_shift_k;
		unsigned int y_mask= y & tc_mask;
		for( unsigned int x= 0; x< tex->SizeX(); x++, tex_data+= 4 )
		{
			unsigned int xs= x >> ts_shift_k;
			unsigned int x_mask= x & tc_mask;
			unsigned int y_interpolated[2]=
			{
				decompressed_data[ xs   + ys * tex_size ] * ( tc_cell_size - y_mask ) + decompressed_data[ xs   + (ys+1) * tex_size ] * y_mask,
				decompressed_data[ xs+1 + ys * tex_size ] * ( tc_cell_size - y_mask ) + decompressed_data[ xs+1 + (ys+1) * tex_size ] * y_mask
			};
			unsigned int val= y_interpolated[1] * x_mask + ( tc_cell_size - x_mask ) * y_interpolated[0];
			tex_data[0]= tex_data[1]= tex_data[2]= tex_data[3]= float( val >> (ts_shift_k+ts_shift_k) ) / 255.0f;
		}
	}
	tex->Smooth();
	tex->Smooth();
	tex->Smooth();

	static const float white_color[]= { 1.0f, 1.0f, 1.0f, 1.0f };
	static const float gray_color[]= { 0.5f, 0.5f, 0.5f, 1.0f };
	static const float sub_color[]= { 1.0f, 1.0f, 1.0f, 1.0f };
	tex->Mix( white_color, gray_color, sub_color );

	{
		mf_Texture noise( tex->SizeXLog2(), tex->SizeXLog2() );
		noise.Noise();
		static const float noise_rescale[]= { 0.7f, 0.7f, 0.7f, 100.0f };
		static const float noise_add[]= { 0.3f, 0.3f, 0.3f, 100.0f };
		noise.Mul( noise_rescale );
		noise.Add( noise_add );

		tex->Mul( &noise );
	}

	mf_Texture circle( tex->SizeXLog2(), tex->SizeXLog2() );
	circle.Fill( g_invisible_color );
	circle.FillEllipse( tex->SizeX() / 2, tex->SizeX() / 2, tex->SizeX() / 2 - 12, sub_color );
	tex->Mul( &circle );
}

void GenDirtTexture( mf_Texture* tex )
{
	tex->Noise();
	tex->Pow(0.3f);
	tex->Mul( g_dirt_color );
}

void GenDirtWithGrassTexture( mf_Texture* tex )
{
	tex->PoissonDiskPoints( 32 );
	tex->SinWaveDeformX( 32.0f, 1.0f/128.0f, 0.0f );
	tex->SinWaveDeformY( 32.0f, 1.0f/128.0f, 0.0f );

	static const float extend_green[]= { 0.0f, 4.0f, 0.0f, 1.0f };
	tex->Mul( extend_green );
	static const float clamp_green[]= { 0.0f, 1.0f, 0.0f, 100500.0f };
	tex->Min( clamp_green );

	tex->Grayscale();

	{ // get alpha mask for blending with dirt
		float* tex_data= tex->GetData();
		for( unsigned int i= 0; i< tex->SizeX() * tex->SizeY() * 4; i+=4 )
		{
			if( tex_data[i] > 0.2f ) tex_data[i+3]= 1.0f;
			else tex_data[i+3]= 0.0f;
		}
	}
	{ // make grass
		mf_Texture noise( tex->SizeXLog2(), tex->SizeYLog2() );
		noise.Noise( 8 );
		static const float grass_color0[4]= { 0.325f*1.5f, 0.65f*1.4f, 0.2f*1.3f, 1.0f };
		static const float grass_color1[4]= { 0.325f*2.6f, 0.65f*2.5f, 0.2f*2.4f, 1.0f };
		static const float sub_color[4]= { 1.0f, 1.0f, 1.0f, 1.0f };
		noise.Mix( grass_color0, grass_color1, sub_color );
		tex->Mul( &noise );
	}
	{ // blend two grass passes with dirt
		mf_Texture dirt( tex->SizeXLog2(), tex->SizeYLog2() );
		GenDirtTexture( &dirt );
		static const float dirt_dark_color[]= { 0.6f, 0.6f, 0.6f, 0.6f };
		dirt.Mul( dirt_dark_color );

		mf_Texture shifted_tex( tex->SizeXLog2(), tex->SizeYLog2() );
		shifted_tex.Copy( tex );
		shifted_tex.Shift( 131, 117 );
		shifted_tex.Rotate( 90.0f );

		float* tex_data= tex->GetData();
		float* shifted_data= shifted_tex.GetData();
		float* dirt_data= dirt.GetData();

		for( unsigned int i= 0; i< tex->SizeX() * tex->SizeY() * 4; i+=4 )
		{
			for( unsigned int j= 0 ; j< 3; j++ )
			{
				tex_data[i+j]= tex_data[i+j] * tex_data[i+3] + (1.0f - tex_data[i+3]) * dirt_data[i+j];
				tex_data[i+j]= shifted_data[i+j] * shifted_data[i+3] + (1.0f - shifted_data[i+3]) * tex_data[i+j];
			}
		}
	}
}

void GenSandTexture( mf_Texture* tex )
{
	mf_Rand randomizer;

	static const unsigned char sand_colors[]=
	{
		188, 166, 129,  131, 111, 74,   167, 143, 107,
		176, 153, 119,  152, 133, 93,   125, 105, 64 ,
		139, 119, 86,   180, 155, 124,  189, 171, 125,
		139, 121, 85,   158, 144, 83,   175, 155, 122,
		145, 163, 106,  175, 157, 111,  72,  57,  28,
	};
	float* tex_data= tex->GetData();
	for( unsigned int i= 0; i< tex->SizeX() * tex->SizeY() * 4; i+=4 )
	{
		unsigned int color_num= randomizer.Rand() % (sizeof(sand_colors)/3);
		color_num*=3;
		for( unsigned int j= 0; j< 3; j++ )
			tex_data[i+j]= float(sand_colors[color_num+j]) * (1.0f/255.0f);
	}

	{ // make waves on sand
		mf_Texture wave( tex->SizeXLog2()-3, tex->SizeYLog2() );
		static const float gray0[4]= { 0.95f, 0.95f, 0.95f, 0.95f };
		static const float gray1[4]= { 1.05f, 1.05f, 1.05f, 1.05f };
		wave.Gradient( 0, 0, wave.SizeX(), 0, gray0, gray1 );
		
		mf_Texture wave_flip( tex->SizeXLog2()-3, tex->SizeYLog2() );
		wave_flip.Copy( &wave );
		wave_flip.FlipX();

		mf_Texture waves( tex->SizeXLog2(), tex->SizeYLog2() );
		for( unsigned int i= 0; i< 4; i++ )
		{
			waves.CopyRect( &wave_flip, wave.SizeX(), wave.SizeY(), i*tex->SizeX()/4, 0, 0, 0 );
			waves.CopyRect( &wave     , wave.SizeX(), wave.SizeY(), i*tex->SizeX()/4 + tex->SizeX()/8, 0, 0, 0 );
		}
		waves.SinWaveDeformY( 12.0f, 1.0f/256.0f, 0.0f );
		waves.SinWaveDeformX( 24.0f, 1.0f/512.0f, 0.0f );
		tex->Mul( &waves );
	}
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

void GenSmokeParticle( mf_Texture* tex )
{
	MF_ASSERT( tex->SizeXLog2() == tex->SizeYLog2() );

	tex->Noise();
	mf_Texture circle( tex->SizeXLog2(), tex->SizeYLog2() );
	static const float center_color[4]= { 1.2f, 1.2f, 1.2f, 4.0f };
	static const float border_color[4]= { 1.2f, 1.2f, 1.2f, 0.0f };
	circle.RadialGradient( tex->SizeX()/2, tex->SizeX()/2, tex->SizeX()/2 - 2, center_color, border_color );

	tex->Mul( &circle );
}

void GenEngineFireParticle( mf_Texture* tex )
{
	MF_ASSERT( tex->SizeXLog2() == tex->SizeYLog2() );

	static const float center_color[4]= { 1.0f, 0.9f, 0.5f, 4.0f };
	static const float border_color[4]= { 1.0f, 0.9f, 0.5f, 0.0f };
	tex->RadialGradient( tex->SizeX()/2, tex->SizeX()/2, tex->SizeX()/2 - 2, center_color, border_color );
}

void GenEnginePlasmaParticle( mf_Texture* tex )
{
	MF_ASSERT( tex->SizeXLog2() == tex->SizeYLog2() );

	static const float center_color[4]= { 0.5f, 0.1f, 1.0f, 4.0f };
	static const float border_color[4]= { 0.5f, 0.1f, 1.0f, 0.0f };
	tex->RadialGradient( tex->SizeX()/2, tex->SizeX()/2, tex->SizeX()/2 - 2, center_color, border_color );
}

void GenPowerupGlowParticle( mf_Texture* tex )
{
	MF_ASSERT( tex->SizeXLog2() == tex->SizeYLog2() );

	static const float center_color[4]= { 1.0f, 0.9f, 0.2f, 0.7f };
	static const float border_color[4]= { 1.0f, 0.9f, 0.2f, 0.0f };
	tex->RadialGradient( tex->SizeX()/2, tex->SizeX()/2, tex->SizeX()/2 - 2, center_color, border_color );
}

void GenPlasmaBallParticle( mf_Texture* tex )
{
	MF_ASSERT( tex->SizeXLog2() == tex->SizeYLog2() );

	static const float color[]= { 1.0f, 0.5f, 0.3f, -0.4f };
	static const float bright_color[]= { 1.0f*2.0f, 0.5f*2.0f, 0.3f*2.0f, 1.5f };
	tex->RadialGradient( tex->SizeX()/2, tex->SizeX()/2, tex->SizeX()/2, bright_color, color );
	tex->FillEllipse( tex->SizeX(), tex->SizeX(), tex->SizeX()/2, g_invisible_color );
}

void GenFireParticle( mf_Texture* tex )
{
	MF_ASSERT( tex->SizeXLog2() == tex->SizeYLog2() );

	tex->PoissonDiskPoints( 16 );
	static const float save_red[4]= { 3.0f, 0.0f, 0.0f, 0.0f };
	tex->Mul( save_red );
	tex->Grayscale();

	static const float red_color[4]= { 1.0f, 0.5f, 0.5f, 1.0f };
	static const float yellow_color[4]= { 1.0f, 1.0f, 0.5f, 1.0f };
	static const float one[4]= { 1.0f, 1.0f, 1.0f, 1.0f };
	tex->Mix( red_color, yellow_color, one );

	tex->SinWaveDeformX( 4.0f, 1.0f / 32.0f, 0.0f );
	tex->SinWaveDeformY( 4.0f, 1.0f / 32.0f, 0.0f );

	mf_Texture circle( tex->SizeXLog2(), tex->SizeYLog2() );
	static const float center_color[4]= { 1.0f, 1.0f, 1.0f, 2.0f };
	static const float border_color[4]= { 1.0f, 1.0f, 1.0f, 0.0f };
	circle.RadialGradient( tex->SizeX()/2, tex->SizeX()/2, tex->SizeX()/2, center_color, border_color );

	tex->Mul( &circle );
}

void GenForcefieldTexture( mf_Texture* tex )
{
	tex->GenHexagonalGrid( float(tex->SizeX()) * (0.5f/1.5f), 2.0f * 1.0f / mf_Math::sqrt(3.0f) );

	static const float mul_color[]= { 0.7f, 0.7f, 0.7f, 0.7f };
	static const float add_color[]= { 0.3f, 0.3f, 0.3f, 0.3f };
	tex->Mul( mul_color );
	tex->Add( add_color );

	static const float forcefield_color[4]= {0.3f, 0.3f, 0.7f, 1.0f };
	tex->Mul( forcefield_color );
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
	GenF2XXXTexture,
	GenV1Texture
};

void GenOakTexture( mf_Texture* tex )
{
	mf_Texture extended_texture( tex->SizeXLog2() + 2, tex->SizeYLog2() );
	{
		mf_Texture extended_texture2( tex->SizeXLog2() + 2, tex->SizeYLog2() );

		extended_texture.PoissonDiskPoints( 40 );
		extended_texture2.PoissonDiskPoints( 19 );

		static const float half_color[]= { 0.5f, 0.5f, 0.5f, 0.5f };
		extended_texture.Mul( half_color );
		extended_texture2.Mul( half_color );
		extended_texture.Add( &extended_texture2 );
	}

	static const float extend_green_color[]= { 0.0f, 3.0f, 0.0f, 0.0f };
	static const float clamp_color[]={ 0.0f, 1.0f, 0.0f, 0.0f };
	extended_texture.Mul( extend_green_color );
	extended_texture.Min( clamp_color );
	extended_texture.DownscaleX();
	extended_texture.DownscaleX();
	tex->CopyRect( &extended_texture, tex->SizeY(), tex->SizeY(), 0, 0, 0, 0 );

	tex->Grayscale();
	static const float mul3_color[4]= { 3.0f, 3.0f, 3.0f, 1.0f };
	tex->Mul( mul3_color );

	static const float oak_color[4]= { 0.51f, 0.3764f, 0.239f, 1.0f };
	static const float oak_color_dark[4]= { 0.159f, 0.1491f, 0.1f, 1.0f };
	static const float sub_color[4]= { 1.0f, 1.0f, 1.0f, 1.0f };
	tex->Mix( oak_color, oak_color_dark, sub_color );

	SetAlphaToOne( tex );
}

void GenLeafs( mf_Texture* tex, const float* base_color, float color_delta )
{
	mf_Rand randomizer;

	tex->PoissonDiskPoints( 19 );
	tex->SinWaveDeformX( 8.0f, 1.0f/64.0f, 0.0f );
	tex->SinWaveDeformY( 8.0f, 1.0f/64.0f, 0.0f );

	static const float extend_green[]= { 0.0f, 4.0f, 0.0f, 1.0f };
	tex->Mul( extend_green );

	static const float clamp_green[]= { 0.0f, 1.0f, 0.0f, 100500.0f };
	tex->Min(clamp_green);

	const unsigned int c_random_leaf_colors_count= 64;
	float random_colors[c_random_leaf_colors_count][4];
	for( unsigned int i= 0; i< c_random_leaf_colors_count; i++ )
		for( unsigned int j= 0; j< 3; j++ )
			random_colors[i][j]= base_color[j] + randomizer.RandF( color_delta );
	
	float* data= tex->GetData();
	for( unsigned int i= 0; i< tex->SizeX() * tex->SizeY(); i++, data+= 4 )
	{
		float c= data[1];
		unsigned int color_ind= ((unsigned int)data[3]) % c_random_leaf_colors_count;
		data[0]= random_colors[color_ind][0];
		data[1]= random_colors[color_ind][1];
		data[2]= random_colors[color_ind][2];
		data[3]= c;
	}

	mf_Texture circle( tex->SizeXLog2(), tex->SizeYLog2() );
	static const float background_alpha_color[]= { 0.0f, 0.0f, 0.0f, 0.2f };
	circle.Fill( background_alpha_color );
	static const float circle_color[]= { 1.0f, 1.0f, 1.0f, 1.0f };
	circle.FillEllipse( tex->SizeX()/2, tex->SizeY()/2, tex->SizeX()/2-2, circle_color );

	tex->Mul( &circle );
}

void GenOakLeafsTexture( mf_Texture* tex )
{
	static const float leaf_color[]= { 0.28f, 0.45f, 0.09f, 1.0f };
	GenLeafs( tex, leaf_color, 0.2f );
}

void GenBirchLeafsTexture( mf_Texture* tex )
{
	static const float leaf_color[]= { 0.18f, 0.42f, 0.09f, 1.0f };
	GenLeafs( tex, leaf_color, 0.15f );
}

void GenSpruceTexture( mf_Texture* tex )
{
	static const float bark_color[]= { 0.356f, 0.18f, 0.03f, 1.0f };
	tex->Noise();
	tex->DownscaleX();
	tex->DownscaleX();
	tex->Mul( bark_color );

	SetAlphaToOne( tex );
}

void GenSpruceBranch_r( mf_Texture* tex, float size, float ang, float* pos, int depth )
{
	mf_Rand randomizer;

	static const float color[]= { 0.492f, 0.335f, 0.139f, 1.0f };
	static const float needle_color[]= { 0.019f, 0.3137f, 0.019f, 1.0f };
	float final_color[4];
	final_color[3]= 1.0f;
	static const float triangle[12]=
	{
		-16.0f, 0.0f, 0.0f,
		 -6.0f, 1012.0f, 0.0f,
		  6.0f, 1012.0f, 0.0f,
		 16.0f, 0.0f,  0.0f
	};
	static const float needle_triangle[]=
	{
		-2.8f, 0.0f, 0.0f,
		 2.8f, 0.0f, 0.0f,
		 0.0f, 24.0f, 0.0f
	};
	float trans_triangle[12];
	float trans_triangle_2d[8];

	float m[16], m_s[16], m_r[16], m_t[16];

	Mat4Identity(m_s);
	Mat4Identity(m_t);

	m_s[0]= m_s[5]= size;

	m_t[12]= pos[0];
	m_t[13]= pos[1];

	Mat4RotateZ( m_r, ang );

	Mat4Mul( m_r, m_s, m );
	Mat4Mul( m, m_t );

	for( int i= 0; i< 4; i++ )
	{
		Vec3Mat4Mul( triangle + 3*i, m, trans_triangle + 3*i );
		trans_triangle_2d[i*2]= trans_triangle[i*3];
		trans_triangle_2d[i*2+1]= trans_triangle[i*3+1];
	}
	tex->FillTriangle( trans_triangle_2d, color );
	trans_triangle_2d[2]= trans_triangle_2d[6];
	trans_triangle_2d[3]= trans_triangle_2d[7];
	tex->FillTriangle( trans_triangle_2d, color );

	const float needle_step= 6.0f;
	int n= int( 1012.0f * size / needle_step);
	float pos2[2]={pos[0],pos[1]};
	float width= size * 16.0f;
	float dw= size * ( 16.0f - 6.0f ) / float(n);
	for( int i= 0; i< n; i++ , width-=dw )
	{
		pos2[0]+=  needle_step * mf_Math::cos(ang+MF_PI2);
		pos2[1]+=  needle_step * mf_Math::sin(ang+MF_PI2);

		Mat4Identity(m_s);;
		Mat4Identity(m_t);

		float color_delta= randomizer.RandF(0.0625f);
		for( int i= 0; i< 3; i++ )
			final_color[i]= needle_color[i] + color_delta;

		float da= -MF_PI3;
		for( int j= 0; j< 2; j++, da+= MF_2PI3 )
		{
			Mat4RotateZ( m_r, ang + da );

			m_t[12]= pos2[0]-width*m_r[1];
			m_t[13]= pos2[1]+width*m_r[0];

			Mat4Mul( m_r, m_t, m );

			for( int k= 0; k< 3; k++ )
			{
				Vec3Mat4Mul( needle_triangle + 3*k, m, trans_triangle );
				trans_triangle_2d[k*2]= trans_triangle[0];
				trans_triangle_2d[k*2+1]= trans_triangle[1];
			}
			tex->FillTriangle( trans_triangle_2d, final_color );
		}
	}

	if( depth != 0 )
	{
		float pos2[]= { pos[0], pos[1] };
		while( size > 0.1f )
		{
			pos2[0]+= size * 0.3333f * 1024.0f * mf_Math::cos(ang+MF_PI2);
			pos2[1]+= size * 0.3333f * 1024.0f * mf_Math::sin(ang+MF_PI2);

			GenSpruceBranch_r( tex, size * 0.3333f , ang +  MF_PI3, pos2, depth-1 );
			GenSpruceBranch_r( tex, size * 0.3333f, ang -   MF_PI3, pos2, depth-1 );

			size*= 0.6666f;
		}
	} // if not null depth
}

void GenSpruceBranch( mf_Texture* tex )
{
	MF_ASSERT( tex->SizeX() == tex->SizeY() );
	mf_Texture big_tex( 10, 10 );

	static const float color[4]= { 0.019f, 0.345f, 0.019f, 0.2f };
	big_tex.Fill(color);

	float pos[]= { 512.0f, 0.0f };
	GenSpruceBranch_r( &big_tex, 1.0f, 0.0f, pos, 3 );

	pos[1]= 80.0f;
	GenSpruceBranch_r( &big_tex, 0.45f, MF_PI3, pos, 2 );
	GenSpruceBranch_r( &big_tex, 0.45f, -MF_PI3, pos, 2 );

	if( tex->SizeXLog2() == 8 )
	{
		big_tex.DownscaleX();
		big_tex.DownscaleY();
		big_tex.DownscaleX();
		big_tex.DownscaleY();
		tex->CopyRect( &big_tex, tex->SizeX(),tex->SizeY(), 0, 0, 0, 0 );
	}
	else if( tex->SizeXLog2() == 9 )
	{
		big_tex.DownscaleX();
		big_tex.DownscaleY();
		tex->CopyRect( &big_tex, tex->SizeX(),tex->SizeY(), 0, 0, 0, 0 );
	}
	else if( tex->SizeYLog2() == 10 )
	{
		tex->Copy( &big_tex );
	}
}

void GenBirchBarkTexture( mf_Texture* tex )
{
	tex->Noise();
	static const float mul_color[]= { 0.2f, 0.2f, 0.2f, 0.2f };
	static const float add_color[]= { 0.6f, 0.6f, 0.6f, 0.6f };
	tex->Mul( mul_color );
	tex->Add( add_color );

	{ // gen black stripes on bark
		mf_Texture stripes_tex( tex->SizeXLog2()+1, tex->SizeYLog2() +2 );
		stripes_tex.PoissonDiskPoints( 96 );

		float* tex_data= stripes_tex.GetData();
		for( unsigned int i= 0; i< stripes_tex.SizeX() * stripes_tex.SizeY() * 4; i+= 4 )
		{
			static const float c_middle= 0.6f;
			float l= 1.0f - tex_data[i+1];
			l= (l - c_middle) * 4.0f + c_middle;
			l= mf_Math::clamp( 0.0f, 1.0f, l );
			tex_data[i]= tex_data[i+1]= tex_data[i+2]= l;
			tex_data[i+3]= l > c_middle ? 0.0f : 1.0f;
		}
		stripes_tex.DownscaleX();
		stripes_tex.DownscaleY();
		stripes_tex.DownscaleY();
		stripes_tex.DownscaleY();
		stripes_tex.DownscaleY();

		mf_Texture stripes_tex_downscaled( tex->SizeXLog2(), tex->SizeYLog2() );
		stripes_tex_downscaled.CopyRect( &stripes_tex, tex->SizeX(), tex->SizeY(), 0, 0, 0, 0 );

		tex->AlphaBlendOneMinusDst( &stripes_tex_downscaled );
	}
	/*{ // gen low bark part

		mf_Texture noise( tex->SizeXLog2(), tex->SizeYLog2() );
		mf_Texture gradient( tex->SizeXLog2(), tex->SizeYLog2() );
		noise.Noise();

		static const float grad_color0[4]= { 0.0f, 0.0f, 0.0f, 0.0f };
		static const float grad_color1[4]= { 3.0f, 3.0f, 3.0f, 3.0f };
		gradient.Gradient( 0, 0, 0, tex->SizeY()/3, grad_color1, grad_color0 );
		gradient.Mul( &noise );
		gradient.Pow( 2.0f );

		tex->Min( &gradient );
	}*/

	SetAlphaToOne( tex );
}

void (* const gui_texture_gen_func[LastGuiTexture])(mf_Texture* t)=
{
	GenNaviballTexture,
	GenControlPanelTexture,
	GenThrottleBarTexture,
	GenThrottleIndicatorTexture,
	GenVerticalSpeedIndicatorTexture,
	GenNumbersTexture,
	GenNaviballGlassTexture,
	GenGuiButtonTexture,
	GenMenuBackgroundTexture,
	GenTargetAircraftTexture
};

void (* const static_level_object_texture_gen_func[LastStaticLevelObjectTexture])(mf_Texture* t)=
{
	GenOakTexture,
	GenOakLeafsTexture,
	GenSpruceTexture,
	GenSpruceBranch,
	GenBirchBarkTexture,
	GenBirchLeafsTexture
};

void (* const particles_texture_gen_func[LastParticleTexture])(mf_Texture* t)=
{
	GenSmokeParticle,
	GenEngineFireParticle,
	GenEnginePlasmaParticle,
	GenPowerupGlowParticle,
	GenPlasmaBallParticle,
	GenFireParticle
};