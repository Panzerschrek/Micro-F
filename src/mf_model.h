#pragma once
//model flags
#define MF_MODEL_VERTEX_BIT   4
#define MF_MODEL_NORMAL_BIT   8
#define MF_MODEL_TEXCOORD_BIT 16


#pragma pack( push, 1 )

struct mf_ModelVertex
{
	char xyz[3];
};

struct mf_ModelTexCoord
{
	unsigned char st[2];
};

struct mf_ModelNormal
{
	char xyz[3];
};

struct mf_ModelHeader
{
	unsigned char format_code[4]; // must be "MFMD" - "Micro-F MoDel"
	unsigned short vertex_count;
	unsigned short trialgle_count;
	
	// final_vertex_coord= xyz * scale + pos
	float scale[3];
	float pos[3];

	unsigned char bytes_per_index; // 1 - ubyte indeces, 2 - ushort indeces
	unsigned char vertex_format_flags;
};

/*
MFMD format:
header
vertex positions
vertex normals( if exist )
vertex texture coords (if exist )
indeces
*/
#pragma pack(pop)