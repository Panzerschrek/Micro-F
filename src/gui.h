#pragma once
#include "micro-f.h"

#include "glsl_program.h"
#include "vertex_buffer.h"
#include "textures_generation.h"

class mf_Text;
class mf_Player;
class mf_MainLoop;

class mf_Gui
{
public:
	mf_Gui( mf_Text* text, const mf_Player* player );
	~mf_Gui();

	void Draw();

	enum NaviBallIcons
	{
		IconDirection,
		IconSpeed,
		IconAntiSpeed,
		LastIcon
	};

private:
	void DrawControlPanel();
	void DrawNaviball();
	void DrawNaviballGlass();

private:
	mf_Text* text_;
	const mf_Player* player_;

	mf_GLSLProgram naviball_shader_;
	mf_VertexBuffer naviball_vbo_;
	GLuint naviball_texture_;

	mf_GLSLProgram naviball_icons_shader_;
	GLuint naviball_icons_texture_array_;

	mf_GLSLProgram gui_shader_;
	mf_VertexBuffer common_vbo_;

	GLuint textures[LastGuiTexture];

	//GLuint control_panel_background_texture_;
	//GLuint throttle_bar_texture_;
	//GLuint throttle_indicator_texture_;
};