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


mf_Gui::mf_Gui( mf_Text* text, const mf_Player* player )
	: text_(text)
	, player_(player)
{
	naviball_shader_.SetAttribLocation( "p", 0 );
	naviball_shader_.SetAttribLocation( "tc", 2 );
	naviball_shader_.Create( mf_Shaders::naviball_shader_v, mf_Shaders::naviball_shader_f );
	static const char* const naviball_shader_uniforms[]= { /*vert*/ "mat",/*frag*/ "tex" };
	naviball_shader_.FindUniforms( naviball_shader_uniforms, sizeof(naviball_shader_uniforms) / sizeof(char*) );

	{ // naviball texture
		mf_Texture tex( 9, 9 );
		GenNaviballTexture( &tex );
		tex.LinearNormalization( 1.0f );

		glGenTextures( 1, &naviball_texture_ );
		glBindTexture( GL_TEXTURE_2D, naviball_texture_ );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8,
			tex.SizeX(), tex.SizeY(), 0, GL_RGBA, GL_UNSIGNED_BYTE, tex.GetNormalizedData() );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
		glGenerateMipmap( GL_TEXTURE_2D );
	}
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

	{
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

	DrawNaviball();
}

void mf_Gui::DrawNaviball()
{
	mf_MainLoop* main_loop= mf_MainLoop::Instance();
	const mf_Aircraft* aircraft= player_->GetAircraft();

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

	static const float translate_vec[]= { 0.0f, -0.75f, 0.0f };
	Mat4Translate( translate_mat, translate_vec );

	const float c_naviball_scale= 0.22f;
	scale_vec[0]= c_naviball_scale  * float(main_loop->ViewportHeight()) / float(main_loop->ViewportWidth());
	scale_vec[1]= c_naviball_scale;
	scale_vec[2]= c_naviball_scale;
	Mat4Scale( scale_mat, scale_vec );

	{
		float axis_mat[16];
		float z_rot_mat[16];
		float tmp_mat[16];
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
		Mat4Invert( axis_mat, tmp_mat );

		Mat4RotateZ( z_rot_mat, MF_PI2 );
		Mat4Mul( z_rot_mat, tmp_mat, rotate_mat );
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

	naviball_shader_.UniformMat4( "mat", mat );

	glDisable( GL_DEPTH_TEST );
	glEnable( GL_CULL_FACE );

	naviball_vbo_.Bind();
	glDrawElements( GL_TRIANGLES, naviball_vbo_.IndexDataSize() / sizeof(unsigned short), GL_UNSIGNED_SHORT, 0 );

	glDisable( GL_CULL_FACE );
	glEnable( GL_DEPTH_TEST );
}