#include "shaders.h"

namespace mf_Shaders
{

//text shader
const char* const text_shader_v=
"#version 330\n"
"in vec2 v;"//in position
"in vec2 tc;"//in texture coord
"in vec4 c;"//in color
"out vec4 fc;"//out color
"out vec2 ftc;"//out texture coord
"void main(void)"
"{"
	"fc=c;"
	"ftc=tc*vec2(1.0/96.0,1.0);"
	"gl_Position=vec4(v,-1.0,1.0);"
"}"
;

const char* const text_shader_f=
"#version 330\n"
"uniform sampler2D tex;"
"in vec4 fc;"
"in vec2 ftc;"
"out vec4 oc;"
"void main(void)"
"{"
	"float c=texture2D(tex,ftc).x;"
	"oc=vec4(fc.xyz*c,max(fc.a,c));"
"}"
;

const char* const terrain_shader_v=
"#version 330\n"
"uniform mat4 mat;" // view matrix
"uniform mat4 smat;" // shadow matrix
"uniform sampler2D hm;" // heightmap
"uniform sampler2D nm;" // normal map
"uniform vec3 sh;" // mesh shift
"uniform float wl;" // water level
"in vec2 p;" // position
"out vec3 fn;" // frag normal
"out vec2 ftc;" // frag tex coord
"out vec3 fstc;" // frag shadowmap tex coord
"void main()"
"{"
	"vec2 sp=p+sh.xy;"
	"ftc=sp;"
	"fn=texelFetch(nm,ivec2(sp),0).xyz;"
	"float h=texelFetch(hm,ivec2(sp),0).x;"
	"vec3 p=vec3(sp,h);"
	"gl_Position=mat*vec4(p,1.0);"
	"p=(smat*vec4(p,1.0)).xyz;"
	"fstc=p*0.5+vec3(0.5,0.5,0.5-0.003);"
	"gl_ClipDistance[0]=h-wl;"
"}"
;

const char* const terrain_shader_f=
"#version 330\n"
"uniform sampler2D tex;" // diffuse texture
"uniform sampler2DShadow stex;" // shadowmap
"uniform vec3 sun;"
"uniform vec3 sl;" //sun light
"uniform vec3 al;" // ambient light
"in vec3 fn;" // frag normal
"in vec2 ftc;" // texture coord for diffuse texture
"in vec3 fstc;"
"out vec4 color;"
"void main()"
"{"
	"float l=max(0.0,texture(stex,fstc)*dot(sun,normalize(fn)));"
	"color=vec4(texture(tex,ftc).xyz*(l*sl+al),1.0);"
"}"
;

const char* const terrain_shadowmap_shader_v=
"#version 330\n"
"uniform mat4 mat;" // view matrix
"uniform sampler2D hm;" // heightmap
"uniform vec3 sh;" // mesh shift
"in vec2 p;" // position
"void main()"
"{"
	"vec2 sp=p+sh.xy;"
	"float h=texelFetch(hm,ivec2(sp),0).x;"
	"gl_Position=mat*vec4(sp,h,1.0);"
"}"
;

const char* const water_shader_v=
"#version 330\n"
"uniform mat4 mat;" // view matrix
"uniform float wl;" // water level
"uniform vec3 tcs;" // terrain cell size
"uniform vec3 cp;" // camera position
"uniform float ph;" // sin phase
"in vec2 p;"
"out vec3 fvtc;" // vec to camera
"const float om=0.25;" // omega
"void main()"
"{"
	"vec3 p3=vec3(p*tcs.xy,wl);"
	"p3.z+=(sin(p.x*om+ph)+sin(p.y*om+ph)-2.08)*0.2;" // sub values for prevent reflection artefacts (like sky pixels near coast )
	"fvtc=cp-p3;"
	"gl_Position=mat*vec4(p3,1.0);"
"}"
;

const char* const water_shader_f=
"#version 330\n"
"uniform sampler2D tex;" // reflection texture
"uniform vec3 its;" // invert texture size
"in vec3 fvtc;" // vec to camera
"out vec4 oc;" // out color
"void main()"
"{"
	"float a=pow(1.0-normalize(fvtc).z,5.0);"
	"oc=vec4(texture(tex,gl_FragCoord.xy*its.xy).xyz*vec3(0.6,0.7,0.8),clamp(a,0.1,0.9));"
"}"
;

const char* const water_shadowmap_shader_v=
"#version 330\n"
"uniform mat4 mat;" // view matrix
"uniform float wl;" // water level
"uniform vec3 tcs;" // terrain cell size
"uniform float ph;" // sin phase
"in vec2 p;"
"const float om=0.25;" // omega
"void main()"
"{"
	"vec3 p3=vec3(p*tcs.xy,wl);"
	"p3.z+=(sin(p.x*om+ph)+sin(p.y*om+ph)-2.08)*0.2;" // sub values for prevent reflection artefacts (like sky pixels near coast )
	"gl_Position=mat*vec4(p3,1.0);"
"}"
;


const char* const models_shader_v=
"#version 330\n"
"uniform mat4 mat;" // view matrix
"uniform vec3 ms;" // model scale
"uniform vec3 mp;" // model position
"in vec3 p;" // position
"in vec3 n;" // normal
"in vec2 tc;" // texture coord
"out vec3 fn;" // fragment normal
"void main()"
"{"
	"fn=n;"
	"vec3 pos=p*ms+mp;"
	"gl_Position=mat*vec4(pos,1.0);"
"}";

const char* const models_shader_f=
"#version 330\n"
"in vec3 fn;" // fragment normal
"out vec4 oc;" // out color
"const vec3 sun=vec3(0.5,0.5, 0.5);"
"void main()"
"{"
	"float l= max(0.0,dot(sun,normalize(fn)));"
	"l+=0.3;"
	"oc=vec4(fn*0.5+vec3(0.5,0.5,0.5),0.5);"
"}";


} // namespace mf_Shaders