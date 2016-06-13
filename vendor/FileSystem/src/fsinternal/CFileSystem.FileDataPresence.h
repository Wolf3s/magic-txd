/*****************************************************************************
*
*  PROJECT:     Multi Theft Auto v1.2
*  LICENSE:     See LICENSE in the top level directory
*  FILE:        FileSystem/src/fsinternal/CFileSystem.FileDataPresence.h
*  PURPOSE:     File data presence scheduling
*
*  Multi Theft Auto is available from http://www.multitheftauto.com/
*
*****************************************************************************/

#ifndef _FILESYSTEM_DATA_PRESENCE_SCHEDULING_
#define _FILESYSTEM_DATA_PRESENCE_SCHEDULING_

struct CFileDataPresenceManager
{
    inline CFileDataPresenceManager( size_t maximumDataQuotaRAM )
    {
        LIST_CLEAR( activeFiles.root );

        this->maximumDataQuotaRAM = maximumDataQuotaRAM;
        this->fileMaxSizeInRAM = 0x40000;           // todo: properly calculate this value.
    }

    inline ~CFileDataPresenceManager( void )
    {
        // Make sure everyone released active files beforehand.
        assert( LIST_EMPTY( activeFiles.root ) == true );
    }

    CFile* AllocateTemporaryDataDestination( fsOffsetNumber_t minimumExpectedSize = 0 );

private:
    // TODO: this has to be turned into a managed object.
    struct swappableDestDevice : public CFile
    {
        inline swappableDestDevice( CFileDataPresenceManager *manager, CFile *dataSource )
        {
            this->manager = manager;
            this->dataSource = dataSource;
        }

        inline ~swappableDestDevice( void )
        {
            // TODO.
            return;
        }

        // We need to manage read and writes.
        size_t Read( void *buffer, size_t sElement, size_t iNumElements ) override;
        size_t Write( const void *buffer, size_t sElement, size_t iNumElements ) override;

        int Seek( long offset, int iType ) override                         { return dataSource->Seek( offset, iType ); }
        int SeekNative( fsOffsetNumber_t offset, int iType ) override       { return dataSource->SeekNative( offset, iType ); }

        long Tell( void ) const override                                    { return dataSource->Tell(); }
        fsOffsetNumber_t TellNative( void ) const override                  { return dataSource->TellNative(); }

        bool IsEOF( void ) const override                                   { return dataSource->IsEOF(); }

        bool Stat( struct stat *stats ) const override                      { return dataSource->Stat( stats ); }
        void PushStat( const struct stat *stats ) override                  { return dataSource->PushStat( stats ); }

        void SetSeekEnd( void ) override                                    { return dataSource->SetSeekEnd(); }

        size_t GetSize( void ) const override                               { return dataSource->GetSize(); }
        fsOffsetNumber_t GetSizeNative( void ) const override               { return dataSource->GetSizeNative(); }

        void Flush( void ) override                                         { return dataSource->Flush(); }

        const filePath& GetPath( void ) const override                      { return dataSource->GetPath(); }

        bool IsReadable( void ) const override                              { return dataSource->IsReadable(); }
        bool IsWriteable( void ) const override                             { return dataSource->IsWriteable(); }

        CFileDataPresenceManager *manager;
        CFile *dataSource;

        RwListEntry <swappableDestDevice> node;
    };

    RwList <swappableDestDevice> activeFiles;

    // Runtime configuration.
    size_t maximumDataQuotaRAM;
    size_t fileMaxSizeInRAM;
};

#endif //_FILESYSTEM_DATA_PRESENCE_SCHEDULING_