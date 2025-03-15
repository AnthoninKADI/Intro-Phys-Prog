
#include "Fileio.h"
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#include <direct.h>
#define GetCurrentDir _getcwd

static char g_ApplicationDirectory[ FILENAME_MAX ];
static bool g_WasInitialized = false;

void InitializeFileSystem() {
	if ( g_WasInitialized ) {
		return;
	}
	g_WasInitialized = true;

	const bool result = GetCurrentDir( g_ApplicationDirectory, sizeof( g_ApplicationDirectory ) );
	assert( result );
	if ( result ) {
		printf( "ApplicationDirectory: %s\n", g_ApplicationDirectory );
	} else {
		printf( "Unable to get current working directory!\n");
	}
}

void RelativePathToFullPath( const char * relativePathName, char * fullPath ) {
	InitializeFileSystem();

	sprintf( fullPath, "%s/%s", g_ApplicationDirectory, relativePathName );
}

bool GetFileData( const char * fileNameLocal, unsigned char ** data, unsigned int & size ) {
	InitializeFileSystem();
	char fileName[ 2048 ];
	sprintf( fileName, "%s/%s", g_ApplicationDirectory, fileNameLocal );
	FILE * file = fopen( fileName, "rb" );
	if ( file == NULL ) {
		return false;
	}
	
	fseek( file, 0, SEEK_END );
	fflush( file );
	size = ftell( file );
	fflush( file );
	rewind( file );
	fflush( file );
	
	*data = (unsigned char*)malloc( ( size + 1 ) * sizeof( unsigned char ) );
	
	if ( *data == NULL ) {
		printf( "ERROR: Could not allocate memory!  %s\n", fileName );
		fclose( file );
		return false;
	}

	memset( *data, 0, ( size + 1 ) * sizeof( unsigned char ) );
	
	unsigned int bytesRead = (unsigned int)fread( *data, sizeof( unsigned char ), size, file );
    fflush( file );
    
    assert( bytesRead == size );
	
	if ( bytesRead != size ) {
		printf( "ERROR: reading file went wrong %s\n", fileName );
		fclose( file );
		if ( *data != NULL ) {
			free( *data );
		}
		return false;
	}
	
	fclose( file );
	printf( "Read file was success %s\n", fileName );
	return true;
}

bool SaveFileData( const char * fileNameLocal, const void * data, unsigned int size ) {
	InitializeFileSystem();

	char fileName[ 2048 ];
	sprintf( fileName, "%s/%s", g_ApplicationDirectory, fileNameLocal );
	
	FILE * file = fopen( fileName, "wb" );
	
	if ( file == NULL ) {
		printf("ERROR: open file for write failed: %s\n", fileName );
		return false;
	}
	
	unsigned int bytesWritten = (unsigned int )fwrite( data, 1, size, file );
    assert( bytesWritten == size );
	
	if ( bytesWritten != size ) {
		printf( "ERROR: writing file went wrong %s\n", fileName );
		fclose( file );
		return false;
	}
	
	fclose( file );
	printf( "Write file was success %s\n", fileName );
	return true;
}