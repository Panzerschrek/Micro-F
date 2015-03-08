#pragma once
#include "micro-f.h"

#define MF_MAX_SHADER_ATTRIBS 8
#define MF_MAX_SHADER_ATTRIB_NAME 8
#define MF_MAX_SHADER_UNIFORMS 20
#define MF_MAX_SHADER_UNIFORM_NAME 8

class mf_GLSLProgram
{
public:
	mf_GLSLProgram();
	~mf_GLSLProgram();

	void Bind();

	void Create( const char* vertex_shader, const char* fragment_shader );

	void SetAttribLocation( const char* attrib_name, unsigned int attrib );
	void FindUniform( const char* name );
	void FindUniforms( const char* const* names, unsigned int count );

	void UniformInt ( const char* name, int i );
	void UniformIntArray ( const char* name, unsigned int count, const int* i );
	void UniformMat4( const char* name, const float* mat );
	void UniformMat3( const char* name, const float* mat );
	void UniformMat4Array( const char* name, unsigned int count, const float* mat );
	void UniformMat3Array( const char* name, unsigned int count, const float* mat );
	void UniformVec3 ( const char* name, const float* v );
	void UniformFloat( const char* name, float f );
	void UniformFloatArray( const char* name, unsigned int count, const float* f );

private:
	GLint GetUniformId( const char* name );

private:
	GLuint v_shader_, f_shader_, program_id_;

	GLuint attribs_[ MF_MAX_SHADER_ATTRIBS ];
	char attrib_names_[ MF_MAX_SHADER_ATTRIBS ][ MF_MAX_SHADER_ATTRIB_NAME ];
	unsigned int attrib_count_;

	GLint uniforms_[ MF_MAX_SHADER_UNIFORMS ];
	char uniform_names_[ MF_MAX_SHADER_UNIFORMS ][ MF_MAX_SHADER_UNIFORM_NAME ];
	unsigned int uniform_count_;
};


inline void mf_GLSLProgram::Bind()
{
	glUseProgram( program_id_ );
}
