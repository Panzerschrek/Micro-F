#include "micro-f.h"
#include "drawing_model.h"

#include "mf_model.h"
#include "mf_math.h"

mf_DrawingModel::mf_DrawingModel()
	: vertices_(NULL), indeces_(NULL)
	, vertex_count_(0), index_count_(0)
{
}

mf_DrawingModel::~mf_DrawingModel()
{
	if( vertices_ != NULL )
		delete[] vertices_;
	if( indeces_ != NULL )
		delete[] indeces_;
}

void mf_DrawingModel::SetVertexData( mf_DrawingModelVertex* vertices, unsigned int vertex_count )
{
	if( vertices_ != NULL ) delete[] vertices_;
	vertices_= vertices;
	vertex_count_= vertex_count;
}

void mf_DrawingModel::SetIndexData( unsigned short* indeces, unsigned int index_count )
{
	if( indeces_ != NULL ) delete[] indeces_;
	indeces_= indeces;
	index_count_= index_count;
}

void mf_DrawingModel::Copy( const mf_DrawingModel* m )
{
	if( vertices_ != NULL )
		delete[] vertices_;
	if( indeces_ != NULL )
		delete[] indeces_;

	vertex_count_= m->vertex_count_;
	index_count_= m->index_count_;
	vertices_= new mf_DrawingModelVertex[ vertex_count_ ];
	indeces_= new unsigned short[ index_count_ ];
	memcpy( vertices_, m->vertices_, vertex_count_ * sizeof(mf_DrawingModelVertex) );
	memcpy( indeces_, m->indeces_, index_count_ * sizeof(unsigned short) );
}

void mf_DrawingModel::LoadFromMFMD( const unsigned char* model_data )
{
	const mf_ModelHeader* header= (const mf_ModelHeader*) model_data;

	mf_ModelVertex* v_p= (mf_ModelVertex*)( model_data + sizeof(mf_ModelHeader) );
	void* next_p= v_p + header->vertex_count;
	mf_ModelNormal* n_p= NULL;
	mf_ModelTexCoord* tc_p= NULL;

	if( header->vertex_format_flags & MF_MODEL_NORMAL_BIT )
	{
		n_p= (mf_ModelNormal*)(next_p);
		next_p= n_p + header->vertex_count;
	}
	if( header->vertex_format_flags & MF_MODEL_TEXCOORD_BIT )
	{
		tc_p= (mf_ModelTexCoord*)(next_p);
		next_p= tc_p + header->vertex_count;
	}
	unsigned short* i_p= (unsigned short*)next_p;

	if( vertices_ != NULL )
		delete[] vertices_;
	vertices_= new mf_DrawingModelVertex[ header->vertex_count ];
	if( indeces_ != NULL )
		delete[] indeces_;
	indeces_= new unsigned short[ header->trialgle_count * 3 ];
	vertex_count_= header->vertex_count;
	index_count_= header->trialgle_count * 3;

	for( unsigned int i= 0; i< vertex_count_; i++ )
	{
		for( unsigned int j= 0; j< 3; j++ )
			vertices_[i].pos[j]= float(v_p[i].xyz[j]) * header->scale[j] + header->pos[j];
		if( n_p != NULL )
		{
			for( unsigned int j= 0; j< 3; j++ )
				vertices_[i].normal[j]= float(n_p[i].xyz[j]) / 127.0f;
		}
		else
		{
			vertices_[i].normal[0]= 0.0f;
			vertices_[i].normal[1]= 0.0f;
			vertices_[i].normal[2]= 1.0f;
		}
		if( tc_p != NULL )
		{
			vertices_[i].tex_coord[0]= float(tc_p[i].st[0]) * (1.0f/255.0f);
			vertices_[i].tex_coord[1]= float(tc_p[i].st[1]) * (1.0f/255.0f);
		}
		else
		{
			vertices_[i].tex_coord[0]= 0.0f;
			vertices_[i].tex_coord[1]= 0.0f;
		}
	} // for out vertices

	if( header->bytes_per_index == 2 )
		memcpy( indeces_, i_p, sizeof(unsigned short) * index_count_ );
	else
	{
		unsigned char* byte_p= (unsigned char*)i_p;
		for( unsigned int i= 0; i < index_count_; i++ )
			indeces_[i]= byte_p[i];
	}
}

void mf_DrawingModel::Add( const mf_DrawingModel* m )
{
	mf_DrawingModelVertex* new_vertices= new mf_DrawingModelVertex[ vertex_count_ + m->vertex_count_ ];
	unsigned short* new_indeces= new unsigned short[ index_count_ + m->index_count_ ];
	
	memcpy( new_vertices, vertices_, vertex_count_ * sizeof(mf_DrawingModelVertex) );
	memcpy( new_vertices + vertex_count_, m->vertices_, m->vertex_count_ * sizeof(mf_DrawingModelVertex) );

	memcpy( new_indeces, indeces_, sizeof(unsigned short) * index_count_ );
	for( unsigned int i= 0; i< m->index_count_; i++ )
		new_indeces[ i + index_count_ ]= (unsigned short)( m->indeces_[i] + vertex_count_ );

	delete[] vertices_;
	delete[] indeces_;
	vertices_= new_vertices;
	indeces_= new_indeces;
	vertex_count_+= m->vertex_count_;
	index_count_+= m->index_count_;
}

void mf_DrawingModel::Scale( const float* scale_vec )
{
	float normal_scale_vec[]= { 1.0f / scale_vec[0], 1.0f / scale_vec[1], 1.0f / scale_vec[2] };

	for( unsigned int i= 0; i < vertex_count_; i++ )
	{
		for( unsigned int j= 0; j< 3; j++ )
		{
			vertices_[i].pos[j]*= scale_vec[j];
			vertices_[i].normal[j]*= normal_scale_vec[j];
		}
		Vec3Normalize( vertices_[i].normal );
	}
}

void mf_DrawingModel::Scale( float s )
{
	float scale_vec[]= { s, s, s };
	Scale( scale_vec );
}

void mf_DrawingModel::Shift( const float* shift_vec )
{
	for( unsigned int i= 0; i < vertex_count_; i++ )
		for( unsigned int j= 0; j< 3; j++ )
			vertices_[i].pos[j]+= shift_vec[j];
}

void mf_DrawingModel::Rotate( const float* normal_mat )
{
	for( unsigned int i= 0; i < vertex_count_; i++ )
	{
		float pos[3];
		float normal[3];
		VEC3_CPY( pos, vertices_[i].pos );
		VEC3_CPY( normal, vertices_[i].normal );
		Vec3Mat3Mul( pos, normal_mat, vertices_[i].pos );
		Vec3Mat3Mul( normal, normal_mat, vertices_[i].normal );
	}
}

void mf_DrawingModel::ScaleTexCoord( const float* scale_vec )
{
	for( unsigned int i= 0; i < vertex_count_; i++ )
	{
		vertices_[i].tex_coord[0]*= scale_vec[0];
		vertices_[i].tex_coord[1]*= scale_vec[1];
	}
}

void mf_DrawingModel::ShiftTexCoord( const float* shift_vec )
{
	for( unsigned int i= 0; i < vertex_count_; i++ )
	{
		vertices_[i].tex_coord[0]+= shift_vec[0];
		vertices_[i].tex_coord[1]+= shift_vec[1];
	}
}

void mf_DrawingModel::TransformTexCoord( const float* transform_mat3x3 )
{
	float vec[3];
	vec[2]= 1.0f;
	for( unsigned int i= 0; i < vertex_count_; i++ )
	{
		vec[0]= vertices_[i].tex_coord[0];
		vec[1]= vertices_[i].tex_coord[1];
		float result_vec[3];
		Vec3Mat4Mul( vec, transform_mat3x3, result_vec );
		vertices_[i].tex_coord[0]= result_vec[0];
		vertices_[i].tex_coord[1]= result_vec[1];
	}
}

void mf_DrawingModel::FlipTriangles()
{
	for( unsigned int i= 0; i< index_count_; i+=3 )
	{
		unsigned short tmp= indeces_[i];
		indeces_[i]= indeces_[i+2];
		indeces_[i+2]= tmp;
	}
}