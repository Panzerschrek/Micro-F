#include "micro-f.h"
#include "glsl_program.h"

mf_GLSLProgram::mf_GLSLProgram()
	: attrib_count_(0), uniform_count_(0)
{
}

mf_GLSLProgram::~mf_GLSLProgram()
{
}

void mf_GLSLProgram::Create( const char* vertex_shader, const char* fragment_shader )
{
	const char* str[2]= { vertex_shader, 0 };
	int len[2]= { strlen( vertex_shader ), 0 };

	v_shader_= glCreateShader( GL_VERTEX_SHADER );
	glShaderSource( v_shader_, 1, str, len );
	glCompileShader( v_shader_ );
#ifdef MF_DEBUG
	int compile_status;
	len[0]= 1024;
	char build_log[1024];

	glGetShaderiv( v_shader_, GL_COMPILE_STATUS, &compile_status );
	if( !compile_status )
	{
		glGetShaderInfoLog( v_shader_, sizeof(build_log)-1, len, build_log );
		printf( "vertex shader error:\n\n%s\nerrors:\n%s\n", vertex_shader, build_log );
	}
#endif

	if( fragment_shader != NULL )
	{
		str[0]= fragment_shader;
		len[0]= strlen( fragment_shader );

		f_shader_= glCreateShader( GL_FRAGMENT_SHADER );
		glShaderSource( f_shader_, 1, str, len );
		glCompileShader( f_shader_ );
#ifdef MF_DEBUG
		glGetShaderiv( f_shader_, GL_COMPILE_STATUS, &compile_status );
		if( !compile_status )
		{
			glGetShaderInfoLog( f_shader_, sizeof(build_log)-1, len, build_log );
			printf( "fragment shader error:\n\n%s\nerrors:\n%s\n", fragment_shader, build_log );
		}
#endif
	}

	program_id_= glCreateProgram();

	glAttachShader( program_id_, v_shader_ );
	if( fragment_shader != NULL )
		glAttachShader( program_id_, f_shader_ );

	for( unsigned int i= 0; i< attrib_count_; i++ )
		glBindAttribLocation( program_id_, attribs_[i], attrib_names_[i] );

	glLinkProgram( program_id_ );

#ifdef MF_DEBUG
	glGetProgramiv( program_id_, GL_LINK_STATUS, &compile_status );

	if( ! compile_status )
	{
		len[0]= 1024;
		glGetProgramInfoLog( program_id_, sizeof(build_log)-1, len, build_log );
		printf( "shader link error:\n %s\n", build_log );
	}
#endif
}

void mf_GLSLProgram::SetAttribLocation( const char* attrib_name, unsigned int attrib )
{
	strcpy( attrib_names_[ attrib_count_ ], attrib_name );
	attribs_[ attrib_count_ ]= attrib;
	attrib_count_++;
}

void mf_GLSLProgram::FindUniform( const char* uniform )
{
	uniforms_[ uniform_count_ ]= glGetUniformLocation( program_id_, uniform );
	strcpy( uniform_names_[uniform_count_], uniform );
	uniform_count_++;
}

void mf_GLSLProgram::FindUniforms( const char* const* names, unsigned int count )
{
	for( unsigned int i= 0; i< count; i++ )
		FindUniform( names[i] );
}

void mf_GLSLProgram::UniformInt( const char* name, int i )
{
	glUniform1i( GetUniformId(name), i );
}

void mf_GLSLProgram::UniformIntArray( const char* name, unsigned int count, const int* i )
{
	int id= GetUniformId(name);
	for( unsigned int j= 0; j< count; j++ )
		glUniform1i( id + j, i[j] );
}


void mf_GLSLProgram::UniformMat4( const char* name, const float* mat )
{
	glUniformMatrix4fv( GetUniformId(name), 1, GL_FALSE, mat );
}

void mf_GLSLProgram::UniformMat3( const char* name, const float* mat )
{
	glUniformMatrix3fv( GetUniformId(name), 1, GL_FALSE, mat );
}

void mf_GLSLProgram::UniformVec3( const char* name, const float* v )
{
	glUniform3f( GetUniformId(name), v[0], v[1], v[2] );
}

void mf_GLSLProgram::UniformMat4Array( const char* name, unsigned int count, const float* mat )
{
	glUniformMatrix4fv( GetUniformId(name), count, GL_FALSE, mat );
}

void mf_GLSLProgram::UniformMat3Array( const char* name, unsigned int count, const float* mat )
{
	glUniformMatrix3fv( GetUniformId(name), count, GL_FALSE, mat );
}

void mf_GLSLProgram::UniformFloat( const char* name, float f )
{
	glUniform1f( GetUniformId(name), f );
}

void mf_GLSLProgram::UniformFloatArray( const char* name, unsigned int count, const float* f )
{
	glUniform1fv( GetUniformId(name), count, f );
}

GLint mf_GLSLProgram::GetUniformId( const char* name )
{
	for( unsigned int i= 0; i< uniform_count_; i++ )
	{
		if( strcmp( uniform_names_[i], name ) == 0 )
			return uniforms_[i];
	}
	return -1;
}

