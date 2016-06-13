/*****************************************************************************
*
*  PROJECT:     Multi Theft Auto v1.2
*  LICENSE:     See LICENSE in the top level directory
*  FILE:        FileSystem/src/fsinternal/CFileSystem.stream.memory.h
*  PURPOSE:     Memory-based read/write stream
*
*  Multi Theft Auto is available from http://www.multitheftauto.com/
*
*****************************************************************************/

#ifndef _FILESYSTEM_MEMORY_STREAM_
#define _FILESYSTEM_MEMORY_STREAM_

// This is a native implementation of memory-based file I/O.
// You can use it if you want to temporarily do abstract read and write operations
// in the system RAM, the fastest storage available to you.
// Using RAM has its advantages because it does not decay your storage media.
struct CMemoryMappedFile : public CFile
{
    CMemoryMappedFile( void );
    ~CMemoryMappedFile( void );

    size_t Read( void *buffer, size_t sElement, size_t iNumElems ) override;
    size_t Write( const void *buffer, size_t sElement, size_t iNumElems ) override;

    int Seek( long offset, int iType ) override;
    int SeekNative( fsOffsetNumber_t offset, int iType ) override;

    long Tell( void ) const override;
    fsOffsetNumber_t TellNative( void ) const override;

    bool IsEOF( void ) const override;

    bool Stat( struct stat *stats ) const override;
    void PushStat( const struct stat *stats ) override;

    void SetSeekEnd( void ) override;

    size_t GetSize( void ) const override;
    fsOffsetNumber_t GetSizeNative( void ) const override;

    void Flush( void ) override;

    const filePath& GetPath( void ) const override;

    bool IsReadable( void ) const override;
    bool IsWriteable( void ) const override;

private:
    void *systemBuffer;
    size_t systemBufferSize;
};

#endif //_FILESYSTEM_MEMORY_STREAM_