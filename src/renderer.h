#pragma once
#include "micro-f.h"

#include "player.h"
#include "level.h"
#include "particles_manager.h"
#include "text.h"
#include "settings.h"
#include "game_logic.h"

#include "mf_model.h"

class mf_GameLogic;

#define MF_MAX_AIRCRAFT_MODELS 32

#define MF_HDR_HISTOGRAM_BINS 20

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

	void GenClouds();
	void GenTerrainMesh();
	void GenWaterMesh();
	void PrepareAircraftModels();
	void PreparePowerupsModels();
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
	void DrawStars( bool draw_to_water_framebuffer );
	void DrawParticles( bool draw_to_water_framebuffer );
	void DrawSky( bool draw_to_water_framebuffer );
	void DrawAircrafts();
	void DrawPowerups();
	void DrawForcefield();
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

		// buffer for calculating of histogram bins values
		GLuint histogram_fetch_framebuffer_id;
		GLuint histogram_fetch_color_tex_id; // format - rgba8
		unsigned int histogram_fetch_texture_size_log2;
		float histogram_edges[ MF_HDR_HISTOGRAM_BINS ]; // max values of histogram bins

		// framebuffer and texture with main histogram data
		GLuint histogram_framebuffer_id;
		GLuint histogram_tex_id; // format - rgba8
		unsigned int current_histogram_bin;

		GLuint brightness_history_framebuffer_id;
		GLuint brightness_history_tex_id; // format - r16f
		unsigned int brightness_history_tex_size;
		unsigned int current_brightness_history_pixel;

		mf_GLSLProgram histogram_fetch_shader; // downscale main frame to one pixel
		mf_GLSLProgram histogram_write_shader; // write this pixel to histogram data
		mf_GLSLProgram brightness_computing_shader; // compute brightness - write to history
		mf_GLSLProgram tonemapping_shader; // draw fullscreen quad
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

		GLuint textures_array;
	} particles_data_;

	struct
	{
		unsigned int texture_size;
		GLuint clouds_cubemap_tex_id;

		mf_VertexBuffer vbo;
		mf_VertexBuffer clouds_dome_vbo;

		mf_GLSLProgram clouds_gen_shader;
		mf_GLSLProgram clouds_downscale_shader;
		mf_GLSLProgram clouds_shader;

	} sky_clouds_data_;

	struct
	{
		mf_ModelInfoForDrawing models[ mf_Powerup::LastType ];
		mf_VertexBuffer vbo;
		mf_GLSLProgram shader;
	} powerups_data_;

	struct
	{
		GLuint texture;
		mf_VertexBuffer vbo;
		mf_GLSLProgram shader;
	} forcefield_data_;
};