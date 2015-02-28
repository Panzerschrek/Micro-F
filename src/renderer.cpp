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
#define MF_TERRAIN_MESH_SIZE_CHUNKS 72

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
		/*vert*/ "mat", "nmat", /*frag*/ "tex", "sun", "sl", "al" };
	aircraft_shader_.FindUniforms( aircraft_shader_uniforms, sizeof(aircraft_shader_uniforms) / sizeof(char*) );

	// sun shader
	sun_shader_.Create( mf_Shaders::sun_shader_v, mf_Shaders::sun_shader_f );
	static const char* const sun_shader_uniforms[]= { "mat", "s", "tex", "i" };
	sun_shader_.FindUniforms( sun_shader_uniforms, sizeof(sun_shader_uniforms) / sizeof(char*) );

	// sky shader
	sky_shader_.SetAttribLocation( "p", 0 );
	sky_shader_.Create( mf_Shaders::sky_shader_v, mf_Shaders::sky_shader_f );
	static const char* const sky_shader_unifroms[]= { "mat" };
	sky_shader_.FindUniforms( sky_shader_unifroms, sizeof(sky_shader_unifroms) / sizeof(char*) );

	GenTerrainMesh();
	GenWaterMesh();
	PrepareAircraftModels();

	{ // sky mesh
		mf_DrawingModel model;
		GenGeosphere( &model, 24, 32 );
		sky_vbo_.VertexData( model.GetVertexData(), model.VertexCount() * sizeof(mf_DrawingModelVertex), sizeof(mf_DrawingModelVertex) );
		sky_vbo_.IndexData( model.GetIndexData(), model.IndexCount() * sizeof(unsigned short) );

		mf_DrawingModelVertex vert;
		unsigned int shift;
		shift= (char*)vert.pos - (char*)&vert;
		sky_vbo_.VertexAttrib( 0, 3, GL_FLOAT, false, shift );
	}

	// terrain heightmap
	glGenTextures( 1, &terrain_heightmap_texture_ );
	glBindTexture( GL_TEXTURE_2D, terrain_heightmap_texture_ );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_R16,
		level_->TerrainSizeX(), level_->TerrainSizeY(), 0,
		GL_RED, GL_UNSIGNED_SHORT, level_->GetTerrianHeightmapData() );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );

	// terrain normalmap
	glGenTextures( 1, &terrain_normal_map_texture_ );
	glBindTexture( GL_TEXTURE_2D, terrain_normal_map_texture_ );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8_SNORM,
		level->TerrainSizeX(), level->TerrainSizeY(), 0,
		GL_RGBA, GL_BYTE, level->GetTerrainNormalTextureMap() );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );

	{ // test texture
		mf_Texture tex( 9, 9 );
		GenF1949Texture( &tex );
		tex.LinearNormalization(1.0f);

		glGenTextures( 1, &test_texture_ );
		glBindTexture( GL_TEXTURE_2D, test_texture_ );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8,
			1 << tex.SizeXLog2(), 1 << tex.SizeYLog2(), 0,
			GL_RGBA, GL_UNSIGNED_BYTE, tex.GetNormalizedData() );
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	{ // sun texture
		mf_Texture sun_tex( 6, 6 );
		GenSunTexture( &sun_tex );
		sun_tex.LinearNormalization(1.0f);

		glGenTextures( 1, &sun_texture_ );
		glBindTexture( GL_TEXTURE_2D, sun_texture_ );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8,
			1 << sun_tex.SizeXLog2(), 1 << sun_tex.SizeYLog2(), 0,
			GL_RGBA, GL_UNSIGNED_BYTE, sun_tex.GetNormalizedData() );
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
		glGenerateMipmap(GL_TEXTURE_2D);
	}

	{
		const unsigned int terrain_texture_size_log2= 9;
		const unsigned int terrain_texture_size= 1 << terrain_texture_size_log2;
		glGenTextures( 1, &terrain_textures_array_ );
		glBindTexture( GL_TEXTURE_2D_ARRAY, terrain_textures_array_ );
		glTexImage3D( GL_TEXTURE_2D_ARRAY, 0, GL_RGBA8,
			terrain_texture_size, terrain_texture_size, LastTexture,
			0, GL_RGBA, GL_UNSIGNED_BYTE, NULL );
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
		mf_Texture tex( terrain_texture_size_log2, terrain_texture_size_log2 );
		for( unsigned int i= 0; i< LastTexture; i++ )
		{
			terrain_texture_gen_func[i]( &tex );
			tex.LinearNormalization( 1.0f );
			glTexSubImage3D( GL_TEXTURE_2D_ARRAY, 0,
				0, 0, i,
				terrain_texture_size, terrain_texture_size, 1,
				GL_RGBA, GL_UNSIGNED_BYTE, tex.GetNormalizedData() );
		} // for textures
		glGenerateMipmap( GL_TEXTURE_2D_ARRAY );
	}

	CreateWaterReflectionFramebuffer();
	CreateShadowmapFramebuffer();
}

mf_Renderer::~mf_Renderer()
{
}

void mf_Renderer::Resize()
{
	glDeleteFramebuffers( 1, &water_reflection_fbo_.fbo_id );
	glDeleteTextures( 1, &water_reflection_fbo_.tex_id );
	glDeleteTextures( 1, &water_reflection_fbo_.depth_tex_id );
	glDeleteTextures( 1, &water_reflection_fbo_.stencil_tex_id );

	CreateWaterReflectionFramebuffer();
}

void mf_Renderer::DrawFrame()
{
	//glClearColor( 0.6f, 0.7f, 1.0f, 0.0f );

	glBindFramebuffer( GL_FRAMEBUFFER, shadowmap_fbo_.fbo_id );
	glViewport( 0, 0, shadowmap_fbo_.size[0], shadowmap_fbo_.size[1] );
	glClear( GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
	DrawShadows();

	glBindFramebuffer( GL_FRAMEBUFFER, water_reflection_fbo_.fbo_id );
	glViewport( 0, 0, water_reflection_fbo_.size[0], water_reflection_fbo_.size[1] );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );
	CreateViewMatrix( view_matrix_, true );
	DrawTerrain( true );
	DrawSky( true );
	DrawSun( true );

	mf_MainLoop* main_loop= mf_MainLoop::Instance();

	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
	glViewport( 0, 0, main_loop->ViewportWidth(), main_loop->ViewportHeight() );
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT );

	CreateViewMatrix( view_matrix_, false );
	DrawTerrain( false );
	DrawAircrafts();
	DrawSky( false );
	DrawSun( false );
	DrawWater();

	{
		unsigned int first_water_quad, water_quad_count;
		CalculateWaterMeshVisiblyPart( &first_water_quad, &water_quad_count );
		char str[64];
		sprintf( str, "water quads: %d\n", water_quad_count );
		text_->AddText( 0, 1, 1, mf_Text::default_color, str );
	}
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
	glTexImage2D( GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24,
		water_reflection_fbo_.size[0], water_reflection_fbo_.size[1],
		0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );

	// stencil buffer
	/*glGenTextures( 1, &water_reflection_fbo_.stencil_tex_id );
	glBindTexture( GL_TEXTURE_2D, water_reflection_fbo_.stencil_tex_id );
	glTexImage2D( GL_TEXTURE_2D, 0, GL_STENCIL_INDEX8,
		water_reflection_fbo_.size[0], water_reflection_fbo_.size[1],
		0, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, NULL );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );*/

	glGenFramebuffers( 1, &water_reflection_fbo_.fbo_id );
	glBindFramebuffer( GL_FRAMEBUFFER, water_reflection_fbo_.fbo_id );
	glFramebufferTexture( GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, water_reflection_fbo_.tex_id, 0 );
	glFramebufferTexture( GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, water_reflection_fbo_.depth_tex_id, 0 );
	//glFramebufferTexture( GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, water_reflection_fbo_.stencil_tex_id, 0 );
	GLuint color_attachment= GL_COLOR_ATTACHMENT0;
	glDrawBuffers( 1, &color_attachment );

	glBindFramebuffer( GL_FRAMEBUFFER, 0 );
}

void mf_Renderer::CreateShadowmapFramebuffer()
{
	shadowmap_fbo_.sun_azimuth= -MF_PI3;
	shadowmap_fbo_.sun_elevation= MF_PI6;

	shadowmap_fbo_.sun_vector[0]= shadowmap_fbo_.sun_vector[1]= mf_Math::cos( shadowmap_fbo_.sun_elevation );
	shadowmap_fbo_.sun_vector[2]= mf_Math::sin( shadowmap_fbo_.sun_elevation );
	shadowmap_fbo_.sun_vector[0]*= -mf_Math::sin( shadowmap_fbo_.sun_azimuth );
	shadowmap_fbo_.sun_vector[1]*= mf_Math::cos( shadowmap_fbo_.sun_azimuth );

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
	unsigned char chunk_lod_table[ MF_TERRAIN_MESH_SIZE_CHUNKS * MF_TERRAIN_MESH_SIZE_CHUNKS ];
	static const int lod_dst[]= { 64*64, 160*160, 352*352, 448*448, 1024*1024, 0x7FFFFFFF };

	// calculate chunks lods
	unsigned int center_xy= MF_TERRAIN_CHUNK_SIZE_CL * MF_TERRAIN_MESH_SIZE_CHUNKS / 2;
	for( unsigned int y= 0; y< MF_TERRAIN_MESH_SIZE_CHUNKS; y++ )
	{
		int cl_y= y * MF_TERRAIN_CHUNK_SIZE_CL + MF_TERRAIN_CHUNK_SIZE_CL / 2;
		int y_dst2= cl_y - center_xy;
		y_dst2*= y_dst2;
		for( unsigned int x= 0; x< MF_TERRAIN_MESH_SIZE_CHUNKS; x++ )
		{
			int cl_x= x * MF_TERRAIN_CHUNK_SIZE_CL + MF_TERRAIN_CHUNK_SIZE_CL / 2;
			int x_dst= cl_x - center_xy;
			int dst2= y_dst2 + x_dst * x_dst;
			for( unsigned int i= 0, i_end= MF_TERRAIN_CHUNK_SIZE_CL_LOG2+1; i< i_end; i++ )
				if( dst2 < lod_dst[i] || i == i_end - 1 )
				{
					chunk_lod_table[ x + y * MF_TERRAIN_MESH_SIZE_CHUNKS ]= (unsigned char)i;
					traingle_count+= 2 * ( ( MF_TERRAIN_CHUNK_SIZE_CL * MF_TERRAIN_CHUNK_SIZE_CL ) >> (i+i) );
					break;
				}
		} // for x
	} // for y

	// calculate patch count
	unsigned int patch_triangle_count= 0;
	for( unsigned int y= 0; y< MF_TERRAIN_MESH_SIZE_CHUNKS-1; y++ )
		for( unsigned int x= 0; x< MF_TERRAIN_MESH_SIZE_CHUNKS-1; x++ )
		{
			unsigned int lod  = chunk_lod_table[ x   +  y    * MF_TERRAIN_MESH_SIZE_CHUNKS ];
			unsigned int lod_x= chunk_lod_table[ x+1 +  y    * MF_TERRAIN_MESH_SIZE_CHUNKS ];
			unsigned int lod_y= chunk_lod_table[ x   + (y+1) * MF_TERRAIN_MESH_SIZE_CHUNKS ];
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
	for( unsigned int y= 0; y< MF_TERRAIN_MESH_SIZE_CHUNKS; y++ )
		for( unsigned int x= 0; x< MF_TERRAIN_MESH_SIZE_CHUNKS; x++ )
		{
			unsigned int lod= chunk_lod_table[ x + y * MF_TERRAIN_MESH_SIZE_CHUNKS ];
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
	for( unsigned int y= 0; y< MF_TERRAIN_MESH_SIZE_CHUNKS-1; y++ )
		for( unsigned int x= 0; x< MF_TERRAIN_MESH_SIZE_CHUNKS-1; x++ )
		{
			unsigned int lod  = chunk_lod_table[ x   +  y    * MF_TERRAIN_MESH_SIZE_CHUNKS ];
			unsigned int lod_x= chunk_lod_table[ x+1 +  y    * MF_TERRAIN_MESH_SIZE_CHUNKS ];
			unsigned int lod_y= chunk_lod_table[ x   + (y+1) * MF_TERRAIN_MESH_SIZE_CHUNKS ];
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
		water_mesh_.quad_rows[y].first_quad_number= (unsigned short)( (q - quads) / 12 );
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
	mf_DrawingModel model;
	model.LoadFromMFMD( mf_Models::f1949 );

	aircrafts_data_.vbo.VertexData(
		model.GetVertexData(),
		model.VertexCount() * sizeof(mf_DrawingModelVertex),
		sizeof(mf_DrawingModelVertex) );

	mf_DrawingModelVertex vert;
	unsigned int shift;
	shift= (char*)vert.pos - (char*)&vert;
	aircrafts_data_.vbo.VertexAttrib( 0, 3, GL_FLOAT, false, shift );
	shift= (char*)vert.normal - (char*)&vert;
	aircrafts_data_.vbo.VertexAttrib( 1, 3, GL_FLOAT, false, shift );
	shift= (char*)vert.tex_coord - (char*)&vert;
	aircrafts_data_.vbo.VertexAttrib( 2, 2, GL_FLOAT, false, shift );

	aircrafts_data_.vbo.IndexData( model.GetIndexData(), sizeof(unsigned short) * model.IndexCount() );

	aircrafts_data_.models[0].first_index= 0;
	aircrafts_data_.models[0].index_count= model.IndexCount();
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
		70.0f * MF_DEG2RAD, 0.5f, 2048.0f );

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

void mf_Renderer::GetTerrainMeshShift( float* out_shift )
{
	const float cell_step= float(MF_TERRAIN_CHUNK_SIZE_CL);
	for( unsigned int i= 0; i< 2; i++ )
	{
		out_shift[i]= floorf( player_->Pos()[i] / ( level_->TerrainCellSize() * cell_step ) ) * cell_step
			- float(MF_TERRAIN_MESH_SIZE_CHUNKS * MF_TERRAIN_CHUNK_SIZE_CL / 2);
		if( out_shift[i] < 0.0f ) out_shift[i]= 0.0f;
		out_shift[i]+= 0.0005f; // compenstation of GPU accuracy problems
	}
}

void mf_Renderer::CalculateWaterMeshVisiblyPart( unsigned int* first_quad, unsigned int* quad_count )
{
	const unsigned int water_raw_distance= ( MF_TERRAIN_MESH_SIZE_CHUNKS * MF_TERRAIN_CHUNK_SIZE_CL / 2 ) / MF_WATER_QUAD_SIZE_CL + 2;
	int cam_y= int( player_->Pos()[1] / level_->TerrainCellSize() ) / MF_WATER_QUAD_SIZE_CL;
	int y_min= cam_y - water_raw_distance;
	int y_max= cam_y + water_raw_distance;
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
	Vec3Mul( shadowmap_fbo_.sun_vector,
		float(MF_TERRAIN_CHUNK_SIZE_CL * MF_TERRAIN_MESH_SIZE_CHUNKS) * level_->TerrainCellSize(),
		translate_vec );
	Vec3Add( translate_vec, player_->Pos() );
	Mat4Translate( translate_mat, translate_vec );
	Mat4Mul( translate_mat, view_matrix_, mat );

	sun_shader_.UniformMat4( "mat", mat );

	float sprite_size;
	if( draw_to_water_framebuffer )
		sprite_size= float(water_reflection_fbo_.size[1]) * 128.0f/1024.0f;
	else
		sprite_size= float(mf_MainLoop::Instance()->ViewportHeight()) * 128.0f/1024.0f;
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

void mf_Renderer::DrawSky(  bool draw_to_water_framebuffer )
{
	sky_shader_.Bind();

	float mat[16];
	float translate_mat[16];
	float scale_mat[16];
	Mat4Scale( scale_mat, 1.5f * float(MF_TERRAIN_CHUNK_SIZE_CL * MF_TERRAIN_MESH_SIZE_CHUNKS ) * level_->TerrainCellSize() );

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

	sky_vbo_.Bind();

	// draw only second part of vbo ( becouse sky is sphere, draw only height triangles )
	glDrawElements(
		GL_TRIANGLES,
		sky_vbo_.IndexDataSize() / ( sizeof(unsigned short) * 3 / 4 ),
		GL_UNSIGNED_SHORT,
		(void*)( sky_vbo_.IndexDataSize() * 1 / 4 ) );
}

void mf_Renderer::DrawAircrafts()
{
	float translate_mat[16];
	float normal_mat[16];
	float rot_z_mat[16];
	float mat[16];

	float translate_vec[3];
	translate_vec[0]= translate_vec[1]= mf_Math::cos( player_->Angle()[0] );
	translate_vec[2]= mf_Math::sin( player_->Angle()[0] );
	translate_vec[0]*= -mf_Math::sin( player_->Angle()[2] );
	translate_vec[1]*= mf_Math::cos( player_->Angle()[2] );
	Vec3Mul( translate_vec, 14.0f );
	Vec3Add( translate_vec, player_->Pos() );
	Mat4Translate( translate_mat, translate_vec );

	Mat4RotateZ( rot_z_mat, player_->Angle()[2] );

	Mat4Mul( rot_z_mat, translate_mat, mat );
	Mat4Mul( mat, view_matrix_ );

	Mat4ToMat3( rot_z_mat, normal_mat );

	aircraft_shader_.Bind();
	aircraft_shader_.UniformMat4( "mat", mat );
	aircraft_shader_.UniformMat3( "nmat", normal_mat );

	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, test_texture_ );
	aircraft_shader_.UniformInt( "tex", 0 );

	aircraft_shader_.UniformVec3( "sun", shadowmap_fbo_.sun_vector );
	aircraft_shader_.UniformVec3( "sl", shadowmap_fbo_.sun_light_intensity );
	aircraft_shader_.UniformVec3( "al", shadowmap_fbo_.ambient_sky_light_intensity );


	glEnable( GL_CULL_FACE );
	glCullFace( GL_BACK );
	aircrafts_data_.vbo.Bind();
	glDrawElements( GL_TRIANGLES,
		aircrafts_data_.vbo.IndexDataSize() / sizeof(unsigned short),
		GL_UNSIGNED_SHORT, 0 );

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
		for( unsigned int i= 0; i< 2; i++ )
		{
			min[i]= terrain_shift[i] * level_->TerrainCellSize();
			max[i]= ( terrain_shift[i] + float(MF_TERRAIN_CHUNK_SIZE_CL*MF_TERRAIN_MESH_SIZE_CHUNKS) ) * level_->TerrainCellSize();
		}
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

}