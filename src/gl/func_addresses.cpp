#include "../micro-f.h"

/*shaders*/
PFNGLCREATESHADERPROC			glCreateShader= NULL;
PFNGLDELETESHADERPROC			glDeleteShader= NULL;
PFNGLSHADERSOURCEPROC			glShaderSource= NULL;
PFNGLCOMPILESHADERPROC			glCompileShader= NULL;
PFNGLRELEASESHADERCOMPILERPROC	glReleaseShaderCompiler= NULL;
PFNGLSHADERBINARYPROC			glShaderBinary= NULL;
PFNGLGETSHADERIVPROC			glGetShaderiv= NULL;
PFNGLGETSHADERINFOLOGPROC		glGetShaderInfoLog= NULL;

/*programs*/
PFNGLCREATEPROGRAMPROC			glCreateProgram= NULL;
PFNGLDELETEPROGRAMPROC			glDeleteProgram= NULL;
PFNGLDETACHSHADERPROC			glDetachShader= NULL;
PFNGLATTACHSHADERPROC			glAttachShader= NULL;
PFNGLLINKPROGRAMPROC			glLinkProgram= NULL;
PFNGLUSEPROGRAMPROC				glUseProgram= NULL;
PFNGLGETPROGRAMIVPROC			glGetProgramiv= NULL;
PFNGLGETPROGRAMINFOLOGPROC		glGetProgramInfoLog= NULL;
PFNGLGETATTRIBLOCATIONPROC		glGetAttribLocation= NULL;
PFNGLGETUNIFORMBLOCKINDEXPROC	glGetUniformBlockIndex= NULL;
PFNGLUNIFORMBLOCKBINDINGPROC	glUniformBlockBinding= NULL;

/*attributes*/
PFNGLBINDATTRIBLOCATIONPROC			glBindAttribLocation= NULL;
PFNGLVERTEXATTRIBPOINTERPROC		glVertexAttribPointer= NULL;
PFNGLVERTEXATTRIBIPOINTERPROC		glVertexAttribIPointer= NULL;
PFNGLENABLEVERTEXATTRIBARRAYPROC	glEnableVertexAttribArray= NULL;
PFNGLDISABLEVERTEXATTRIBARRAYPROC 	glDisableVertexAttribArray= NULL;

/*VBO*/
PFNGLGENBUFFERSPROC			glGenBuffers= NULL;
PFNGLBINDBUFFERPROC			glBindBuffer= NULL;
PFNGLBINDBUFFERRANGEPROC	glBindBufferRange= NULL;
PFNGLBUFFERDATAPROC			glBufferData= NULL;
PFNGLBUFFERSUBDATAPROC		glBufferSubData= NULL;
PFNGLGENVERTEXARRAYSPROC	glGenVertexArrays= NULL;
PFNGLBINDVERTEXARRAYPROC	glBindVertexArray= NULL;
PFNGLDELETEVERTEXARRAYSPROC	glDeleteVertexArrays= NULL;
PFNGLDELETEBUFFERSPROC		glDeleteBuffers= NULL;
PFNGLMULTIDRAWELEMENTSPROC  glMultiDrawElements= NULL;
PFNGLDRAWELEMENTSBASEVERTEXPROC glDrawElementsBaseVertex= NULL;
PFNGLDRAWELEMENTSINSTANCEDPROC	glDrawElementsInstanced= NULL;
PFNGLMULTIDRAWELEMENTSBASEVERTEXPROC glMultiDrawElementsBaseVertex= NULL;
PFNGLMULTIDRAWARRAYSPROC glMultiDrawArrays= NULL;
PFNGLTEXBUFFERPROC glTexBuffer= NULL;

/*uniforms*/
PFNGLGETUNIFORMLOCATIONPROC	glGetUniformLocation= NULL;
PFNGLUNIFORM1IPROC			glUniform1i= NULL;
PFNGLUNIFORM3FPROC			glUniform3f= NULL;
PFNGLUNIFORM4FPROC			glUniform4f= NULL;
PFNGLUNIFORMMATRIX4FVPROC	glUniformMatrix4fv= NULL;
PFNGLUNIFORMMATRIX3FVPROC	glUniformMatrix3fv= NULL;
PFNGLUNIFORM1FPROC          glUniform1f= NULL;
PFNGLUNIFORM1FVPROC			glUniform1fv= NULL;

/*textures*/
PFNGLACTIVETEXTUREPROC	glActiveTexture= NULL;
PFNGLGENERATEMIPMAPPROC	glGenerateMipmap= NULL;
PFNGLTEXIMAGE3DPROC     glTexImage3D= NULL;
PFNGLTEXSUBIMAGE3DPROC  glTexSubImage3D= NULL;

/*FBO*/
PFNGLGENFRAMEBUFFERSPROC	glGenFramebuffers= NULL;
PFNGLDELETEFRAMEBUFFERSPROC	glDeleteFramebuffers= NULL;
PFNGLBINDFRAMEBUFFERPROC	glBindFramebuffer= NULL;
PFNGLFRAMEBUFFERTEXTUREPROC	glFramebufferTexture= NULL;
PFNGLFRAMEBUFFERTEXTURE2DPROC glFramebufferTexture2D= NULL;
PFNGLFRAMEBUFFERTEXTURELAYERPROC glFramebufferTextureLayer= NULL;
PFNGLBINDFRAGDATALOCATIONPROC	glBindFragDataLocation= NULL;
PFNGLMINSAMPLESHADINGPROC		glMinSampleShading= NULL;
PFNGLDRAWBUFFERSPROC			glDrawBuffers= NULL;
PFNGLSTENCILOPSEPARATEPROC		glStencilOpSeparate= NULL;



PFNGLWAITSYNCPROC glWaitSync= NULL;
PFNGLFENCESYNCPROC glFenceSync= NULL;

//GLAPI void APIENTRY  ( * glClampColor ) (GLenum target, GLenum clamp);

//GLAPI const GLubyte * APIENTRY (*glGetStringi) (GLenum name, GLuint index);

void GetGLFunctions( void* (*GetProcAddressFunc)(const char*) )
{
	// commented lines - currently unused functions

	#define GET_PROC_ADDRESS(x) (GetProcAddressFunc(#x))

	glCreateShader= (PFNGLCREATESHADERPROC) GET_PROC_ADDRESS(glCreateShader);
	//glDeleteShader= (PFNGLDELETESHADERPROC ) GET_PROC_ADDRESS( glDeleteShader );
	glShaderSource= ( PFNGLSHADERSOURCEPROC ) GET_PROC_ADDRESS( glShaderSource );
	glCompileShader= ( PFNGLCOMPILESHADERPROC ) GET_PROC_ADDRESS( glCompileShader );
	//glReleaseShaderCompiler= ( PFNGLRELEASESHADERCOMPILERPROC ) GET_PROC_ADDRESS( glReleaseShaderCompiler );
	glGetShaderiv= ( PFNGLGETSHADERIVPROC )GET_PROC_ADDRESS( glGetShaderiv );
	glGetShaderInfoLog= (PFNGLGETSHADERINFOLOGPROC )GET_PROC_ADDRESS( glGetShaderInfoLog );


	glCreateProgram= (PFNGLCREATEPROGRAMPROC) GET_PROC_ADDRESS( glCreateProgram );
	//glDeleteProgram= ( PFNGLDELETEPROGRAMPROC ) GET_PROC_ADDRESS( glDeleteProgram );
	//glDetachShader=(PFNGLDETACHSHADERPROC) GET_PROC_ADDRESS( glDetachShader );
	glAttachShader=(PFNGLATTACHSHADERPROC) GET_PROC_ADDRESS( glAttachShader );
	glLinkProgram= (PFNGLLINKPROGRAMPROC)	GET_PROC_ADDRESS( glLinkProgram );
	glUseProgram= (PFNGLUSEPROGRAMPROC)		GET_PROC_ADDRESS( glUseProgram );
	glGetProgramiv= (PFNGLGETPROGRAMIVPROC ) GET_PROC_ADDRESS( glGetProgramiv );
	glGetProgramInfoLog= ( PFNGLGETPROGRAMINFOLOGPROC ) GET_PROC_ADDRESS( glGetProgramInfoLog );

	glGetAttribLocation= ( PFNGLGETATTRIBLOCATIONPROC	)GET_PROC_ADDRESS( glGetAttribLocation );
	glVertexAttribPointer= ( PFNGLVERTEXATTRIBPOINTERPROC ) GET_PROC_ADDRESS( glVertexAttribPointer );
	glVertexAttribIPointer= ( PFNGLVERTEXATTRIBIPOINTERPROC )		GET_PROC_ADDRESS( glVertexAttribIPointer );
	glEnableVertexAttribArray= ( PFNGLENABLEVERTEXATTRIBARRAYPROC ) GET_PROC_ADDRESS( glEnableVertexAttribArray );
	glDisableVertexAttribArray= ( PFNGLDISABLEVERTEXATTRIBARRAYPROC )	GET_PROC_ADDRESS( glDisableVertexAttribArray );
	glBindAttribLocation= ( PFNGLBINDATTRIBLOCATIONPROC	) GET_PROC_ADDRESS( glBindAttribLocation );


	glGenBuffers= (PFNGLGENBUFFERSPROC ) GET_PROC_ADDRESS( glGenBuffers );
	glBindBuffer= (PFNGLBINDBUFFERPROC ) GET_PROC_ADDRESS( glBindBuffer );
	glBindBufferRange= (PFNGLBINDBUFFERRANGEPROC) GET_PROC_ADDRESS( glBindBufferRange );
	glBufferData= ( PFNGLBUFFERDATAPROC	) GET_PROC_ADDRESS( glBufferData );
	glBufferSubData= (PFNGLBUFFERSUBDATAPROC ) GET_PROC_ADDRESS( glBufferSubData );
	//glDeleteBuffers= (PFNGLDELETEBUFFERSPROC ) GET_PROC_ADDRESS( glDeleteBuffers );
	//glTexBuffer= ( PFNGLTEXBUFFERPROC ) GET_PROC_ADDRESS( glTexBuffer );

	glGenVertexArrays= (PFNGLGENVERTEXARRAYSPROC ) GET_PROC_ADDRESS( glGenVertexArrays );
	glBindVertexArray= ( PFNGLBINDVERTEXARRAYPROC ) GET_PROC_ADDRESS( glBindVertexArray );
	//glDeleteVertexArrays= ( PFNGLDELETEVERTEXARRAYSPROC	) GET_PROC_ADDRESS( glDeleteVertexArrays );

	//glMultiDrawElements= ( PFNGLMULTIDRAWELEMENTSPROC )  GET_PROC_ADDRESS( glMultiDrawElements );
	//glMultiDrawElementsBaseVertex= (PFNGLMULTIDRAWELEMENTSBASEVERTEXPROC) GET_PROC_ADDRESS( glMultiDrawElementsBaseVertex );
	//glMultiDrawArrays= ( PFNGLMULTIDRAWARRAYSPROC) GET_PROC_ADDRESS( glMultiDrawArrays );
	//glDrawElementsBaseVertex= ( PFNGLDRAWELEMENTSBASEVERTEXPROC ) GET_PROC_ADDRESS( glDrawElementsBaseVertex );

	glGetUniformLocation= (	PFNGLGETUNIFORMLOCATIONPROC	 ) GET_PROC_ADDRESS( glGetUniformLocation );
	glUniform1i= (PFNGLUNIFORM1IPROC ) GET_PROC_ADDRESS( glUniform1i );
	glUniformMatrix4fv=( PFNGLUNIFORMMATRIX4FVPROC ) GET_PROC_ADDRESS( glUniformMatrix4fv );
	glUniformMatrix3fv=( PFNGLUNIFORMMATRIX4FVPROC ) GET_PROC_ADDRESS( glUniformMatrix3fv );
	glUniform3f= (PFNGLUNIFORM3FPROC ) GET_PROC_ADDRESS( glUniform3f );
	glUniform4f= (PFNGLUNIFORM4FPROC ) GET_PROC_ADDRESS( glUniform4f );
	glUniform1f= ( PFNGLUNIFORM1FPROC ) GET_PROC_ADDRESS( glUniform1f );
	glUniform1fv= ( PFNGLUNIFORM1FVPROC ) GET_PROC_ADDRESS( glUniform1fv );
	glGetUniformBlockIndex= (PFNGLGETUNIFORMBLOCKINDEXPROC) GET_PROC_ADDRESS( glGetUniformBlockIndex );
	glUniformBlockBinding= (PFNGLUNIFORMBLOCKBINDINGPROC) GET_PROC_ADDRESS( glUniformBlockBinding );

	glActiveTexture= ( PFNGLACTIVETEXTUREPROC ) GET_PROC_ADDRESS( glActiveTexture );
	glGenerateMipmap= (PFNGLGENERATEMIPMAPPROC	) GET_PROC_ADDRESS( glGenerateMipmap );
	glTexImage3D= ( PFNGLTEXIMAGE3DPROC ) GET_PROC_ADDRESS( glTexImage3D );
	glTexSubImage3D= ( PFNGLTEXSUBIMAGE3DPROC )  GET_PROC_ADDRESS( glTexSubImage3D );

	glGenFramebuffers= ( PFNGLGENFRAMEBUFFERSPROC ) GET_PROC_ADDRESS( glGenFramebuffers );
	glDeleteFramebuffers= ( PFNGLDELETEFRAMEBUFFERSPROC ) GET_PROC_ADDRESS( glDeleteFramebuffers );
	glBindFramebuffer= (PFNGLBINDFRAMEBUFFERPROC ) GET_PROC_ADDRESS( glBindFramebuffer );
	glFramebufferTexture= (PFNGLFRAMEBUFFERTEXTUREPROC) GET_PROC_ADDRESS( glFramebufferTexture );
	glFramebufferTexture2D= (PFNGLFRAMEBUFFERTEXTURE2DPROC) GET_PROC_ADDRESS( glFramebufferTexture2D );
	glFramebufferTextureLayer= (PFNGLFRAMEBUFFERTEXTURELAYERPROC) GET_PROC_ADDRESS( glFramebufferTextureLayer );
	//glBindFragDataLocation= ( PFNGLBINDFRAGDATALOCATIONPROC )	GET_PROC_ADDRESS( glBindFragDataLocation );
	//glMinSampleShading= ( PFNGLMINSAMPLESHADINGPROC	) GET_PROC_ADDRESS( glMinSampleShading );
	glDrawBuffers= ( PFNGLDRAWBUFFERSPROC ) GET_PROC_ADDRESS( glDrawBuffers );

	glStencilOpSeparate= (PFNGLSTENCILOPSEPARATEPROC) GET_PROC_ADDRESS( glStencilOpSeparate );

	glDrawElementsInstanced= ( PFNGLDRAWELEMENTSINSTANCEDPROC )	GET_PROC_ADDRESS( glDrawElementsInstanced );


	//glWaitSync= (PFNGLWAITSYNCPROC)    GET_PROC_ADDRESS( glWaitSync );
	//glFenceSync= (PFNGLFENCESYNCPROC) GET_PROC_ADDRESS( glFenceSync );
}

