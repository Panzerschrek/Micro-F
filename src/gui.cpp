#include "micro-f.h"
#include "gui.h"

#include "text.h"
#include "aircraft.h"
#include "player.h"
#include "main_loop.h"

#include "texture.h"
#include "shaders.h"
#include "mf_math.h"
#include "drawing_model.h"
#include "models_generation.h"
#include "textures_generation.h"

#define MF_GUI_ICON_SIZE 32

static const unsigned char icons_data[mf_Gui::LastIcon][ MF_GUI_ICON_SIZE * MF_GUI_ICON_SIZE / 8 ]=
{
#include "../textures/aircraft_direction.h"
,
#include "../textures/front_speed.h"
,
#include "../textures/back_speed.h"
};

#define MF_MAX_COMMON_VERTICES_BYTES 4096
#define MF_MAX_COMMON_INDECES_BYTES 2048

struct mf_GuiVertex
{
	float pos[2];
	float tex_coord[3];
};

static void Set2DTextureTrilinearFiltration()
{
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
	glGenerateMipmap( GL_TEXTURE_2D );
}

mf_Gui::mf_Gui( mf_Text* text, const mf_Player* player )
	: text_(text)
	, player_(player)
{

	naviball_shader_.SetAttribLocation( "p", 0 );
	naviball_shader_.SetAttribLocation( "tc", 2 );
	naviball_shader_.Create( mf_Shaders::naviball_shader_v, mf_Shaders::naviball_shader_f );
	static const char* const naviball_shader_uniforms[]= { /*vert*/ "mat",/*frag*/ "tex" };
	naviball_shader_.FindUniforms( naviball_shader_uniforms, sizeof(naviball_shader_uniforms) / sizeof(char*) );

	naviball_icons_shader_.Create( mf_Shaders::naviball_icon_shader_v, mf_Shaders::naviball_icon_shader_f );
	static const char* const naviball_icons_shader_uniforms[]= { "p", "ps", "tex", "tn", "c" };
	naviball_icons_shader_.FindUniforms( naviball_icons_shader_uniforms, sizeof(naviball_icons_shader_uniforms) / sizeof(char*) );

	gui_shader_.SetAttribLocation( "p", 0 );
	gui_shader_.SetAttribLocation( "tc", 1 );
	gui_shader_.Create( mf_Shaders::gui_shader_v, mf_Shaders::gui_shader_f );
	gui_shader_.FindUniform( "tex" );

	{
		mf_DrawingModel model;
		GenGeosphere( &model, 24, 16 );

		naviball_vbo_.VertexData( model.GetVertexData(), model.VertexCount() * sizeof(mf_DrawingModelVertex), sizeof(mf_DrawingModelVertex) );
		naviball_vbo_.IndexData( model.GetIndexData(), model.IndexCount() * sizeof(unsigned short) );

		mf_DrawingModelVertex vert;
		unsigned int shift;
		shift= (char*)vert.pos - (char*)&vert;
		naviball_vbo_.VertexAttrib( 0, 3, GL_FLOAT, false, shift );
		shift= (char*)vert.tex_coord - (char*)&vert;
		naviball_vbo_.VertexAttrib( 2, 2, GL_FLOAT, false, shift );
	}
	{ // vbo for gui vertices
		common_vbo_.VertexData( NULL, MF_MAX_COMMON_VERTICES_BYTES, sizeof(mf_GuiVertex) );
		common_vbo_.IndexData( NULL, MF_MAX_COMMON_INDECES_BYTES );

		mf_GuiVertex vert;
		unsigned int shift= ((char*)vert.pos) - ((char*)&vert);
		common_vbo_.VertexAttrib( 0, 2, GL_FLOAT, false, shift );
		shift= ((char*)vert.tex_coord) - ((char*)&vert);
		common_vbo_.VertexAttrib( 1, 3, GL_FLOAT, false, shift );
	}

	{ // naviball texture
		mf_Texture tex( 9, 9 );
		GenNaviballTexture( &tex );
		tex.LinearNormalization( 1.0f );

		glGenTextures( 1, &naviball_texture_ );
		glBindTexture( GL_TEXTURE_2D, naviball_texture_ );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8,
			tex.SizeX(), tex.SizeY(), 0, GL_RGBA, GL_UNSIGNED_BYTE, tex.GetNormalizedData() );
		Set2DTextureTrilinearFiltration();
	}
	{ // naviball icons
		unsigned char decompressed_data[ MF_GUI_ICON_SIZE * MF_GUI_ICON_SIZE * LastIcon ];
		mfMonochromeImageTo8Bit( (unsigned char*)icons_data, decompressed_data, sizeof(decompressed_data) );

		glGenTextures( 1, &naviball_icons_texture_array_ );
		glBindTexture( GL_TEXTURE_2D_ARRAY, naviball_icons_texture_array_ );

		glTexImage3D( GL_TEXTURE_2D_ARRAY, 0, GL_R8,
			MF_GUI_ICON_SIZE, MF_GUI_ICON_SIZE, LastIcon,
			0, GL_RED, GL_UNSIGNED_BYTE, decompressed_data );
		glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D_ARRAY, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
		glGenerateMipmap( GL_TEXTURE_2D_ARRAY );
	}
	{
		// control panel
		mf_Texture tex( 8, 7 );
		GenControlPanelTexture( &tex );
		tex.LinearNormalization( 1.0f );

		glGenTextures( 1, &control_panel_background_texture_ );
		glBindTexture( GL_TEXTURE_2D, control_panel_background_texture_ );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8,
			tex.SizeX(), tex.SizeY(), 0, GL_RGBA, GL_UNSIGNED_BYTE, tex.GetNormalizedData() );
		Set2DTextureTrilinearFiltration();
	}
	{
		// throttle bar
		mf_Texture tex( 5, 8 );
		GenThrottleBarTexture( &tex );
		tex.LinearNormalization( 1.0f );

		glGenTextures( 1, &throttle_bar_texture_ );
		glBindTexture( GL_TEXTURE_2D, throttle_bar_texture_ );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8,
			tex.SizeX(), tex.SizeY(), 0, GL_RGBA, GL_UNSIGNED_BYTE, tex.GetNormalizedData() );
		Set2DTextureTrilinearFiltration();
	}

	{
		// throttle indicator
		mf_Texture tex( 4, 4 );
		GenThrottleIndicatorTexture( &tex );
		tex.LinearNormalization( 1.0f );

		glGenTextures( 1, &throttle_indicator_texture_ );
		glBindTexture( GL_TEXTURE_2D, throttle_indicator_texture_ );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8,
			tex.SizeX(), tex.SizeY(), 0, GL_RGBA, GL_UNSIGNED_BYTE, tex.GetNormalizedData() );
		Set2DTextureTrilinearFiltration();
	}
}

mf_Gui::~mf_Gui()
{
}

void mf_Gui::Draw()
{
	const mf_Aircraft* aircraft= player_->GetAircraft();

	const float* vel= aircraft->Velocity();
	float horizontal_speed= mf_Math::sqrt( vel[0] * vel[0] + vel[1] * vel[1] );

	float lon, lat;
	VecToSphericalCoordinates( aircraft->AxisVec(1), &lon, &lat );
	char str[128];
	sprintf( str,
		"horizontal speed: %3.2f\nvertical speed: %3.2f\nazimuth: %3.1f\nelevation: %3.1f",
		horizontal_speed, vel[2], lon * MF_RAD2DEG, lat * MF_RAD2DEG );
	text_->AddText( 1, 3, 1, mf_Text::default_color, str );

	DrawControlPanel();
	DrawNaviball();
}

void mf_Gui::DrawControlPanel()
{
	mf_MainLoop* main_loop= mf_MainLoop::Instance();
	const mf_Aircraft* aircraft= player_->GetAircraft();

	gui_shader_.Bind();

	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, control_panel_background_texture_ );
	glActiveTexture( GL_TEXTURE1 );
	glBindTexture( GL_TEXTURE_2D, throttle_bar_texture_ );
	glActiveTexture( GL_TEXTURE2 );
	glBindTexture( GL_TEXTURE_2D, throttle_indicator_texture_ );

	int textures_id[6];
	for( unsigned int i= 0; i< 6; i++ )
		textures_id[i]= i;
	gui_shader_.UniformIntArray( "tex", 6, textures_id );

	mf_GuiVertex vertices[256];
	mf_GuiVertex* v= vertices;

	float k= float(main_loop->ViewportHeight()) / float(main_loop->ViewportWidth());
	v[0].pos[0]= -0.4f * k;
	v[0].pos[1]= -1.0f;
	v[1].pos[0]=  0.5f * k;
	v[1].pos[1]= -1.0f;
	v[2].pos[0]=  0.5f * k;
	v[2].pos[1]= -0.5f;
	v[3].pos[0]= -0.4f * k;
	v[3].pos[1]= -0.5f;
	v[0].tex_coord[0]= 0.0f;
	v[0].tex_coord[1]= 0.0f;
	v[1].tex_coord[0]= 1.0f;
	v[1].tex_coord[1]= 0.0f;
	v[2].tex_coord[0]= 1.0f;
	v[2].tex_coord[1]= 1.0f;
	v[3].tex_coord[0]= 0.0f;
	v[3].tex_coord[1]= 1.0f;
	v[0].tex_coord[2]= v[1].tex_coord[2]=
	v[2].tex_coord[2]= v[3].tex_coord[2]= 0.01f;
	v[4]= v[0];
	v[5]= v[2];
	v+= 6;

	const float throttle_bar_top= -0.52f;
	const float throttle_bar_bottom= -0.98f;
	v[0].pos[0]= -0.35f * k;
	v[0].pos[1]= throttle_bar_bottom;
	v[1].pos[0]= -0.3f * k;
	v[1].pos[1]= throttle_bar_bottom;
	v[2].pos[0]= -0.3f * k;
	v[2].pos[1]= throttle_bar_top;
	v[3].pos[0]= -0.35f * k;
	v[3].pos[1]= throttle_bar_top;
	v[0].tex_coord[0]= 0.0f;
	v[0].tex_coord[1]= 0.0f;
	v[1].tex_coord[0]= 1.0f;
	v[1].tex_coord[1]= 0.0f;
	v[2].tex_coord[0]= 1.0f;
	v[2].tex_coord[1]= 1.0f;
	v[3].tex_coord[0]= 0.0f;
	v[3].tex_coord[1]= 1.0f;
	v[0].tex_coord[2]= v[1].tex_coord[2]=
	v[2].tex_coord[2]= v[3].tex_coord[2]= 1.01f;
	v[4]= v[0];
	v[5]= v[2];
	v+= 6;

	float throttle= aircraft->Throttle();
	if( throttle < 0.02f ) throttle= 0.02f;
	else if( throttle > 0.97f ) throttle= 0.98f;
	float throttle_indicator_pos= throttle_bar_top * throttle + throttle_bar_bottom * ( 1.0f - throttle );
	v[0].pos[0]= -0.305f * k;
	v[0].pos[1]= throttle_indicator_pos;
	v[1].pos[0]= -0.35f * k;
	v[1].pos[1]= throttle_indicator_pos + 0.005f;
	v[2].pos[0]= -0.35f * k;
	v[2].pos[1]= throttle_indicator_pos - 0.005f;
	v[0].tex_coord[0]= v[0].tex_coord[1]=
	v[1].tex_coord[0]= v[1].tex_coord[1]=
	v[2].tex_coord[0]= v[2].tex_coord[1]= 0.0f;
	v[0].tex_coord[2]= v[1].tex_coord[2]= v[2].tex_coord[2]= 2.01f;
	v+= 3;
	
	common_vbo_.Bind();
	common_vbo_.VertexSubData( vertices, (v - vertices) * sizeof(mf_GuiVertex), 0 );

	glDisable( GL_DEPTH_TEST );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	glDrawArrays( GL_TRIANGLES, 0, v - vertices );

	glDisable( GL_BLEND );
	glEnable( GL_DEPTH_TEST );
}

void mf_Gui::DrawNaviball()
{
	static const float naviball_pos[3]= { 0.0f, -0.75f, 0.0f };
	mf_MainLoop* main_loop= mf_MainLoop::Instance();
	const mf_Aircraft* aircraft= player_->GetAircraft();

	float rot_mat_for_space_vectors[16];
	{
		naviball_shader_.Bind();

		glActiveTexture( GL_TEXTURE0 );
		glBindTexture( GL_TEXTURE_2D, naviball_texture_ );
		naviball_shader_.UniformInt( "tex", 0 );

		float tmp_mat[16];
		float mat[16];
		float translate_mat[16];
		float rotate_mat[16];
		float basis_change_mat[16];
		float scale_mat[16];
		float scale_vec[3];

		Mat4Translate( translate_mat, naviball_pos );

		const float c_naviball_scale= 0.22f;
		scale_vec[0]= c_naviball_scale  * float(main_loop->ViewportHeight()) / float(main_loop->ViewportWidth());
		scale_vec[1]= c_naviball_scale;
		scale_vec[2]= c_naviball_scale;
		Mat4Scale( scale_mat, scale_vec );

		{
			float axis_mat[16];
			Mat4Identity( axis_mat );
			axis_mat[ 0]= -aircraft->AxisVec(0)[0];
			axis_mat[ 4]= -aircraft->AxisVec(0)[1];
			axis_mat[ 8]= -aircraft->AxisVec(0)[2];
			axis_mat[ 1]= -aircraft->AxisVec(1)[0];
			axis_mat[ 5]= -aircraft->AxisVec(1)[1];
			axis_mat[ 9]= -aircraft->AxisVec(1)[2];
			axis_mat[ 2]= aircraft->AxisVec(2)[0];
			axis_mat[ 6]= aircraft->AxisVec(2)[1];
			axis_mat[10]= aircraft->AxisVec(2)[2];
			Mat4Transpose( axis_mat );
			Mat4Invert( axis_mat, rotate_mat );
			/*float lon, lat;
			VecToSphericalCoordinates( aircraft->AxisVec(1), &lon, &lat );
			float rot_z[16];
			float rot_x[16];
			Mat4RotateX( rot_x, lat );
			Mat4RotateZ( rot_z, lon );
			Mat4Mul( rot_z, rot_x, rotate_mat );*/
		}
		{
			Mat4RotateX( basis_change_mat, -MF_PI2 );
			float tmp_mat_bc[16];
			Mat4Identity( tmp_mat_bc );
			tmp_mat_bc[10]= -1.0f;
			Mat4Mul( basis_change_mat, tmp_mat_bc );
		}

		Mat4Mul( rotate_mat, basis_change_mat, mat );
		Mat4Mul( mat, scale_mat, tmp_mat );
		Mat4Mul( tmp_mat, translate_mat, mat );
		memcpy( rot_mat_for_space_vectors, mat, sizeof(mat) );

		naviball_shader_.UniformMat4( "mat", mat );

		glDisable( GL_DEPTH_TEST );
		glEnable( GL_CULL_FACE );

		naviball_vbo_.Bind();
		glDrawElements( GL_TRIANGLES, naviball_vbo_.IndexDataSize() / sizeof(unsigned short), GL_UNSIGNED_SHORT, 0 );

		glDisable( GL_CULL_FACE );
		glEnable( GL_DEPTH_TEST );
	}
	{// naviball icons

		static const float direction_color[3]= { 0.4f, 0.3f, 0.05f };
		static const float speed_colord[3]= { 0.3f, 1.0f, 0.3f };
		const unsigned int max_icons= 16;

		NaviBallIcons icons_type[ max_icons ];
		float icons_pos[ max_icons ][3];
		const float* icons_color[ max_icons ];

		unsigned int icon_count= 3;

		icons_pos[0][0]= 0.0f;
		icons_pos[0][1]= naviball_pos[1];
		icons_pos[0][2]= -1.0f;
		icons_type[0]= IconDirection;
		icons_color[0]= direction_color;

		float speed_vec[3];
		VEC3_CPY( speed_vec, aircraft->Velocity() );
		Vec3Normalize( speed_vec );
		Vec3Mat4Mul( speed_vec, rot_mat_for_space_vectors );

		icons_pos[1][0]= speed_vec[0];
		icons_pos[1][1]= speed_vec[1];
		icons_pos[1][2]= speed_vec[2];
		icons_type[1]= IconSpeed;
		icons_color[1]= speed_colord;

		VEC3_CPY( speed_vec, aircraft->Velocity() );
		Vec3Normalize( speed_vec );
		Vec3Mul( speed_vec, -1.0f );
		Vec3Mat4Mul( speed_vec, rot_mat_for_space_vectors );

		icons_pos[2][0]= speed_vec[0];
		icons_pos[2][1]= speed_vec[1];
		icons_pos[2][2]= speed_vec[2];
		icons_type[2]= IconAntiSpeed;
		icons_color[2]= speed_colord;

		naviball_icons_shader_.Bind();

		glActiveTexture( GL_TEXTURE0 );
		glBindTexture( GL_TEXTURE_2D_ARRAY, naviball_icons_texture_array_ );
		naviball_icons_shader_.UniformInt( "tex", 0 );

		naviball_icons_shader_.UniformFloat( "ps", float(main_loop->ViewportHeight()) / 16.0f );

		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		glDisable( GL_DEPTH_TEST );
		glEnable( GL_PROGRAM_POINT_SIZE );
		for( unsigned int i= 0; i< icon_count; i++ )
		{
			if (icons_pos[i][2] > -0.1f) continue;
			naviball_icons_shader_.UniformVec3( "p", icons_pos[i] );
			naviball_icons_shader_.UniformVec3( "c", icons_color[i] );
			naviball_icons_shader_.UniformFloat( "tn", float(icons_type[i]) + 0.01f );
			glDrawArrays( GL_POINTS, 0, 1 );
		}
		glDisable( GL_PROGRAM_POINT_SIZE );
		glEnable( GL_DEPTH_TEST );
		glDisable( GL_BLEND );
	} // draw icons
}