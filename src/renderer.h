#pragma once
#include "micro-f.h"

#include "player.h"
#include "level.h"
#include "text.h"

#pragma pack( push, 1 )
struct mf_GPUModelVertex
{
	char pos[3];
	char normal[3];
	unsigned char tc[2];
};
#pragma pack(pop)

class mf_Renderer
{
public:
	mf_Renderer( mf_Player* player, mf_Level* level, mf_Text* text );
	~mf_Renderer();

	void Resize();
	void DrawFrame();

private:
	void CreateWaterReflectionFramebuffer();
	void CreateShadowmapFramebuffer();

	void GenTerrainMesh();
	void GenWaterMesh();
	void PrepareAircraftModels();

	void CreateViewMatrix( float* out_matrix, bool water_reflection );
	void CreateTerrainMatrix( float* out_matrix );
	void GetTerrainMeshShift( float* out_shift );

	void CalculateWaterMeshVisiblyPart( unsigned int* first_quad, unsigned int* quad_count );

	void DrawTerrain(bool draw_to_water_framebuffer= false );
	void DrawAircrafts();
	void DrawWater();
	void DrawShadows();

private:
	mf_Player* player_;
	mf_Level* level_;
	mf_Text* text_;

	float view_matrix_[16];
	float terrain_shadow_matrix_[16];

	mf_GLSLProgram terrain_shader_;
	mf_VertexBuffer terrain_vbo_;
	GLuint terrain_heightmap_texture_;
	GLuint terrain_normal_map_texture_;

	struct
	{
		unsigned int size[2];
		GLuint fbo_id;
		GLuint tex_id;
		GLuint depth_tex_id;
		GLuint stencil_tex_id;
	} water_reflection_fbo_;
	mf_GLSLProgram water_shader_;

	struct
	{
		// sun spherical coordinates, in radiants
		float sun_azimuth;
		float sun_elevation; //0 - in horizont, pi - in zenith
		float sun_vector[3]; // final vector to sun
		float sun_light_intensity[3];
		float ambient_sky_light_intensity[3];

		unsigned int size[2];
		GLuint fbo_id;
		GLuint depth_tex_id;
	}shadowmap_fbo_;
	mf_GLSLProgram terrain_shadowmap_shader_;
	mf_GLSLProgram water_shadowmap_shader_;

	struct QuadRow
	{
		unsigned short first_quad_number;
		unsigned short quad_count;
	};
	struct
	{
		QuadRow* quad_rows;
		unsigned int quad_row_count;
		mf_VertexBuffer vbo;
		float waves_frequency;

	} water_mesh_;

	mf_VertexBuffer aircraft_vbo_;
	mf_GLSLProgram aircraft_shader_;

	GLuint test_texture_;
};