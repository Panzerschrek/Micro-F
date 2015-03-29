#pragma once
#include "micro-f.h"

#include "player.h"
#include "level.h"
#include "particles_manager.h"
#include "text.h"
#include "settings.h"

#include "mf_model.h"

class mf_GameLogic;

#define MF_MAX_AIRCRAFT_MODELS 32

class mf_Renderer
{
public:
	mf_Renderer( const mf_Player* player, const mf_GameLogic* game_logic, mf_Text* text, const mf_Settings* settings );
	~mf_Renderer();

	void Resize();
	void DrawFrame();

private:
	void CreateWaterReflectionFramebuffer();
	void CreateShadowmapFramebuffer();
	void CreateHDRFramebuffer();
	void CreateBrightnessFetchFramebuffer();

	void GenTerrainMesh();
	void GenWaterMesh();
	void PrepareAircraftModels();
	void PrepareLevelStaticObjectsModels();

	void CreateViewMatrix( float* out_matrix, bool water_reflection );
	void CreateTerrainMatrix( float* out_matrix );
	void CreateAircraftMatrix( const mf_Aircraft* aircraft, float* out_matrix, float* optional_normal_matrix= NULL, float* output_player_relative_mat= NULL );
	// eturns shift in terrain cells
	void GetTerrainMeshShift( float* out_shift );
	float GetSceneRadius(); // returns half of iagonal of scene bounding box
	void PrepareParticles();

	void CalculateWaterMeshVisiblyPart( unsigned int* first_quad, unsigned int* quad_count );
	void CalculateStaticLevelObjectsVisiblyRows( unsigned int* out_first_row, unsigned int* out_last_row );

	void DrawTerrain( bool draw_to_water_framebuffer );
	void DrawSun( bool draw_to_water_framebuffer );
	void DrawStars( bool draw_to_water_framebuffer );
	void DrawParticles( bool draw_to_water_framebuffer );
	void DrawSky(  bool draw_to_water_framebuffer );
	void DrawAircrafts( const mf_Aircraft* const* aircrafts, unsigned int count );
	void DrawPowerups();
	void DrawLevelStaticObjects( bool draw_to_water_framebuffer );
	void DrawWater();
	void DrawShadows();

private:
	struct mf_ModelInfoForDrawing
	{
		unsigned int first_index;
		unsigned int index_count;
	};

	const mf_Player* player_;
	const mf_Level* level_;
	const mf_GameLogic* game_logic_;
	mf_Text* text_;

	mf_Settings settings_;

	float view_matrix_[16];
	float terrain_shadow_matrix_[16];
	float common_shadow_matrix_[16];

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
	} water_reflection_fbo_;
	mf_GLSLProgram water_shader_;

	struct
	{
		GLuint main_framebuffer_id;
		GLuint main_framebuffer_color_tex_id;
		GLuint main_framebuffer_depth_tex_id;

		GLuint brightness_fetch_framebuffer_id;
		GLuint brightness_fetch_color_tex_id;
		unsigned int brightness_fetch_texture_size_log2;

		GLuint brightness_history_framebiffer_id;
		GLuint brightness_history_color_tex_id;
		unsigned int brightness_history_tex_width;
		unsigned int current_brightness_history_pixel;
	
		mf_GLSLProgram brightness_fetch_shader;
		mf_GLSLProgram brightness_history_shader;
		mf_GLSLProgram tonemapping_shader;
	} hdr_data_;

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
		unsigned int first_quad_number;
		unsigned int quad_count;
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
		mf_ModelInfoForDrawing models[ mf_Aircraft::LastType ];
		mf_VertexBuffer vbo;
		GLuint textures_array;
	} aircrafts_data_;
	mf_GLSLProgram aircraft_shader_;
	mf_GLSLProgram aircraft_shadowmap_shader_;
	mf_GLSLProgram aircraft_stencil_shadow_shader_;

	GLuint sun_texture_;
	mf_GLSLProgram sun_shader_;

	mf_GLSLProgram sky_shader_;
	mf_VertexBuffer sky_vbo_;

	mf_GLSLProgram stars_shader_;
	mf_VertexBuffer stars_vbo_;

	struct
	{
		unsigned int matrices_data_offset;
		unsigned int sun_vectors_data_offset;
		GLuint matrices_sun_vectors_ubo;

		mf_ModelInfoForDrawing models[ mf_StaticLevelObject::LastType ];
		GLuint textures_array;
	} level_static_objects_data_;

	mf_VertexBuffer level_static_objects_vbo_;
	mf_GLSLProgram level_static_objects_shader_;
	mf_GLSLProgram level_static_objects_shadowmap_shader_;

	struct 
	{
		mf_ParticleVertex vertices[ MF_MAX_PARTICLES ];
		mf_ParticleVertex sorted_vertices[ MF_MAX_PARTICLES ];
		mf_VertexBuffer particles_vbo;
		mf_GLSLProgram particles_shader;
	} particles_data_;
};