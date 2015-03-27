#pragma once
#include "micro-f.h"

#include "glsl_program.h"
#include "vertex_buffer.h"
#include "textures_generation.h"

class mf_Text;
class mf_Player;
class mf_MainLoop;

#define MF_GUI_TEXT_MAX_LENTH 32
#define MF_GUI_MENU_MAX_BUTTONS 16
#define MF_GUI_MAX_TEXTS 16

class mf_Gui
{
public:
	mf_Gui( mf_Text* text, const mf_Player* player );
	~mf_Gui();

	void SetCursor( unsigned int x, unsigned int y );
	void MouseClick( unsigned int x, unsigned int y );
	void MouseHover( unsigned int x, unsigned int y );
	void Draw();
	void Resize();

	enum NaviBallIcons
	{
		IconSpeed,
		IconAntiSpeed,
		LastIcon
	};

private:

	typedef void ( mf_Gui::*GuiButtonCallback )();

	struct GuiText
	{
		char text[ MF_GUI_TEXT_MAX_LENTH ];
		unsigned char color[4];
		unsigned int row, colomn;
		unsigned int size;
	};

	struct GuiButton
	{
		unsigned int x, y;
		unsigned int width, height;
		GuiText text;
		bool is_hover;
		GuiButtonCallback callback;
		unsigned int user_data; // user data for different using
	};

	struct GuiMenu
	{
		GuiButton buttons[ MF_GUI_MENU_MAX_BUTTONS ];
		unsigned int button_count;
		GuiText texts[ MF_GUI_MAX_TEXTS ];
		unsigned int text_count;
		bool has_backgound;
	};

	void PrepareMenus();
	void DrawControlPanel();
	void DrawNaviball();
	void DrawNaviballGlass();
	void DrawMenu( const GuiMenu* menu );
	void DrawCursor();

	void OnPlayButton();
	void OnSettingsButton();
	void OnQuitButton();

	void OnChangeDayTimeButton();
	void OnHdrButton();
	void OnSettingsBackButton();

private:
	mf_MainLoop* main_loop_;
	mf_Text* text_;
	const mf_Player* player_;

	unsigned int cursor_pos_[2];

	mf_GLSLProgram naviball_shader_;
	mf_VertexBuffer naviball_vbo_;
	GLuint naviball_texture_;

	mf_GLSLProgram naviball_icons_shader_;
	GLuint naviball_icons_texture_array_;

	mf_GLSLProgram gui_shader_;
	mf_VertexBuffer common_vbo_;

	GLuint textures[LastGuiTexture];
	GLuint cursor_texture_;

	GuiMenu main_menu_;
	GuiMenu settings_menu_;
	GuiMenu* current_menu_;
};