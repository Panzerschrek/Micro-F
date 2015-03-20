#include "micro-f.h"
#include "renderer.h"

#include "main_loop.h"
#include "shaders.h"
#include "mf_math.h"
#include "texture.h"

#include "mf_model.h"
#include "drawing_model.h"
#include "models_generation.h"
#include "textures_generation.h"

#include "../models/models.h"

#define MF_WATER_QUAD_SIZE_CL 4 // size in terrain cells

#define MF_TERRAIN_CHUNK_SIZE_CL 8
#define MF_TERRAIN_CHUNK_SIZE_CL_LOG2 3
#define MF_TERRAIN_MESH_SIZE_X_CHUNKS 96
#define MF_TERRAIN_MESH_SIZE_Y_CHUNKS 144

#define MF_ZERO_STENCIL_VALUE 128

struct mf_StarVertex
{
	char xyz[3];
	unsigned char intencity;
};

static const float g_sun_distance_scaler= 1.5f;
static const float g_stars_distance_scaler= 1.7f;
static const float g_sky_radius_scaler= 2.0f;
static const float g_zfar_scaler= 2.5f;

mf_Renderer::mf_Renderer( mf_Player* player, mf_Level* level, mf_Text* text )
	: player_(player), level_(level)
	, text_(text)
{
	// prepare terrain shader
	terrain_shader_.SetAttribLocation( "p", 0 );
	terrain_shader_.Create( mf_Shaders::terrain_shader_v, mf_Shaders::terrain_shader_f );
	static const char* const terrain_shader_uniforms[]= {
		/*vert*/ "mat", "smat", "hm", "nm", "sh", "wl", /*frag*/ "tex", "stex", "sun", "sl", "al" };
	terrain_shader_.FindUniforms( terrain_shader_uniforms,
		sizeof(terrain_shader_uniforms) / sizeof(char*) );

	// prepare terrain shadowmap shader
	terrain_shadowmap_shader_.SetAttribLocation( "p", 0 );
	terrain_shadowmap_shader_.Create( mf_Shaders::terrain_shadowmap_shader_v, NULL );
	static const char* const terrain_shadowmap_shader_uniforms[]= {"mat", "hm", "sh" };
	terrain_shadowmap_shader_.FindUniforms( terrain_shadowmap_shader_uniforms,
		sizeof(terrain_shadowmap_shader_uniforms) / sizeof(char*) );

	//water shader preparing
	water_shader_.SetAttribLocation( "p", 0 );
	water_shader_.Create( mf_Shaders::water_shader_v, mf_Shaders::water_shader_f );
	static const char* const water_shader_uniforms[]= { "mat", "wl", "tcs", "tex", "its", "cp", "ph" };
	water_shader_.FindUniforms( water_shader_uniforms,
		sizeof(water_shader_uniforms) / sizeof(char*) );

	// water shadowmap shader
	water_shadowmap_shader_.SetAttribLocation( "p", 0 );
	water_shadowmap_shader_.Create( mf_Shaders::water_shadowmap_shader_v, NULL );
	static const char* const water_shadowmap_shader_uniforms[]= { "mat", "wl", "tcs", "ph" };
	water_shadowmap_shader_.FindUniforms( water_shadowmap_shader_uniforms,
		sizeof(water_shadowmap_shader_uniforms) / sizeof(char*) );

	// aircraf shader
	aircraft_shader_.SetAttribLocation( "p", 0 );
	aircraft_shader_.SetAttribLocation( "n", 1 );
	aircraft_shader_.SetAttribLocation( "tc", 2 );
	aircraft_shader_.Create( mf_Shaders::models_shader_v, mf_Shaders::models_shader_f );
	static const char* const aircraft_shader_uniforms[]= {
		/*vert*/ "mat", "nmat", "texn",/*frag*/ "tex", "sun", "sl", "al" };
	aircraft_shader_.FindUniforms( aircraft_shader_uniforms, sizeof(aircraft_shader_uniforms) / sizeof(char*) );

	// aircraft shadow shader
	aircraft_shadowmap_shader_.SetAttribLocation( "p", 0 );
	aircraft_shadowmap_shader_.Create( mf_Shaders::models_shadowmap_shader_v, NULL );
	aircraft_shadowmap_shader_.FindUniform( "mat" );

	// aircraft stencil shadow
	aircraft_stencil_shadow_shader_.SetAttribLocation( "p", 0 );
	aircraft_stencil_shadow_shader_.Create( mf_Shaders::models_stencil_shadow_shader_v, NULL, mf_Shaders::models_stencil_shadow_shader_g );
	aircraft_stencil_shadow_shader_.FindUniform( "mat" );
	aircraft_stencil_shadow_shader_.FindUniform( "sun" );

	level_static_objects_shader_.SetAttribLocation( "p", 0 );
	level_static_objects_shader_.SetAttribLocation( "n", 1 );
	level_static_objects_shader_.SetAttribLocation( "tc", 2 );
	level_static_objects_shader_.Create( mf_Shaders::static_models_shader_v, mf_Shaders::models_shader_f );
	static const char* const static_objects_shader_uniforms[]= {
		/*vert*/ "mat", "nmat", "texn",/*frag*/ "tex", "sun", "sl", "al" };
	level_static_objects_shader_.FindUniforms( static_objects_shader_uniforms, sizeof(static_objects_shader_uniforms) / sizeof(char*) );

	// sun shader
	sun_shader_.Create( mf_Shaders::sun_shader_v, mf_Shaders::sun_shader_f );
	static const char* const sun_shader_uniforms[]= { "mat", "s", "tex", "i" };
	sun_shader_.FindUniforms( sun_shader_uniforms, sizeof(sun_shader_uniforms) / sizeof(char*) );

	// sky shader
	sky_shader_.SetAttribLocation( "p", 0 );
	sky_shader_.Create( mf_Shaders::sky_shader_v, mf_Shaders::sky_shader_f );
	static const char* const sky_shader_unifroms[]= { "mat", "sun", "sky_k", "tu" };
	sky_shader_.FindUniforms( sky_shader_unifroms, sizeof(sky_shader_unifroms) / sizeof(char*) );

	// stars shader
	stars_shader_.SetAttribLocation( "p", 0 );
	stars_shader_.SetAttribLocation( "i", 1 );
	stars_shader_.Create( mf_Shaders::stars_shader_v, mf_Shaders::stars_shader_f );
	static const char* const stars_shader_uniforms[]= { "mat", "s" };
	stars_shader_.FindUniforms( stars_shader_uniforms, sizeof(stars_shader_uniforms) / sizeof(char*) );

	GenTerrainMesh();
	GenWaterMesh();
	PrepareAircraftModels();

	{ // sky mesh
		mf_DrawingModel model;
		GenSkySphere( &model );
		sky_vbo_.VertexData( model.GetVertexData(), model.VertexCount() * sizeof(mf_DrawingModelVertex), sizeof(mf_DrawingModelVertex) );
		sky_vbo_.IndexData( model.GetIndexData(), model.IndexCount() * sizeof(unsigned short) );

		mf_DrawingModelVertex vert;
		unsigned int shift;
		shift= (char*)vert.pos - (char*)&vert;
		sky_vbo_.VertexAttrib( 0, 3, GL_FLOAT, false, shift );
	}
	{ // stars
		mf_Rand randomizer;
		const unsigned int result_stars_count= 1024;
		unsigned int count= 0;

		mf_StarVertex* vertices= new mf_StarVertex[ result_stars_count ];
		while( count < result_stars_count )
		{
			float pos[3];
			for( unsigned int i= 0; i< 3; i++ )
				pos[i]= randomizer.RandF( -1.0f, 1.0f );
			float len2= Vec3Dot( pos, pos );
			if( len2 <= 1.0f )
			{
				Vec3Normalize( pos );
				if( pos[2] > -0.1f )
				{
					for( unsigned int i= 0; i< 3; i++ )
						vertices[count].xyz[i]= (char)( pos[i] * 126.9f );
					vertices[count].intencity= (unsigned char) randomizer.RandI(32,255);
					count++;
				}
			}
		}

		stars_vbo_.VertexData( vertices, result_stars_count * sizeof(mf_StarVertex), sizeof(mf_StarVertex) );
		delete[] vertices;

		mf_StarVertex v;
		unsigned int shift;
		shift= ((char*)v.xyz) - ((char*)&v);
		stars_vbo_.VertexAttrib( 0, 3, GL_BYTE, true, shift );
		shift= ((char*)&v.intencity) - ((char*)&v);
		stars_vbo_.VertexAttrib( 1, 1, GL_UNSIGNED_BYTE, true, shift );
	} // stars
	{ // static level meshes
		mf_DrawingModel model;
		GenOak( &model );
		//GenCylinder( &model, 8, 1, true );

		level_static_objects_vbo_.VertexData( model.GetVertexData(), model.VertexCount() * sizeof(mf_DrawingModelVertex), sizeof(mf_DrawingModelVertex) );
		level_static_objects_vbo_.IndexData( model.GetIndexData(), model.IndexCount() * sizeof(unsigned short) );

		mf_DrawingModelVertex vert;
		unsigned int shift;
		shift= (char*)vert.pos - (char*)&vert;
		level_static_objects_vbo_.VertexAttrib( 0, 3, GL_FLOAT, false, shift );
		shift= (char*)vert.normal - (char*)&vert;
		level_static_objects_vbo_.VertexAttrib( 1, 3, GL_FLOAT, false, shift );
		shift= (char*)vert.tex_coord - (char*)&vert;
		level_static_objects_vbo_.VertexAttrib( 2, 2, GL_FLOAT, false, shift );
	}

	{
		// terrain heightmap
		glGenTextures( 1, &terrain_heightmap_texture_ );
		glBindTexture( GL_TEXTURE_2D, terrain_heightmap_texture_ );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_R16,
			level_->TerrainSizeX(), level_->TerrainSizeY(), 0,
			GL_RED, GL_UNSIGNED_SHORT, level_->GetTerrianHeightmapData() );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );

		// terrain normalmap
		glGenTextures( 1, &terrain_normal_map_texture_ );
		glBindTexture( GL_TEXTURE_2D, terrain_normal_map_texture_ );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8_SNORM,
			level->TerrainSizeX(), level->TerrainSizeY(), 0,
			GL_RGBA, GL_BYTE, level->GetTerrainNormalTextureMap() );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	}

	{ // test texture
		/*mf_Texture tex( 4, 4 );
		tex.LinearNormalization(1.0f);

		glGenTextures( 1, &test_texture_ );
		glBindTexture( GL_TEXTURE_2D, test_texture_ );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8,
			1 << tex.SizeXLog2(), 1 << tex.SizeYLog2(), 0,
			GL_RGBA, GL_UNSIGNED_BYTE, tex.GetNormalizedData() );
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
		glGenerateMipmap(GL_TEXTURE_2D);*/
	}

	{ // aircraft textures
		mf_Texture tex( 10, 10 );

		glGenTextures( 1, &aircrafts_data_.textures_array );
		glBindTexture( GL_TEXTURE_2D_ARRAY, aircrafts_data_.textures_array );
		glTexImage3D( GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8,
		tex.SizeX(), tex.SizeY(), mf_Aircraft::LastType,
			0, GL_RGBA, GL_UNSIGNED_BYTE, NULL );
		for (unsigned int i= 0; i< mf_Aircraft::LastType; i++ )
		{
			aircraft_texture_gen_func[i]( &tex );
			tex.LinearNormalization( 1.0f );
			glTexSubImage3D( GL_TEXTURE_2D_ARRAY, 0,
				0, 0, i,
				tex.SizeX(), tex.SizeY(), 1,
				GL_RGBA, GL_UNSIGNED_BYTE, tex.GetNormalizedData() );
		}
		glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
		glGenerateMipmap( GL_TEXTURE_2D_ARRAY );
	}

	{ // sun texture
		mf_Texture sun_tex( 6, 6 );
		GenSunTexture( &sun_tex );
		sun_tex.LinearNormalization(1.0f);

		glGenTextures( 1, &sun_texture_ );
		glBindTexture( GL_TEXTURE_2D, sun_texture_ );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8,
			sun_tex.SizeX(), sun_tex.SizeY(), 0,
			GL_RGBA, GL_UNSIGNED_BYTE, sun_tex.GetNormalizedData() );
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
		glGenerateMipmap( GL_TEXTURE_2D );
	}

	{ // terrain textures
		const unsigned int terrain_texture_size_log2= 9;
		const unsigned int terrain_texture_size= 1 << terrain_texture_size_log2;
		glGenTextures( 1, &terrain_textures_array_ );
		glBindTexture( GL_TEXTURE_2D_ARRAY, terrain_textures_array_ );
		glTexImage3D( GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8,
			terrain_texture_size, terrain_texture_size, LastTerrainTexture,
			0, GL_RGBA, GL_UNSIGNED_BYTE, NULL );
		mf_Texture tex( terrain_texture_size_log2, terrain_texture_size_log2 );
		for( unsigned int i= 0; i< LastTerrainTexture; i++ )
		{
			terrain_texture_gen_func[i]( &tex );
			tex.LinearNormalization( 1.0f );
			glTexSubImage3D( GL_TEXTURE_2D_ARRAY, 0,
				0, 0, i,
				terrain_texture_size, terrain_texture_size, 1,
				GL_RGBA, GL_UNSIGNED_BYTE, tex.GetNormalizedData() );
		} // for textures
		glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
		glGenerateMipmap( GL_TEXTURE_2D_ARRAY );
	}

	CreateWaterReflectionFramebuffer();
	CreateShadowmapFramebuffer();

	/*{
		mf_Texture tex[6]=
		{
			mf_Texture(8,8),
			mf_Texture(8,8),
			mf_Texture(8,8),
			mf_Texture(8,8),
			mf_Texture(8,8),
			mf_Texture(8,8)
		};
		GenMoonTexture( tex );

		GLuint tex_id;

		glGenTextures( 1, &tex_id );
		glBindTexture( GL_TEXTURE_CUBE_MAP, tex_id );
		for( unsigned int i= 0; i< 6; i++ )
		{
			tex[i].LinearNormalization( 1.0f );

			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGBA8,
				tex[i].SizeX(), tex[i].SizeY(), 0, GL_RGBA, GL_UNSIGNED_BYTE, tex[i].GetNormalizedData() );
		}
		glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	}*/
}

mf_Renderer::~mf_Renderer()
{
}

void mf_Renderer::Resize()
{
	glDeleteFramebuffers( 1, &water_reflection_fbo_.fbo_id );
	glDeleteTextures( 1, &water_reflection_fbo_.tex_id );
	glDeleteTextures( 1, &water_reflection_fbo_.depth_tex_id );

	CreateWaterReflectionFramebuffer();
}

void mf_Renderer::DrawFrame()
{
	glClearStencil( MF_ZERO_STENCIL_VALUE );

	glBindFramebuffer( GL_FRAMEBUFFER, shadowmap_fbo_.fbo_id );
	glViewport( 0, 0, shadowmap_fbo_.size[0], shadowmap_fbo_.size[1] );
	glClear( GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
	DrawShadows();

	glBindFramebuffer( GL_FRAMEBUFFER, water_reflection_fbo_.fbo_id );
	glViewport( 0, 0, water_reflection_fbo_.size[0], water_reflection_fbo_.size[1] );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
	CreateViewMatrix( view_matrix_, true );
	DrawTerrain( true );
	{
		const mf_Aircraft* aircrafts[1];
		aircrafts[0]= player_->GetAircraft();
		DrawAircrafts( aircrafts, 1 );
	}
	DrawLevelStaticObjects( true );
	DrawSky( true );
	DrawStars( true );
	DrawSun( true );

	mf_MainLoop* main_loop= mf_MainLoop::Instance();

	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
	glViewport( 0, 0, main_loop->ViewportWidth(), main_loop->ViewportHeight() );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

	CreateViewMatrix( view_matrix_, false );
	DrawTerrain( false );
	{
		const mf_Aircraft* aircrafts[1];
		aircrafts[0]= player_->GetAircraft();
		DrawAircrafts( aircrafts, 1 );
	}
	DrawLevelStaticObjects( false );
	DrawSky( false );
	DrawStars( false );
	DrawSun( false );
	DrawWater();

#ifdef MF_DEBUG
	{
		unsigned int first_water_quad, water_quad_count;
		CalculateWaterMeshVisiblyPart( &first_water_quad, &water_quad_count );
		char str[64];
		sprintf( str, "water quads: %d\n", water_quad_count );
		text_->AddText( 0, 1, 1, mf_Text::default_color, str );
	}
#endif
}

void mf_Renderer::CreateWaterReflectionFramebuffer()
{
	mf_MainLoop* main_loop = mf_MainLoop::Instance();
	water_reflection_fbo_.size[0]= main_loop->ViewportWidth() / 2;
	water_reflection_fbo_.size[1]= main_loop->ViewportHeight() / 2;

	// color texture
	glGenTextures( 1, &water_reflection_fbo_.tex_id );
	glBindTexture( GL_TEXTURE_2D, water_reflection_fbo_.tex_id );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8,
		water_reflection_fbo_.size[0], water_reflection_fbo_.size[1],
		0, GL_RGBA, GL_UNSIGNED_BYTE, NULL );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

	// depth buffer
	glGenTextures( 1, &water_reflection_fbo_.depth_tex_id );
	glBindTexture( GL_TEXTURE_2D, water_reflection_fbo_.depth_tex_id );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH24_STENCIL8,
		water_reflection_fbo_.size[0], water_reflection_fbo_.size[1],
		0, GL_DEPTH_STENCIL, GL_UNSIGNED_INT_24_8, NULL );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

	glGenFramebuffers( 1, &water_reflection_fbo_.fbo_id );
	glBindFramebuffer( GL_FRAMEBUFFER, water_reflection_fbo_.fbo_id );
	glFramebufferTexture( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, water_reflection_fbo_.tex_id, 0 );
	glFramebufferTexture( GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, water_reflection_fbo_.depth_tex_id, 0 );
	GLuint color_attachment= GL_COLOR_ATTACHMENT0;
	glDrawBuffers( 1, &color_attachment );

	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
}

void mf_Renderer::CreateShadowmapFramebuffer()
{
	shadowmap_fbo_.sun_azimuth= -MF_PI3;
	shadowmap_fbo_.sun_elevation= MF_PI6;

	SphericalCoordinatesToVec( shadowmap_fbo_.sun_azimuth, shadowmap_fbo_.sun_elevation, shadowmap_fbo_.sun_vector );

	shadowmap_fbo_.sun_light_intensity[0]= 0.9f;
	shadowmap_fbo_.sun_light_intensity[1]= 0.9f;
	shadowmap_fbo_.sun_light_intensity[2]= 0.8f;
	shadowmap_fbo_.ambient_sky_light_intensity[0]= 0.2f;
	shadowmap_fbo_.ambient_sky_light_intensity[1]= 0.23f;
	shadowmap_fbo_.ambient_sky_light_intensity[2]= 0.29f;

	shadowmap_fbo_.size[0]= 2048;
	shadowmap_fbo_.size[1]= 2048;

	glGenTextures( 1, &shadowmap_fbo_.depth_tex_id );
	glBindTexture( GL_TEXTURE_2D, shadowmap_fbo_.depth_tex_id );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16,
		shadowmap_fbo_.size[0], shadowmap_fbo_.size[1],
		0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_REF_TO_TEXTURE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL );

	glGenFramebuffers( 1, &shadowmap_fbo_.fbo_id );
	glBindFramebuffer( GL_FRAMEBUFFER, shadowmap_fbo_.fbo_id );
	glFramebufferTexture( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, shadowmap_fbo_.depth_tex_id, 0 );
	GLuint color_attachment= GL_NONE;
	glDrawBuffers( 1, &color_attachment );

	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
}

void mf_Renderer::GenTerrainMesh()
{
	unsigned int traingle_count= 0;
	unsigned char chunk_lod_table[ MF_TERRAIN_MESH_SIZE_X_CHUNKS * MF_TERRAIN_MESH_SIZE_Y_CHUNKS ];
	static const int lod_dst[]= { 64*64, 160*160, 352*352, 448*448, 1024*1024, 0x7FFFFFFF };

	// calculate chunks lods
	unsigned int center_x= MF_TERRAIN_CHUNK_SIZE_CL * MF_TERRAIN_MESH_SIZE_X_CHUNKS / 2;
	unsigned int center_y= MF_TERRAIN_CHUNK_SIZE_CL * MF_TERRAIN_MESH_SIZE_Y_CHUNKS / 2;
	for( unsigned int y= 0; y< MF_TERRAIN_MESH_SIZE_Y_CHUNKS; y++ )
	{
		int cl_y= y * MF_TERRAIN_CHUNK_SIZE_CL + MF_TERRAIN_CHUNK_SIZE_CL / 2;
		int y_dst2= cl_y - center_y;
		y_dst2*= y_dst2;
		for( unsigned int x= 0; x< MF_TERRAIN_MESH_SIZE_X_CHUNKS; x++ )
		{
			int cl_x= x * MF_TERRAIN_CHUNK_SIZE_CL + MF_TERRAIN_CHUNK_SIZE_CL / 2;
			int x_dst= cl_x - center_x;
			int dst2= y_dst2 + x_dst * x_dst;
			for( unsigned int i= 0, i_end= MF_TERRAIN_CHUNK_SIZE_CL_LOG2+1; i< i_end; i++ )
				if( dst2 < lod_dst[i] || i == i_end - 1 )
				{
					chunk_lod_table[ x + y * MF_TERRAIN_MESH_SIZE_X_CHUNKS ]= (unsigned char)i;
					traingle_count+= 2 * ( ( MF_TERRAIN_CHUNK_SIZE_CL * MF_TERRAIN_CHUNK_SIZE_CL ) >> (i+i) );
					break;
				}
		} // for x
	} // for y

	// calculate patch count
	unsigned int patch_triangle_count= 0;
	for( unsigned int y= 0; y< MF_TERRAIN_MESH_SIZE_Y_CHUNKS-1; y++ )
		for( unsigned int x= 0; x< MF_TERRAIN_MESH_SIZE_X_CHUNKS-1; x++ )
		{
			unsigned int lod  = chunk_lod_table[ x   +  y    * MF_TERRAIN_MESH_SIZE_X_CHUNKS ];
			unsigned int lod_x= chunk_lod_table[ x+1 +  y    * MF_TERRAIN_MESH_SIZE_X_CHUNKS ];
			unsigned int lod_y= chunk_lod_table[ x   + (y+1) * MF_TERRAIN_MESH_SIZE_X_CHUNKS ];
			if( lod != lod_x )
			{
				unsigned int max_lod= (lod > lod_x) ? lod : lod_x;
				patch_triangle_count+= MF_TERRAIN_CHUNK_SIZE_CL >> max_lod;
			}
			if( lod != lod_y )
			{
				unsigned int max_lod= (lod > lod_y) ? lod : lod_y;
				patch_triangle_count+= MF_TERRAIN_CHUNK_SIZE_CL >> max_lod;
			}
		}
	traingle_count+= patch_triangle_count;

	unsigned short* quads= new unsigned short[ traingle_count * 6 ];
	unsigned short* q= quads;

	// gen terrain triangles
	for( unsigned int y= 0; y< MF_TERRAIN_MESH_SIZE_Y_CHUNKS; y++ )
		for( unsigned int x= 0; x< MF_TERRAIN_MESH_SIZE_X_CHUNKS; x++ )
		{
			unsigned int lod= chunk_lod_table[ x + y * MF_TERRAIN_MESH_SIZE_X_CHUNKS ];
			unsigned int chunk_quad_count= MF_TERRAIN_CHUNK_SIZE_CL >> lod;
			unsigned short quad_size= 1 << lod;
			for( unsigned int j= 0; j< chunk_quad_count; j++ )
				for( unsigned int i= 0; i< chunk_quad_count; i++ )
				{
					unsigned short sx= (unsigned short)( x * MF_TERRAIN_CHUNK_SIZE_CL );
					sx+= (unsigned short)(i * quad_size);
					unsigned short sy= (unsigned short)( y * MF_TERRAIN_CHUNK_SIZE_CL );
					sy+= (unsigned short)(j * quad_size);

					q[ 0]= sx;
					q[ 1]= sy;
					q[ 2]= sx + quad_size;
					q[ 3]= sy;
					q[ 5]= sy + quad_size;
					q[ 6]= sx;
					q[ 8]= sx + quad_size;
					q[11]= sy + quad_size;
					if( ((i^j)&1) != 0 || chunk_quad_count == 1 )
					{
						q[ 4]= sx + quad_size;
						q[ 7]= sy;
						q[ 9]= sy + quad_size;
						q[10]= sx;
					}
					else
					{
						q[ 4]= sx;
						q[ 7]= sy + quad_size;
						q[ 9]= sy;
						q[10]= sx + quad_size;
					}
					q+= 12;
				}
		} // for chunk x

	// gen patches
	for( unsigned int y= 0; y< MF_TERRAIN_MESH_SIZE_Y_CHUNKS-1; y++ )
		for( unsigned int x= 0; x< MF_TERRAIN_MESH_SIZE_X_CHUNKS-1; x++ )
		{
			unsigned int lod  = chunk_lod_table[ x   +  y    * MF_TERRAIN_MESH_SIZE_X_CHUNKS ];
			unsigned int lod_x= chunk_lod_table[ x+1 +  y    * MF_TERRAIN_MESH_SIZE_X_CHUNKS ];
			unsigned int lod_y= chunk_lod_table[ x   + (y+1) * MF_TERRAIN_MESH_SIZE_X_CHUNKS ];
			if( lod != lod_x )
			{
				unsigned short s_x= (unsigned short) ( (x+1) * MF_TERRAIN_CHUNK_SIZE_CL );
				unsigned short s_y= (unsigned short) (  y    * MF_TERRAIN_CHUNK_SIZE_CL );

				unsigned int max_lod= (lod > lod_x) ? lod : lod_x;
				unsigned int triangle_count= MF_TERRAIN_CHUNK_SIZE_CL >> max_lod;
				unsigned int triangle_small_size= 1<< (max_lod-1);
				for( unsigned int py= 0; py< triangle_count; py++ )
				{
					q[0]= s_x;
					q[1]= s_y + (unsigned short)(py * triangle_small_size * 2);
					q[2]= s_x;
					q[3]= s_y + (unsigned short)(py * triangle_small_size * 2 + triangle_small_size);
					q[4]= s_x;
					q[5]= s_y + (unsigned short)(py * triangle_small_size * 2 + triangle_small_size * 2);
					q+= 6;
				}
			} // if( lod != lod_x )
			if( lod != lod_y )
			{
				unsigned short s_x= (unsigned short) (  x    * MF_TERRAIN_CHUNK_SIZE_CL );
				unsigned short s_y= (unsigned short) ( (y+1) * MF_TERRAIN_CHUNK_SIZE_CL );

				unsigned int max_lod= (lod > lod_y) ? lod : lod_y;
				unsigned int triangle_count= MF_TERRAIN_CHUNK_SIZE_CL >> max_lod;
				unsigned int triangle_small_size= 1<< (max_lod-1);
				for( unsigned int px= 0; px< triangle_count; px++ )
				{
					q[0]= s_x + (unsigned short)(px * triangle_small_size * 2);
					q[1]= s_y;
					q[2]= s_x + (unsigned short)(px * triangle_small_size * 2 + triangle_small_size);
					q[3]= s_y;
					q[4]= s_x + (unsigned short)(px * triangle_small_size * 2 + triangle_small_size * 2);
					q[5]= s_y;
					q+= 6;
				}
			} // if( lod != lod_y )
		} // for x

	terrain_mesh_patch_triangle_count_= patch_triangle_count;
	terrain_vbo_.VertexData( quads, traingle_count* 6 * sizeof(short), sizeof(short) * 2 );
	delete[] quads;
	terrain_vbo_.VertexAttrib( 0, 2, GL_UNSIGNED_SHORT, false, 0 );

	MF_DEBUG_INFO_STR_I( "terrain patch triangle count: ", patch_triangle_count );
	MF_DEBUG_INFO_STR_I( "terrain triangle count: ", traingle_count );
}

void mf_Renderer::GenWaterMesh()
{
	const unsigned short* heightmap= level_->GetTerrianHeightmapData();
	unsigned short water_level= (unsigned short) ( float(0xFFFF) * 1.02f * level_->TerrainWaterLevel() / level_->TerrainAmplitude() );

	unsigned int size_x= level_->TerrainSizeX() / MF_WATER_QUAD_SIZE_CL;
	unsigned int size_y= level_->TerrainSizeY() / MF_WATER_QUAD_SIZE_CL;

	water_mesh_.waves_frequency= 0.2f;

	water_mesh_.quad_rows= new QuadRow[ size_y ];
	water_mesh_.quad_row_count= size_y;

	bool* is_underwater_matrix= new bool[ size_x * size_y ];
	memset( is_underwater_matrix, 0, size_x * size_y * sizeof(bool) );

	for( unsigned int y= 0; y< level_->TerrainSizeY(); y++ )
	{
		const unsigned short* hm= heightmap + y * level_->TerrainSizeX();
		bool* matrix_raw= is_underwater_matrix + (y/MF_WATER_QUAD_SIZE_CL) * size_x;
		for( unsigned int x= 0; x< level_->TerrainSizeX(); x++ )
		{
			if( hm[x] <= water_level )
				matrix_raw[x / MF_WATER_QUAD_SIZE_CL ]= true;
		}
	}

	unsigned int quad_count= 0;
	for( unsigned int i= 0; i< size_x * size_y; i++ )
		if( is_underwater_matrix[i] ) quad_count++;

	unsigned short* quads= new unsigned short[ quad_count * 12 ];
	unsigned short* q= quads;

	for( unsigned int y= 0; y< size_y; y++ )
	{
		water_mesh_.quad_rows[y].first_quad_number= (q - quads) / 12;
		water_mesh_.quad_rows[y].quad_count= 0;

		for( unsigned int x= 0; x< size_x; x++ )
			if( is_underwater_matrix[ x + y * size_x ] )
			{
				unsigned short sx= (unsigned short)( x * MF_WATER_QUAD_SIZE_CL );
				unsigned short sy= (unsigned short)( y * MF_WATER_QUAD_SIZE_CL );
				q[ 0]= sx;
				q[ 1]= sy;
				q[ 2]= sx + MF_WATER_QUAD_SIZE_CL;
				q[ 3]= sy;
				q[ 5]= sy + MF_WATER_QUAD_SIZE_CL;
				q[ 6]= sx;
				q[ 8]= sx + MF_WATER_QUAD_SIZE_CL;
				q[11]= sy + MF_WATER_QUAD_SIZE_CL;
				if(((x^y)&1) != 0)
				{
					q[ 4]= sx + MF_WATER_QUAD_SIZE_CL;
					q[ 7]= sy;
					q[ 9]= sy + MF_WATER_QUAD_SIZE_CL;
					q[10]= sx;
				}
				else
				{
					q[ 4]= sx;
					q[ 7]= sy + MF_WATER_QUAD_SIZE_CL;
					q[ 9]= sy;
					q[10]= sx + MF_WATER_QUAD_SIZE_CL;
				}
				water_mesh_.quad_rows[y].quad_count++;
				q+= 12;
			}// if is quad
	} // for y

	delete[] is_underwater_matrix;

	water_mesh_.vbo.VertexData( quads, sizeof(short) * 12 * quad_count, sizeof(short) * 2 );
	delete[] quads;
	water_mesh_.vbo.VertexAttrib( 0, 2, GL_UNSIGNED_SHORT, false, 0 );

	MF_DEBUG_INFO_STR_I( "water triangle count: ", quad_count * 2 );
}

void mf_Renderer::PrepareAircraftModels()
{
	unsigned int total_vertex_count= 0;
	unsigned int total_index_count= 0;

	for (unsigned int i= 0; i< mf_Aircraft::LastType; i++ )
	{
		const mf_ModelHeader* header= (const mf_ModelHeader*) mf_Models::aircraft_models[i];
		total_vertex_count+= header->vertex_count;
		total_index_count+= header->trialgle_count * 3;
	}

	aircrafts_data_.vbo.VertexData( NULL, total_vertex_count * sizeof(mf_DrawingModelVertex), sizeof(mf_DrawingModelVertex) );
	aircrafts_data_.vbo.IndexData( NULL, total_index_count * sizeof(unsigned short) );

	mf_DrawingModelVertex vert;
	unsigned int shift;
	shift= (char*)vert.pos - (char*)&vert;
	aircrafts_data_.vbo.VertexAttrib( 0, 3, GL_FLOAT, false, shift );
	shift= (char*)vert.normal - (char*)&vert;
	aircrafts_data_.vbo.VertexAttrib( 1, 3, GL_FLOAT, false, shift );
	shift= (char*)vert.tex_coord - (char*)&vert;
	aircrafts_data_.vbo.VertexAttrib( 2, 2, GL_FLOAT, false, shift );

	mf_DrawingModel model;

	unsigned int vertex_shift= 0;
	unsigned int index_shift= 0;
	for (unsigned int i= 0; i< mf_Aircraft::LastType; i++ )
	{
		model.LoadFromMFMD( mf_Models::aircraft_models[i] );

		aircrafts_data_.vbo.VertexSubData(
			model.GetVertexData(), 
			model.VertexCount() * sizeof(mf_DrawingModelVertex),
			vertex_shift * sizeof(mf_DrawingModelVertex) );

		unsigned short* indeces= (unsigned short*) model.GetIndexData();
		for( unsigned int ind= 0; ind< model.IndexCount(); ind++ )
			indeces[ind]+= (unsigned short)vertex_shift;

		aircrafts_data_.vbo.IndexSubData(
			model.GetIndexData(),
			model.IndexCount() * sizeof(unsigned short),
			index_shift * sizeof(unsigned short) );

		aircrafts_data_.models[i].first_index= index_shift;
		aircrafts_data_.models[i].index_count= model.IndexCount();

		vertex_shift+= model.VertexCount();
		index_shift+= model.IndexCount();
	}
}

void mf_Renderer::CreateViewMatrix( float* out_matrix, bool water_reflection )
{
	mf_MainLoop* main_loop= mf_MainLoop::Instance();

	float pers_mat[16];
	float rot_z_mat[16];
	float rot_x_mat[16];
	float rot_y_mat[16];
	float basis_change_mat[16];
	float translate_mat[16];
	float final_flip_mat[16];

	float translate_vec[3];

	if( water_reflection )
	{
		translate_vec[0]= -player_->Pos()[0];
		translate_vec[1]= -player_->Pos()[1];
		translate_vec[2]= -( 2.0f * level_->TerrainWaterLevel() - player_->Pos()[2] );
	}
	else
		Vec3Mul( player_->Pos(), -1.0f, translate_vec );
	Mat4Translate( translate_mat, translate_vec );

	Mat4Perspective( pers_mat,
		float(main_loop->ViewportWidth())/ float(main_loop->ViewportHeight()),
		player_->Fov(), 0.5f, GetSceneRadius() * g_zfar_scaler );

	Mat4RotateZ( rot_z_mat, -player_->Angle()[2] );
	Mat4RotateX( rot_x_mat, water_reflection ? player_->Angle()[0] : -player_->Angle()[0] );
	Mat4RotateY( rot_y_mat, water_reflection ? player_->Angle()[1] : -player_->Angle()[1] );

	{
		Mat4RotateX( basis_change_mat, -MF_PI2 );
		float tmp_mat[16];
		Mat4Identity( tmp_mat );
		tmp_mat[10]= -1.0f;
		Mat4Mul( basis_change_mat, tmp_mat );
	}

	if( water_reflection )
	{
		Mat4Identity( final_flip_mat );
		final_flip_mat[5]= -1.0f;
	}

	Mat4Mul( translate_mat, rot_z_mat, out_matrix );
	Mat4Mul( out_matrix, rot_x_mat );
	Mat4Mul( out_matrix, rot_y_mat );
	Mat4Mul( out_matrix, basis_change_mat );
	Mat4Mul( out_matrix, pers_mat );
	if( water_reflection )
		Mat4Mul( out_matrix, final_flip_mat );
}

void mf_Renderer::CreateTerrainMatrix( float* out_matrix )
{
	float terrain_scale_vec[3];

	terrain_scale_vec[0]= level_->TerrainCellSize();
	terrain_scale_vec[1]= level_->TerrainCellSize();
	terrain_scale_vec[2]= level_->TerrainAmplitude();
	Mat4Scale( out_matrix, terrain_scale_vec );

}

void mf_Renderer::CreateAircraftMatrix( const mf_Aircraft* aircraft, float* out_matrix, float* optional_normal_matrix )
{
	float translate_mat[16];
	float common_rotate_mat[16];
	float tmp_mat[16];

	Mat4Identity( common_rotate_mat );
	VEC3_CPY( common_rotate_mat+0, aircraft->AxisVec(0) );
	VEC3_CPY( common_rotate_mat+4, aircraft->AxisVec(1) );
	VEC3_CPY( common_rotate_mat+8, aircraft->AxisVec(2) );

	Mat4Translate( translate_mat, aircraft->Pos() );

	Mat4Mul( common_rotate_mat, translate_mat, tmp_mat );
	Mat4Mul( tmp_mat, view_matrix_, out_matrix );

	if( optional_normal_matrix != NULL )
		Mat4ToMat3( common_rotate_mat, optional_normal_matrix );
}

void mf_Renderer::GetTerrainMeshShift( float* out_shift )
{
	const float cell_step= float(MF_TERRAIN_CHUNK_SIZE_CL);
	static const float half_terrain_size_cl[]=
	{
		float(MF_TERRAIN_MESH_SIZE_X_CHUNKS * MF_TERRAIN_CHUNK_SIZE_CL / 2),
		float(MF_TERRAIN_MESH_SIZE_Y_CHUNKS * MF_TERRAIN_CHUNK_SIZE_CL / 2)
	};
	for( unsigned int i= 0; i< 2; i++ )
	{
		out_shift[i]= floorf( player_->Pos()[i] / ( level_->TerrainCellSize() * cell_step ) ) * cell_step
			- half_terrain_size_cl[i];
		if( out_shift[i] < 0.0f ) out_shift[i]= 0.0f;
		out_shift[i]+= 0.0005f; // compenstation of GPU accuracy problems
	}
}

float mf_Renderer::GetSceneRadius()
{
	float d[3];
	d[0]= float(MF_TERRAIN_MESH_SIZE_X_CHUNKS * MF_TERRAIN_CHUNK_SIZE_CL) * level_->TerrainCellSize();
	d[1]= float(MF_TERRAIN_MESH_SIZE_X_CHUNKS * MF_TERRAIN_CHUNK_SIZE_CL) * level_->TerrainCellSize();
	d[2]= level_->TerrainAmplitude();
	return Vec3Len(d) * 0.5f;
}

void mf_Renderer::CalculateWaterMeshVisiblyPart( unsigned int* first_quad, unsigned int* quad_count )
{
	const unsigned int water_raw_distance_y= ( MF_TERRAIN_MESH_SIZE_Y_CHUNKS * MF_TERRAIN_CHUNK_SIZE_CL / 2 ) / MF_WATER_QUAD_SIZE_CL + 2;
	int cam_y= int( player_->Pos()[1] / level_->TerrainCellSize() ) / MF_WATER_QUAD_SIZE_CL;
	int y_min= cam_y - water_raw_distance_y;
	int y_max= cam_y + water_raw_distance_y;
	if( y_min < 0 ) y_min= 0;
	else if( y_min >= int(water_mesh_.quad_row_count) ) y_min= water_mesh_.quad_row_count - 1;
	if( y_max < 0 ) y_max= 0;
	else if( y_max >= int(water_mesh_.quad_row_count) ) y_max= water_mesh_.quad_row_count - 1;
	*quad_count= water_mesh_.quad_rows[y_max].first_quad_number - water_mesh_.quad_rows[y_min].first_quad_number;
	*first_quad= water_mesh_.quad_rows[y_min].first_quad_number;

}

void mf_Renderer::DrawTerrain(bool draw_to_water_framebuffer )
{
	float mat[16];
	float terrrain_mat[16];

	CreateTerrainMatrix( terrrain_mat );
	Mat4Mul( terrrain_mat, view_matrix_, mat );

	terrain_shader_.Bind();
	terrain_shader_.UniformMat4( "mat", mat );

	terrain_shader_.UniformMat4( "smat", terrain_shadow_matrix_ );

	float terrain_shift[3];
	GetTerrainMeshShift( terrain_shift );
	terrain_shader_.UniformVec3( "sh", terrain_shift );

	terrain_shader_.UniformFloat( "wl", level_->TerrainWaterLevel() / level_->TerrainAmplitude() );

	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, terrain_heightmap_texture_ );
	terrain_shader_.UniformInt( "hm", 0 );

	glActiveTexture( GL_TEXTURE1 );
	glBindTexture( GL_TEXTURE_2D, terrain_normal_map_texture_ );
	terrain_shader_.UniformInt( "nm", 1 );

	glActiveTexture( GL_TEXTURE2 );
	glBindTexture( GL_TEXTURE_2D_ARRAY, terrain_textures_array_ );
	terrain_shader_.UniformInt( "tex", 2 );

	glActiveTexture( GL_TEXTURE3 );
	glBindTexture( GL_TEXTURE_2D, shadowmap_fbo_.depth_tex_id );
	terrain_shader_.UniformInt( "stex", 3 );

	float terrain_invert_size[3];
	terrain_invert_size[0]= 1.0f / float(level_->TerrainSizeX());
	terrain_invert_size[1]= 1.0f / float(level_->TerrainSizeY());
	terrain_shader_.UniformVec3( "tis", terrain_invert_size );

	terrain_shader_.UniformVec3( "sun", shadowmap_fbo_.sun_vector );
	terrain_shader_.UniformVec3( "sl", shadowmap_fbo_.sun_light_intensity );
	terrain_shader_.UniformVec3( "al", shadowmap_fbo_.ambient_sky_light_intensity );

	terrain_vbo_.Bind();
	unsigned int patches_vertex_count= terrain_mesh_patch_triangle_count_ * 3;
	if( draw_to_water_framebuffer )
	{
		glCullFace( GL_FRONT );
		glEnable( GL_CLIP_DISTANCE0 );
	}
	else glCullFace( GL_BACK );

	glEnable( GL_CULL_FACE );
	glDrawArrays( GL_TRIANGLES, 0, terrain_vbo_.VertexCount() - patches_vertex_count );
	glDisable( GL_CULL_FACE );
	// raw terrain patches separatly, without culling
	glDrawArrays( GL_TRIANGLES, terrain_vbo_.VertexCount() - patches_vertex_count, patches_vertex_count );

	if( draw_to_water_framebuffer )
		glDisable( GL_CLIP_DISTANCE0 );
}

void mf_Renderer::DrawSun( bool draw_to_water_framebuffer )
{
	sun_shader_.Bind();

	float mat[16];
	float translate_mat[16];
	float translate_vec[3];
	Vec3Mul( shadowmap_fbo_.sun_vector, GetSceneRadius() * g_sun_distance_scaler, translate_vec );
	Vec3Add( translate_vec, player_->Pos() );
	Mat4Translate( translate_mat, translate_vec );
	Mat4Mul( translate_mat, view_matrix_, mat );

	sun_shader_.UniformMat4( "mat", mat );

	float sprite_size= (128.0f/1024.0f) / mf_Math::tan(player_->Fov() * 0.5f);
	if( draw_to_water_framebuffer )
		sprite_size*= float(water_reflection_fbo_.size[1]);
	else
		sprite_size*= float(mf_MainLoop::Instance()->ViewportHeight());
	sun_shader_.UniformFloat( "s", sprite_size );

	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, sun_texture_ );
	sun_shader_.UniformInt( "tex", 0 );

	sun_shader_.UniformFloat( "i", 4.0f ); // sun light intencity

	glEnable( GL_PROGRAM_POINT_SIZE );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	glDrawArrays( GL_POINTS, 0, 1 );

	glDisable( GL_BLEND );
	glDisable( GL_PROGRAM_POINT_SIZE );
}

void mf_Renderer::DrawStars( bool draw_to_water_framebuffer )
{
	stars_shader_.Bind();

	float mat[16];
	float translate_mat[16];
	float scale_mat[16];
	Mat4Scale( scale_mat, g_stars_distance_scaler * GetSceneRadius() );

	float translate_vec[3];
	translate_vec[0]= player_->Pos()[0];
	translate_vec[1]= player_->Pos()[1];
	if( draw_to_water_framebuffer )
		translate_vec[2]= ( 2.0f * level_->TerrainWaterLevel() - player_->Pos()[2] );
	else translate_vec[2]= player_->Pos()[2];
	Mat4Translate( translate_mat, translate_vec );

	Mat4Mul( scale_mat, translate_mat, mat );
	Mat4Mul( mat, view_matrix_ );

	stars_shader_.UniformMat4( "mat", mat );

	float sprite_size;
	const float init_sprite_size= 6.0f / 1024.0f;
	if( draw_to_water_framebuffer )
		sprite_size= float(water_reflection_fbo_.size[1]) * init_sprite_size;
	else
		sprite_size= float(mf_MainLoop::Instance()->ViewportHeight()) * init_sprite_size;
	stars_shader_.UniformFloat( "s", sprite_size );

	glEnable( GL_PROGRAM_POINT_SIZE );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	stars_vbo_.Bind();
	glDrawArrays( GL_POINTS, 0, stars_vbo_.VertexCount() );

	glDisable( GL_BLEND );
	glDisable( GL_PROGRAM_POINT_SIZE );
}

void mf_Renderer::DrawSky(  bool draw_to_water_framebuffer )
{
	sky_shader_.Bind();

	float mat[16];
	float translate_mat[16];
	float scale_mat[16];
	Mat4Scale( scale_mat, g_sky_radius_scaler * GetSceneRadius() );

	float translate_vec[3];
	translate_vec[0]= player_->Pos()[0];
	translate_vec[1]= player_->Pos()[1];
	if( draw_to_water_framebuffer )
		translate_vec[2]= ( 2.0f * level_->TerrainWaterLevel() - player_->Pos()[2] );
	else translate_vec[2]= player_->Pos()[2];
	Mat4Translate( translate_mat, translate_vec );

	Mat4Mul( scale_mat, translate_mat, mat );
	Mat4Mul( mat, view_matrix_ );

	sky_shader_.UniformMat4( "mat", mat );

	sky_shader_.UniformVec3( "sun", shadowmap_fbo_.sun_vector );

	{ // setup Perez sky model parameters
		static const float tu= 1.8f; // now ( while we not use HDR ) it is optimal paremeter
		static const float sky_k[]=
		{
			 0.17872f * tu - 1.46303f,
			-0.35540f * tu + 0.42749f,
			-0.02266f * tu + 5.32505f,
			 0.12064f * tu - 2.57705f,
			-0.06696f * tu + 0.37027f,
			-0.01925f * tu - 0.25922f,
			-0.06651f * tu + 0.00081f,
			-0.00041f * tu + 0.21247f,
			-0.06409f * tu - 0.89887f,
			-0.00325f * tu + 0.04517f,
			-0.01669f * tu - 0.26078f,
			-0.09495f * tu + 0.00921f,
			-0.00792f * tu + 0.21023f,
			-0.04405f * tu - 1.65369f,
			-0.01092f * tu + 0.05291f,
		};
		sky_shader_.UniformFloatArray( "sky_k", 15, sky_k );
		sky_shader_.UniformFloat( "tu", tu );
	}

	sky_vbo_.Bind();

	glDrawElements(
		GL_TRIANGLES,
		sky_vbo_.IndexDataSize() / sizeof(unsigned short),
		GL_UNSIGNED_SHORT,
		0 );
}

void mf_Renderer::DrawAircrafts( const mf_Aircraft* const* aircrafts, unsigned int count )
{
	aircrafts_data_.vbo.Bind();
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D_ARRAY, aircrafts_data_.textures_array );

	{ // depth pass and ambient light
		aircraft_shader_.Bind();

		aircraft_shader_.UniformInt( "tex", 0 );
		aircraft_shader_.UniformVec3( "sun", shadowmap_fbo_.sun_vector );
		static const float zero_light[3]= { 0.0f, 0.0f, 0.0f };
		aircraft_shader_.UniformVec3( "sl", zero_light );
		aircraft_shader_.UniformVec3( "al", shadowmap_fbo_.ambient_sky_light_intensity );

		for( unsigned int i= 0; i< count; i++ )
		{
			const mf_Aircraft* aircraft= aircrafts[i];
			float normal_mat[9];
			float mat[16];

			CreateAircraftMatrix( aircraft, mat, normal_mat );
			aircraft_shader_.UniformMat4( "mat", mat );
			aircraft_shader_.UniformMat3( "nmat", normal_mat );

			aircraft_shader_.UniformFloat( "texn", float(aircraft->GetType()) + 0.1f );
			
			glDrawElements( GL_TRIANGLES,
				aircrafts_data_.models[ aircraft->GetType() ].index_count,
				GL_UNSIGNED_SHORT,
				(void*) ( aircrafts_data_.models[ aircraft->GetType() ].first_index * sizeof(unsigned short) ) );
		}
	} // depth pass and ambient light
	{ // draw to stencil
		glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );
		glDepthMask( GL_FALSE );

		glEnable( GL_STENCIL_TEST );
		glStencilFunc( GL_ALWAYS, MF_ZERO_STENCIL_VALUE, 255 );
		glStencilOpSeparate( GL_FRONT, GL_KEEP, GL_KEEP, GL_INCR_WRAP );
		glStencilOpSeparate( GL_BACK, GL_KEEP, GL_KEEP, GL_DECR_WRAP );

		aircraft_stencil_shadow_shader_.Bind();

		for( unsigned int i= 0; i< count; i++ )
		{
			const mf_Aircraft* aircraft= aircrafts[i];

			float mat[16];;
			float transformed_sun[3];
			CreateAircraftMatrix( aircraft, mat, NULL );

			for( unsigned int j= 0; j< 3; j++ ) // transform sun vector to model space
				transformed_sun[j]= Vec3Dot( aircraft->AxisVec(j), shadowmap_fbo_.sun_vector );

			aircraft_stencil_shadow_shader_.UniformVec3( "sun", transformed_sun );
			aircraft_stencil_shadow_shader_.UniformMat4( "mat", mat );

			glDrawElements( GL_TRIANGLES,
			aircrafts_data_.models[ aircraft->GetType() ].index_count,
				GL_UNSIGNED_SHORT,
				(void*) ( aircrafts_data_.models[ aircraft->GetType() ].first_index * sizeof(unsigned short) ) );
		}

		glDepthMask( GL_TRUE );
		glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );
	} // draw to stencil
	{ // directional light pass

		glStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );
		glStencilFunc( GL_EQUAL, MF_ZERO_STENCIL_VALUE, 255 );

		aircraft_shader_.Bind();

		aircraft_shader_.UniformInt( "tex", 0 );
		aircraft_shader_.UniformVec3( "sun", shadowmap_fbo_.sun_vector );
		aircraft_shader_.UniformVec3( "sl", shadowmap_fbo_.sun_light_intensity );
		aircraft_shader_.UniformVec3( "al", shadowmap_fbo_.ambient_sky_light_intensity );

		for( unsigned int i= 0; i< count; i++ )
		{
			const mf_Aircraft* aircraft= aircrafts[i];
			float normal_mat[9];
			float mat[16];

			CreateAircraftMatrix( aircraft, mat, normal_mat );
			aircraft_shader_.UniformMat4( "mat", mat );
			aircraft_shader_.UniformMat3( "nmat", normal_mat );

			aircraft_shader_.UniformFloat( "texn", float(aircraft->GetType()) + 0.1f );
			
			glDrawElements( GL_TRIANGLES,
				aircrafts_data_.models[ aircraft->GetType() ].index_count,
				GL_UNSIGNED_SHORT,
				(void*) ( aircrafts_data_.models[ aircraft->GetType() ].first_index * sizeof(unsigned short) ) );
		}
		glDisable( GL_STENCIL_TEST );
	} // directional light pass
}

void mf_Renderer::DrawLevelStaticObjects( bool draw_to_water_framebuffer )
{
	const unsigned int objects_per_instance= 16;

	level_static_objects_shader_.Bind();

	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D_ARRAY, aircrafts_data_.textures_array );
	level_static_objects_shader_.UniformInt( "tex", 0 );

	level_static_objects_shader_.UniformVec3( "sun", shadowmap_fbo_.sun_vector );
	level_static_objects_shader_.UniformVec3( "sl", shadowmap_fbo_.sun_light_intensity );
	level_static_objects_shader_.UniformVec3( "al", shadowmap_fbo_.ambient_sky_light_intensity );

	glEnable( GL_CULL_FACE );
	if (draw_to_water_framebuffer ) glCullFace( GL_FRONT );
	else glCullFace( GL_BACK );
	level_static_objects_vbo_.Bind();

	// init independent parts of main object matrix
	float rot_z_scale_translate_mat[16];
	rot_z_scale_translate_mat[ 2]= 0.0f;
	rot_z_scale_translate_mat[ 3]= 0.0f;
	rot_z_scale_translate_mat[ 6]= 0.0f;
	rot_z_scale_translate_mat[ 7]= 0.0f;
	rot_z_scale_translate_mat[ 8]= 0.0f;
	rot_z_scale_translate_mat[ 9]= 0.0f;
	rot_z_scale_translate_mat[11]= 0.0f;
	rot_z_scale_translate_mat[15]= 1.0f;

	const mf_StaticLevelObject* objects= level_->GetStaticObjectsRows()[0].objects;
	unsigned int objects_count= level_->GetStaticObjectsRows()[0].objects_count;
	for( unsigned int i= 0; i< objects_count; i+= objects_per_instance )
	{
		float mat[objects_per_instance][16];
		float normal_mat[objects_per_instance][9];
		float textures[objects_per_instance];

		for( unsigned int j= 0; j< objects_per_instance && (i+j) < objects_count; j++ )
		{
			const mf_StaticLevelObject* obj= &objects[i+j];

			// fast calculating of model matrix ( without 2 full matrix multiplications )
			float s_sin_z= mf_Math::sin( obj->z_angle ) * obj->scale;
			float s_cos_z= mf_Math::cos( obj->z_angle ) * obj->scale;
			rot_z_scale_translate_mat[ 0]= s_cos_z;
			rot_z_scale_translate_mat[ 1]= s_sin_z;
			rot_z_scale_translate_mat[ 4]= -s_sin_z;
			rot_z_scale_translate_mat[ 5]= s_cos_z;
			rot_z_scale_translate_mat[10]= obj->scale;
			rot_z_scale_translate_mat[12]= obj->pos[0];
			rot_z_scale_translate_mat[13]= obj->pos[1];
			rot_z_scale_translate_mat[14]= obj->pos[2];
			Mat4Mul( rot_z_scale_translate_mat, view_matrix_, mat[j] );

			float rot_z_mat[16];
			Mat4RotateZ( rot_z_mat, obj->z_angle );
			Mat4ToMat3( rot_z_mat, normal_mat[j] );

			textures[j]= 0.01f;
		}

		level_static_objects_shader_.UniformFloatArray( "texn", objects_per_instance, textures );
		level_static_objects_shader_.UniformMat4Array( "mat", objects_per_instance, (float*)mat );
		level_static_objects_shader_.UniformMat3Array( "nmat", objects_per_instance, (float*)normal_mat );

		glDrawElementsInstanced( GL_TRIANGLES,
			level_static_objects_vbo_.IndexDataSize() / sizeof(unsigned short),
			GL_UNSIGNED_SHORT,
			0, objects_per_instance );
	} // for static objects
	glDisable( GL_CULL_FACE );
}

void mf_Renderer::DrawWater()
{
	water_shader_.Bind();

	water_shader_.UniformMat4( "mat", view_matrix_ );
	water_shader_.UniformFloat( "wl", level_->TerrainWaterLevel() );

	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, water_reflection_fbo_.tex_id );
	water_shader_.UniformInt( "tex", 0 );

	mf_MainLoop* main_loop= mf_MainLoop::Instance();

	float invert_tex_size[3];
	invert_tex_size[0]= 1.0f / float(main_loop->ViewportWidth());
	invert_tex_size[1]= 1.0f / float(main_loop->ViewportHeight());
	water_shader_.UniformVec3( "its", invert_tex_size );

	water_shader_.UniformVec3( "cp", player_->Pos() );

	float terrain_cell_size[3];
	terrain_cell_size[0]= terrain_cell_size[1]= level_->TerrainCellSize();
	water_shader_.UniformVec3( "tcs", terrain_cell_size );

	water_shader_.UniformFloat( "ph", MF_2PI * water_mesh_.waves_frequency * float(clock())/float(CLOCKS_PER_SEC) );

	unsigned int first_quad, quad_count;
	CalculateWaterMeshVisiblyPart( &first_quad, &quad_count );

	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	//glStencilMask(255);
	//glStencilOp( GL_KEEP, GL_KEEP, GL_REPLACE );
	//glStencilFunc( GL_ALWAYS, 255, 255 );

	water_mesh_.vbo.Bind();
	glDrawArrays( GL_TRIANGLES, first_quad * 6 , quad_count * 6 );

	//glStencilMask(0);

	glDisable( GL_BLEND );
}

void mf_Renderer::DrawShadows()
{
	float projection_final_mat[16];
	{
		float max[3], min[3];
		float terrain_shift[3];
		float rot_x_mat[16];
		float rot_z_mat[16];
		float primary_proj_mat[16];
		float corrected_proj_mat[16];

		Mat4RotateZ( rot_z_mat, -shadowmap_fbo_.sun_azimuth );
		Mat4RotateX( rot_x_mat, MF_PI2 - shadowmap_fbo_.sun_elevation );
		Mat4Mul( rot_z_mat, rot_x_mat, primary_proj_mat );

		GetTerrainMeshShift( terrain_shift );
		min[0]= terrain_shift[0] * level_->TerrainCellSize();
		min[1]= terrain_shift[1] * level_->TerrainCellSize();
		max[0]= ( terrain_shift[0] + float(MF_TERRAIN_CHUNK_SIZE_CL*MF_TERRAIN_MESH_SIZE_X_CHUNKS) ) * level_->TerrainCellSize();
		max[1]= ( terrain_shift[1] + float(MF_TERRAIN_CHUNK_SIZE_CL*MF_TERRAIN_MESH_SIZE_Y_CHUNKS) ) * level_->TerrainCellSize();

		min[2]= 0.0f;
		max[2]= level_->TerrainAmplitude();

		const float inf= 1e24f;
		float min_proj[3]= { inf, inf, inf }, max_proj[3]= { -inf, -inf, -inf };
		for( unsigned int i= 0; i< 8; i++ )
		{
			float pos[3];
			pos[0]= (i&1) ? min[0] : max[0];
			pos[1]= ((i>>1)&1) ? min[1] : max[1];
			pos[2]= ((i>>2)&1) ? min[2] : max[2];
			float proj_pos[3];
			Vec3Mat4Mul( pos, primary_proj_mat, proj_pos );
			for( unsigned int j= 0; j< 3; j++ )
			{
				if( proj_pos[j] > max_proj[j] ) max_proj[j]= proj_pos[j];
				if( proj_pos[j] < min_proj[j] ) min_proj[j]= proj_pos[j];
			}
		}// for bounding box vertices

		Mat4Identity( corrected_proj_mat );
		corrected_proj_mat[ 0]= 2.0f / ( max_proj[0] - min_proj[0] );
		corrected_proj_mat[12]= 1.0f - corrected_proj_mat[ 0] * max_proj[0];
		corrected_proj_mat[ 5]= 2.0f / ( max_proj[1] - min_proj[1] );
		corrected_proj_mat[13]= 1.0f - corrected_proj_mat[ 5] * max_proj[1];
		corrected_proj_mat[10]= 2.0f / ( max_proj[2] - min_proj[2] );
		corrected_proj_mat[14]= 1.0f - corrected_proj_mat[10] * max_proj[2];
		corrected_proj_mat[10]*= -1.0f;
		corrected_proj_mat[14]*= -1.0f;
		Mat4Mul( primary_proj_mat, corrected_proj_mat, projection_final_mat );
	} // calculate projection matrix
	{ // draw terrain
		float terrain_mat[16];

		CreateTerrainMatrix( terrain_mat );
		Mat4Mul( terrain_mat, projection_final_mat, terrain_shadow_matrix_ );

		terrain_shadowmap_shader_.Bind();
		terrain_shadowmap_shader_.UniformMat4( "mat", terrain_shadow_matrix_ );

		glActiveTexture( GL_TEXTURE0 );
		glBindTexture( GL_TEXTURE_2D, terrain_heightmap_texture_ );
		terrain_shadowmap_shader_.UniformInt( "hm", 0 );

		float terrain_shift[3];
		GetTerrainMeshShift( terrain_shift );
		terrain_shadowmap_shader_.UniformVec3( "sh", terrain_shift );

		terrain_vbo_.Bind();

		glEnable( GL_CULL_FACE );
		glCullFace( GL_FRONT );
		glDrawArrays(GL_TRIANGLES, 0, terrain_vbo_.VertexCount() );
		glDisable( GL_CULL_FACE );

	} // draw terrain
	{ // draw water
		water_shadowmap_shader_.Bind();
		water_shadowmap_shader_.UniformMat4( "mat", projection_final_mat );
		water_shadowmap_shader_.UniformFloat( "wl", level_->TerrainWaterLevel() );

		water_shadowmap_shader_.UniformVec3( "cp", player_->Pos() );

		float terrain_cell_size[3];
		terrain_cell_size[0]= terrain_cell_size[1]= level_->TerrainCellSize();
		water_shadowmap_shader_.UniformVec3( "tcs", terrain_cell_size );

		water_shadowmap_shader_.UniformFloat( "ph", MF_2PI * water_mesh_.waves_frequency * float(clock())/float(CLOCKS_PER_SEC) );

		unsigned int first_quad, quad_count;
		CalculateWaterMeshVisiblyPart( &first_quad, &quad_count );

		water_mesh_.vbo.Bind();
		glDrawArrays( GL_TRIANGLES, first_quad * 6 , quad_count * 6 );
	} // draw water
	{ // draw aircrafts
		aircraft_shadowmap_shader_.Bind();
		aircrafts_data_.vbo.Bind();

		const mf_Aircraft* aircrafts[1];
		unsigned int aircraft_count= 1;
		aircrafts[0]= player_->GetAircraft();
		for( unsigned int i= 0; i< aircraft_count; i++ )
		{
			const mf_Aircraft* aircraft= aircrafts[i];

			float translate_mat[16];
			float axis_mat[16];
			float common_rotate_mat[16];
			float mat[16];

			Mat4Identity( axis_mat );
			axis_mat[ 0]= aircraft->AxisVec(0)[0];
			axis_mat[ 4]= aircraft->AxisVec(0)[1];
			axis_mat[ 8]= aircraft->AxisVec(0)[2];
			axis_mat[ 1]= aircraft->AxisVec(1)[0];
			axis_mat[ 5]= aircraft->AxisVec(1)[1];
			axis_mat[ 9]= aircraft->AxisVec(1)[2];
			axis_mat[ 2]= aircraft->AxisVec(2)[0];
			axis_mat[ 6]= aircraft->AxisVec(2)[1];
			axis_mat[10]= aircraft->AxisVec(2)[2];
			Mat4Invert( axis_mat, common_rotate_mat );

			Mat4Translate( translate_mat, aircraft->Pos() );

			Mat4Mul( common_rotate_mat, translate_mat, mat );
			Mat4Mul( mat, projection_final_mat );

			aircraft_shadowmap_shader_.UniformMat4( "mat", mat );

			glDrawElements( GL_TRIANGLES,
				aircrafts_data_.models[ aircraft->GetType() ].index_count,
				GL_UNSIGNED_SHORT,
				(void*) ( aircrafts_data_.models[ aircraft->GetType() ].first_index * sizeof(unsigned short) ) );
		}
	} // draw aircrafts
}