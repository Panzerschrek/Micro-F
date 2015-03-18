#include "micro-f.h"

#include "textures_generation.h"
#include "texture.h"
#include "mf_math.h"
#include "aircraft.h"

static const float g_dirt_color[]= { 0.588f, 0.349f, 0.211f, 0.0f };
static const float g_indicators_background_color[]= { 0.1f, 0.1f, 0.1f, 1.0f };
static const float g_indicators_lines_color[]= { 0.5f, 0.5f, 0.5f, 1.0f };
static const float g_indicators_border_color[]= { 0.4f, 0.3f, 0.2f, 1.0f };
static const float g_invisible_color[]= { 0.0f, 0.0f, 0.0f, 0.0f };


// returns noise in range [0;0xfffffff]
int Noise3(int x, int y, int z, int seed )
{
#if 1
	const int X_NOISE_GEN = 1;
	const int Y_NOISE_GEN = 31337;
	const int Z_NOISE_GEN = 263;
	const int SEED_NOISE_GEN = 1013;
	//const int SHIFT_NOISE_GEN = 13;
#else
	const int X_NOISE_GEN = 1619;
	const int Y_NOISE_GEN = 31337;
	const int Z_NOISE_GEN = 6971;
	const int SEED_NOISE_GEN = 1013;
	//const int SHIFT_NOISE_GEN = 8;
#endif

	// All constants are primes and must remain prime in order for this noise
	// function to work correctly.
	int n = (
	X_NOISE_GEN    * x
	+ Y_NOISE_GEN    * y
	+ Z_NOISE_GEN    * z
	+ SEED_NOISE_GEN * seed)
	& 0x7fffffff;
	n = (n >> 13) ^ n;
	return (n * (n * n * 60493 + 19990303) + 1376312589) & 0x7fffffff;
}

// return result in range [0;0xFFFF]
unsigned short Noise3Interpolated( int x, int y, int z, int k, int seed )
{
	int k_mask1= 1 << k;
	int k_mask= k_mask1 - 1;
	int X= x >> k;
	int Y= y >> k;
	int Z= z >> k;

	int k16= k + 16;
	int noise[8]=
	{
		Noise3( X  , Y  , Z  , seed )>>k16,
		Noise3( X+1, Y  , Z  , seed )>>k16,
		Noise3( X  , Y+1, Z  , seed )>>k16,
		Noise3( X+1, Y+1, Z  , seed )>>k16,
		Noise3( X  , Y  , Z+1, seed )>>k16,
		Noise3( X+1, Y  , Z+1, seed )>>k16,
		Noise3( X+1, Y+1, Z+1, seed )>>k16,
		Noise3( X  , Y+1, Z+1, seed )>>k16
	};

	int dx= x & k_mask;
	int dy= y & k_mask;
	int dz= z & k_mask;
	int inv_dx= k_mask1 - dx;
	int inv_dy= k_mask1 - dy;
	int inv_dz= k_mask1 - dz;

	int z_interpolated[4]=
	{
		dz * noise[4] + inv_dz * noise[0], // x   y  
		dz * noise[5] + inv_dz * noise[1], // x+1 y  
		dz * noise[6] + inv_dz * noise[2], // x   y+1
		dz * noise[7] + inv_dz * noise[3]  // x+1 y+1
	};
	int y_interpolated[2]=
	{
		dy * z_interpolated[2] + inv_dy * z_interpolated[0], //x
		dy * z_interpolated[3] + inv_dy * z_interpolated[1] // x+1
	};

	return (unsigned short)(
		(y_interpolated[1] * dx + inv_dx * y_interpolated[0]) >> (k*2) );
};

// return result in range [0;0xFFFF]
unsigned short Noise3Final( int x, int y, int z, int seed, unsigned int octave_count )
{
	unsigned short r=0;

	int od= 8 - octave_count;
	for( int i= 1 + od, j= 7 - od; i< 8; i++, j-- )
		r+= Noise3Interpolated( x,y,z,i, seed ) >> j;
	return r;
}

void GenNaviballTexture( mf_Texture* tex )
{
	MF_ASSERT(tex->SizeX() == tex->SizeY());
	unsigned int size= tex->SizeX();
	unsigned int size_2= tex->SizeX()/2;

	static const float sky_color[]= { 0.125f, 0.62f, 0.96f, 0.0f };
	static const float ground_color[]= { 0.7f, 0.45f, 0.07f, 0.0f };
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

void GenGuiButtonTexture( mf_Texture* tex )
{
	static const float button_color[]= { 0.2f, 0.7f, 0.1f, 0.8f };
	tex->Fill( button_color );
}

void GenMenuBackgroundTexture( mf_Texture* tex )
{
	tex->Noise();
	static const float mul_color[]= { 0.1f, 0.6f, 0.1f, 1.0f };
	tex->Mul( mul_color );
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
	MF_ASSERT( tex->SizeX() == tex->SizeY() );
	MF_ASSERT( tex->SizeX() >= 512 );
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

	static const float main_color[]= { 0.6f, 0.6f, 0.6f, 0.0f };
	tex->Fill( main_color );

	{
		mf_Texture noise_tex( tex->SizeXLog2(), tex->SizeYLog2() );
		noise_tex.Noise();
		static const float mul_color[]= { 0.25f, 0.25f, 0.25f, 0.0f };
		static const float add_color[]= { 0.75f, 0.75f, 0.75f, 0.0f };
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
	unsigned int size_x= 1 << tex->SizeXLog2();
	unsigned int size_y= 1 << tex->SizeYLog2();
	static const float sun_color0[]= { 1.0f, 0.95f, 0.9f, 1.0f };
	static const float sun_color1[]= { 1.0f, 0.9f, 0.85f, 0.0f };
	tex->RadialGradient( size_x / 2, size_y / 2, size_x/2, sun_color0, sun_color1 );

	tex->Pow( 0.7f );
}

void GenMoonTexture( mf_Texture* textures )
{
	static const char side_basises[6*9]=
	{
		//TODO make correct basis for convertion uv to xyz
		0,1,0 , 1,0,0, 0,0,0,
		0,-1,0, 1,0,0, 0,0,0,
		1,0,0 , 0,0,1, 0,-1,0,
		-1,0,0, 0,0,1, 0,-1,0,
		0,0,1 , 1,0,0, 0,-1,0,
		0,0,-1, 1,0,0, 0,-1,0
	};

	for( unsigned int i= 0; i< 6; i++ )
	{
		float* tex_data= textures[i].GetData();

		int size= textures[0].SizeX();
		const char* current_basis= &side_basises[ i*9 ];

		//textures[i].Noise();
		for( int v= 0; v< size; v++ )
			for( int u= 0; u< size; u++, tex_data+= 4 )
			{
				int xyz[3];
				xyz[0]= current_basis[0] * u + current_basis[1] * v + current_basis[2] * size;
				xyz[1]= current_basis[3] * u + current_basis[4] * v + current_basis[5] * size;
				xyz[2]= current_basis[6] * u + current_basis[7] * v + current_basis[8] * size;

				unsigned short noise= Noise3Final( xyz[0], xyz[1], xyz[2], 0, 6 );
				tex_data[0]= tex_data[1]= tex_data[2]= tex_data[3]= 
					float(noise) / 65536.0f;
			}

	}
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

	tex->PoissonDiskPoints( 12 );
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
	GenF2XXXTexture,
	GenV1Texture
};

void (* const gui_texture_gen_func[LastGuiTexture])(mf_Texture* t)=
{
	GenNaviballTexture,
	GenControlPanelTexture,
	GenThrottleBarTexture,
	GenThrottleIndicatorTexture,
	GenVerticalSpeedIndicatorTexture,
	GenNaviballGlassTexture,
	GenGuiButtonTexture,
	GenMenuBackgroundTexture
};