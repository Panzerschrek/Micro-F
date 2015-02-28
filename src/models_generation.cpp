#include "micro-f.h"
#include "models_generation.h"

#include "drawing_model.h"
#include "mf_math.h"



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

void GenPalm( mf_DrawingModel* model )
{
	static const float palm_size[]= { 0.2f, 0.2f, 8.0f };
	static const float palm_shift[]= { 0.0f, 0.0f, 8.0f };
	GenCylinder( model, 12, 12, false );
	model->Scale( palm_size );
	model->Shift( palm_shift );

	mf_DrawingModelVertex* v= (mf_DrawingModelVertex*) model->GetVertexData();
	for( unsigned int i= 0; i< model->VertexCount(); i++ )
	{
		float xy_delta= 0.5f * v[i].pos[2] / ( palm_size[2] * 0.5f );
		xy_delta= xy_delta * xy_delta;
		v[i].pos[0]+= xy_delta;
		v[i].pos[1]+= xy_delta;
		//TODO: deform normal
	}
}