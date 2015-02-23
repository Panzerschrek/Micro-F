#pragma once

#include "micro-f.h"
#include "glsl_program.h"
#include "vertex_buffer.h"

class Texture;

#pragma pack( push, 1 )
struct mf_TextVertex
{
	float pos[2];
	unsigned short tex_coord[2];
	unsigned char color[4];
};
#pragma pack (pop)


#define MF_MAX_TEXT_BUFFER_SIZE 8192

#define LETTER_WIDTH 8
#define LETTER_HEIGHT 18
#define FONT_BITMAP_WIDTH 768
#define FONT_BITMAP_HEIGHT 18

class mf_Text
{
	friend class Texture;

public:
	mf_Text();
	~mf_Text();
	void AddText( unsigned int colomn, unsigned int row, unsigned int size, const unsigned char* color, const char* text );

	void Draw();
	void SetViewport( unsigned int viewport_width, unsigned int viewport_height );

private:
	void CreateTexture();

	mf_GLSLProgram text_shader_;
	mf_TextVertex* vertices_;
	mf_VertexBuffer text_vbo_;
	unsigned int vertex_buffer_size_;
	unsigned int vertex_buffer_pos_;
	float viewport_width_, viewport_height_;

	unsigned char* text_texture_data_;

	GLuint font_texture_id_;

public:
	static const unsigned char default_color[4];
};


inline void mf_Text::SetViewport( unsigned int viewport_width, unsigned int viewport_height )
{
	viewport_width_ = float(viewport_width );
	viewport_height_= float(viewport_height);
}
