#include "shaders.h"

/*
common attributes:
"p" - position
"n" - normal
"tc" - texture coordinates
"c" - color

common uniforms names:
"mat" - view matrix ( or view matrix, combined with other transformation )
"nmat" - normal matrix
"smat" - shadow matrix
"tex" - diffuse texture
"sun" - normalized vector of directional light source (sun/moon/etc)
"sl" - color of directional light
"al" - color of ambient light

common input/output names
"fp" - fragment position
"fn" - fragment normal
"ftc" - fragment texture coordinates
"fc" - fragment color
"c_" - output color of fragment shader
*/

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
"out vec4 c_;"
"void main(void)"
"{"
	"float x=texture(tex,ftc).x;"
	"c_=vec4(fc.xyz*x,max(fc.a,x));"
"}"
;

const char* const terrain_shader_v=
"#version 330\n"
"uniform mat4 mat;" // view matrix
"uniform mat4 smat;" // shadow matrix
"uniform sampler2D hm;" // heightmap
"uniform sampler2D nm;" // normal map and texture map
"uniform vec3 sh;" // mesh shift
"uniform float wl;" // water level
"in vec2 p;" // position
"out vec3 fn;" // frag normal
"out vec2 ftc;" // frag tex coord
"out vec3 fstc;" // frag shadowmap tex coord
"out vec2 fp;" // frag position
"void main()"
"{"
	"vec2 sp=p+sh.xy;"
	"fp=sp;"
	"ftc=sp*0.5;"
	"fn=texelFetch(nm,ivec2(sp),0).xyz;"
	"float h=texelFetch(hm,ivec2(sp),0).x;"
	"vec3 p=vec3(sp,h);"
	"gl_Position=mat*vec4(p,1.0);"
	"p=(smat*vec4(p,1.0)).xyz;"
	"fstc=p*0.5+vec3(0.5,0.5,0.5);"
	"gl_ClipDistance[0]=h-wl;"
"}"
;

const char* const terrain_shader_f=
"#version 330\n"
"uniform sampler2DArray tex;" // diffuse texture
"uniform sampler2DShadow stex;" // shadowmap
"uniform sampler2D nm;" // normal map and texture map
"uniform vec3 sun;"
"uniform vec3 sl;" //sun light
"uniform vec3 al;" // ambient light
"in vec3 fn;" // frag normal
"in vec2 ftc;" // texture coord for diffuse texture
"in vec3 fstc;"
"in vec2 fp;"
"out vec4 c_;"
"void main()"
"{"
	"ivec2 ifp=ivec2(fp);"
	"float t00=texelFetch(nm,ifp,0).a*127.1;"
	"float t10=texelFetch(nm,ifp+ivec2(1,0),0).a*127.1;"
	"float t01=texelFetch(nm,ifp+ivec2(0,1),0).a*127.1;"
	"float t11=texelFetch(nm,ifp+ivec2(1,1),0).a*127.1;"
	"vec2 tcmod=mod(fp,vec2(1.0,1.0));"
	"vec4 c00=texture(tex,vec3(ftc,t00));"
	"vec4 c10=texture(tex,vec3(ftc,t10));"
	"vec4 c01=texture(tex,vec3(ftc,t01));"
	"vec4 c11=texture(tex,vec3(ftc,t11));"
	"vec4 cy0=mix(c00,c01,tcmod.y);"
	"vec4 cy1=mix(c10,c11,tcmod.y);"
	"float l=max(0.0,texture(stex,fstc)*dot(sun,normalize(fn)));"
	"c_=vec4(mix(cy0,cy1,tcmod.x).xyz*(l*sl+al),1.0);"
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
"out vec2 fp;" // frag position
"const float om=0.25;" // omega
"void main()"
"{"
	"vec3 p3=vec3(p*tcs.xy,wl);"
	"fp=p3.xy;"
	"p3.z+=(sin(p.x*om+ph)+sin(p.y*om+ph)-2.08)*0.2;" // sub values for prevent reflection artefacts (like sky pixels near coast )
	"fvtc=cp-p3;"
	"gl_Position=mat*vec4(p3,1.0);"
"}"
;

const char* const water_shader_f=
"#version 330\n"
"uniform sampler2D tex;" // reflection texture
"uniform vec3 its;" // invert texture size
"uniform float ph;" // sin phase
"in vec3 fvtc;" // vec to camera
"in vec2 fp;"
"out vec4 c_;" // out color
"void main()"
"{"
	"float s=0.005*sin(fp.x*1.7+ph*2.8);"
	"float c=0.005*cos(fp.y*1.5+ph*2.7);"
	"float a=pow(1.0-normalize(fvtc).z,5.0);"
	"c_=vec4(texture(tex,(gl_FragCoord.xy*its.xy)+vec2(s,c)).xyz*vec3(0.6,0.7,0.8),clamp(a,0.1,0.9));"
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
"uniform mat3 nmat;" // normal matrix
"in vec3 p;" // position
"in vec3 n;" // normal
"in vec2 tc;" // texture coord
"out vec3 fn;" // fragment normal
"out vec2 ftc;" // fragment tex coord
"void main()"
"{"
	"fn=nmat*n;"
	"ftc=tc;"
	"gl_Position=mat*vec4(p,1.0);"
"}";

const char* const models_shader_f=
"#version 330\n"
"uniform sampler2D tex;" // diffuse texture
"uniform vec3 sun;"
"uniform vec3 sl;" //sun light
"uniform vec3 al;" // ambient light
"in vec3 fn;" // fragment normal
"in vec2 ftc;" // fragment tex coord
"out vec4 c_;" // out color
"void main()"
"{"
	"float l= max(0.0,dot(sun,normalize(fn)));"
	"c_=vec4(texture(tex,ftc).xyz*(al+sl*l),0.5);"
"}";

const char* const sun_shader_v=
"#version 330\n"
"uniform mat4 mat;"
"uniform float s;" // sprite size
"void main()"
"{"
	"gl_PointSize=s;"
	"gl_Position=mat*vec4(0.0,0.0,0.0,1.0);"
"}";

const char* const sun_shader_f=
"#version 330\n"
"uniform sampler2D tex;"
"uniform float i;" // sun light intencity
"out vec4 c_;"
"void main()"
"{"
	"vec4 t=texture(tex,gl_PointCoord);"
	"c_=vec4(t.xyz*i,t.a);"
"}";
;

const char* const sky_shader_v=
"#version 330\n"
"uniform mat4 mat;"
"uniform vec3 sun;"
"uniform float sky_k[15];" // koefficents for function
"uniform float tu;" // turbidity
"in vec3 p;"
"out vec3 fc;"

"vec3 perezZenith(float thetaSun)"
"{"
	"const float pi = 3.1415926;"
	"const vec4 cx1=vec4(0,       0.00209, -0.00375, 0.00165 );"
	"const vec4 cx2=vec4(0.00394, -0.03202, 0.06377, -0.02903);"
	"const vec4 cx3=vec4(0.25886, 0.06052, -0.21196, 0.11693 );"
	"const vec4 cy1=vec4(0.0,     0.00317, -0.00610, 0.00275 );"
	"const vec4 cy2=vec4(0.00516, -0.04153, 0.08970, -0.04214);"
	"const vec4 cy3=vec4(0.26688, 0.06670, -0.26756, 0.15346 );"
	"float t2= tu*tu;"
	"float chi=(4.0/9.0 - tu/120.0) * (pi - 2.0*thetaSun);"
	"vec4 theta=vec4(1,thetaSun,thetaSun*thetaSun,thetaSun*thetaSun*thetaSun );"
	"float Y=(4.0453 * tu - 4.9710) * tan(chi) - 0.2155 * tu + 2.4192;"
	"float x=t2*dot(cx1, theta) + tu*dot(cx2, theta) + dot(cx3, theta);"
	"float y=t2*dot(cy1, theta) + tu*dot(cy2, theta) + dot(cy3, theta);"
	"return vec3(Y, x, y);"
"}"
"vec3 perezFunc(float cosTheta, float cosGamma)"
"{"
	"float gamma=acos(cosGamma);"
	"float cosGamma2=cosGamma*cosGamma;"
	"return vec3("
		"(1.0 + sky_k[ 0] * exp(sky_k[ 1]/cosTheta))*(1.0 + sky_k[ 2] * exp(sky_k[ 3]*gamma) + sky_k[ 4]*cosGamma2),"
		"(1.0 + sky_k[ 5] * exp(sky_k[ 6]/cosTheta))*(1.0 + sky_k[ 7] * exp(sky_k[ 8]*gamma) + sky_k[ 9]*cosGamma2),"
		"(1.0 + sky_k[10] * exp(sky_k[11]/cosTheta))*(1.0 + sky_k[12] * exp(sky_k[13]*gamma) + sky_k[14]*cosGamma2));"
"}"
"vec3 perezSky(float cosTheta,float cosGamma,float cosThetaSun)"
"{"
	"float thetaSun= acos(cosThetaSun);"
	"vec3 zenith= perezZenith(thetaSun );"
	"vec3 clrYxy= zenith * perezFunc(cosTheta,cosGamma) / perezFunc(1.0,cosThetaSun);"
	"clrYxy[0]*=smoothstep(0.0,0.05,cosThetaSun);" // make sure when thetaSun > PI/2 we have black color
	"return clrYxy;"
"}"
"void main()"
"{"
	"float cosTheta=max((p.z+0.05)/1.05,0.01);" // poniženije linii gorizonta, t. k. v igre samolöt videt daleko i cutj niže sebä
	"fc=perezSky(cosTheta,dot(sun,p),sun.z);"
	"gl_Position=mat*vec4(p,1.0);"
"}"
;
const char* const sky_shader_f=
"#version 330\n"
"in vec3 fc;"
"out vec4 c_;"
"void main()"
"{"
	"vec3 clrYxy=fc;"
	"clrYxy[0]=1.0 - exp(-clrYxy [0]/25.0);" // now rescale Y component
	"float ratio=clrYxy[0]/clrYxy [2];" // Y / y = X + Y + Z
	"vec3 XYZ;"
	"XYZ.x=clrYxy[1] * ratio;" // X = x * ratio
	"XYZ.y=clrYxy[0];" // Y = Y
	"XYZ.z=ratio - XYZ.x - XYZ.y;" // Z = ratio - X - Y
	"const vec3 rCoeffs=vec3(3.240479, -1.53715, -0.49853 );"
	"const vec3 gCoeffs=vec3(-0.969256, 1.875991, 0.041556);"
	"const vec3 bCoeffs=vec3(0.055684, -0.204043, 1.057311);"
	"c_=vec4(dot(rCoeffs, XYZ), dot(gCoeffs, XYZ), dot(bCoeffs, XYZ), 1.0);"
"}";

} // namespace mf_Shaders