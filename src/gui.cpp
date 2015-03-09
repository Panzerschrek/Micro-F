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

static void GenGuiQuadTextureCoords( mf_GuiVertex* v, mf_GuiTexture tex )
{
	v[0].tex_coord[0]= 0.0f;
	v[0].tex_coord[1]= 0.0f;
	v[1].tex_coord[0]= 1.0f;
	v[1].tex_coord[1]= 0.0f;
	v[2].tex_coord[0]= 1.0f;
	v[2].tex_coord[1]= 1.0f;
	v[3].tex_coord[0]= 0.0f;
	v[3].tex_coord[1]= 1.0f;
	v[0].tex_coord[2]= v[1].tex_coord[2]=
	v[2].tex_coord[2]= v[3].tex_coord[2]= float(tex) + 0.01f;
}

mf_Gui::mf_Gui( mf_Text* text, const mf_Player* player )
	: main_loop_(mf_MainLoop::Instance())
	, text_(text)
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

	static const unsigned int textures_size_log2[]=
	{
		9, 9, // naviball
		8, 7, // control panel
		5, 8, // throttle bar
		3, 3, // throttle indicator
		7, 7, // vertical speed indicator
		8, 8 // naviball glass
	};
	for( unsigned int i= 0; i< LastGuiTexture; i++ )
	{
		mf_Texture tex( textures_size_log2[i*2], textures_size_log2[i*2+1] );
		gui_texture_gen_func[i]( &tex );
		tex.LinearNormalization( 1.0f );

		glGenTextures( 1, &textures[i] );
		glBindTexture( GL_TEXTURE_2D, textures[i] );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8,
			tex.SizeX(), tex.SizeY(), 0, GL_RGBA, GL_UNSIGNED_BYTE, tex.GetNormalizedData() );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
		glGenerateMipmap( GL_TEXTURE_2D );
	}

	PrepareMenus();
}

mf_Gui::~mf_Gui()
{
}

void mf_Gui::MouseClick( unsigned int x, unsigned int y )
{
	(void)x; (void)y;
}

void mf_Gui::MouseHover( unsigned int x, unsigned int y )
{
	(void)x; (void)y;
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
	DrawNaviballGlass();
	//DrawMainMenu();
}

void mf_Gui::PrepareMenus()
{
	const unsigned int cell_size= 16;
	const unsigned int border_size= 1;

	unsigned int screen_size_cl[2];
	screen_size_cl[0]= main_loop_->ViewportWidth() / cell_size;
	screen_size_cl[1]= main_loop_->ViewportHeight() / cell_size;

	main_menu_.buttons[0].x= (screen_size_cl[0]/2) * cell_size + border_size;
	main_menu_.buttons[0].y= (screen_size_cl[1]/2) * cell_size + border_size;
	main_menu_.buttons[0].width=  cell_size * 4 - border_size;
	main_menu_.buttons[0].height= cell_size - border_size;

	main_menu_.button_count= 1;
}

void mf_Gui::DrawControlPanel()
{
	const mf_Aircraft* aircraft= player_->GetAircraft();
	const float tex_z_delta= 0.01f;

	gui_shader_.Bind();

	int textures_id[LastGuiTexture];
	for( unsigned int i= TextureControlPanel; i< LastGuiTexture; i++ )
	{
		glActiveTexture( GL_TEXTURE0 + i - TextureControlPanel );
		glBindTexture( GL_TEXTURE_2D, textures[i] );
		textures_id[i]= i - TextureControlPanel ;
	}
	gui_shader_.UniformIntArray( "tex", 6, textures_id );

	mf_GuiVertex vertices[256];
	mf_GuiVertex* v= vertices;

	float k= float(main_loop_->ViewportHeight()) / float(main_loop_->ViewportWidth());

	// control panel
	{
		v[0].pos[0]= -0.4f * k;
		v[0].pos[1]= -1.0f;
		v[1].pos[0]=  0.5f * k;
		v[1].pos[1]= -1.0f;
		v[2].pos[0]=  0.5f * k;
		v[2].pos[1]= -0.5f;
		v[3].pos[0]= -0.4f * k;
		v[3].pos[1]= -0.5f;
		GenGuiQuadTextureCoords( v, TextureControlPanel );
		v[4]= v[0];
		v[5]= v[2];
		v+= 6;
	}
	// throttle bar
	{
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
		GenGuiQuadTextureCoords( v, TextureThrottleBar );
		v[4]= v[0];
		v[5]= v[2];
		v+= 6;

		// throttle indicator
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
		v[0].tex_coord[2]= v[1].tex_coord[2]= v[2].tex_coord[2]= float(TextureThrottleIndicator) + tex_z_delta;
		v+= 3;
	}
	// vertical speed indicator
	{
		const float vertical_speed_indicator_center[]= { 0.35f, -0.65f };
		const float vertical_speed_indicator_half_size= 0.11f;
		v[0].pos[0]= ( vertical_speed_indicator_center[0] - vertical_speed_indicator_half_size ) * k;
		v[0].pos[1]= vertical_speed_indicator_center[1] - vertical_speed_indicator_half_size;
		v[1].pos[0]= ( vertical_speed_indicator_center[0] + vertical_speed_indicator_half_size ) * k;
		v[1].pos[1]= vertical_speed_indicator_center[1] - vertical_speed_indicator_half_size;
		v[2].pos[0]= ( vertical_speed_indicator_center[0] + vertical_speed_indicator_half_size ) * k;
		v[2].pos[1]= vertical_speed_indicator_center[1] + vertical_speed_indicator_half_size;
		v[3].pos[0]= ( vertical_speed_indicator_center[0] - vertical_speed_indicator_half_size ) * k;
		v[3].pos[1]= vertical_speed_indicator_center[1] + vertical_speed_indicator_half_size;
		GenGuiQuadTextureCoords( v, TextureVerticalSpeedIndicator );
		v[4]= v[0];
		v[5]= v[2];
		v+= 6;

		const float max_vertical_speed_indicator_angle= 135.0f * MF_DEG2RAD;
		const float max_vertical_speed= 50.0f;
		float v_speed_k= aircraft->Velocity()[2] / max_vertical_speed;
		{
			// make indicator logarithmic
			float abs_k= mf_Math::fabs(v_speed_k);
			abs_k= mf_Math::log( abs_k * 1.718281828f + 1.0f );
			v_speed_k= abs_k * mf_Math::sign( v_speed_k );
		}
		if(v_speed_k > 1.0f) v_speed_k= 1.0f;
		else if(v_speed_k< -1.0f) v_speed_k= -1.0f;
		float vertical_speed_indicator_angle= max_vertical_speed_indicator_angle * v_speed_k;

		static const float vertical_speed_indicator_arrow[]=
		{
			0.0f, -0.009f, 0.0f,
			0.0f, +0.009f, 0.0f,
			-vertical_speed_indicator_half_size * 0.9f, 0.0f, 0.0f
		};
		float vertical_speed_indicator_arraow_mat[16];
		Mat4RotateZ( vertical_speed_indicator_arraow_mat, -vertical_speed_indicator_angle );
		for( unsigned int i= 0; i< 3; i++ )
		{
			float tmp_v[3];
			Vec3Mat4Mul( vertical_speed_indicator_arrow + i * 3, vertical_speed_indicator_arraow_mat, tmp_v );
			v[i].pos[0]= ( tmp_v[0] + vertical_speed_indicator_center[0] ) * k;
			v[i].pos[1]= tmp_v[1] + vertical_speed_indicator_center[1];
			v[i].tex_coord[0]= 0.0f;
			v[i].tex_coord[1]= 0.0f;
			v[i].tex_coord[2]= float(TextureThrottleIndicator) + tex_z_delta;
		}
		v+= 3;
	}
	
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
	const mf_Aircraft* aircraft= player_->GetAircraft();

	float rot_mat_for_space_vectors[16];
	{
		naviball_shader_.Bind();

		glActiveTexture( GL_TEXTURE0 );
		glBindTexture( GL_TEXTURE_2D, textures[TextureNaviball] );
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
		scale_vec[0]= c_naviball_scale  * float(main_loop_->ViewportHeight()) / float(main_loop_->ViewportWidth());
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

		unsigned int icon_count= 2;

		float speed_vec[3];
		VEC3_CPY( speed_vec, aircraft->Velocity() );
		Vec3Normalize( speed_vec );
		Vec3Mat4Mul( speed_vec, rot_mat_for_space_vectors );

		icons_pos[0][0]= speed_vec[0];
		icons_pos[0][1]= speed_vec[1];
		icons_pos[0][2]= speed_vec[2];
		icons_type[0]= IconSpeed;
		icons_color[0]= speed_colord;

		VEC3_CPY( speed_vec, aircraft->Velocity() );
		Vec3Normalize( speed_vec );
		Vec3Mul( speed_vec, -1.0f );
		Vec3Mat4Mul( speed_vec, rot_mat_for_space_vectors );

		icons_pos[1][0]= speed_vec[0];
		icons_pos[1][1]= speed_vec[1];
		icons_pos[1][2]= speed_vec[2];
		icons_type[1]= IconAntiSpeed;
		icons_color[1]= speed_colord;

		naviball_icons_shader_.Bind();

		glActiveTexture( GL_TEXTURE0 );
		glBindTexture( GL_TEXTURE_2D_ARRAY, naviball_icons_texture_array_ );
		naviball_icons_shader_.UniformInt( "tex", 0 );

		naviball_icons_shader_.UniformFloat( "ps", float(main_loop_->ViewportHeight()) / 18.0f );

		glEnable( GL_BLEND );
		glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
		glDisable( GL_DEPTH_TEST );
		glEnable( GL_PROGRAM_POINT_SIZE );
		for( unsigned int i= 0; i< icon_count; i++ )
		{
			if (icons_pos[i][2] > -0.11f) continue;
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

void mf_Gui::DrawNaviballGlass()
{
	mf_GuiVertex vertices[6];
	mf_GuiVertex* v= vertices;

	float k= float(main_loop_->ViewportHeight()) / float(main_loop_->ViewportWidth());
	const float naviball_radius= 0.24f;
	const float naviball_center[]= { 0.0f, -0.75f };
	v[0].pos[0]= (naviball_center[0] - naviball_radius) * k;
	v[0].pos[1]= naviball_center[1] - naviball_radius;
	v[1].pos[0]= (naviball_center[0] + naviball_radius) * k;
	v[1].pos[1]= naviball_center[1] - naviball_radius;
	v[2].pos[0]= (naviball_center[0] + naviball_radius) * k;
	v[2].pos[1]= naviball_center[1] + naviball_radius;
	v[3].pos[0]= (naviball_center[0] - naviball_radius) * k;
	v[3].pos[1]= naviball_center[1] + naviball_radius;
	GenGuiQuadTextureCoords( v, TextureNaviballGlass );
	v[4]= v[0];
	v[5]= v[2];
	v+= 6;

	gui_shader_.Bind();
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, textures[TextureNaviballGlass] );
	gui_shader_.UniformInt( "tex", 0 );

	common_vbo_.Bind();
	common_vbo_.VertexSubData( vertices, (v - vertices) * sizeof(mf_GuiVertex), 0 );

	glDisable( GL_DEPTH_TEST );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	glDrawArrays( GL_TRIANGLES, 0, v - vertices );

	glDisable( GL_BLEND );
	glEnable( GL_DEPTH_TEST );
}

void mf_Gui::DrawMainMenu()
{
	float inv_viewport_width2 = +2.0f / float( main_loop_->ViewportWidth () );
	float inv_viewport_height2= -2.0f / float( main_loop_->ViewportHeight() );

	mf_GuiVertex vertices[256];
	mf_GuiVertex* v= vertices;

	GuiMenu* menu= &main_menu_;
	GuiButton* buttons= menu->buttons;
	for( unsigned int i= 0; i < menu->button_count; i++ )
	{
		v[0].pos[0]= float(buttons[i].x) * inv_viewport_width2  - 1.0f;
		v[0].pos[1]= float(buttons[i].y) * inv_viewport_height2 + 1.0f;
		v[1].pos[0]= float(buttons[i].x + buttons[i].width) * inv_viewport_width2  - 1.0f;
		v[1].pos[1]= v[0].pos[1];
		v[2].pos[0]= v[1].pos[0];
		v[2].pos[1]= float(buttons[i].y + buttons[i].height) * inv_viewport_height2 + 1.0f;
		v[3].pos[0]= v[0].pos[0];
		v[3].pos[1]= v[2].pos[1];
		GenGuiQuadTextureCoords( v, TextureThrottleIndicator );

		v[4]= v[0];
		v[5]= v[2];
		v+= 6;
	}

	gui_shader_.Bind();
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, textures[TextureThrottleIndicator] );
	gui_shader_.UniformInt( "tex", 0 );

	common_vbo_.Bind();
	common_vbo_.VertexSubData( vertices, (v - vertices) * sizeof(mf_GuiVertex), 0 );

	glDisable( GL_DEPTH_TEST );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	glDrawArrays( GL_TRIANGLES, 0, v - vertices );

	glDisable( GL_BLEND );
	glEnable( GL_DEPTH_TEST );
}