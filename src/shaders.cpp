#include "shaders.h"

#define MF_GPSM_ADD_K 0.4
#define MF_GPSM_MUL_K (MF_GPSM_ADD_K+1.0)

#define MF_MACRO_TO_STR(X) #X
#define MF_MACRO_VALUE_TO_STR(X) MF_MACRO_TO_STR(X)
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
"uniform vec3 c;"
"in vec3 ftc;"
"out vec4 c_;"
"void main()"
"{"
	"c_=texture(tex[uint(ftc.z)],ftc.xy)*vec4(c,1.0);"
"}";

const char* const machinegun_circle_shader_v=
"#version 330\n"
"uniform float tpidis;" // 2 * pi / segment_count
"uniform vec3 r;" // x - x scaler, y - y scaler, z - circle width
"void main()"
"{"
	"float a=tpidis*float(gl_VertexID/2);"
	"float rs=1.0+r.z*float(gl_VertexID&1);"
	"gl_Position=vec4(cos(a)*r.x*rs,sin(a)*r.y*rs,0.0,1.0);"
"}";

const char* const machinegun_circle_shader_f=
"#version 330\n"
"out vec4 c_;"
"void main()"
"{"
	"c_=vec4(0.5,1.0,0.5,0.5);"
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
"out float fwdk;" // fragment water depth k
"void main()"
"{"
	"vec2 sp=p+sh.xy;"
	"fp=sp;"
	"ftc=sp*0.333;"
	"fn=texelFetch(nm,ivec2(sp),0).xyz;"
	"float h=texelFetch(hm,ivec2(sp),0).x;"
	"vec3 p=vec3(sp,h);"
	"gl_Position=mat*vec4(p,1.0);"
	"fwdk=smoothstep(0.5*wl,wl,p.z);"
	"p=(smat*vec4(p,1.0)).xyz;"
	"p.xy=p.xy*" MF_MACRO_VALUE_TO_STR(MF_GPSM_MUL_K) "/(abs(p.xy)+" MF_MACRO_VALUE_TO_STR(MF_GPSM_ADD_K) ");"
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
"in float fwdk;" // fragment water depth k
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
	"float lk=fwdk*fwdk;" // underwater fog factor
	"c_=vec4(mix(cy0,cy1,tcmod.x).xyz*(lk*l*sl+al+(1.0-lk)*vec3(0.1,0.1,0.3)),1.0);"
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
	"vec4 s=mat*vec4(sp,h,1.0);"
	"s.xy=s.xy*" MF_MACRO_VALUE_TO_STR(MF_GPSM_MUL_K) "/(abs(s.xy)+" MF_MACRO_VALUE_TO_STR(MF_GPSM_ADD_K) ");"
	"gl_Position=s;"
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
	"fp=0.0625*p3.xy;"
	"p3.z+=(sin(p.x*om+ph)+sin(p.y*om+ph)-2.9)*0.2;" // sub values for prevent reflection artefacts (like sky pixels near coast )
	"fvtc=cp-p3;"
	"gl_Position=mat*vec4(p3,1.0);"
"}"
;

const char* const water_shader_f=
"#version 330\n"
"uniform sampler2D tex;" // reflection texture
"uniform sampler3D ntex;" // normalmap
"uniform vec3 its;" // invert texture size
"uniform float ph;" // sin phase
"in vec3 fvtc;" // vec to camera
"in vec2 fp;"
"out vec4 c_;" // out color
"void main()"
"{"
	"vec4 t=0.2*(texture(ntex,vec3(fp,ph))+2.0*texture(ntex,vec3(fp*0.5,0.5*ph))+4.0*texture(ntex,vec3(fp*0.25,0.25*ph)));"
	"vec3 n=normalize(t.xyz*2.0-vec3(1.0,1.0,1.0));"
	"float ndot=max(0.0,normalize(fvtc).z);"
	"float l=0.4*inversesqrt(length(fvtc));"
	"float s=min(0.5,n.x*l);"
	"float c=min(0.5,ndot*n.y*l);"
	"float a=pow(1.0-ndot,5.0);"
	"c_=vec4(texture(tex,(gl_FragCoord.xy*its.xy)+vec2(s,c)).xyz,clamp(a,0.1,0.9));"
"}"
;

/*const char* const water_shadowmap_shader_v=
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
;*/

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
	"vec4 s=mat*vec4(p,1.0);"
	"s.xy=s.xy*" MF_MACRO_VALUE_TO_STR(MF_GPSM_MUL_K) "/(abs(s.xy)+" MF_MACRO_VALUE_TO_STR(MF_GPSM_ADD_K) ");"
	"gl_Position=s;"
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
"#extension GL_ARB_gpu_shader5 : require\n"
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
	"sp.xy=sp.xy*" MF_MACRO_VALUE_TO_STR(MF_GPSM_MUL_K) "/(abs(sp.xy)+" MF_MACRO_VALUE_TO_STR(MF_GPSM_ADD_K) ");"
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
	"vec4 s=mat[gl_InstanceID]*vec4(p,1.0);"
	"s.xy=s.xy*" MF_MACRO_VALUE_TO_STR(MF_GPSM_MUL_K) "/(abs(s.xy)+" MF_MACRO_VALUE_TO_STR(MF_GPSM_ADD_K) ");"
	"gl_Position=s;"
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

const char* const powerups_shader_v=
"#version 330\n"
"uniform mat4 mat;" // view matrix
"uniform mat3 nmat;" // normal matrix
"uniform mat4 mmat;" // model matrix
"in vec3 p;"
"in vec3 n;"
"in vec2 tc;"
"out vec3 fp;" // fragmnet position relative camera
"out vec3 fn;" // fragment normal
"void main()"
"{"
	"fp=(mmat*vec4(p,1.0)).xyz;"
	"fn=nmat*n;"
	"gl_Position=mat*vec4(p,1.0);"
"}";

const char* const powerups_shader_f=
"#version 330\n"
"uniform vec3 c;" // color of powerup
"uniform vec3 sun;" // normalized vector to sun
"uniform vec3 sl;" // sun light
"uniform vec3 alcube[6];" // ambient light cube
"uniform float trans;" // transparency of powerup
"in vec3 fp;" // fragment world position
"in vec3 fn;" // fragment normal
"out vec4 c_;" // out color
"void main()"
"{"
	"vec3 nn=normalize(fn);"
	"float ldot=max(dot(sun,nn),0.0);"
	"vec3 ref=reflect(normalize(fp),nn);"
	"float l=ldot+step(0.01,ldot)*8.0*pow(max(dot(ref,sun),0.01),32.0);"
	// make fetch from ambient light cube
	"vec3 side_step=step(0.0,ref);"
	"ref*=ref;"
	"vec3 als="
		"ref.x*mix(alcube[1],alcube[0],side_step.x)+"
		"ref.y*mix(alcube[3],alcube[2],side_step.y)+"
		"ref.z*mix(alcube[5],alcube[4],side_step.z);"
	"c_=vec4(c*(als+sl*l),trans);"
"}";

const char* const forcefield_shader_v=
"#version 330\n"
"uniform mat4 mat;"
"uniform float iffh;" // inverted forcefield height
"in vec3 p;"
"in vec2 tc;"
"out vec3 fp;"
"out vec2 ftc;"
"out float fhk;"
"void main()"
"{"
	"fp=p;"
	"float hk=1.0-0.3*p.z*iffh;"
	"fhk=hk*hk;"
	"ftc=tc;"
	"gl_Position=mat*vec4(p,1.0);"
"}";

const char* const forcefield_shader_f=
"#version 330\n"
"uniform sampler2D tex;"
"uniform vec3 aircp;" // aircraft position
"in vec3 fp;" // fragment world position
"in vec2 ftc;" // texture coord
"in float fhk;"
"out vec4 c_;" // out color
"void main()"
"{"
	"vec3 v=fp-aircp;"
	"float id=1.0/dot(v,v);"
	"c_=fhk*clamp(256*id,0.7,1.0)*texture(tex,ftc)+min(id*128,0.7)*vec4(1.0,0.5,0.5,0.0);"
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
"uniform float a;" // angle of rotation
"out vec4 c_;"
"void main()"
"{"
	"vec2 tc=gl_PointCoord-vec2(0.5,0.5);"
	"tc=vec2( tc.x*cos(a)-tc.y*sin(a),tc.x*sin(a)+tc.y*cos(a));"
	"vec4 t=texture(tex,tc+vec2(0.5,0.5));"
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

const char* const clouds_shader_v=
"#version 330\n"
"uniform mat4 mat;"
"in vec3 p;"
"out vec3 fp;"
"void main()"
"{"
	"fp=p;"
	"gl_Position=mat*vec4(p,1.0);"
"}";

const char* const clouds_shader_f=
"#version 330\n"
"uniform samplerCube tex;"
"uniform float l;"
"in vec3 fp;"
"out vec4 c_;"
"void main()"
"{"
	"vec4 c=texture(tex,vec3(fp.xy,fp.z*0.95+0.05));"
	"c_=vec4(l*c.xyz,c.a*smoothstep(0.05,0.15,fp.z));"
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
	"gl_PointSize=s*(1.0+i);"
	"fi=i*2.0;"
	"gl_Position=mat*vec4(p,1.0);"
"}";

const char* const stars_shader_f=
"#version 330\n"
"in float fi;"
"out vec4 c_;"
"void main()"
"{"
	"vec2 rv=gl_PointCoord-vec2(0.5,0.5);"
	"float a=min(1.0,0.25-dot(rv,rv));"
	"c_=vec4(fi,fi,fi,a);"
"}";

const char* const particles_shader_v=
"#version 330\n"
"uniform mat4 mat;" // view matrix
"uniform mat4 smat;" // shadowmap
"uniform sampler2DShadow stex;"
"uniform float s;" // point size
"in vec4 p;" // position + size
"in float i;" // intensity
"in vec4 c;" // transparency, diffuse texture, luminance texture
"out float fi;" // frag intensity
"out float fbm;" // backgound_multipler
"out float fsl;" // sun light intencity
"flat out vec2 ftn;" // frag texture number
"void main()"
"{"
	"fi=i;"
	"ftn=c.yz*255.1;"
	"fbm= c[0];"
	"vec4 p4=mat*vec4(p.xyz,1.0);"
	"gl_Position=p4;"
	"vec3 sp=(smat*vec4(p.xyz,1.0)).xyz;"
	"sp.xy=sp.xy*" MF_MACRO_VALUE_TO_STR(MF_GPSM_MUL_K) "/(abs(sp.xy)+" MF_MACRO_VALUE_TO_STR(MF_GPSM_ADD_K) ");"
	"fsl=textureLod(stex,sp*0.5+vec3(0.5,0.5,0.5),0.0);"
	"gl_PointSize=p.w*s/p4.w;"
"}";

const char* const particles_shader_f=
"#version 330\n"
"uniform sampler2DArray tex;"
"uniform vec3 sl;" // sun light intencity
"uniform vec3 al;" // ambient light intencity
"in float fi;"
"in float fbm;" // backgound_multipler
"in float fsl;" // sun light
"flat in vec2 ftn;" // frag texture number
"out vec4 c_;"
"void main()"
"{"
	"vec4 dtex=texture(tex,vec3(gl_PointCoord,ftn.x));" // diffuse texture
	"vec4 ltex=texture(tex,vec3(gl_PointCoord,ftn.y));" // luminance texture
	"vec3 l=fsl*sl+al;"
	"vec3 c=dtex.xyz*l*(dtex.a*fbm);" // diffuse color * light
	"c+=ltex.xyz*(fi*ltex.a);" // self-luminanse
	"c_=vec4(c,1.0-dtex.a*fbm);"
"}";

const char* const tonemapping_shader_v=
"#version 330\n"
"uniform sampler2D btex;"
"uniform float ck;"
"const vec2 coord[6]=vec2[6]"
"("
	"vec2(0.0,0.0),vec2(1.0,0.0),vec2(1.0,1.0),"
	"vec2(0.0,0.0),vec2(0.0,1.0),vec2(1.0,1.0)"
");"
"noperspective out vec2 ftc;"
"noperspective out float b;"
"void main()"
"{"
	"b=ck*log(0.5)*texelFetch(btex,ivec2(0,0),0).x;"
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

const char* const histogram_fetch_shader_v=
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

const char* const histogram_fetch_shader_f=
"#version 330\n"
"uniform sampler2D tex;"
"uniform vec4 pin[2];" // min and max values of pins
"in vec2 ftc;"
"out vec4 c_;"
"void main()"
"{"
	"float c=dot(texture(tex,ftc).xyz,vec3(0.3333,0.3333,0.3333));"
	"vec4 cv= vec4(c,c,c,c);"
	"c_=step(pin[0],cv)*step(cv,pin[1]);"
"}";

const char* const histogram_write_shader_v=
"#version 330\n"
"uniform float p;" // position to write
"void main()"
"{"
	"gl_Position=vec4(p,0.0,0.0,1.0);"
"}";

const char* const histogram_write_shader_f=
"#version 330\n"
"uniform sampler2D tex;" // input texture with histogram from one frame
"out vec4 c_;"
"void main()"
"{"
	"c_=texelFetch(tex,ivec2(0,0),7);"
"}";

const char* const brightness_computing_shader_v=
"#version 330\n"
"void main()"
"{"
	"gl_Position=vec4(0.0,0.0,0.0,1.0);"
"}";

const char* const brightness_computing_shader_f=
"#version 330\n"
"uniform sampler2D tex;" // input texture with full histogram
"const int HISTOGRAM_BINS= 20;" // if it changed, it must be changed int renderer.h too
"uniform float pins[HISTOGRAM_BINS];" // borders for pins
"out vec4 c_;"
"void main()"
"{"
	"const vec4 mask[4]=vec4[4]"
	"("
		"vec4(1.0,0.0,0.0,0.0),"
		"vec4(0.0,1.0,0.0,0.0),"
		"vec4(0.0,0.0,1.0,0.0),"
		"vec4(0.0,0.0,0.0,1.0)"
	");"
	"float full_sum= pins[0]*texelFetch(tex,ivec2(0,0),0).x;" // full_sum - is avg brightness
	"for(int i=1;i<HISTOGRAM_BINS;i++)"
		"full_sum+=(pins[i]-pins[i-1])*dot(texelFetch(tex,ivec2(i/4,0),0),mask[i&3]);"
	"float portion_sum=pins[0]/full_sum * texelFetch(tex,ivec2(0,0),0).x;"
	"float b95=pins[HISTOGRAM_BINS-1];"
	"for(int i=1;i<HISTOGRAM_BINS;i++)"
	"{"
		"float sum_add=(pins[i]-pins[i-1])/full_sum*dot(texelFetch(tex,ivec2(i/4,0),0),mask[i&3]);"
		"if(sum_add+portion_sum >=0.95)"
		"{"
			"b95=mix(pins[i-1],pins[i],(0.95-portion_sum)/sum_add);"
			"break;"
		"}"
		"portion_sum+= sum_add;"
	"}"
	"float k=1.0/min(b95,full_sum*4.0);"
	"c_=vec4(k,k,k,k);"
"}";

const char* const tonemapping_factor_accumulate_shader_v=
"#version 330\n"
"void main()"
"{"
	"gl_Position=vec4(0.0,0.0,0.0,1.0);"
"}";

const char* const tonemapping_factor_accumulate_shader_f=
"#version 330\n"
"uniform sampler2D tex;"
"uniform float a;"
"out vec4 c_;"
"void main()"
"{"
	"c_=vec4(texelFetch(tex,ivec2(0,0),0).xyz,a);"
"}";

#ifdef MF_DEBUG
const char* const histogram_show_shader_v=
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
	"gl_Position=vec4(coord[gl_VertexID]*0.5 + vec2(0.5,0.5),0.0,1.0);"
"}";

const char* const histogram_show_shader_f=
"#version 330\n"
"uniform sampler2D tex;" // input texture with full histogram
"in vec2 ftc;"
"out vec4 c_;"
"void main()"
"{"
	"int ts=textureSize(tex,0).x;"
	"int tcx= int(float(ts)*4.0*pow(ftc.x,1.0/1.5));"
	"const vec4 component_mask[4]= vec4[4]"
	"("
		"vec4(1.0,0.0,0.0,0.0),"
		"vec4(0.0,1.0,0.0,0.0),"
		"vec4(0.0,0.0,1.0,0.0),"
		"vec4(0.0,0.0,0.0,1.0)"
	");"
	"float c=dot(texelFetch(tex,ivec2(tcx/4,0),0),component_mask[tcx&3]);"
	"c=1.0-step(c,ftc.y);"
	"c_=c*0.5*(component_mask[tcx&3]+vec4(1.0,1.0,1.0,1.0));"
"}";

const char* const histogram_buffer_show_shader_v=
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
	"gl_Position=vec4(coord[gl_VertexID]*0.5 + vec2(0.5,0.0),0.0,1.0);"
"}";

const char* const histogram_buffer_show_shader_f=
"#version 330\n"
"uniform sampler2D tex;"
"in vec2 ftc;"
"out vec4 c_;"
"void main()"
"{"
"c_=texture(tex,ftc);"
"}";
#endif // MF_DEBUG

const char* const clouds_gen_shader_v=
"#version 330\n"
"uniform mat4 mat;"
"in vec3 p;"
"in vec2 tc;"
"out vec3 fp;" // position
"void main()"
"{"
	"fp=p;"
	"gl_Position=mat*vec4(tc*2.0-vec2(1.0,1.0),1.0,1.0);"
"}";

const char* const clouds_gen_shader_f=
"#version 330\n"
"uniform vec3 sun;"
"uniform vec3 sl;"
"uniform vec3 al;"
"uniform vec3 inv_tex_size;"
"uniform float horizont;"
"in vec3 fp;" // frag position
"out vec4 c_;"

"const int seed=%d;"
// Noise3D
"int N3(int x,int y,int z)" // returns value in range [0;65536)
"{"
	//"const int X_NOISE_GEN = 1;"
	//"const int Y_NOISE_GEN = 31337;"
	//"const int Z_NOISE_GEN = 263;"
	//"const int SEED_NOISE_GEN = 1013;"
	"int n=("
		"x+"
		"31337*y+"
		"263*z+"
		"1013*seed)"
		"&0x7fffffff;"
		"n=(n>>13)^n;"
		"return((n*(n*n*60493+19990303)+1376312589)&0x7fffffff)>>16;"
"}"

"float N3I(vec3 xyz,int k)" // Noise3D interpolated
"{"
	"float k_val=float(1<<k);"
	"ivec3 i_xyz=ivec3(floor(xyz));"
	"vec3 delta=mod(xyz,k_val)/k_val;"
	"i_xyz>>=k;"
	"int no[8]=int[8]" // noise values
	"("
		"N3(i_xyz[0]  ,i_xyz[1]  ,i_xyz[2]  ),"
		"N3(i_xyz[0]+1,i_xyz[1]  ,i_xyz[2]  ),"
		"N3(i_xyz[0]  ,i_xyz[1]+1,i_xyz[2]  ),"
		"N3(i_xyz[0]+1,i_xyz[1]+1,i_xyz[2]  ),"
		"N3(i_xyz[0]  ,i_xyz[1]  ,i_xyz[2]+1),"
		"N3(i_xyz[0]+1,i_xyz[1]  ,i_xyz[2]+1),"
		"N3(i_xyz[0]  ,i_xyz[1]+1,i_xyz[2]+1),"
		"N3(i_xyz[0]+1,i_xyz[1]+1,i_xyz[2]+1)"
	");"
	"vec4 interp_z=vec4"
	"("
		"mix(float(no[0]),float(no[4]),delta.z),"
		"mix(float(no[1]),float(no[5]),delta.z),"
		"mix(float(no[2]),float(no[6]),delta.z),"
		"mix(float(no[3]),float(no[7]),delta.z)"
	");"
	"vec2 interp_y=vec2"
	"("
		"mix(interp_z.x,interp_z.z,delta.y),"
		"mix(interp_z.y,interp_z.w,delta.y)"
	");"
	"return mix(interp_y.x,interp_y.y,delta.x)/32768.0;"
"}"

"float ON(vec3 xyz,int o)" // ocatve noise
"{"
	"float f=0.0;"
	"float m=0.5;"
	"for( int i=0;i<o;i++,m*=0.5)"
		"f+=N3I(xyz,o-i-1)*m;"
	"return f;"
"}"

"const float c_clouds_height= 400.0;"
"const float c_clouds_depth= 128.0;"
"const float c_inv_clouds_depth=1.0/c_clouds_depth;"
"float c_cloud_destiny_border=%f;" // not uniform for perfomance

"float CF(vec3 xyz)" // CloudFunc
"{"
	"float d=(xyz.z-c_clouds_height)*c_inv_clouds_depth;"
	"return 4.0*d*(1.0-d)*ON(xyz*0.25,5);"
"}"

"float CF2D(float v)" //CloudFuncToDestiny
"{"
	"return 1.0-clamp((v-c_cloud_destiny_border)*0.5,0.0,1.0);"
"}"

"void main()"
"{"
	"vec3 view_vec=normalize(fp);"
	"vec3 pos=view_vec*c_clouds_height/view_vec.z;"
	"vec3 step=normalize(pos);"
	"if(step.z<horizont){discard;return;}"
	"float sky_k=1.0;"
	"bool is_touch_pos=false;"
	"vec3 touch_pos;"
	"while(pos.z<c_clouds_height+c_clouds_depth)"
	"{"
		"float val=CF(pos);"
		"sky_k*=CF2D(val);"
		"if(val>c_cloud_destiny_border)"
		"{"
			"if(!is_touch_pos)"
			"{"
				"touch_pos=pos;"
				"is_touch_pos=true;"
			"}"
		"}"
		"pos+=step;"
	"}"

	"vec4 c;"
	"if(is_touch_pos)"
	"{"
		"float sun_k=1.0;"
		"pos=touch_pos;"
		"while(pos.z<c_clouds_height+c_clouds_depth)"
		"{"
			"sun_k*=CF2D(CF(pos));"
			"pos+=sun;"
		"}"
		"vec3 normal=vec3"
		"("
			"CF(touch_pos-vec3(2.0,0.0,0.0))-CF(touch_pos+vec3(2.0,0.0,0.0)),"
			"CF(touch_pos-vec3(0.0,2.0,0.0))-CF(touch_pos+vec3(0.0,2.0,0.0)),"
			"CF(touch_pos-vec3(0.0,0.0,2.0))-CF(touch_pos+vec3(0.0,0.0,2.0))"
		");"
		"normal= normalize(normal);"
		"float sun_dot=dot(normal,sun);"
		"vec3 final_sun_light;"
		"final_sun_light=0.5*sun_k*sl*(max(0.0,sun_dot)+0.5*(1.0+dot(normalize(-view_vec),normal)));"
		"float ambient_light_scaler=normal.z*0.25+1.05;"
		"c= vec4(al*ambient_light_scaler+final_sun_light,1.0-sky_k);"
	"}"
		"else c= vec4(0.0,0.0,0.0,0.0);"
	"c_=c;"
"}"; // multiline string


const char* const clouds_downscale_shader_v=
"#version 330\n"
"const vec2 coord[6]=vec2[6]"
"("
	"vec2(0.0,0.0),vec2(1.0,0.0),vec2(1.0,1.0),"
	"vec2(0.0,0.0),vec2(0.0,1.0),vec2(1.0,1.0)"
");"
"void main()"
"{"
	"gl_Position=vec4(coord[gl_VertexID]*2.0-vec2(1.0,1.0),0.0,1.0);"
"}";

const char* const clouds_downscale_shader_f=
"#version 330\n"
"uniform sampler2D tex;"
"out vec4 c_;"
"void main()"
"{"
	"ivec2 tc=ivec2(gl_FragCoord.xy)*2;"
	"vec4 t[4];"
	"t[0]=texelFetch(tex,tc,0);"
	"t[1]=texelFetch(tex,tc+ivec2(0,1),0);"
	"t[2]=texelFetch(tex,tc+ivec2(1,0),0);"
	"t[3]=texelFetch(tex,tc+ivec2(1,1),0);"
	"const float b=1.0/512.0;"
	"vec3 rgb=(t[0].xyz+t[1].xyz+t[2].xyz+t[3].xyz)/(step(b,t[0].a)+step(b,t[1].a)+step(b,t[2].a)+step(b,t[3].a));"
	"c_=vec4(rgb,(t[0].a+t[1].a+t[2].a+t[3].a)*0.25);"
"}";

} // namespace mf_Shaders
