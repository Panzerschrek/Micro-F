#include "micro-f.h"
#include "models_generation.h"

#include "level.h"

#include "drawing_model.h"
#include "../models/models.h"
#include "mf_math.h"
#include "textures_generation.h"


#define MF_TEX_COORD_SHIFT_FOR_TEXTURE_LAYER 16
#define MF_TEX_COORD_SHIFT_EPS 0.001953125f

struct mf_BezierCurve
{
	float control_points[3][3];
	float control_points_normals[3][3];
};

void GenGeosphere( mf_DrawingModel* model, unsigned int segments, unsigned int partition )
{
	unsigned int vertex_count= (segments + 1 ) * (partition + 1 );
	unsigned int index_count= segments * partition * 6;
	mf_DrawingModelVertex* v= new mf_DrawingModelVertex[ vertex_count ];
	unsigned short* ind= new unsigned short[ index_count ];

	float inv_segments= 1.0f / float(segments);
	float inv_partition= 1.0f / float(partition);

	float a, da= MF_2PI * inv_segments;
	float phi= -MF_PI2, d_phi= MF_PI * inv_partition;
	unsigned int i, j, k;
	for( j= 0, k=0; j<= partition; j++, phi+= d_phi )
	{
		float xy= mf_Math::cos(phi);
		float z= mf_Math::sin(phi);
		float tex_coord_y= float(j) * inv_partition;
		for( i= 0, a= 0.0f; i<= segments; i++, a+=da, k++ )
		{
			v[k].pos[0]= xy * mf_Math::cos(a);
			v[k].pos[1]= xy * mf_Math::sin(a);
			v[k].pos[2]= z;
			v[k].normal[0]= v[k].pos[0];
			v[k].normal[1]= v[k].pos[1];
			v[k].normal[2]= v[k].pos[2];

			v[k].tex_coord[0]= float(i) * inv_segments;
			v[k].tex_coord[1]= tex_coord_y;
		}
	}

	for( j= 0, k=0; j< partition; j++  )
		for( i= 0; i< segments; i++, k+=6 )
		{
			ind[k  ]= (unsigned short)( i + j * (segments + 1 ) );
			ind[k+2]= (unsigned short)( ind[k] + segments + 1 );
			ind[k+1]= (unsigned short)( ind[k+2] + 1 );

			ind[k+5]= (unsigned short)( ind[k] );
			ind[k+4]= (unsigned short)( ind[k+1] );
			ind[k+3]= (unsigned short)( ind[k]+1 );
		}

	model->SetVertexData( v, vertex_count );
	model->SetIndexData( ind, index_count );
}

void GenCylinder( mf_DrawingModel* model, unsigned int segments, unsigned int partition, bool gen_caps )
{
	unsigned int vertex_count= (segments + 1 ) * (partition + 1 );
	unsigned int caps_vertex_shift= vertex_count;
	if( gen_caps )
		vertex_count+= (segments + 2) * 2;
	unsigned int index_count= segments * partition * 6;
	unsigned int caps_index_shift= index_count;
	if( gen_caps )
		index_count+= segments * 3 * 2;
	mf_DrawingModelVertex* v= new mf_DrawingModelVertex[ vertex_count ];
	unsigned short* ind= new unsigned short[ index_count ];

	float inv_segments= 1.0f / float(segments);
	float inv_partition= 1.0f / float(partition);

	float a, da= MF_2PI * inv_segments;
	float h= -1.0f, dh= 2.0f * inv_partition;
	unsigned int i, j, k, k2;
	for( j= 0, k=0; j<= partition; j++, h+= dh )
	{
		float tex_coord_y= float(j) * inv_partition;
		for( i= 0, a= 0.0f; i<= segments; i++, a+=da, k++ )
		{
			v[k].pos[0]= mf_Math::cos(a);
			v[k].pos[1]= mf_Math::sin(a);
			v[k].pos[2]= h;
			v[k].normal[0]= v[k].pos[0];
			v[k].normal[1]= v[k].pos[1];
			v[k].normal[2]= 0.0f;

			v[k].tex_coord[0]= float(i) * inv_segments;
			v[k].tex_coord[1]= tex_coord_y;
		}
	}
	if( gen_caps )
	{
		k=  caps_vertex_shift;
		k2= caps_vertex_shift + segments + 2;

		v[k].pos[0]= 0.0f;
		v[k].pos[1]= 0.0f;
		v[k].pos[2]= -1.0f;
		v[k].normal[0]= 0.0f;
		v[k].normal[1]= 0.0f;
		v[k].normal[2]= -1.0f;
		v[k].tex_coord[0]= 0.0f;
		v[k].tex_coord[1]= 0.0f;
		v[k2]= v[k];
		v[k2].pos[2]= 1.0f;
		v[k2].normal[2]= 1.0f;

		k++;
		k2++;
		for( i= 0, a= 0.0f; i<= segments; i++, a+=da, k++, k2++ )
		{
			v[k].pos[0]= mf_Math::cos(a);
			v[k].pos[1]= mf_Math::sin(a);
			v[k].pos[2]= -1.0f;
			v[k].normal[0]= 0.0f;
			v[k].normal[1]= 0.0f;
			v[k].normal[2]= -1.0f;
			v[k].tex_coord[0]= v[k].pos[0];
			v[k].tex_coord[1]= v[k].pos[1];

			v[k2]= v[k];
			v[k2].pos[2]= 1.0f;
			v[k2].normal[2]= 1.0f;
		}
	}

	for( j= 0, k=0; j< partition; j++  )
		for( i= 0; i< segments; i++, k+=6 )
		{
			ind[k  ]= (unsigned short)( i + j * (segments + 1 ) );
			ind[k+2]= (unsigned short)( ind[k] + segments + 1 );
			ind[k+1]= (unsigned short)( ind[k+2] + 1 );

			ind[k+5]= (unsigned short)( ind[k] );
			ind[k+4]= (unsigned short)( ind[k+1] );
			ind[k+3]= (unsigned short)( ind[k]+1 );
		}

	if( gen_caps )
	{
		unsigned short* ind2= ind + caps_index_shift;
		for( i= 0, k= caps_vertex_shift+1, k2= caps_vertex_shift + segments + 3;
			i< segments; i++, ind2+= 6, k++, k2++ )
		{
			ind2[0]= (unsigned short)( caps_vertex_shift );
			ind2[1]= (unsigned short)( k+1 );
			ind2[2]= (unsigned short)( k );
			ind2[3]= (unsigned short)( caps_vertex_shift + segments + 2 );
			ind2[4]= (unsigned short)( k2 );
			ind2[5]= (unsigned short)( k2+1 );
		}
	}

	model->SetVertexData( v, vertex_count );
	model->SetIndexData( ind, index_count );
}


void GenSkySphere( mf_DrawingModel* model )
{
	const unsigned int partitiion= 18;

	mf_DrawingModel icosahderon;
	icosahderon.LoadFromMFMD( mf_Models::icosahedron );

	// count of vertces and triangles in 1 tesseleted triangle
	const unsigned int vertex_count= ( ( partitiion + 1 ) * ( partitiion + 2 ) ) / 2;
	const unsigned int triangle_count= partitiion * partitiion;

	unsigned int icosahderon_triangle_count= icosahderon.IndexCount() / 3;
	mf_DrawingModelVertex* vertices= new mf_DrawingModelVertex[ vertex_count * icosahderon_triangle_count ];
	unsigned short* indeces= new unsigned short[ 3 * triangle_count * icosahderon_triangle_count ];

	mf_DrawingModelVertex* v= vertices;
	unsigned short* ind= indeces;

	const unsigned short* input_indeces= icosahderon.GetIndexData();
	for( unsigned int i= 0; i< icosahderon_triangle_count; i++ )
	{
		const mf_DrawingModelVertex* input_v[3]=
		{
			&( (icosahderon.GetVertexData())[ input_indeces[0] ] ),
			&( (icosahderon.GetVertexData())[ input_indeces[1] ] ),
			&( (icosahderon.GetVertexData())[ input_indeces[2] ] )
		};
		input_indeces+= 3;

		float yk= 0.0f;
		float dyk= 1.0f / float(partitiion);
		for( unsigned int y= 0; y<= partitiion; y++, yk+= dyk )
		{
			float pos_left[3];
			float pos_right[3];
			float tmp_pos_up[3];
			Vec3Mul( input_v[0]->pos, 1.0f - yk, pos_left );
			Vec3Mul( input_v[1]->pos, 1.0f - yk, pos_right );
			Vec3Mul( input_v[2]->pos, yk, tmp_pos_up );
			Vec3Add( pos_left, tmp_pos_up );
			Vec3Add( pos_right, tmp_pos_up );

			float dxk;
			if( y != partitiion ) dxk= 1.0f / ( partitiion - y );
			else dxk= 0.0f;
			float xk= 0.0f;
			for( unsigned int x= 0; x<= partitiion - y; x++, xk+= dxk, v++ )
			{
				Vec3Mul( pos_left, 1.0f - xk, v[0].pos );
				float tmp_v[3];
				Vec3Mul( pos_right, xk, tmp_v );
				Vec3Add( v[0].pos, tmp_v );
				Vec3Normalize( v[0].pos );
				VEC3_CPY( v[0].normal, v[0].pos );
			} // for x
		} // for y

		unsigned int first_vertex_ind= v - vertices - vertex_count;
		// generate indeces
		for( unsigned int y= 0; y< partitiion; y++ )
		{
			for( unsigned int x= 0; x< partitiion-y-1; x++ )
			{
				ind[0]= (unsigned short)( first_vertex_ind + x );
				ind[1]= (unsigned short)( ind[0] + 1 );
				ind[2]= (unsigned short)( ind[0] + partitiion - y + 1 );
				
				ind[3]= (unsigned short)( ind[1] );
				ind[4]= (unsigned short)( ind[2] + 1 );
				ind[5]= (unsigned short)( ind[2] );
				ind+= 6;
			}
			ind[0]= (unsigned short)( first_vertex_ind + partitiion - y - 1 );
			ind[1]= (unsigned short)( ind[0] + 1 );
			ind[2]= (unsigned short)( ind[0] +  + partitiion - y + 1 );
			ind+= 3;

			first_vertex_ind+= partitiion - y + 1;
		} // for y
	} // for original triagles

	model->SetVertexData( vertices, vertex_count * icosahderon_triangle_count );
	model->SetIndexData( indeces, 3 * triangle_count * icosahderon_triangle_count );
}

/*void GenPalmLeaf( mf_DrawingModel* model, const mf_BezierCurve* curve, unsigned int segments )
{
	unsigned int vertex_count= ( segments + 1 ) * 2;
	unsigned int index_count= segments * 6;

	mf_DrawingModelVertex* v= new mf_DrawingModelVertex[ vertex_count ];
	unsigned short* ind= new unsigned short[ index_count ];

	for( unsigned int i= 0; i< segments + 1; i++ )
	{
	}
}*/

void GenPalm( mf_DrawingModel* model )
{
	const float c_palm_height= 10.0f;
	static const float palm_size[]= { 0.5f, 0.5f, c_palm_height * 0.5f };
	static const float palm_shift[]= { 0.0f, 0.0f, c_palm_height * 0.5f };
	GenCylinder( model, 12, 12, false );
	model->Scale( palm_size );
	model->Shift( palm_shift );

	mf_DrawingModelVertex* v= (mf_DrawingModelVertex*) model->GetVertexData();
	for( unsigned int i= 0; i< model->VertexCount(); i++ )
	{
		float mult= (3.0f - v[i].pos[2] / c_palm_height ) * 0.333333f;
		v[i].pos[0]*= mult;
		v[i].pos[1]*= mult;

		float x_delta= v[i].pos[2] / c_palm_height;
		v[i].pos[0]+= x_delta * x_delta * ( c_palm_height * 0.25f );
		//TODO: deform normal
	}

	static const float shift_vec[]= { float(MF_TEX_COORD_SHIFT_FOR_TEXTURE_LAYER * TexturePalmBark) + MF_TEX_COORD_SHIFT_EPS, 0.f };
	model->ShiftTexCoord( shift_vec );
}

void GenOak( mf_DrawingModel* model )
{
	const float c_oak_height= 10.0f;
	const float c_oak_trunk_diameter= 2.3f;

	static const float oak_scale[]= { c_oak_trunk_diameter * 0.5f, c_oak_trunk_diameter * 0.5f, c_oak_height * 0.5f };
	static const float oak_shift[]= { 0.0f, 0.0f, c_oak_height * 0.5f };

	const int c_trunk_segments= 6;
	const int c_trunk_partitions= 8;
	GenCylinder( model, c_trunk_segments, c_trunk_partitions, false );
	model->Scale( oak_scale );
	model->Shift( oak_shift );

	mf_Rand randomizer;

	mf_DrawingModelVertex* vertices= (mf_DrawingModelVertex*) model->GetVertexData();
	for( unsigned int i= 0; i<= c_trunk_partitions; i++ )
	{
		mf_DrawingModelVertex* v= vertices + i * (c_trunk_segments+1);

		float diameter_multipler= 1.0f - float(i) / float(c_trunk_partitions);
		diameter_multipler= ( diameter_multipler + 1.0f ) / 2.0f;

		float shift_x= randomizer.RandF(-0.3f, 0.3f );
		float shift_y= randomizer.RandF(-0.3f, 0.3f );

		for( unsigned int j= 0; j< c_trunk_segments+1; j++ )
		{
			v[j].pos[0]= (v[j].pos[0] + shift_x) * diameter_multipler;
			v[j].pos[1]= (v[j].pos[1] + shift_y) * diameter_multipler;
		}
	}

	static const float trunk_shift_vec[]= { float(MF_TEX_COORD_SHIFT_FOR_TEXTURE_LAYER * TextureOakBark) + MF_TEX_COORD_SHIFT_EPS, 0.f };
	model->ShiftTexCoord( trunk_shift_vec );

	// genearate leafs
	const unsigned int c_leaf_segments_count= 9;
	const unsigned int c_vertices_per_leaf= 4;
	const unsigned int c_indeces_per_leaf= 6;
	static const float c_leaf_scale= 3.0f;
	static const float leaf_segment_vertices[c_vertices_per_leaf * 3]=
	{
		-1.0f *c_leaf_scale, 0.0f *c_leaf_scale, -1.5f,
		 1.0f *c_leaf_scale, 0.0f *c_leaf_scale, -1.5f,
		-2.5f *c_leaf_scale, 4.0f *c_leaf_scale, -1.5f,
		 2.5f *c_leaf_scale, 4.0f *c_leaf_scale, -1.5f,
	};
	static const float leaf_segment_vertices_tc[c_vertices_per_leaf * 2]=
	{
		-0.25f, 0.0f,    0.25f, 0.0f,
		 -1.0f, 1.0f,     1.0f, 1.0f
	};
	static unsigned short leaf_segment_indeces[c_indeces_per_leaf]=
	{
		2,1,0,  2,3,1
	};
	static const float leaf_normal[]= { 0.0f, 0.0f, -1.0f };

	mf_DrawingModelVertex* leafs_vertices= new mf_DrawingModelVertex[ c_vertices_per_leaf * c_leaf_segments_count ];
	unsigned short* leafs_indeces= new unsigned short[ c_indeces_per_leaf * c_leaf_segments_count ];
	for( unsigned int seg= 0; seg< c_leaf_segments_count; seg++ )
	{
		float transformed_normal[3];
		float z_angle= randomizer.RandF( 0.17f ) + MF_2PI * float(seg) / float(c_leaf_segments_count);
		float x_angle= randomizer.RandF( 0.98f, 1.18f );

		float rotate_z_mat[16];
		float rotate_x_mat[16];
		float mat[16];
		Mat4RotateZ( rotate_z_mat, z_angle );
		Mat4RotateX( rotate_x_mat, x_angle );
		Mat4Mul( rotate_x_mat, rotate_z_mat, mat );

		mf_DrawingModelVertex* vert= leafs_vertices + seg * c_vertices_per_leaf;
		Vec3Mat4Mul( leaf_normal, mat, transformed_normal );
		for( unsigned int v= 0; v< c_vertices_per_leaf; v++ )
		{
			Vec3Mat4Mul( leaf_segment_vertices + v*3, mat, vert[v].pos );
			vert[v].pos[2]+= 8.0f;
			VEC3_CPY(vert[v].normal, transformed_normal);
			vert[v].tex_coord[0]= leaf_segment_vertices_tc[v*2  ];
			vert[v].tex_coord[1]= leaf_segment_vertices_tc[v*2+1];
		}
		unsigned short* ind= leafs_indeces + seg * c_indeces_per_leaf;
		for( unsigned int i= 0; i< c_indeces_per_leaf; i++ )
			ind[i]= (unsigned short) (leaf_segment_indeces[i] + seg * c_vertices_per_leaf);
		
	}// for segments

	mf_DrawingModel leafs_model;
	leafs_model.SetVertexData( leafs_vertices, c_vertices_per_leaf * c_leaf_segments_count );
	leafs_model.SetIndexData( leafs_indeces, c_indeces_per_leaf * c_leaf_segments_count );

	static const float leafs_shift_vec[]= { float(MF_TEX_COORD_SHIFT_FOR_TEXTURE_LAYER * TextureOakLeafs) + MF_TEX_COORD_SHIFT_EPS, 0.f };
	leafs_model.ShiftTexCoord( leafs_shift_vec );

	model->Add( &leafs_model );
}

void GenSpruce( mf_DrawingModel* model )
{
	const float c_spruce_height= 15.0f;
	const float c_spruce_trunk_diameter= 1.5f;

	static const float spruce_scale[]= { c_spruce_trunk_diameter * 0.5f, c_spruce_trunk_diameter * 0.5f, c_spruce_height * 0.5f };
	static const float spruce_shift[]= { 0.0f, 0.0f, c_spruce_height * 0.5f };

	const int c_trunk_segments= 6;
	const int c_trunk_partitions= 5;
	GenCylinder( model, c_trunk_segments, c_trunk_partitions, false );
	model->Scale( spruce_scale );
	model->Shift( spruce_shift );

	mf_DrawingModelVertex* vertices= (mf_DrawingModelVertex*) model->GetVertexData();
	for( unsigned int i= 0; i<= c_trunk_partitions; i++ )
	{
		mf_DrawingModelVertex* v= vertices + i * (c_trunk_segments+1);

		float diameter_multipler= 1.0f - float(i) / float(c_trunk_partitions);
		diameter_multipler= ( diameter_multipler + 1.0f ) / 2.0f;
		for( unsigned int j= 0; j< c_trunk_segments+1; j++ )
		{
			v[j].pos[0]*= diameter_multipler;
			v[j].pos[1]*= diameter_multipler;
		}
	}
	static const float shift_vec[]= { float(MF_TEX_COORD_SHIFT_FOR_TEXTURE_LAYER * TextureSpruceBark) + MF_TEX_COORD_SHIFT_EPS, 0.f };
	model->ShiftTexCoord( shift_vec );
}

void (* const level_static_models_gen_func[mf_StaticLevelObject::LastType])(mf_DrawingModel* model)=
{
	GenPalm,
	GenOak,
	GenSpruce
};