#include "micro-f.h"
#include "vertex_buffer.h"

#define MF_BUFFER_NORT_CREATED 0xffffffff

mf_VertexBuffer::mf_VertexBuffer()
	: vertex_size_(0)
	, index_vbo_(MF_BUFFER_NORT_CREATED), vertex_vbo_(MF_BUFFER_NORT_CREATED), vao_(MF_BUFFER_NORT_CREATED)
	, vertex_count_(0), index_data_size_(0)
{
}

mf_VertexBuffer::~mf_VertexBuffer()
{
}

void mf_VertexBuffer::VertexData( const void* data, unsigned int data_size, unsigned int vertex_size )
{
	if( vao_ == MF_BUFFER_NORT_CREATED )
		glGenVertexArrays( 1, &vao_ );
	glBindVertexArray(vao_);

	if( vertex_vbo_ == MF_BUFFER_NORT_CREATED )
		glGenBuffers( 1, &vertex_vbo_ );

	glBindBuffer( GL_ARRAY_BUFFER, vertex_vbo_ );
	glBufferData( GL_ARRAY_BUFFER, data_size, data, GL_STATIC_DRAW );

	vertex_size_= vertex_size;
	vertex_count_= data_size / vertex_size_;
}

void mf_VertexBuffer::IndexData( const void* data, unsigned int data_size )
{
	if( vao_ == MF_BUFFER_NORT_CREATED )
		glGenVertexArrays( 1, &vao_ );
	glBindVertexArray(vao_);

	if( index_vbo_ == MF_BUFFER_NORT_CREATED )
		glGenBuffers( 1, &index_vbo_ );

	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, index_vbo_ );
	glBufferData( GL_ELEMENT_ARRAY_BUFFER, data_size, data, GL_STATIC_DRAW );

	index_data_size_= data_size;
}

void mf_VertexBuffer::VertexSubData( const void* data, unsigned int data_size, unsigned int shift )
{
	glBindBuffer( GL_ARRAY_BUFFER, vertex_vbo_ );
	glBufferSubData( GL_ARRAY_BUFFER, shift, data_size, data );
}

void mf_VertexBuffer::IndexSubData( const void* data, unsigned int data_size, unsigned int shift )
{
	glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, index_vbo_ );
	glBufferSubData( GL_ELEMENT_ARRAY_BUFFER, shift, data_size, data );
}

void mf_VertexBuffer::VertexAttrib( int attrib, unsigned int components, GLenum type, bool normalized, unsigned int shift )
{
	glVertexAttribPointer( attrib, components, type, normalized, vertex_size_, (void*) shift );
	glEnableVertexAttribArray(attrib);
}

void mf_VertexBuffer::VertexAttribInt( int attrib, unsigned int components, GLenum type, unsigned int shift )
{
	glVertexAttribIPointer( attrib, components, type, vertex_size_, (void*) shift );
	glEnableVertexAttribArray( attrib );
}