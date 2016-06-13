/*****************************************************************************
*
*  PROJECT:     Multi Theft Auto v1.2
*  LICENSE:     See LICENSE in the top level directory
*  FILE:        FileSystem/src/fsinternal/CFileSystem.FileDataPresence.cpp
*  PURPOSE:     File data presence scheduling
*
*  Multi Theft Auto is available from http://www.multitheftauto.com/
*
*****************************************************************************/

#include "StdInc.h"

// Sub modules.
#include "CFileSystem.internal.h"

CFile* CFileDataPresenceManager::AllocateTemporaryDataDestination( fsOffsetNumber_t minimumExpectedSize )
{
    // TODO.
    return NULL;
}

size_t CFileDataPresenceManager::swappableDestDevice::Read( void *buffer, size_t sElement, size_t iNumElements )
{
    return dataSource->Read( buffer, sElement, iNumElements );
}

size_t CFileDataPresenceManager::swappableDestDevice::Write( const void *buffer, size_t sElement, size_t iNumElements )
{
    // TODO: guard the size of this file; it must not overshoot certain limits or else the file has to be relocated.

    return dataSource->Write( buffer, sElement, iNumElements );
}