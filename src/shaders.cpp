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
"texn" - number of layer in texture array
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
"void main()"
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
"void main()"
"{"
	"float x=texture(tex,ftc).x;"
	"c_=vec4(fc.xyz*x,max(fc.a,x));"
"}"
;

const char* const naviball_icon_shader_v=
"#version 330\n"
"uniform vec3 p;" // position
"uniform float ps;" // point size
"void main()"
"{"
	"gl_PointSize=ps;"
	"gl_Position=vec4(p.xy,0.0,1.0);"
"}";

const char* const naviball_icon_shader_f=
"#version 330\n"
"uniform sampler2DArray tex;"
"uniform vec3 c;" // input color
"uniform float tn;" // point size
"out vec4 c_;"
"void main()"
"{"
	"float x= texture(tex,vec3(gl_PointCoord.x, 1.0-gl_PointCoord.y,tn)).x;"
	"if(x<0.1)discard;"
	"c_=vec4(c,x);"
"}";

const char* const gui_shader_v=
"#version 330\n"
"in vec2 p;"
"in vec3 tc;"
"out vec3 ftc;"
"void main()"
"{"
	"ftc=tc;"
	"gl_Position=vec4(p,0.0,1.0);"
"}";

const char* const gui_shader_f=
"#version 330\n"
"uniform sampler2D tex[6];"
"in vec3 ftc;"
"out vec4 c_;"
"void main()"
"{"
	"c_=texture(tex[uint(ftc.z)],ftc.xy);"
"}";

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
	"ftc=sp*0.333;"
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
	"c_=vec4(texture(tex,(gl_FragCoord.xy*its.xy)+vec2(s,c)).xyz,clamp(a,0.1,0.9));"
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

const char* const aircrafts_shader_v=
"#version 330\n"
"uniform mat4 mat;" // view matrix
"uniform mat3 nmat;" // normal matrix
"uniform mat4 mmat;" // model matrix - for convertion of model to world space
"uniform float texn;" // texture number ( in array of textures )
"in vec3 p;" // position
"in vec3 n;" // normal
"in vec2 tc;" // texture coord
"out vec3 fp;"
"out vec3 fn;" // fragment normal
"out vec3 ftc;" // fragment tex coord
"void main()"
"{"
	"fn=nmat*n;"
	"ftc=vec3(tc,texn);"
	"fp=(mmat*vec4(p,1.0)).xyz;"
	"gl_Position=mat*vec4(p,1.0);"
"}";

const char* const aircrafts_shader_f=
"#version 330\n"
"uniform sampler2DArray tex;" // diffuse texture
"uniform vec3 sun;"
"uniform vec3 sl;" //sun light
"uniform vec3 al;" // ambient light
"in vec3 fp;" // fragment position
"in vec3 fn;" // fragment normal
"in vec3 ftc;" // fragment tex coord
"out vec4 c_;" // out color
"void main()"
"{"
	"vec4 texc=texture(tex,ftc);"
	"vec3 nn=normalize(fn);"
	"float ldot=max(0.0,dot(sun,nn));"
	"float l=ldot;"
	"l+=step(0.0,ldot)*texc.a*4.0*pow(max(dot(reflect(normalize(fp),nn),sun),0.01),texc.a*255.0);"
	"c_=vec4(texc.xyz*(al+sl*l),0.5);"
"}";

const char* const aircrafts_shadowmap_shader_v=
"#version 330\n"
"uniform mat4 mat;" // view matrix
"in vec3 p;" // position
"void main()"
"{"
	"gl_Position=mat*vec4(p,1.0);"
"}";

const char* const aircrafts_stencil_shadow_shader_v=
"#version 330\n"
"in vec3 p;" // position
"void main()"
"{"
	"gl_Position=vec4(p,1.0);"
"}";

const char* const aircrafts_stencil_shadow_shader_g=
"#version 330\n"
"layout(triangles,invocations=1)in;"
"layout(triangle_strip,max_vertices=24)out;"
"uniform mat4 mat;"
"uniform vec3 sun;"
"const uint ind[24]=uint[24]"
"("
	"0,1,2," "5,4,3," // src triangle and back triangle
	"0,3,1," "1,3,4,"
	"1,4,2," "2,4,5,"
	"0,2,5," "0,5,3"
");"
"void main()"
"{"
	"vec4 sun4=vec4(sun,0.0);"
	"vec4 ov[6];" // out vertices positions
	"int i;"
	"for(i=0;i<3;i++)"
	"{"
		"ov[i]=mat*(gl_in[i].gl_Position-sun4*0.05);"
		"ov[i+3]=mat*(gl_in[i].gl_Position-sun4*10.0);"
	"}"
	"vec3 v0=(gl_in[1].gl_Position-gl_in[0].gl_Position).xyz;"
	"vec3 v1=(gl_in[2].gl_Position-gl_in[0].gl_Position).xyz;"
	"if(dot(cross(v0,v1),sun)>0.0)"
		"for(i=0;i<24;i+=3)"
		"{"
			"gl_Position=ov[ind[i]];EmitVertex();"
			"gl_Position=ov[ind[i+1]];EmitVertex();"
			"gl_Position=ov[ind[i+2]];EmitVertex();"
			"EndPrimitive();"
		"}"
"}";

const char* const static_models_shader_v=
"#version 330\n"
"layout(std140)uniform mat_block"
"{"
	"mat4 mat[256];" // 128 pairs of matrices. First matrix in pair - view matrix, second - shadow matrix.
"};"
"in vec3 p;" // position
"in vec3 n;" // normal
"in vec2 tc;" // texture coord
"out vec3 fstc;" // fragment shadow tex coord
"out vec3 fn;" // fragment normal
"out vec3 ftc;" // fragment tex coord
"out float fiid;" // fragment instance id
"void main()"
"{"
	"fn=n;"
	"fiid=float(gl_InstanceID)+0.1;"
	"ftc=vec3(tc,tc.x/16.0);"
	"vec3 sp=(mat[gl_InstanceID*2+1]*vec4(p,1.0)).xyz;"
	"fstc=sp*0.5+vec3(0.5,0.5,0.5);"
	"gl_Position=mat[gl_InstanceID*2]*vec4(p,1.0);"
"}";

const char* const static_models_shader_f=
"#version 330\n"
"uniform sampler2DArray tex;" // diffuse texture
"uniform sampler2DShadow stex;" // shadowmap
"layout(std140)uniform sun_block"
"{"
	"vec4 sun[128];" // model space sun
"};"
"uniform vec3 sl;" //sun light
"uniform vec3 al;" // ambient light
"in vec3 fstc;" // fragment shadow tex coord
"in vec3 fn;" // fragment normal
"in vec3 ftc;" // fragment tex coord
"in float fiid;" // fragment instance id
"out vec4 c_;" // out color
"void main()"
"{"
	"vec4 texc=texture(tex,ftc);"
	"if(texc.a<0.5)discard;"
	"float l= max(0.0,dot(sun[uint(fiid)].xyz,normalize(fn)))*texture(stex,fstc);"
	"c_=vec4(texc.xyz*(al+sl*l),0.5);"
"}";

const char* const static_models_shadowmap_shader_v=
"#version 330\n"
"layout(std140)uniform mat_block"
"{"
	"mat4 mat[128 + 128];" // view matrix. last 128 - unused
"};"
"in vec3 p;" // position
"in vec2 tc;" // texture coord
"out vec3 ftc;" // fragment tex coord
"void main()"
"{"
	"ftc=vec3(tc,tc.x/16.0);"
	"gl_Position=mat[gl_InstanceID]*vec4(p,1.0);"
"}";

const char* const static_models_shadowmap_shader_f=
"#version 330\n"
"uniform sampler2DArray tex;" // diffuse texture
"in vec3 ftc;" // fragment tex coord
"out vec4 c_;" // out color
"void main()"
"{"
	"if(texture(tex,ftc).a<0.5)discard;"
	"c_=vec4(0.0,0.0,0.0,0.5);"
"}";

const char* const naviball_shader_v=
"#version 330\n"
"uniform mat4 mat;"
"in vec3 p;" // position
"in vec2 tc;" // texture coord
"out vec2 ftc;" // fragment tex coord
"void main()"
"{"
	"ftc=tc;"
	"gl_Position=mat*vec4(p,1.0);"
"}";

const char* const naviball_shader_f=
"#version 330\n"
"uniform sampler2D tex;"
"in vec2 ftc;" // texture coord
"out vec4 c_;" // out color
"void main()"
"{"
	"c_=texture(tex,ftc);"
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
	"clrYxy[0]*=smoothstep(0.0,0.01,cosThetaSun);" // make sure when thetaSun > PI/2 we have black color
	"return clrYxy;"
"}"
"void main()"
"{"
	"float cosTheta=max((p.z+0.01)/1.01,0.01);" // poniženije linii gorizonta, t. k. v igre samolöt videt daleko i cutj niže sebä
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
	"%s" // here was luminance correction function, defined in outer code
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


const char* const stars_shader_v=
"#version 330\n"
"uniform mat4 mat;"
"uniform float s;" // point size
"in vec3 p;" // position
"in float i;" // intensity
"out float fi;" // frag intensity
"void main()"
"{"
	"gl_PointSize=s;"
	"fi=i;"
	"gl_Position=mat*vec4(p,1.0);"
"}";

const char* const stars_shader_f=
"#version 330\n"
"out vec4 c_;"
"in float fi;"
"void main()"
"{"
	"vec2 rv=gl_PointCoord-vec2(0.5,0.5);"
	"float a=min(1.0,0.25-dot(rv,rv));"
	"c_=vec4(fi,fi,fi,a);"
"}";

const char* const particles_shader_v=
"#version 330\n"
"uniform mat4 mat;"
"uniform float s;" // point size
"in vec4 p;" // position + size
"in float i;" // intensity
"in vec4 c;" // transparency_multipler__backgound_multipler__texture_id__reserved
"out float fi;" // frag intensity
"out float fbm;" // backgound_multipler
"void main()"
"{"
	"fi=i;"
	"fbm= c[1];"
	"vec4 p4=mat*vec4(p.xyz,1.0);"
	"gl_Position=p4;"
	"gl_PointSize=p.w*s/p4.w;"
"}";

const char* const particles_shader_f=
"#version 330\n"
"out vec4 c_;"
"in float fi;"
"in float fbm;" // backgound_multipler
"void main()"
"{"
	"vec2 rv=gl_PointCoord-vec2(0.5,0.5);"
	"float a=1.0-clamp(length(rv)*2.0,0.0,1.0);" // 1 - center, 0 - border
	"vec3 c= vec3(0.9,0.9,0.9) * 0.5 * a * fbm;" // diffuse color * light
	"c+=vec3(1.0,0.9,0.5) * (fi*a);"
	"c_=vec4(c,1.0-a * fbm);"
"}";

const char* const tonemapping_shader_v=
"#version 330\n"
"uniform sampler2D btex;"
"uniform int bhn;" // current brightness history value
"const vec2 coord[6]=vec2[6]"
"("
	"vec2(0.0,0.0),vec2(1.0,0.0),vec2(1.0,1.0),"
	"vec2(0.0,0.0),vec2(0.0,1.0),vec2(1.0,1.0)"
");"
"noperspective out vec2 ftc;"
"noperspective out float b;"
"void main()"
"{"
	"float bsum=0.0;"
	"int ts=textureSize(btex,0).x;"
	"for(int i=0; i< ts; i++)"
	"bsum+=float(ts-i)*texelFetch(btex,ivec2((bhn-i)%ts,0),0).x;"
	"bsum=bsum/float(ts*ts/2);"
	"b=log(0.5)*clamp(1.0/bsum,0.0005,20.0);"
	"ftc=coord[gl_VertexID];"
	"gl_Position=vec4(coord[gl_VertexID]*2.0-vec2(1.0,1.0),0.0,1.0);"
"}";

const char* const tonemapping_shader_f=
"#version 330\n"
"uniform sampler2D tex;"
"uniform float ck;" // color k - coefficent for final color calculation
"noperspective in vec2 ftc;"
"noperspective in float b;"
"out vec4 c_;"
"void main()"
"{"
	"vec3 c= texture(tex,ftc).xyz;"
	"c=vec3(1.0,1.0,1.0)-exp(c*b);"
	"c_=vec4(c,1.0);"
"}";

const char* const brightness_fetch_shader_v=
"#version 330\n"
"const vec2 coord[6]=vec2[6]"
"("
	"vec2(0.0,0.0),vec2(1.0,0.0),vec2(1.0,1.0),"
	"vec2(0.0,0.0),vec2(0.0,1.0),vec2(1.0,1.0)"
");"
"out vec2 ftc;"
"void main()"
"{"
	"ftc=coord[gl_VertexID];"
	"gl_Position=vec4(coord[gl_VertexID]*2.0-vec2(1.0,1.0),0.0,1.0);"
"}";

const char* const brightness_fetch_shader_f=
"#version 330\n"
"uniform sampler2D tex;"
"in vec2 ftc;"
"out vec4 c_;"
"void main()"
"{"
	"vec3 c= texture(tex,ftc).xyz;"
	"c_=vec4((c.x+c.y+c.z)*0.33333,0.0,0.0,1.0);"
"}";

const char* const brightness_history_write_shader_v=
"#version 330\n"
"uniform float p;" // position to write
"void main()"
"{"
	"gl_Position=vec4(p,0.0,0.0,1.0);"
"}";

const char* const brightness_history_write_shader_f=
"#version 330\n"
"uniform sampler2D tex;" // input lowres texture with scene brightness
"out vec4 c_;"
"void main()"
"{"
	"c_=texelFetch(tex,ivec2(0,0),6);"
"}";

} // namespace mf_Shaders