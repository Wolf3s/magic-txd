/*****************************************************************************
*
*  PROJECT:     Multi Theft Auto v1.2
*  LICENSE:     See LICENSE in the top level directory
*  FILE:        FileSystem/src/fsinternal/CFileSystem.platformutils.hxx
*  PURPOSE:     FileSystem platform dependant utilities
*
*  Multi Theft Auto is available from http://www.multitheftauto.com/
*
*****************************************************************************/

#ifndef _FILESYSTEM_PLATFORM_UTILITIES_
#define _FILESYSTEM_PLATFORM_UTILITIES_

// Function for creating an OS native directory.
inline bool _File_CreateDirectory( const filePath& thePath )
{
#ifdef __linux__
    if ( mkdir( osPath, FILE_ACCESS_FLAG ) == 0 )
        return true;

    switch( errno )
    {
    case EEXIST:
    case 0:
        return true;
    }

    return false;
#elif defined(_WIN32)
    // Make sure a file with that name does not exist.
    DWORD attrib = INVALID_FILE_ATTRIBUTES;

    filePath pathToMaybeFile = thePath;
    pathToMaybeFile.resize( pathToMaybeFile.size() - 1 );

    if ( const char *ansiPath = pathToMaybeFile.c_str() )
    {
        attrib = GetFileAttributesA( ansiPath );
    }
    else if ( const wchar_t *widePath = pathToMaybeFile.w_str() )
    {
        attrib = GetFileAttributesW( widePath );
    }

    if ( attrib != INVALID_FILE_ATTRIBUTES )
    {
        if ( !( attrib & FILE_ATTRIBUTE_DIRECTORY ) )
            return false;
    }

    BOOL dirSuccess = FALSE;

    if ( const char *ansiPath = thePath.c_str() )
    {
        dirSuccess = CreateDirectoryA( ansiPath, NULL );
    }
    else if ( const wchar_t *widePath = thePath.w_str() )
    {
        dirSuccess = CreateDirectoryW( widePath, NULL );
    }

    return ( dirSuccess == TRUE || GetLastError() == ERROR_ALREADY_EXISTS );
#else
    return false;
#endif //OS DEPENDANT CODE
}

#endif //_FILESYSTEM_PLATFORM_UTILITIES_