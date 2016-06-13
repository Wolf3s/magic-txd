/*****************************************************************************
*
*  PROJECT:     Multi Theft Auto v1.2
*  LICENSE:     See LICENSE in the top level directory
*  FILE:        FileSystem/src/CFileSystem.img.cpp
*  PURPOSE:     IMG R* Games archive management
*
*  Multi Theft Auto is available from http://www.multitheftauto.com/
*
*****************************************************************************/

#include <StdInc.h>

// Include internal (private) definitions.
#include "fsinternal/CFileSystem.internal.h"
#include "fsinternal/CFileSystem.img.internal.h"

extern CFileSystem *fileSystem;

#include "CFileSystem.Utils.hxx"


void imgExtension::Initialize( CFileSystemNative *sys )
{
    return;
}

void imgExtension::Shutdown( CFileSystemNative *sys )
{
    return;
}

template <typename charType>
inline const charType* GetReadWriteMode( bool isNew )
{
    static_assert( false, "invalid character type" );
}

template <>
inline const char* GetReadWriteMode <char> ( bool isNew )
{
    return ( isNew ? "wb" : "rb+" );
}

template <>
inline const wchar_t* GetReadWriteMode <wchar_t> ( bool isNew )
{
    return ( isNew ? L"wb" : L"rb+" );
}

template <typename charType>
inline CFile* OpenSeperateIMGRegistryFile( CFileTranslator *srcRoot, const charType *imgFilePath, bool isNew )
{
    CFile *registryFile = NULL;

    filePath dirOfArchive;
    filePath extention;

    filePath nameItem = FileSystem::GetFileNameItem( imgFilePath, false, &dirOfArchive, &extention );

    if ( nameItem.size() != 0 )
    {
        filePath regFilePath = dirOfArchive + nameItem + ".DIR";

        // Open a seperate registry file.
        registryFile = srcRoot->Open( regFilePath, GetReadWriteMode <wchar_t> ( isNew ), FILE_FLAG_WRITESHARE );
    }

    return registryFile;
}

template <typename charType, typename handlerType, typename extraParams>
static AINLINE CIMGArchiveTranslatorHandle* GenNewArchiveTemplate( imgExtension *env, CFileTranslator *srcRoot, const charType *srcPath, eIMGArchiveVersion version, handlerType handler, extraParams theParams... )
{
    // Create an archive depending on version.
    CIMGArchiveTranslatorHandle *resultArchive = NULL;
    {
        CFile *contentFile = NULL;
        CFile *registryFile = NULL;

        if ( version == IMG_VERSION_1 )
        {
            // Just open the content file.
            contentFile = srcRoot->Open( srcPath, GetReadWriteMode <charType> ( true ), FILE_FLAG_WRITESHARE );

            // We need to create a seperate registry file.
            registryFile = OpenSeperateIMGRegistryFile( srcRoot, srcPath, true );
        }
        else if ( version == IMG_VERSION_2 )
        {
            // Just create a content file.
            contentFile = srcRoot->Open( srcPath, GetReadWriteMode <charType> ( true ), FILE_FLAG_WRITESHARE );

            registryFile = contentFile;
        }

        if ( contentFile && registryFile )
        {
            resultArchive = handler( env, registryFile, contentFile, version, theParams );
        }

        if ( !resultArchive )
        {
            if ( contentFile )
            {
                delete contentFile;
            }

            if ( registryFile && registryFile != contentFile )
            {
                delete registryFile;
            }
        }
    }
    return resultArchive;
}

static AINLINE CIMGArchiveTranslator* _regularIMGConstructor( imgExtension *env, CFile *registryFile, CFile *contentFile, eIMGArchiveVersion version, bool isLiveMode )
{
    return new CIMGArchiveTranslator( *env, contentFile, registryFile, version, isLiveMode );
}

template <typename charType>
static inline CIMGArchiveTranslatorHandle* GenNewArchive( imgExtension *env, CFileTranslator *srcRoot, const charType *srcPath, eIMGArchiveVersion version, bool isLiveMode )
{
    return GenNewArchiveTemplate( env, srcRoot, srcPath, version, _regularIMGConstructor, isLiveMode );
}

CIMGArchiveTranslatorHandle* imgExtension::NewArchive( CFileTranslator *srcRoot, const char *srcPath, eIMGArchiveVersion version, bool isLiveMode )
{ return GenNewArchive( this, srcRoot, srcPath, version, isLiveMode ); }
CIMGArchiveTranslatorHandle* imgExtension::NewArchive( CFileTranslator *srcRoot, const wchar_t *srcPath, eIMGArchiveVersion version, bool isLiveMode )
{ return GenNewArchive( this, srcRoot, srcPath, version, isLiveMode ); }

template <typename charType>
inline const charType* GetOpenArchiveFileMode( bool writeAccess )
{
    static_assert( false, "fixme: unsupported character type for generic IMG open routine" );
}

template <>
inline const char* GetOpenArchiveFileMode <char> ( bool writeAccess )
{
    return ( writeAccess ? "rb+" : "rb" );
}

template <>
inline const wchar_t* GetOpenArchiveFileMode <wchar_t> ( bool writeAccess )
{
    return ( writeAccess ? L"rb+" : L"rb" );
}

template <typename charType, typename constructionHandler, typename extraParamsType>
static inline CIMGArchiveTranslatorHandle* GenOpenArchiveTemplate( imgExtension *env, CFileTranslator *srcRoot, const charType *srcPath, bool writeAccess, constructionHandler constr, extraParamsType extraParams... )
{
    CIMGArchiveTranslatorHandle *transOut = NULL;
        
    bool hasValidArchive = false;
    eIMGArchiveVersion theVersion;

    CFile *contentFile = srcRoot->Open( srcPath, GetOpenArchiveFileMode <charType> ( false ), FILE_FLAG_WRITESHARE );

    if ( !contentFile )
    {
        return NULL;
    }

    bool hasUniqueRegistryFile = false;
    CFile *registryFile = NULL;

    // Check for version 2.
    struct mainHeader
    {
        union
        {
            unsigned char version[4];
            fsUInt_t checksum;
        };
    };

    mainHeader imgHeader;

    bool hasReadMainHeader = contentFile->ReadStruct( imgHeader );

    if ( hasReadMainHeader && imgHeader.checksum == '2REV' )
    {
        hasValidArchive = true;
        theVersion = IMG_VERSION_2;

        registryFile = contentFile;
    }

    if ( !hasValidArchive )
    {
        // Check for version 1.
        hasUniqueRegistryFile = true;

        registryFile = OpenSeperateIMGRegistryFile( srcRoot, srcPath, false );
        
        if ( registryFile )
        {
            hasValidArchive = true;
            theVersion = IMG_VERSION_1;
        }
    }

    if ( hasValidArchive )
    {
        CIMGArchiveTranslator *translator = constr( env, registryFile, contentFile, theVersion, extraParams );

        if ( translator )
        {
            bool loadingSuccess = translator->ReadArchive();

            if ( loadingSuccess )
            {
                transOut = translator;
            }
            else
            {
                delete translator;

                contentFile = NULL;
                registryFile = NULL;
            }
        }
    }

    if ( !transOut )
    {
        if ( contentFile )
        {
            delete contentFile;

            contentFile = NULL;
        }

        if ( hasUniqueRegistryFile && registryFile )
        {
            delete registryFile;

            registryFile = NULL;
        }
    }

    return transOut;
}

template <typename charType>
static inline CIMGArchiveTranslatorHandle* GenOpenArchive( imgExtension *env, CFileTranslator *srcRoot, const charType *srcPath, bool writeAccess, bool isLiveMode )
{
    return GenOpenArchiveTemplate( env, srcRoot, srcPath, writeAccess, _regularIMGConstructor, isLiveMode );
}

CIMGArchiveTranslatorHandle* imgExtension::OpenArchive( CFileTranslator *srcRoot, const char *srcPath, bool writeAccess, bool isLiveMode )
{ return GenOpenArchive( this, srcRoot, srcPath, writeAccess, isLiveMode ); }
CIMGArchiveTranslatorHandle* imgExtension::OpenArchive( CFileTranslator *srcRoot, const wchar_t *srcPath, bool writeAccess, bool isLiveMode )
{ return GenOpenArchive( this, srcRoot, srcPath, writeAccess, isLiveMode ); }

CFileTranslator* imgExtension::GetTempRoot( void )
{
    return repo.GetTranslator();
}

template <typename charType>
static inline CIMGArchiveTranslatorHandle* GenOpenIMGArchive( CFileSystem *sys, CFileTranslator *srcRoot, const charType *srcPath, bool writeAccess, bool isLiveMode )
{
    imgExtension *imgExt = imgExtension::Get( sys );

    if ( imgExt )
    {
        return imgExt->OpenArchive( srcRoot, srcPath, writeAccess, isLiveMode );
    }
    return NULL;
}

CIMGArchiveTranslatorHandle* CFileSystem::OpenIMGArchive( CFileTranslator *srcRoot, const char *srcPath, bool writeAccess, bool isLiveMode )
{ return GenOpenIMGArchive( this, srcRoot, srcPath, writeAccess, isLiveMode ); }
CIMGArchiveTranslatorHandle* CFileSystem::OpenIMGArchive( CFileTranslator *srcRoot, const wchar_t *srcPath, bool writeAccess, bool isLiveMode )
{ return GenOpenIMGArchive( this, srcRoot, srcPath, writeAccess, isLiveMode ); }

template <typename charType>
static inline CIMGArchiveTranslatorHandle* GenCreateIMGArchive( CFileSystem *sys, CFileTranslator *srcRoot, const charType *srcPath, eIMGArchiveVersion version, bool isLiveMode )
{
    imgExtension *imgExt = imgExtension::Get( sys );

    if ( imgExt )
    {
        return imgExt->NewArchive( srcRoot, srcPath, version, isLiveMode );
    }
    return NULL;
}

CIMGArchiveTranslatorHandle* CFileSystem::CreateIMGArchive( CFileTranslator *srcRoot, const char *srcPath, eIMGArchiveVersion version, bool isLiveMode )
{ return GenCreateIMGArchive( this, srcRoot, srcPath, version, isLiveMode ); }
CIMGArchiveTranslatorHandle* CFileSystem::CreateIMGArchive( CFileTranslator *srcRoot, const wchar_t *srcPath, eIMGArchiveVersion version, bool isLiveMode )
{ return GenCreateIMGArchive( this, srcRoot, srcPath, version, isLiveMode ); }

#pragma warning(push)
#pragma warning(disable:4250)

struct CIMGArchiveTranslator_lzo : public CIMGArchiveTranslator
{
    inline CIMGArchiveTranslator_lzo( imgExtension& imgExt, CFile *contentFile, CFile *registryFile, eIMGArchiveVersion theVersion, bool isLiveMode )
        : CIMGArchiveTranslator( imgExt, contentFile, registryFile, theVersion, isLiveMode )
    {
        // Set the compression provider.
        this->SetCompressionHandler( &compression );
    }

    inline ~CIMGArchiveTranslator_lzo( void )
    {
        // We must unset the compression handler.
        this->SetCompressionHandler( NULL );
    }

    // We need a compressor per translator, so we can compress simultaneously on multiple threads.
    xboxIMGCompression compression;
};

#pragma warning(pop)

static AINLINE CIMGArchiveTranslator* _lzoCompressedIMGConstructor( imgExtension *env, CFile *registryFile, CFile *contentFile, eIMGArchiveVersion version, bool isLiveMode )
{
    return new CIMGArchiveTranslator_lzo( *env, contentFile, registryFile, version, isLiveMode );
}

template <typename charType>
static inline CIMGArchiveTranslatorHandle* GenOpenCompressedIMGArchive( CFileSystem *sys, CFileTranslator *srcRoot, const charType *srcPath, bool writeAccess, bool isLiveMode )
{
    CIMGArchiveTranslatorHandle *archiveHandle = NULL;
    {
        imgExtension *imgExt = imgExtension::Get( sys );

        if ( imgExt )
        {
            // Create a translator specifically with the LZO compression algorithm.
            archiveHandle = GenOpenArchiveTemplate( imgExt, srcRoot, srcPath, writeAccess, _lzoCompressedIMGConstructor, isLiveMode );
        }
    }

    return archiveHandle;
}

CIMGArchiveTranslatorHandle* CFileSystem::OpenCompressedIMGArchive( CFileTranslator *srcRoot, const char *srcPath, bool writeAccess, bool isLiveMode )
{ return GenOpenCompressedIMGArchive( this, srcRoot, srcPath, writeAccess, isLiveMode ); }
CIMGArchiveTranslatorHandle* CFileSystem::OpenCompressedIMGArchive( CFileTranslator *srcRoot, const wchar_t *srcPath, bool writeAccess, bool isLiveMode )
{ return GenOpenCompressedIMGArchive( this, srcRoot, srcPath, writeAccess, isLiveMode ); }

template <typename charType>
static inline CIMGArchiveTranslatorHandle* GenCreateCompressedIMGArchive( CFileSystem *sys, CFileTranslator *srcRoot, const charType *srcPath, eIMGArchiveVersion version, bool isLiveMode )
{
    CIMGArchiveTranslatorHandle *archiveHandle = NULL;
    {
        imgExtension *imgExt = imgExtension::Get( sys );

        if ( imgExt )
        {
            // Create a translator specifically with the LZO compression algorithm.
            archiveHandle = GenNewArchiveTemplate( imgExt, srcRoot, srcPath, version, _lzoCompressedIMGConstructor, isLiveMode );
        }
    }

    return archiveHandle;
}

CIMGArchiveTranslatorHandle* CFileSystem::CreateCompressedIMGArchive( CFileTranslator *srcRoot, const char *srcPath, eIMGArchiveVersion version, bool isLiveMode )
{ return GenCreateCompressedIMGArchive( this, srcRoot, srcPath, version, isLiveMode ); }
CIMGArchiveTranslatorHandle* CFileSystem::CreateCompressedIMGArchive( CFileTranslator *srcRoot, const wchar_t *srcPath, eIMGArchiveVersion version, bool isLiveMode )
{ return GenCreateCompressedIMGArchive( this, srcRoot, srcPath, version, isLiveMode ); }

// Sub modules.
extern void InitializeXBOXIMGCompressionEnvironment( const fs_construction_params& params );

extern void ShutdownXBOXIMGCompressionEnvironment( void );

fileSystemFactory_t::pluginOffset_t imgExtension::_imgPluginOffset = fileSystemFactory_t::INVALID_PLUGIN_OFFSET;

void CFileSystemNative::RegisterIMGDriver( const fs_construction_params& params )
{
    imgExtension::_imgPluginOffset =
        _fileSysFactory.RegisterDependantStructPlugin <imgExtension> ( fileSystemFactory_t::ANONYMOUS_PLUGIN_ID );

    // Register sub modules.
    InitializeXBOXIMGCompressionEnvironment( params );
}

void CFileSystemNative::UnregisterIMGDriver( void )
{
    // Unregister sub modules.
    ShutdownXBOXIMGCompressionEnvironment();

    if ( imgExtension::_imgPluginOffset != fileSystemFactory_t::INVALID_PLUGIN_OFFSET )
    {
        _fileSysFactory.UnregisterPlugin( imgExtension::_imgPluginOffset );
    }
}