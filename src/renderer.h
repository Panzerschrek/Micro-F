#pragma once
#include "micro-f.h"

#include "player.h"
#include "level.h"
#include "text.h"

#include "mf_model.h"

#define MF_MAX_AIRCRAFT_MODELS 32

struct mf_AircratfModelInfo
{
	unsigned int first_index;
	unsigned int index_count;
};

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
	// eturns shift in terrain cells
	void GetTerrainMeshShift( float* out_shift );

	void CalculateWaterMeshVisiblyPart( unsigned int* first_quad, unsigned int* quad_count );

	void DrawTerrain(bool draw_to_water_framebuffer= false );
	void DrawSun();
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
	unsigned int terrain_mesh_patch_triangle_count_;


	GLuint terrain_heightmap_texture_;
	GLuint terrain_normal_map_texture_;
	GLuint terrain_textures_array_;

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
	} shadowmap_fbo_;
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

	struct
	{
		unsigned int model_count;
		mf_AircratfModelInfo models[ MF_MAX_AIRCRAFT_MODELS ];
		mf_VertexBuffer vbo;
	} aircrafts_data_;
	mf_GLSLProgram aircraft_shader_;

	GLuint sun_texture_;
	mf_GLSLProgram sun_shader_;

	GLuint test_texture_;
};