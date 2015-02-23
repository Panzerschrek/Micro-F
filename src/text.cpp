#include "micro-f.h"
#include "text.h"
#include "shaders.h"

#include "font_data.h"

const unsigned char mf_Text::default_color[4]= {255, 255, 255, 32 };

void mf_Text::CreateTexture()
{
	unsigned char* decompressed_font= new unsigned char[ FONT_BITMAP_WIDTH * FONT_BITMAP_HEIGHT ];

	for( unsigned int i= 0; i< FONT_BITMAP_WIDTH * FONT_BITMAP_HEIGHT / 8; i++ )
		for( unsigned int j= 0; j< 8; j++ )
			decompressed_font[ (i<<3) + (7-j) ]= ( (font_data[i] & (1<<j)) >> j ) * 255;

	glGenTextures( 1, &font_texture_id_ );

	glBindTexture( GL_TEXTURE_2D, font_texture_id_ );

	glTexImage2D( GL_TEXTURE_2D, 0, GL_R8,
		FONT_BITMAP_WIDTH, FONT_BITMAP_HEIGHT, 0,
		GL_RED, GL_UNSIGNED_BYTE, decompressed_font );

	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
	glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );

	text_texture_data_= decompressed_font;
}

void mf_Text::AddText( unsigned int colomn, unsigned int row, unsigned int size, const unsigned char* color, const char* text )
{
	const char* str= text;

	float x, x0, y;
	float dx, dy;

	x0= x=  2.0f * float( colomn * LETTER_WIDTH ) / viewport_width_ - 1.0f;
	y= -2.0f * float( (row + 1) * LETTER_HEIGHT ) / viewport_height_ + 1.0f;

	dx= 2.0f * float( LETTER_WIDTH * size ) / viewport_width_;
	dy= 2.0f * float( LETTER_HEIGHT * size ) / viewport_height_;


	mf_TextVertex* v= vertices_ + vertex_buffer_pos_;
	while( *str != 0 )
	{
		if( *str == '\n' )
		{
			x= x0;
			y-=dy;
			str++;
			continue;
		}
		v[0].pos[0]= x;
		v[0].pos[1]= y;
		v[0].tex_coord[0]= *str - 32;
		v[0].tex_coord[1]= 0;

		v[1].pos[0]= x;
		v[1].pos[1]= y + dy;
		v[1].tex_coord[0]= *str - 32;
		v[1].tex_coord[1]= 1;

		v[2].pos[0]= x + dx;
		v[2].pos[1]= y + dy;
		v[2].tex_coord[0]= *str - 32 + 1;
		v[2].tex_coord[1]= 1;

		v[3].pos[0]= x + dx;
		v[3].pos[1]= y;
		v[3].tex_coord[0]= *str - 32 + 1;
		v[3].tex_coord[1]= 0;

		for( unsigned int i= 0; i< 4; i++ )
			*((int*)v[i].color)= *((int*)color);//copy 4 bytes per one asm command

		x+= dx;
		v+= 4;
		str++;
	}
	vertex_buffer_pos_= v - vertices_;
}

void mf_Text::Draw()
{
	glActiveTexture( GL_TEXTURE0 );
	glBindTexture( GL_TEXTURE_2D, font_texture_id_ );

	text_vbo_.Bind();
	text_vbo_.VertexSubData( vertices_, vertex_buffer_pos_ * sizeof(mf_TextVertex), 0 );

	text_shader_.Bind();
	text_shader_.UniformInt( "tex", 0 );

	glEnable( GL_BLEND );
	glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
	glDrawElements( GL_TRIANGLES, vertex_buffer_pos_ * 6 / 4, GL_UNSIGNED_SHORT, NULL );
	glDisable( GL_BLEND );
	vertex_buffer_pos_= 0;
}


mf_Text::mf_Text()
	: vertex_buffer_pos_(0)
{
	CreateTexture();

	vertices_= new mf_TextVertex[ MF_MAX_TEXT_BUFFER_SIZE * 4 ];

	int viewport_params[4];
	glGetIntegerv( GL_VIEWPORT, viewport_params );
	viewport_width_= float(viewport_params[2]);
	viewport_height_= float(viewport_params[3]);

	unsigned short* quad_indeces= new unsigned short[ MF_MAX_TEXT_BUFFER_SIZE * 6 ];
	for( unsigned int i= 0, j= 0; i< MF_MAX_TEXT_BUFFER_SIZE*6; i+= 6, j+=4 )
	{
		quad_indeces[i]= (unsigned short)j;
		quad_indeces[i + 1]= (unsigned short)(j + 1);
		quad_indeces[i + 2]= (unsigned short)(j + 2);

		quad_indeces[i + 3]= (unsigned short)(j + 2);
		quad_indeces[i + 4]= (unsigned short)(j + 3);
		quad_indeces[i + 5]= (unsigned short)(j);
	}

	text_vbo_.VertexData( NULL, MF_MAX_TEXT_BUFFER_SIZE * 4 * sizeof(mf_TextVertex), sizeof(mf_TextVertex) );
	text_vbo_.IndexData( quad_indeces, MF_MAX_TEXT_BUFFER_SIZE * 6 * sizeof(short) );
	delete[] quad_indeces;

	text_vbo_.VertexAttrib( 0, 2, GL_FLOAT, false, 0 );//vertex coordinates
	text_vbo_.VertexAttrib( 1, 2, GL_UNSIGNED_SHORT, false, sizeof(float)*2 );//texture coordinates
	text_vbo_.VertexAttrib( 2, 4, GL_UNSIGNED_BYTE, true, sizeof(float)*2 + 2*sizeof(short) );//color

	text_shader_.SetAttribLocation( "v", 0 );
	text_shader_.SetAttribLocation( "tc", 1 );
	text_shader_.SetAttribLocation( "c", 2 );
	text_shader_.Create( mf_Shaders::text_shader_v, mf_Shaders::text_shader_f );
	text_shader_.FindUniform( "tex" );
}

mf_Text::~mf_Text()
{
	glDeleteTextures( 1, &font_texture_id_ );
}