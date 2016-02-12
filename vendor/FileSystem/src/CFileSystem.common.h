/*****************************************************************************
*
*  PROJECT:     Multi Theft Auto v1.2
*  LICENSE:     See LICENSE in the top level directory
*  FILE:        FileSystem/src/CFileSystem.common.h
*  PURPOSE:     Common definitions of the FileSystem module
*
*  Multi Theft Auto is available from http://www.multitheftauto.com/
*
*****************************************************************************/

#ifndef _FILESYSTEM_COMMON_DEFINITIONS_
#define _FILESYSTEM_COMMON_DEFINITIONS_

#include <vector>

#include <sdk/MemoryRaw.h>

#include "CFileSystem.common.filePath.h"

// Definition of the offset type that accomodates for all kinds of files.
// Realistically speaking, the system is not supposed to have files that are bigger than
// this number type can handle.
// Use this number type whenever correct operations are required.
// REMEMBER: be sure to check that your compiler supports the number representation you want!
typedef long long int fsOffsetNumber_t;

// Types used to keep binary compatibility between compiler implementations and architectures.
// Binary compatibility is required in streams, so that reading streams is not influenced by
// compilation mode.
typedef bool fsBool_t;
typedef char fsChar_t;
typedef unsigned char fsUChar_t;
typedef short fsShort_t;
typedef unsigned short fsUShort_t;
typedef int fsInt_t;
typedef unsigned int fsUInt_t;
typedef long long int fsWideInt_t;
typedef unsigned long long int fsUWideInt_t;
typedef float fsFloat_t;
typedef double fsDouble_t;

// Compiler compatibility
#ifndef _MSC_VER
#define abstract
#endif //_MSC:VER

#ifdef __linux__
#include <sys/stat.h>

#define _strdup strdup
#define _vsnprintf vsnprintf
#endif //__linux__

typedef std::vector <filePath> dirTree;

enum eFileException
{
    FILE_STREAM_TERMINATED  // user attempts to perform on a terminated file stream, ie. http timeout
};

#endif //_FILESYSTEM_COMMON_DEFINITIONS_