/*****************************************************************************
*
*  PROJECT:     Multi Theft Auto v1.2
*  LICENSE:     See LICENSE in the top level directory
*  FILE:        FileSystem/src/fsinternal/CFileSystem.stream.memory.cpp
*  PURPOSE:     Memory-based read/write stream
*
*  Multi Theft Auto is available from http://www.multitheftauto.com/
*
*****************************************************************************/

#include "StdInc.h"

// Sub modules.
#include "CFileSystem.internal.h"
#include "CFileSystem.platform.h"
#include "CFileSystem.stream.memory.h"

CMemoryMappedFile::CMemoryMappedFile( void )
{
    this->systemBuffer = NULL;
    this->systemBufferSize = 0;
}

CMemoryMappedFile::~CMemoryMappedFile( void )
{
    // TODO.
}

size_t CMemoryMappedFile::Read( void *buffer, size_t sElement, size_t iNumElems )
{
    // TODO.
    return 0;
}

size_t CMemoryMappedFile::Write( const void *buffer, size_t sElement, size_t iNumElems )
{
    // TODO.
    return 0;
}

int CMemoryMappedFile::Seek( long offset, int iType )
{
    return -1;
}

int CMemoryMappedFile::SeekNative( fsOffsetNumber_t offset, int iType )
{
    return -1;
}

long CMemoryMappedFile::Tell( void ) const
{
    return 0;
}

fsOffsetNumber_t CMemoryMappedFile::TellNative( void ) const
{
    return 0;
}

bool CMemoryMappedFile::IsEOF( void ) const
{
    return true;
}

bool CMemoryMappedFile::Stat( struct stat *stats ) const
{
    return false;
}

void CMemoryMappedFile::PushStat( const struct stat *stats )
{
    // TODO.
    return;
}

void CMemoryMappedFile::SetSeekEnd( void )
{
    // TODO.
    return;
}

size_t CMemoryMappedFile::GetSize( void ) const
{
    return 0;
}

fsOffsetNumber_t CMemoryMappedFile::GetSizeNative( void ) const
{
    return 0;
}

void CMemoryMappedFile::Flush( void )
{
    // Nothing to do, since we have no backing storage.
    return;
}

static filePath _memFilePath = "<memory>";

const filePath& CMemoryMappedFile::GetPath( void ) const
{
    return _memFilePath;
}

bool CMemoryMappedFile::IsReadable( void ) const
{
    // TODO.
    return false;
}

bool CMemoryMappedFile::IsWriteable( void ) const
{
    // TODO.
    return false;
}