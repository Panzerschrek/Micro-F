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
,
#include "../textures/enemy_front.h"
,
#include "../textures/enemy_back.h"
};

#define MF_CURSOR_SIZE 32
static const unsigned char cursor_data[ MF_CURSOR_SIZE * MF_CURSOR_SIZE / 8 ]=
#include "../textures/cursor.h"
;

#define MF_MAX_COMMON_VERTICES_BYTES 4096
#define MF_MAX_COMMON_INDECES_BYTES 2048

#define COLOR_CPY(dst,src) *((int*)(dst))= *((int*)(src))
#define COLOR_CPY_INT(dst,src) *((int*)(dst))= src

/*
UI STRINGS
*/

static const char* day_times[]=
{
	"sunrise",
	"morning",
	"midday",
	"evening",
	"sunset",
	"night"
};

static const char* low_normal_height[]=
{
	"low", "normal", "height"
};

static const char* off_on[]=
{
	"off", "on",
};

static const char* clouds_intensity[]=
{
	"disabled", "low", "medium", "height"
};

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

void IntToStr( int i, char* str, int digits )
{
	str[0]= i >= 0 ? '+' : '-';
	str++;
	if( i < 0 ) i= -i;

	int ten_pow= 1;
	for( int k= 0; k< digits-1; k++ )
		ten_pow*= 10;
	for( int k= 0; k< digits; k++ )
	{
		str[k]= (i / ten_pow) %10 + '0';
		ten_pow/= 10;
	}
}

mf_Gui::mf_Gui( mf_Text* text, const mf_Player* player )
	: main_loop_(mf_MainLoop::Instance())
	, text_(text)
	, player_(player)
	, current_menu_(NULL)
{
	cursor_pos_[0]= cursor_pos_[1]= 0;

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

	static const unsigned int textures_size_log2[ LastGuiTexture * 2 ]=
	{
		9, 9, // naviball
		8, 7, // control panel
		5, 8, // throttle bar
		3, 3, // throttle indicator
		7, 7, // vertical speed indicator
		4, 9, // numbers
		8, 8, // naviball glass
		3, 3, // gui button
		9, 9, // backgound
		8, 8  // target aircraft
	};
	for( unsigned int i= 0; i< LastGuiTexture; i++ )
	{
		mf_Texture tex( textures_size_log2[i*2], textures_size_log2[i*2+1] );
		gui_texture_gen_func[i]( &tex );
		tex.LinearNormalization( 1.0f );

		glGenTextures( 1, &textures_[i] );
		glBindTexture( GL_TEXTURE_2D, textures_[i] );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8,
			tex.SizeX(), tex.SizeY(), 0, GL_RGBA, GL_UNSIGNED_BYTE, tex.GetNormalizedData() );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
		glGenerateMipmap( GL_TEXTURE_2D );
	}
	{ // cursor texture
		unsigned char decompressed_data[ MF_CURSOR_SIZE * MF_CURSOR_SIZE ];
		unsigned char decompressed_data_rgba[ MF_CURSOR_SIZE * MF_CURSOR_SIZE * 4 ];
		mfMonochromeImageTo8Bit( (unsigned char*)cursor_data, decompressed_data, sizeof(decompressed_data) );
		mf8BitImageToWhiteWithAlpha( decompressed_data, decompressed_data_rgba, sizeof(decompressed_data_rgba) );

		static const unsigned char cursor_color[4]= { 32, 196, 32, 0 };
		for( unsigned int i= 0; i< MF_CURSOR_SIZE * MF_CURSOR_SIZE * 4; i+=4 )
		{
			decompressed_data_rgba[i+0]= cursor_color[0];
			decompressed_data_rgba[i+1]= cursor_color[1];
			decompressed_data_rgba[i+2]= cursor_color[2];
		}

		glGenTextures( 1, &cursor_texture_ );
		glBindTexture( GL_TEXTURE_2D, cursor_texture_ );
		glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA8,
			MF_CURSOR_SIZE, MF_CURSOR_SIZE, 0, GL_RGBA, GL_UNSIGNED_BYTE, decompressed_data_rgba );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
		glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
		glGenerateMipmap( GL_TEXTURE_2D );
	}

	PrepareMenus();

	current_menu_= &menus_[ MainMenu ];
	//OnPlayButton(); // hack for fast development
}

mf_Gui::~mf_Gui()
{
}

void mf_Gui::SetCursor( unsigned int x, unsigned int y )
{
	cursor_pos_[0]= x;
	cursor_pos_[1]= y;
}

void mf_Gui::MouseClick( unsigned int x, unsigned int y )
{
	if( current_menu_ == NULL ) return;

	for( unsigned int i= 0; i< current_menu_->button_count; i++ )
	{
		GuiButton* button= &current_menu_->buttons[i];
		if( x >= button->x && y >= button->y )
			if( x < button->x + button->width && y <button->y + button->height )
			{
				if( button->callback != NULL )
					(this->*(button->callback))();

				break;
			}
	}
}

void mf_Gui::MouseHover( unsigned int x, unsigned int y )
{
	if( current_menu_ == NULL ) return;

	for( unsigned int i= 0; i< current_menu_->button_count; i++ )
	{
		GuiButton* button= &current_menu_->buttons[i];
		if( x >= button->x && y >= button->y &&
			x < button->x + button->width && y <button->y + button->height )
			button->is_hover= true;
		else
			button->is_hover= false;
	}
}

void mf_Gui::Draw()
{
	if( current_menu_ == &menus_[ InGame ] )
	{
		const mf_Aircraft* aircraft= player_->GetAircraft();

		{ // aircraft parameters
		#ifdef MF_DEBUG
			char str[256];
			sprintf( str, "angle of attack: %+2.3f\nlift force k: %+2.3f\npitch/yaw/roll factors: %+1.2f %+1.2f %+1.2f",
				aircraft->debug_angle_of_attack_deg_, aircraft->debug_cyk_,
				aircraft->debug_pitch_control_factor_, aircraft->debug_yaw_control_factor_, aircraft->debug_roll_control_factor_ );
			text_->AddText( 1, 9, 1, mf_Text::default_color, str );
		#endif
		}

		{ // hp, score
			unsigned int bottom_row= main_loop_->ViewportHeight() / MF_LETTER_HEIGHT;
			unsigned int right_column= main_loop_->ViewportWidth() / MF_LETTER_WIDTH;
			static const unsigned char c_hp_color[3][4]=
			{
				{ 64, 220, 64, 0  },
				{ 220, 220, 64, 0 },
				{ 220, 64, 64, 0  }
			};
			static const unsigned char c_score_color[4]= { 240, 230, 220, 0 };
			static const unsigned char c_rockets_color[4]= { 160, 160, 255, 0 };

			unsigned int color_index;
			if( aircraft->HP() >= 600 ) color_index= 0;
			else if( aircraft->HP() >= 200 ) color_index= 1;
			else color_index= 2;

			char hp_str[]= "HP: sdddd";
			IntToStr( aircraft->HP(), hp_str+4, 4 );
			text_->AddText( right_column - 20, bottom_row - 2, 2, c_hp_color[color_index], hp_str );

			char score_str[]= "Score: sdddd";
			IntToStr( player_->Score(), score_str+7, 4 );
			text_->AddText( right_column - 26, bottom_row - 4, 2, c_score_color, score_str );

			char rockets_str[32];
			sprintf( rockets_str, "Rockets: %d", aircraft->RocketsCount() );
			text_->AddText( 2, bottom_row - 2, 2, c_rockets_color, rockets_str );
		}

		DrawTargetAircraftAim();
		DrawControlPanel();
		DrawNaviball();
		DrawNaviballGlass();
	}
	DrawMenu( current_menu_ );
	DrawCursor();
}

void mf_Gui::Resize()
{
	PrepareMenus();
}

void mf_Gui::PrepareMenus()
{
	const char* const c_title_text= "Micro-F";
	const char* const c_subtitle_text= "96k game";
	const char* const c_play_button_text= " play ";
	const char* const c_settings_button_text= " settings ";
	const char* const c_quit_button_text= " quit ";

	const char* const c_button_back= "back";
	const char* const c_daytime_text= "daytime: ";
	const char* const c_hdr_text= "hdr: ";
	const char* const c_clouds_text= "clouds: ";
	const char* const c_sky_quality_text= "sky quality: ";

	const unsigned int cell_size[2]= { MF_LETTER_WIDTH, MF_LETTER_HEIGHT };
	const unsigned int border_size= 1;

	static const unsigned char text_color[]= { 220, 255, 220, 0 };

	unsigned int screen_size_cl[2];
	screen_size_cl[0]= main_loop_->ViewportWidth() / cell_size[0];
	screen_size_cl[1]= main_loop_->ViewportHeight() / cell_size[1];

	GuiMenu* menu;
	GuiText* text;
	GuiButton* button;

	/*
	MAIN MENU
	*/

	menu= &menus_[ MainMenu ];
	menu->button_count=0;
	menu->text_count= 0;
	menu->has_backgound= true;

	// title
	text= &menu->texts[0];
	strcpy( text->text, c_title_text );
	text->size= 4;
	text->colomn= screen_size_cl[0]/2 - strlen(c_title_text) * text->size / 2;
	text->row= 1;
	COLOR_CPY( text->color, text_color );
	menu->text_count++;

	// subtitle
	text= &menu->texts[1];
	strcpy( text->text, c_subtitle_text );
	text->size= 1;
	text->colomn= screen_size_cl[0]/2 + 4;
	text->row= 4;
	COLOR_CPY( text->color, text_color );
	menu->text_count++;

	unsigned int button_altitude= screen_size_cl[1]/2 - 4;

	// Play button
	text= &menu->buttons[0].text;
	strcpy( text->text, c_play_button_text );
	text->size= 2;
	text->colomn= screen_size_cl[0]/2 - text->size * strlen(c_play_button_text) / 2;
	text->row= button_altitude;
	COLOR_CPY( text->color, text_color );

	button= &menu->buttons[0];
	button->x= text->colomn * cell_size[0] + border_size;
	button->y= text->row * cell_size[1] + border_size;
	button->width=  text->size * cell_size[0] * strlen(c_play_button_text) - border_size;
	button->height= text->size * cell_size[1] - border_size;
	button->callback= &mf_Gui::OnPlayButton;
	menu->button_count++;

	button_altitude+= text->size + 1;

	// Settings button
	text= &menu->buttons[1].text;
	strcpy( text->text, c_settings_button_text );
	text->size= 2;
	text->colomn= screen_size_cl[0]/2 - text->size * strlen(c_settings_button_text) / 2;
	text->row= button_altitude;
	COLOR_CPY( text->color, text_color );

	button= &menu->buttons[1];
	button->x= text->colomn * cell_size[0] + border_size;
	button->y= text->row * cell_size[1] + border_size;
	button->width=  text->size * cell_size[0] * strlen(c_settings_button_text) - border_size;
	button->height= text->size * cell_size[1] - border_size;
	button->callback= &mf_Gui::OnSettingsButton;
	menu->button_count++;

	button_altitude+= text->size + 1;

	// Quit button
	text= &menu->buttons[2].text;
	strcpy( text->text, c_quit_button_text );
	text->size= 2;
	text->colomn= screen_size_cl[0]/2 - text->size * strlen(c_quit_button_text) / 2;
	text->row= button_altitude;
	COLOR_CPY( text->color, text_color );

	button= &menu->buttons[2];
	button->x= text->colomn * cell_size[0] + border_size;
	button->y= text->row * cell_size[1] + border_size;
	button->width=  text->size * cell_size[0] * strlen(c_quit_button_text) - border_size;
	button->height= text->size * cell_size[1] - border_size;
	button->callback= &mf_Gui::OnQuitButton;
	menu->button_count++;

	/*
	SETTING MENU
	*/

	menu= &menus_[ SettingsMenu ];
	menu->button_count=0;
	menu->text_count= 0;
	menu->has_backgound= true;

	const unsigned int c_settings_button_width= 9;

	// daytime
	text= &menu->texts[0];
	strcpy( text->text, c_daytime_text );
	text->size= 2;
	text->colomn= screen_size_cl[0]/2 -text->size * strlen(c_daytime_text);
	text->row= 4;
	COLOR_CPY( text->color, text_color );
	menu->text_count++;
	// daytime button text
	text= &menu->buttons[0].text;
	strcpy( text->text, day_times[1] );
	text->size= 2;
	text->colomn= screen_size_cl[0]/2;
	text->row= 4;
	COLOR_CPY( text->color, text_color );
	// daytime button
	button= &menu->buttons[0];
	button->x= text->colomn * cell_size[0] + border_size;
	button->y= text->row * cell_size[1] + border_size;
	button->width=  text->size * cell_size[0] * c_settings_button_width - border_size;
	button->height= text->size * cell_size[1] - border_size;
	button->callback= &mf_Gui::OnChangeDayTimeButton;
	button->user_data= 1;
	menu->button_count++;

	// hdr
	text= &menu->texts[1];
	strcpy( text->text, c_hdr_text );
	text->size= 2;
	text->colomn= screen_size_cl[0]/2 -text->size * strlen(c_hdr_text);
	text->row= 7;
	COLOR_CPY( text->color, text_color );
	menu->text_count++;
	// hdr button text
	text= &menu->buttons[1].text;
	strcpy( text->text, off_on[0] );
	text->size= 2;
	text->colomn= screen_size_cl[0]/2;
	text->row= 7;
	COLOR_CPY( text->color, text_color );
	// hdr button
	button= &menu->buttons[1];
	button->x= text->colomn * cell_size[0] + border_size;
	button->y= text->row * cell_size[1] + border_size;
	button->width=  text->size * cell_size[0] * c_settings_button_width - border_size;
	button->height= text->size * cell_size[1] - border_size;
	button->callback= &mf_Gui::OnHdrButton;
	button->user_data= 0;
	menu->button_count++;

	// clouds
	text= &menu->texts[2];
	strcpy( text->text, c_clouds_text );
	text->size= 2;
	text->colomn= screen_size_cl[0]/2 -text->size * strlen(c_clouds_text);
	text->row= 10;
	COLOR_CPY( text->color, text_color );
	menu->text_count++;
	// clouds button text
	text= &menu->buttons[2].text;
	strcpy( text->text, clouds_intensity[0] );
	text->size= 2;
	text->colomn= screen_size_cl[0]/2;
	text->row= 10;
	COLOR_CPY( text->color, text_color );
	// clouds button
	button= &menu->buttons[2];
	button->x= text->colomn * cell_size[0] + border_size;
	button->y= text->row * cell_size[1] + border_size;
	button->width=  text->size * cell_size[0] * c_settings_button_width - border_size;
	button->height= text->size * cell_size[1] - border_size;
	button->callback= &mf_Gui::OnCloudsButton;
	button->user_data= 0;
	menu->button_count++;

	// sky quality
	text= &menu->texts[3];
	strcpy( text->text, c_sky_quality_text );
	text->size= 2;
	text->colomn= screen_size_cl[0]/2 -text->size * strlen(c_sky_quality_text);
	text->row= 13;
	COLOR_CPY( text->color, text_color );
	menu->text_count++;
	// sky quality button text
	text= &menu->buttons[3].text;
	strcpy( text->text, low_normal_height[0] );
	text->size= 2;
	text->colomn= screen_size_cl[0]/2;
	text->row= 13;
	COLOR_CPY( text->color, text_color );
	// sky quality button
	button= &menu->buttons[3];
	button->x= text->colomn * cell_size[0] + border_size;
	button->y= text->row * cell_size[1] + border_size;
	button->width=  text->size * cell_size[0] * c_settings_button_width - border_size;
	button->height= text->size * cell_size[1] - border_size;
	button->callback= &mf_Gui::OnSkyQualityButton;
	button->user_data= 0;
	menu->button_count++;

	// back button text
	text= &menu->buttons[4].text;
	strcpy( text->text, c_button_back );
	text->size= 2;
	text->colomn= 2;
	text->row= screen_size_cl[1] - 3;
	COLOR_CPY( text->color, text_color );
	// back button
	button= &menu->buttons[4];
	button->x= text->colomn * cell_size[0] + border_size;
	button->y= text->row * cell_size[1] + border_size;
	button->width=  text->size * cell_size[0] * strlen(c_button_back) - border_size;
	button->height= text->size * cell_size[1] - border_size;
	button->callback= &mf_Gui::OnSettingsBackButton;
	button->user_data= 0;
	menu->button_count++;

	/*
	INGAME MENU
	*/
	menu= &menus_[ InGame ];
	menu->button_count= 0;
	menu->text_count= 0;
	menu->has_backgound= false;
}

void mf_Gui::DrawControlPanel()
{
	const mf_Aircraft* aircraft= player_->GetAircraft();
	const float tex_z_delta= 0.01f;

	gui_shader_.Bind();

	int textures_id[ LastGuiTexture ];
	textures_id[0]= TextureNaviball;
	for( unsigned int i= TextureControlPanel; i<= TextureNumbers; i++ )
	{
		glActiveTexture( GL_TEXTURE0 + i );
		glBindTexture( GL_TEXTURE_2D, textures_[i] );
		textures_id[i]= i;
	}
	gui_shader_.UniformIntArray( "tex", LastGuiTexture, textures_id );

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

	// speed indicator and altitude indicator
	for( unsigned int indicator_type= 0; indicator_type< 2; indicator_type++ )
	{
		static const float speed_indicator_digit_size[2]= { 0.06f * 0.44f, 0.06f };
		static const float speed_indicator_pos[4]= {
			0.26f, -0.85f,
			0.26f + speed_indicator_digit_size[0], -0.85f - speed_indicator_digit_size[1] - 0.006f };

		int s[7];
		unsigned int symbols_count;
		if( indicator_type == 0 )
		{
			int speed= int(mf_Math::round(Vec3Len(aircraft->Velocity())));
			// first 3 symbols - value of speed. last - string " m/s"
			s[0]= speed / 100; s[1]= (speed / 10) % 10; s[2]= speed % 10;
			s[3]= 10; s[4]= 11; s[5]= 12; s[6]= 13;
			symbols_count= 7;
		}
		else
		{
			int alt= int(mf_Math::round(aircraft->Pos()[2]));
			s[0]= alt / 100; s[1]= (alt / 10) % 10; s[2]= alt % 10;
			s[3]= 10; s[4]= 11;
			symbols_count= 5;
		}
		float x= speed_indicator_pos[ indicator_type * 2 ];
		for( unsigned int i= 0; i< symbols_count; i++, x+= speed_indicator_digit_size[0] )
		{
			v[0].pos[0]= x * k;
			v[0].pos[1]= speed_indicator_pos[ 1 + indicator_type * 2 ];
			v[1].pos[0]= ( x + speed_indicator_digit_size[0] ) * k;
			v[1].pos[1]= v[0].pos[1];
			v[2].pos[0]= ( x + speed_indicator_digit_size[0] ) * k;
			v[2].pos[1]= v[0].pos[1] +  speed_indicator_digit_size[1];
			v[3].pos[0]= x * k;
			v[3].pos[1]= v[0].pos[1] +  speed_indicator_digit_size[1];
			v[0].tex_coord[0]= 0.0f;
			v[0].tex_coord[1]= float( s[i]* 2 * MF_LETTER_HEIGHT ) * (1.0f/512.0f);
			v[1].tex_coord[0]= 1.0f;
			v[1].tex_coord[1]= v[0].tex_coord[1];
			v[2].tex_coord[0]= 1.0f;
			v[2].tex_coord[1]= v[0].tex_coord[1] + float( 2 * MF_LETTER_HEIGHT ) * (1.0f/512.0f);
			v[3].tex_coord[0]= 0.0f;
			v[3].tex_coord[1]= v[2].tex_coord[1];
			v[0].tex_coord[2]= v[1].tex_coord[2]= v[2].tex_coord[2]= v[3].tex_coord[2]= TextureNumbers;
			v[4]= v[0];
			v[5]= v[2];
			v+= 6;
		}
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
		glBindTexture( GL_TEXTURE_2D, textures_[TextureNaviball] );
		naviball_shader_.UniformInt( "tex", 0 );

		float tmp_mat[16];
		float mat[16];
		float translate_mat[16];
		float model_mirror_mat[16];
		float rotate_mat[16];
		float basis_change_mat[16];
		float scale_mat[16];
		float scale_vec[3];

		Mat4Translate( translate_mat, naviball_pos );

		const float c_naviball_scale= 0.22f;
		scale_vec[0]= c_naviball_scale  * float(main_loop_->ViewportHeight()) / float(main_loop_->ViewportWidth());
		scale_vec[1]= c_naviball_scale;
		scale_vec[2]= -c_naviball_scale;
		Mat4Scale( scale_mat, scale_vec );

		// rotation matrix is transposed basis
		Mat4Identity( rotate_mat );
		rotate_mat[ 0]= aircraft->AxisVec(0)[0];
		rotate_mat[ 4]= aircraft->AxisVec(0)[1];
		rotate_mat[ 8]= aircraft->AxisVec(0)[2];
		rotate_mat[ 1]= aircraft->AxisVec(1)[0];
		rotate_mat[ 5]= aircraft->AxisVec(1)[1];
		rotate_mat[ 9]= aircraft->AxisVec(1)[2];
		rotate_mat[ 2]= aircraft->AxisVec(2)[0];
		rotate_mat[ 6]= aircraft->AxisVec(2)[1];
		rotate_mat[10]= aircraft->AxisVec(2)[2];

		Mat4RotateX( basis_change_mat, -MF_PI2 );
		float tmp_mat_bc[16];
		Mat4Identity( tmp_mat_bc );
		tmp_mat_bc[10]= -1.0f;
		Mat4Mul( basis_change_mat, tmp_mat_bc );

		Mat4Identity( model_mirror_mat );
		model_mirror_mat[0]= -1.0f;

		Mat4Mul( rotate_mat, basis_change_mat, tmp_mat );
		Mat4Mul( tmp_mat, scale_mat, mat );
		Mat4Mul( mat, translate_mat, tmp_mat );
		Mat4Mul( model_mirror_mat, tmp_mat, mat );
		memcpy( rot_mat_for_space_vectors, tmp_mat, sizeof(mat) );

		naviball_shader_.UniformMat4( "mat", mat );

		glDisable( GL_DEPTH_TEST );
		glEnable( GL_CULL_FACE );

		naviball_vbo_.Bind();
		glDrawElements( GL_TRIANGLES, naviball_vbo_.IndexDataSize() / sizeof(unsigned short), GL_UNSIGNED_SHORT, 0 );

		glDisable( GL_CULL_FACE );
		glEnable( GL_DEPTH_TEST );
	}
	{// naviball icons

		static const float c_speed_color[3]= { 0.3f, 1.0f, 0.3f };
		static const float c_enemy_color[3]= { 1.0f, 0.3f, 0.3f };
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
		icons_color[0]= c_speed_color;

		VEC3_CPY( speed_vec, aircraft->Velocity() );
		Vec3Normalize( speed_vec );
		Vec3Mul( speed_vec, -1.0f );
		Vec3Mat4Mul( speed_vec, rot_mat_for_space_vectors );

		icons_pos[1][0]= speed_vec[0];
		icons_pos[1][1]= speed_vec[1];
		icons_pos[1][2]= speed_vec[2];
		icons_type[1]= IconAntiSpeed;
		icons_color[1]= c_speed_color;

		for( unsigned int j= 0; j< player_->EnemiesAircraftsCount(); j++ )
		{
			float dir_to_aircraft[3];
			float transformed_pos[3];
			Vec3Sub( player_->GetAircraft()->Pos(), player_->GetEnemiesAircrafts()[j]->Pos(), dir_to_aircraft );
			Vec3Normalize( dir_to_aircraft );

			Vec3Mat4Mul( dir_to_aircraft, rot_mat_for_space_vectors, transformed_pos );
			icons_pos[icon_count][0]= transformed_pos[0];
			icons_pos[icon_count][1]= transformed_pos[1];
			icons_pos[icon_count][2]= transformed_pos[2];
			icons_type[icon_count]= IconEnemyAntiDirection;
			icons_color[icon_count]= c_enemy_color;
			icon_count++;

			Vec3Mul( dir_to_aircraft, -1.0f );
			Vec3Mat4Mul( dir_to_aircraft, rot_mat_for_space_vectors, transformed_pos );
			icons_pos[icon_count][0]= transformed_pos[0];
			icons_pos[icon_count][1]= transformed_pos[1];
			icons_pos[icon_count][2]= transformed_pos[2];
			icons_type[icon_count]= IconEnemyDirection;
			icons_color[icon_count]= c_enemy_color;
			icon_count++;
		}

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
	GenGuiQuadTextureCoords( v, mf_GuiTexture(0) );
	v[4]= v[0];
	v[5]= v[2];
	v+= 6;

	gui_shader_.Bind();
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, textures_[TextureNaviballGlass] );
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

void mf_Gui::DrawMenu( const GuiMenu* menu )
{
	float inv_viewport_width2 = +2.0f / float( main_loop_->ViewportWidth () );
	float inv_viewport_height2= -2.0f / float( main_loop_->ViewportHeight() );

	mf_GuiVertex vertices[256];
	mf_GuiVertex* v= vertices;

	if ( menu->has_backgound )
	{
		float kx= -2.0f * inv_viewport_height2 / inv_viewport_width2;
		float ky= 2.0f;
		v[0].pos[0]= -1.0f; v[0].tex_coord[0]= 0.0f;
		v[0].pos[1]= -1.0f; v[0].tex_coord[1]= 0.0f;
		v[1].pos[0]= +1.0f; v[1].tex_coord[0]= kx;
		v[1].pos[1]= -1.0f; v[1].tex_coord[1]= 0.0f;
		v[2].pos[0]= +1.0f; v[2].tex_coord[0]= kx;
		v[2].pos[1]= +1.0f; v[2].tex_coord[1]= ky;
		v[3].pos[0]= -1.0f; v[3].tex_coord[0]= 0.0f;
		v[3].pos[1]= +1.0f; v[3].tex_coord[1]= ky;
		v[0].tex_coord[2]= v[1].tex_coord[2]= v[2].tex_coord[2]= v[3].tex_coord[2]= 1;

		float time= float(clock()) / float(CLOCKS_PER_SEC );
		float d_tc[2];
		d_tc[0]= 1.6f * mf_Math::sin( time * 0.125f );
		d_tc[1]= 2.0f * mf_Math::sin( time * 0.1f );
		for( unsigned int t= 0; t< 4; t++ )
		{
			v[t].tex_coord[0]+= d_tc[0];
			v[t].tex_coord[1]+= d_tc[1];
		}

		v[4]= v[0];
		v[5]= v[2];
		v+= 6;
	}

	const GuiButton* buttons= menu->buttons;
	for( unsigned int i= 0; i < menu->button_count; i++ )
	{
		unsigned int shift= buttons[i].is_hover ? 2 : 0;

		v[0].pos[0]= float(buttons[i].x - shift) * inv_viewport_width2  - 1.0f;
		v[0].pos[1]= float(buttons[i].y - shift) * inv_viewport_height2 + 1.0f;
		v[1].pos[0]= float(buttons[i].x + buttons[i].width + shift) * inv_viewport_width2  - 1.0f;
		v[1].pos[1]= v[0].pos[1];
		v[2].pos[0]= v[1].pos[0];
		v[2].pos[1]= float(buttons[i].y + buttons[i].height + shift) * inv_viewport_height2 + 1.0f;
		v[3].pos[0]= v[0].pos[0];
		v[3].pos[1]= v[2].pos[1];
		GenGuiQuadTextureCoords( v, mf_GuiTexture(0) );

		v[4]= v[0];
		v[5]= v[2];
		v+= 6;
	}

	gui_shader_.Bind();

	glActiveTexture( GL_TEXTURE0 + 0 );
	glBindTexture( GL_TEXTURE_2D, textures_[TextureGuiButton] );
	glActiveTexture( GL_TEXTURE0 + 1 );
	glBindTexture( GL_TEXTURE_2D, textures_[TextureMenuBackgound] );
	int textures_binding[]={ 0, 1 };
	gui_shader_.UniformIntArray( "tex", 2, textures_binding );

	common_vbo_.Bind();
	common_vbo_.VertexSubData( vertices, (v - vertices) * sizeof(mf_GuiVertex), 0 );

	glDisable( GL_DEPTH_TEST );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	glDrawArrays( GL_TRIANGLES, 0, v - vertices );

	glDisable( GL_BLEND );
	glEnable( GL_DEPTH_TEST );

	for( unsigned int i= 0; i < menu->button_count; i++ )
	{
		const GuiText* text= &buttons[i].text;
		text_->AddText( text->colomn, text->row, text->size, text->color, text->text );
	}

	const GuiText* texts= menu->texts;
	for( unsigned int i= 0; i< menu->text_count; i++ )
		text_->AddText( texts[i].colomn, texts[i].row, texts[i].size, texts[i].color, texts[i].text );
}

void mf_Gui::DrawCursor()
{
	mf_GuiVertex vertices[6];
	
	float f_pos[2];
	float f_size[2];
	f_pos[0]= 2.0f * float(cursor_pos_[0]) / main_loop_->ViewportWidth () - 1.0f;
	f_pos[1]= -2.0f * float(cursor_pos_[1]) / main_loop_->ViewportHeight() + 1.0f;

	f_size[0]= 2.0f * float(MF_CURSOR_SIZE) / main_loop_->ViewportWidth ();
	f_size[1]= 2.0f * float(MF_CURSOR_SIZE) / main_loop_->ViewportHeight();

	vertices[0].pos[0]= f_pos[0] - f_size[0] * 0.5f;
	vertices[0].pos[1]= f_pos[1] - f_size[1] * 0.5f;
	vertices[1].pos[0]= f_pos[0] + f_size[0] * 0.5f;
	vertices[1].pos[1]= f_pos[1] - f_size[1] * 0.5f;
	vertices[2].pos[0]= f_pos[0] + f_size[0] * 0.5f;
	vertices[2].pos[1]= f_pos[1] + f_size[1] * 0.5f;
	vertices[3].pos[0]= f_pos[0] - f_size[0] * 0.5f;
	vertices[3].pos[1]= f_pos[1] + f_size[1] * 0.5f;
	GenGuiQuadTextureCoords( vertices, mf_GuiTexture(0) );
	vertices[4]= vertices[0];
	vertices[5]= vertices[2];

	gui_shader_.Bind();

	glActiveTexture( GL_TEXTURE0 + 0 );
	glBindTexture( GL_TEXTURE_2D, cursor_texture_ );
	gui_shader_.UniformInt( "tex", 0 );

	common_vbo_.Bind();
	common_vbo_.VertexSubData( vertices, 6 * sizeof(mf_GuiVertex), 0 );

	glDisable( GL_DEPTH_TEST );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	glDrawArrays( GL_TRIANGLES, 0, 6 );

	glDisable( GL_BLEND );
	glEnable( GL_DEPTH_TEST );
}

void mf_Gui::DrawTargetAircraftAim()
{
	const mf_Aircraft* aircraft= player_->TargetAircraft();
	if( aircraft == NULL ) return;

	float aspect= float(main_loop_->ViewportWidth())/ float(main_loop_->ViewportHeight());
	float transform_mtarix[16];
	{
		float pers_mat[16];
		float rot_z_mat[16];
		float rot_x_mat[16];
		float rot_y_mat[16];
		float basis_change_mat[16];
		float translate_mat[16];

		float translate_vec[3];

		Vec3Mul( player_->Pos(), -1.0f, translate_vec );
		Mat4Translate( translate_mat, translate_vec );

		Mat4Perspective( pers_mat, aspect, player_->Fov(), 0.5f, 2048.0f );

		Mat4RotateZ( rot_z_mat, -player_->Angle()[2] );
		Mat4RotateX( rot_x_mat, -player_->Angle()[0] );
		Mat4RotateY( rot_y_mat, -player_->Angle()[1] );
		{
			Mat4RotateX( basis_change_mat, -MF_PI2 );
			float tmp_mat[16];
			Mat4Identity( tmp_mat );
			tmp_mat[10]= -1.0f;
			Mat4Mul( basis_change_mat, tmp_mat );
		}

		Mat4Mul( translate_mat, rot_z_mat, transform_mtarix );
		Mat4Mul( transform_mtarix, rot_x_mat );
		Mat4Mul( transform_mtarix, rot_y_mat );
		Mat4Mul( transform_mtarix, basis_change_mat );
		Mat4Mul( transform_mtarix, pers_mat );
	}
	float target_vec[4];
	VEC3_CPY( target_vec, aircraft->Pos() ); target_vec[3]= 1.0f;
	float view_space_target_vec[4];
	Vec4Mat4Mul( target_vec, transform_mtarix, view_space_target_vec );
	Vec3Mul( view_space_target_vec, 1.0f / view_space_target_vec[3] );

	if( view_space_target_vec[3] < -1.0f ) return;

	mf_GuiVertex vertices[6];
	const float c_sprite_radius= 0.08f;
	vertices[0].pos[0]= -c_sprite_radius;
	vertices[0].pos[1]= -c_sprite_radius;
	vertices[1].pos[0]= +c_sprite_radius;
	vertices[1].pos[1]= -c_sprite_radius;
	vertices[2].pos[0]= +c_sprite_radius;
	vertices[2].pos[1]= +c_sprite_radius;
	vertices[3].pos[0]= -c_sprite_radius;
	vertices[3].pos[1]= +c_sprite_radius;

	float rot_mat[16];
	Mat4RotateZ( rot_mat, main_loop_->CurrentTime() );
	for( unsigned int i= 0; i< 4; i++ )
	{
		float pos[3];
		pos[0]= vertices[i].pos[0];
		pos[1]= vertices[i].pos[1];
		Vec3Mat4Mul( pos, rot_mat );
		vertices[i].pos[0]= pos[0] + view_space_target_vec[0];
		vertices[i].pos[1]= pos[1] * aspect + view_space_target_vec[1];
	}

	GenGuiQuadTextureCoords( vertices, mf_GuiTexture(0) );
	vertices[4]= vertices[0];
	vertices[5]= vertices[2];

	gui_shader_.Bind();

	glActiveTexture( GL_TEXTURE0 + 0 );
	glBindTexture( GL_TEXTURE_2D, textures_[TextureTargetAircraft] );
	gui_shader_.UniformInt( "tex", 0 );

	common_vbo_.Bind();
	common_vbo_.VertexSubData( vertices, 6 * sizeof(mf_GuiVertex), 0 );

	glDisable( GL_DEPTH_TEST );
	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

	glDrawArrays( GL_TRIANGLES, 0, 6 );

	glDisable( GL_BLEND );
	glEnable( GL_DEPTH_TEST );
}

void mf_Gui::OnPlayButton()
{
	current_menu_= NULL;

	mf_Settings settings;
	settings.use_hdr= menus_[ SettingsMenu ].buttons[1].user_data == 1;
	settings.shadows_quality= mf_Settings::QualityHeight;
	settings.terrain_quality= mf_Settings::QualityMedium;
	settings.sky_quality= mf_Settings::Quality( menus_[ SettingsMenu ].buttons[3].user_data );
	settings.clouds_intensity= mf_Settings::CloudsIntensity( menus_[ SettingsMenu ].buttons[2].user_data );
	settings.daytime= mf_Settings::Daytime( menus_[ SettingsMenu ].buttons[0].user_data );

	main_loop_->Play( &settings );
	current_menu_= &menus_[ InGame ];
}

void mf_Gui::OnSettingsButton()
{
	current_menu_= &menus_[ SettingsMenu ];
}

void mf_Gui::OnHdrButton()
{
	GuiButton* button= &menus_[ SettingsMenu ].buttons[1];
	button->user_data^= 1;
	strcpy( button->text.text, off_on[ button->user_data ] );
}

void mf_Gui::OnCloudsButton()
{
	GuiButton* button= &menus_[ SettingsMenu ].buttons[2];
	button->user_data++;
	if( button->user_data >= sizeof(clouds_intensity) / sizeof(char*) )
		button->user_data= 0;
	strcpy( button->text.text, clouds_intensity[ button->user_data ] );
}

void mf_Gui::OnSkyQualityButton()
{
	GuiButton* button= &menus_[ SettingsMenu ].buttons[3];
	button->user_data++;
	if( button->user_data >= sizeof(low_normal_height) / sizeof(char*) )
		button->user_data= 0;
	strcpy( button->text.text, low_normal_height[ button->user_data ] );
}

void mf_Gui::OnQuitButton()
{
	main_loop_->Quit();
}

void mf_Gui::OnChangeDayTimeButton()
{
	GuiButton* button= &menus_[ SettingsMenu ].buttons[0];
	button->user_data++;
	if( button->user_data >= sizeof(day_times) / sizeof(char*) )
		button->user_data= 0;
	strcpy( button->text.text, day_times[ button->user_data ] );
}

void mf_Gui::OnSettingsBackButton()
{
	current_menu_= &menus_[ MainMenu ];
}