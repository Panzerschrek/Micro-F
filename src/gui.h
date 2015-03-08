#pragma once
#include "micro-f.h"

#include "glsl_program.h"
#include "vertex_buffer.h"

class mf_Text;
class mf_Player;
class mf_MainLoop;

class mf_Gui
{
public:
	mf_Gui( mf_Text* text, const mf_Player* player );
	~mf_Gui();

	void Draw();

private:
	void DrawNaviball();

private:
	mf_Text* text_;
	const mf_Player* player_;

	mf_GLSLProgram naviball_shader_;
	mf_VertexBuffer naviball_vbo_;
	GLuint naviball_texture_;
};