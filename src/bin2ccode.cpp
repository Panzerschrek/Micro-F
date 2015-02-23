#include <stdlib.h>
#include <stdio.h>
#include <string.h>

int main( int argc, char* argv[] )
{
	char src_file_name[1024];
	char dst_file_name[1024];

	unsigned int src_file_len;
	unsigned int i;

	unsigned char* bin_data;
	int is_output_name= 0;
	unsigned int text_mode= 0;

	for( i= 1; i< (unsigned int)argc; i++ )
	{
		if( !strcmp( argv[i], "-o" ) )
		{
			if( (int)i < argc - 1 )
			{
				strcpy( dst_file_name, argv[++i] );
				is_output_name= 1;
			}
			else
			{
				printf( "error, missing output file name before -o\n" );
				return 1;
			}
		}
		else if( !strcmp( argv[i], "-t" ) )
			text_mode= 1;
		else
			strcpy( src_file_name, argv[i] );
	}
	if( argc == 1 )
	{
		printf( "error, no input file\n" );
		return 2;
	}

	if( ! is_output_name )
	{
		printf( "error, no output file name\n" );
		return 3;
	}


	FILE* f_src=fopen( src_file_name, "rb" );
	if( f_src == NULL )
	{
		printf( "file %s not found\n", src_file_name );
		return 4;
	}

	FILE* f_dst= fopen( dst_file_name, "wt" );


	fseek( f_src, 0, SEEK_END );
	src_file_len= ftell( f_src );
	fseek( f_src, 0, SEEK_SET );

	bin_data= (unsigned char*) malloc( src_file_len );

	fread( bin_data, 1, src_file_len, f_src );
	fclose( f_src );


	fprintf( f_dst, "/*data from file \"%s\". Generated automatically*/\n", src_file_name );
	if( text_mode )
	{
		fprintf( f_dst, "\"" );
		for( i= 0; i< src_file_len; i++ )
		{
			if( bin_data[i] >= 0x20 && bin_data[i] <= 0x7E )
				fprintf( f_dst, "%c", bin_data[i] );
			else if( bin_data[i] == '\n' )
				fprintf( f_dst, "\\n\"\n\"" );
			else if( bin_data[i] == '\t' )
				fprintf( f_dst, "\\t" );// tab
			else if( bin_data[i] == '"' )
				fprintf( f_dst, "\\\"" );// "
			else if( bin_data[i] == '\\' )
				fprintf( f_dst, "\\" );
			else if( bin_data[i] != 0x0D )//if not mustdie and-of-line symbol
				fprintf( f_dst, "\\x%X", bin_data[i] );
		}
		fprintf( f_dst, "\";" );
	}
	else
	{
		fprintf( f_dst, "{\n" );
		for( i= 0; i< src_file_len - 1; i++ )
		{
			fprintf( f_dst, " 0x%02X,", bin_data[i] );
			if( (i & 15) == 15 )
				fprintf( f_dst, "\n" );
		}

		fprintf( f_dst, " 0x%02X };", bin_data[ src_file_len - 1 ] );
	}

	fclose( f_dst );

	return 0;
}
