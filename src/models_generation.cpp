#include <cstring>
#include "micro-f.h"
#include "models_generation.h"

#include "level.h"

#include "drawing_model.h"
#include "../models/models.h"
#include "mf_math.h"
#include "textures_generation.h"


#define MF_TEX_COORD_SHIFT_FOR_TEXTURE_LAYER 16
#define MF_TEX_COORD_SHIFT_EPS 0.001953125f

void ApplyTexture( mf_DrawingModel* model, mf_StaticLevelObjectTexture texture )
{
	float shift_vec[]= { float(MF_TEX_COORD_SHIFT_FOR_TEXTURE_LAYER * texture) + MF_TEX_COORD_SHIFT_EPS, 00.f };
	model->ShiftTexCoord( shift_vec );
};

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

void GenForcefieldModel( mf_DrawingModel* model, float radius, float length, const float* pos )
{
	static const unsigned int c_cylinder_partitions= 64;
	static const unsigned int c_partitions_left= 22;
	GenCylinder( model, c_cylinder_partitions/*rounder*/, 1/*longer*/, false );
	float scale_vec[3]=
	{
		radius,
		radius,
		length
	};
	model->Scale( scale_vec );

	float x_mat[16];
	float y_mat[16];
	float mat[16];
	Mat4RotateX( x_mat, MF_PI2 );
	Mat4RotateY( y_mat, -MF_PI2 + MF_PI * float(c_partitions_left) / float(c_cylinder_partitions) );
	Mat4Mul( x_mat, y_mat, mat );
	Mat4ToMat3( mat );
	model->Rotate( mat );

	model->Shift( pos );

	static const float c_hex_grid_scaler= 1.0f / 32.0f;
	float tc_scale_vec[2]=
	{
		c_hex_grid_scaler * scale_vec[0] * MF_PI,
		c_hex_grid_scaler * scale_vec[2] * mf_Math::sqrt(3.0f) * 0.5f
	};
	model->ScaleTexCoord( tc_scale_vec );

	unsigned short* ind = new unsigned short[ c_partitions_left * 6 ];
	std::memcpy( ind, model->GetIndexData(), c_partitions_left * 6 * sizeof(unsigned short) );
	model->SetIndexData( ind, c_partitions_left * 6 );
}

void GenSkySphere( mf_DrawingModel* model, unsigned int partitiion )
{
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

void GenLeafs( mf_DrawingModel* model)
{
	model->LoadFromMFMD( mf_Models::leafs );
	ApplyTexture( model, TextureOakLeafs );
}

void GenSmallOak( mf_DrawingModel* model )
{
	const float c_small_oak_height= 8.0f;
	static const float c_trunk_rotate_angle= MF_PI12;
	static const float palm_size[]= { 0.4f, 0.4f, c_small_oak_height * 0.5f };
	static const float palm_shift[]= { 0.0f, 0.0f, c_small_oak_height * 0.5f };
	GenCylinder( model, 9, 1, false );
	model->Scale( palm_size );
	model->Shift( palm_shift );

	float mat[16];
	Mat4RotateX( mat, c_trunk_rotate_angle );
	Mat4ToMat3( mat );
	model->Rotate( mat );

	ApplyTexture( model, TextureOakBark );

	mf_DrawingModel leafs;
	GenLeafs( &leafs );
	leafs.Rotate( mat );
	static const float leafs_scale[3]= { 3.0f, 3.0f, 4.5f };
	static const float leafs_shift[3]= { 0.0f,  - mf_Math::sin(c_trunk_rotate_angle) * c_small_oak_height, c_small_oak_height };
	leafs.Scale( leafs_scale );
	leafs.Shift( leafs_shift );

	model->Add( &leafs );
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

	ApplyTexture( model, TextureOakBark );

	static const int c_leafs_globes= 7;
	mf_DrawingModel leafs;
	static const float leafs_globes_pos[ c_leafs_globes * 3 ]=
	{
		0.0f, 0.0f, c_oak_height,
		3.5f, 0.0f, c_oak_height + 2.1f,
		-3.4f, 0.0f, c_oak_height + 1.9f,
		0.0f, 3.2f, c_oak_height + 2.0f,
		0.0f, -3.1f, c_oak_height + 2.2f,
		0.0f, 1.0f, c_oak_height + 5.0f,
		0.0f, -0.8f, c_oak_height + 5.3f,
	};
	for( unsigned int i= 0; i< c_leafs_globes; i++ )
	{
		float radius= 4.0f;

		float rot_vec[3];
		float rot_mat[16];
		for( unsigned int j= 0; j< 3; j++ )
			rot_vec[j]= randomizer.RandF( -1.0f, 1.0f );
		Mat4RotateAroundVector( rot_mat, rot_vec, randomizer.RandF( MF_2PI ) );
		Mat4ToMat3( rot_mat );

		GenLeafs( &leafs );
		
		leafs.Rotate( rot_mat );
		leafs.Scale( radius );

		leafs.Shift( leafs_globes_pos + i * 3 );
		model->Add( &leafs );
	}
}

void GenSpruce( mf_DrawingModel* model )
{
	static const float c_spruce_height= 25.0f;
	static const float c_spruce_trunk_diameter= 1.1f;

	static const float spruce_scale[]= { c_spruce_trunk_diameter * 0.5f, c_spruce_trunk_diameter * 0.5f, c_spruce_height * 0.5f };
	static const float spruce_shift[]= { 0.0f, 0.0f, c_spruce_height * 0.5f };

	const int c_trunk_segments= 6;
	const int c_trunk_partitions= 3;
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
			v[j].normal[2]= 0.5f * c_spruce_trunk_diameter / c_spruce_height;
			Vec3Normalize( v[j].normal );
		}
	}
	ApplyTexture( model, TextureSpruceBark );

	{
		static const float spruce_side_mesh_vertices[4*3]=
		{
			0.0f, 0.0f, 0.0f,  1.1f * c_spruce_height * 0.45f, 0.0f, 0.0f,
			0.0f, 0.0f, 1.1f * c_spruce_height,  -1.1f * c_spruce_height * 0.45f, 0.0f, 0.0f
		};
		static const float spruce_side_mesh_tc[4*2]=
		{
			0.5f, 0.0f,  1.0f, 0.2f,
			0.5f, 1.0f,  0.0f, 0.2f
		};
		static const unsigned short spruce_side_mesh_indeces[]= { 0,2,1, 0,3,2 };
		static const float side_normal[3]= { 0.0f, 1.0f, 0.0f };
		const unsigned int c_sides_count= 4;

		mf_DrawingModelVertex* side_vertices= new mf_DrawingModelVertex[ 4 * 2 * c_sides_count ];
		unsigned short* side_indeces= new unsigned short[ c_sides_count * 2 * 6 ];
		for( unsigned int i= 0; i< c_sides_count*2; i++ )
		{
			float angle= MF_PI * float(i/2) / float(c_sides_count) + MF_PI * float(i&1);
			float rot_mat[16];
			Mat4RotateZ( rot_mat, angle );

			float transformed_normal[3];
			Vec3Mat4Mul( side_normal, rot_mat, transformed_normal );
			for( unsigned int j= 0; j< 4; j++ )
			{
				mf_DrawingModelVertex* v= &side_vertices[i*4+j];
				Vec3Mat4Mul( spruce_side_mesh_vertices +j*3, rot_mat, v->pos );
				VEC3_CPY( v->normal, transformed_normal );
				v->tex_coord[0]= spruce_side_mesh_tc[j*2  ];
				v->tex_coord[1]= spruce_side_mesh_tc[j*2+1];
			}
			unsigned int vert_shift= i * 4;
			unsigned short* ind= side_indeces + i * 6;
			for( unsigned int j= 0; j< 6; j++ )
				ind[j]= (unsigned short)( vert_shift + spruce_side_mesh_indeces[j] );
		} // for sides

		mf_DrawingModel sides_model;
		sides_model.SetVertexData( side_vertices, 4 * 2 * c_sides_count );
		sides_model.SetIndexData( side_indeces, c_sides_count * 2 * 6 );
		ApplyTexture( &sides_model, TextureSpruceBranch );

		model->Add( &sides_model );
	} // generate side triangles
}

void GenBirch( mf_DrawingModel* model )
{
	static const unsigned int good_seeds[]= { 13 };
	mf_Rand randomizer( good_seeds[0] );

	static const float c_birch_height= 25.0f;
	static const float c_birch_trunk_diameter= 0.8f;

	static const float birch_scale[]= { c_birch_trunk_diameter * 0.5f, c_birch_trunk_diameter * 0.5f, c_birch_height * 0.5f };
	static const float birch_shift[]= { 0.0f, 0.0f, c_birch_height * 0.5f };

	const int c_trunk_segments= 6;
	const int c_trunk_partitions= 3;
	GenCylinder( model, c_trunk_segments, c_trunk_partitions, false );
	model->Scale( birch_scale );
	model->Shift( birch_shift );

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
			v[j].normal[2]= 0.5f * c_birch_trunk_diameter / c_birch_height;
			Vec3Normalize( v[j].normal );
		}
	}
	ApplyTexture( model, TextureBirchBark );
	static const float c_tex_scaler[]= { 1.0f, 6.0f };
	model->ScaleTexCoord( c_tex_scaler );

	mf_DrawingModel leafs;
	for( unsigned int i= 0; i< 18; i++ )
	{
		float h_k= ( 0.2f + 0.8f * float(i) / float(18) );
		float radius= 4.0f;
		if( h_k < 0.4f ) radius*= h_k / 0.4f;
		else if( h_k > 0.7f ) radius*= (1.0f - h_k) / 0.3f;
		if( radius < 1.5f) radius= 1.5f;

		float angle= randomizer.RandF( MF_2PI );

		float pos[3];
		pos[0]= radius * mf_Math::cos(angle);
		pos[1]= radius * mf_Math::sin(angle);
		pos[2]= ( h_k + 0.15f ) * c_birch_height;

		float rot_vec[3];
		float rot_mat[16];
		for( unsigned int j= 0; j< 3; j++ )
			rot_vec[j]= randomizer.RandF( -1.0f, 1.0f );
		Mat4RotateAroundVector( rot_mat, rot_vec, randomizer.RandF( MF_2PI ) );
		Mat4ToMat3( rot_mat );

		GenLeafs( &leafs );
		ApplyTexture( &leafs, TextureBirchLeafs );
		
		leafs.Rotate( rot_mat );
		leafs.Scale( radius );

		leafs.Shift( pos );
		model->Add( &leafs );
	}
}

void (* const level_static_models_gen_func[mf_StaticLevelObject::LastType])(mf_DrawingModel* model)=
{
	GenSmallOak,
	GenOak,
	GenSpruce,
	GenBirch
};
