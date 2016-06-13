/*****************************************************************************
*
*  PROJECT:     Multi Theft Auto v1.2
*  LICENSE:     See LICENSE in the top level directory
*  FILE:        FileSystem/src/fsinternal/CFileSystem.platform.h
*  PURPOSE:     Platform native include header file, for platform features
*
*  Multi Theft Auto is available from http://www.multitheftauto.com/
*
*****************************************************************************/

#ifndef _FILESYSTEM_PLATFORM_INCLUDE_
#define _FILESYSTEM_PLATFORM_INCLUDE_

#ifdef __linux__
#define FILE_ACCESS_FLAG ( S_IRUSR | S_IWUSR )

#include <sys/errno.h>
#endif //__linux__

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#pragma warning(disable: 4996)
#endif //_WIN32

/*===================================================
    CSystemCapabilities

    This class determines the system-dependant capabilities
    and exports methods that return them to the runtime.

    It does not depend on a properly initialized CFileSystem
    environment, but CFileSystem depends on it.

    Through-out the application runtime, this class stays
    immutable.
===================================================*/
class CSystemCapabilities
{
private:

#ifdef _WIN32
    struct diskInformation
    {
        // Immutable information of a hardware data storage.
        // Modelled on the basis of a hard disk.
        DWORD sectorsPerCluster;
        DWORD bytesPerSector;
        DWORD totalNumberOfClusters;
    };
#endif //_WIN32

public:
    CSystemCapabilities( void )
    {
        
    }

    inline size_t GetFailSafeSectorSize( void )
    {
        return 2048;
    }

    // Only applicable to raw-files.
    size_t GetSystemLocationSectorSize( char driveLetter )
    {
#ifdef _WIN32
        char systemDriveLocatorBuf[4];

        // systemLocation is assumed to be an absolute path to hardware.
        // Whether the input makes sense ultimatively lies on the shoulders of the callee.
        systemDriveLocatorBuf[0] = driveLetter;
        systemDriveLocatorBuf[1] = ':';
        systemDriveLocatorBuf[2] = '/';
        systemDriveLocatorBuf[3] = 0;

        // Attempt to get disk information.
        diskInformation diskInfo;
        DWORD freeSpace;

        BOOL success = GetDiskFreeSpaceA(
            systemDriveLocatorBuf,
            &diskInfo.sectorsPerCluster,
            &diskInfo.bytesPerSector,
            &freeSpace,
            &diskInfo.totalNumberOfClusters
        );

        // If the retrieval fails, we return a good value for anything.
        if ( success == FALSE )
            return GetFailSafeSectorSize();

        return diskInfo.bytesPerSector;
#elif __linux__
        // 2048 is always a good solution :)
        return GetFailSafeSectorSize();
#endif //PLATFORM DEPENDENT CODE
    }
};

// Export it from the main class file.
extern CSystemCapabilities systemCapabilities;

#define NUMELMS(x)      ( sizeof(x) / sizeof(*x) )

#endif //_FILESYSTEM_PLATFORM_INCLUDE_