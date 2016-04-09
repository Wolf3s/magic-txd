#include "StdInc.h"

#include <cstring>
#include <assert.h>
#include <bitset>
#define _USE_MATH_DEFINES
#include <math.h>
#include <map>
#include <algorithm>
#include <cmath>

#include "txdread.natcompat.hxx"

#include "pluginutil.hxx"

#include "txdread.common.hxx"

#include "rwserialize.hxx"

namespace rw
{

/*
 * Texture Dictionary
 */

TexDictionary* texDictionaryStreamPlugin::CreateTexDictionary( EngineInterface *engineInterface ) const
{
    GenericRTTI *rttiObj = engineInterface->typeSystem.Construct( engineInterface, this->txdTypeInfo, NULL );

    if ( rttiObj == NULL )
    {
        return NULL;
    }
    
    TexDictionary *txdObj = (TexDictionary*)RwTypeSystem::GetObjectFromTypeStruct( rttiObj );

    return txdObj;
}

TexDictionary* texDictionaryStreamPlugin::ToTexDictionary( EngineInterface *engineInterface, RwObject *rwObj )
{
    if ( isRwObjectInheritingFrom( engineInterface, rwObj, this->txdTypeInfo ) )
    {
        return (TexDictionary*)rwObj;
    }

    return NULL;
}

const TexDictionary* texDictionaryStreamPlugin::ToConstTexDictionary( EngineInterface *engineInterface, const RwObject *rwObj )
{
    if ( isRwObjectInheritingFrom( engineInterface, rwObj, this->txdTypeInfo ) )
    {
        return (const TexDictionary*)rwObj;
    }

    return NULL;
}

void texDictionaryStreamPlugin::Deserialize( Interface *intf, BlockProvider& inputProvider, RwObject *objectToDeserialize ) const
{
    EngineInterface *engineInterface = (EngineInterface*)intf;

    // Cast our object.
    TexDictionary *txdObj = (TexDictionary*)objectToDeserialize;

    // Read the textures.
    {
        BlockProvider texDictMetaStructBlock( &inputProvider );

        uint32 textureBlockCount = 0;
        bool requiresRecommendedPlatform = true;
        uint16 recDevicePlatID = 0;

        texDictMetaStructBlock.EnterContext();

        try
        {
            if ( texDictMetaStructBlock.getBlockID() == CHUNK_STRUCT )
            {
                // Read the header block depending on version.
                LibraryVersion libVer = texDictMetaStructBlock.getBlockVersion();

                if (libVer.rwLibMajor <= 2 || libVer.rwLibMajor == 3 && libVer.rwLibMinor <= 5)
                {
                    textureBlockCount = texDictMetaStructBlock.readUInt32();
                }
                else
                {
                    textureBlockCount = texDictMetaStructBlock.readUInt16();
                    uint16 recommendedPlatform = texDictMetaStructBlock.readUInt16();

                    // So if there is a recommended platform set, we will also give it one if we will write it.
                    requiresRecommendedPlatform = ( recommendedPlatform != 0 );
                    recDevicePlatID = recommendedPlatform;  // we want to store it aswell.
                }
            }
            else
            {
                engineInterface->PushWarning( "could not find texture dictionary meta information" );
            }
        }
        catch( ... )
        {
            texDictMetaStructBlock.LeaveContext();
            
            throw;
        }

        // We finished reading meta data.
        texDictMetaStructBlock.LeaveContext();

        txdObj->hasRecommendedPlatform = requiresRecommendedPlatform;
        txdObj->recDevicePlatID = recDevicePlatID;

        // Now follow multiple TEXTURENATIVE blocks.
        // Deserialize all of them.

        for ( uint32 n = 0; n < textureBlockCount; n++ )
        {
            BlockProvider textureNativeBlock( &inputProvider );

            // Deserialize this block.
            RwObject *rwObj = NULL;

            std::string errDebugMsg;

            try
            {
                rwObj = engineInterface->DeserializeBlock( textureNativeBlock );
            }
            catch( RwException& except )
            {
                // Catch the exception and try to continue.
                rwObj = NULL;

                if ( textureNativeBlock.doesIgnoreBlockRegions() )
                {
                    // If we failed any texture parsing in the "ignoreBlockRegions" parse mode,
                    // there is no point in continuing, since the environment does not recover.
                    throw;
                }

                errDebugMsg = except.message;
            }

            if ( rwObj )
            {
                // If it is a texture, add it to our TXD.
                bool hasBeenAddedToTXD = false;

                GenericRTTI *rttiObj = RwTypeSystem::GetTypeStructFromObject( rwObj );

                RwTypeSystem::typeInfoBase *typeInfo = RwTypeSystem::GetTypeInfoFromTypeStruct( rttiObj );

                if ( engineInterface->typeSystem.IsTypeInheritingFrom( engineInterface->textureTypeInfo, typeInfo ) )
                {
                    TextureBase *texture = (TextureBase*)rwObj;

                    texture->AddToDictionary( txdObj );

                    hasBeenAddedToTXD = true;
                }

                // If it has not been added, delete it.
                if ( hasBeenAddedToTXD == false )
                {
                    engineInterface->DeleteRwObject( rwObj );
                }
            }
            else
            {
                std::string pushWarning;

                if ( errDebugMsg.empty() == false )
                {
                    pushWarning = "texture native reading failure: ";
                    pushWarning += errDebugMsg;
                }
                else
                {
                    pushWarning = "failed to deserialize texture native block in texture dictionary";
                }

                engineInterface->PushWarning( pushWarning.c_str() );
            }
        }
    }

    // Read extensions.
    engineInterface->DeserializeExtensions( txdObj, inputProvider );
}

TexDictionary::TexDictionary( const TexDictionary& right ) : RwObject( right )
{
    // Create a new dictionary with all the textures.
    this->hasRecommendedPlatform = right.hasRecommendedPlatform;
    this->recDevicePlatID = right.recDevicePlatID;
    
    this->numTextures = 0;

    LIST_CLEAR( textures.root );

    Interface *engineInterface = right.engineInterface;

    LIST_FOREACH_BEGIN( TextureBase, right.textures.root, texDictNode )

        TextureBase *texture = item;

        // Clone the texture and insert it into us.
        TextureBase *newTex = (TextureBase*)engineInterface->CloneRwObject( texture );

        if ( newTex )
        {
            newTex->AddToDictionary( this );
        }

    LIST_FOREACH_END
}

TexDictionary::~TexDictionary( void )
{
    // Delete all textures that are part of this dictionary.
    while ( LIST_EMPTY( this->textures.root ) == false )
    {
        TextureBase *theTexture = LIST_GETITEM( TextureBase, this->textures.root.next, texDictNode );

        // Remove us from this TXD.
        // This is done because we cannot be sure that the texture is actually destroyed.
        theTexture->RemoveFromDictionary();

        // Delete us.
        this->engineInterface->DeleteRwObject( theTexture );
    }
}

static PluginDependantStructRegister <texDictionaryStreamPlugin, RwInterfaceFactory_t> texDictionaryStreamStore;

void TexDictionary::clear(void)
{
	// We remove the links of all textures inside of us.
    while ( LIST_EMPTY( this->textures.root ) == false )
    {
        TextureBase *texture = LIST_GETITEM( TextureBase, this->textures.root.next, texDictNode );

        // Call the texture's own removal.
        texture->RemoveFromDictionary();
    }
}

const char* TexDictionary::GetRecommendedDriverPlatform( void ) const
{
    EngineInterface *engineInterface = (EngineInterface*)this->engineInterface;

    texNativeTypeProvider *provider = NULL;

    uint16 driverID = GetTexDictionaryRecommendedDriverID( engineInterface, this, &provider );

    if ( !provider )
        return NULL;

    // HACK: reach into the type system name info directly.
    return provider->managerData.rwTexType->name;
}

TexDictionary* CreateTexDictionary( Interface *intf )
{
    EngineInterface *engineInterface = (EngineInterface*)intf;

    TexDictionary *texDictOut = NULL;

    texDictionaryStreamPlugin *txdStream = texDictionaryStreamStore.GetPluginStruct( engineInterface );

    if ( txdStream )
    {
        texDictOut = txdStream->CreateTexDictionary( engineInterface );
    }

    return texDictOut;
}

TexDictionary* ToTexDictionary( Interface *intf, RwObject *rwObj )
{
    EngineInterface *engineInterface = (EngineInterface*)intf;

    texDictionaryStreamPlugin *txdStream = texDictionaryStreamStore.GetPluginStruct( engineInterface );

    if ( txdStream )
    {
        return txdStream->ToTexDictionary( engineInterface, rwObj );
    }

    return NULL;
}

const TexDictionary* ToConstTexDictionary( Interface *intf, const RwObject *rwObj )
{
    EngineInterface *engineInterface = (EngineInterface*)intf;

    texDictionaryStreamPlugin *txdStream = texDictionaryStreamStore.GetPluginStruct( engineInterface );

    if ( txdStream )
    {
        return txdStream->ToConstTexDictionary( engineInterface, rwObj );
    }

    return NULL;
}

/*
 * Texture Base
 */

TextureBase::TextureBase( const TextureBase& right ) : RwObject( right )
{
    // General cloning business.
    this->texRaster = AcquireRaster( right.texRaster );
    this->name = right.name;
    this->maskName = right.maskName;
    this->filterMode = right.filterMode;
    this->uAddressing = right.uAddressing;
    this->vAddressing = right.vAddressing;
    
    // We do not want to belong to a TXD by default.
    // Even if the original texture belonged to one.
    this->texDict = NULL;
}

TextureBase::~TextureBase( void )
{
    // Clear our raster.
    this->SetRaster( NULL );

    // Make sure we are not in a texture dictionary.
    this->RemoveFromDictionary();
}

void TextureBase::SetRaster( Raster *texRaster )
{
    // If we had a previous raster, unlink it.
    if ( Raster *prevRaster = this->texRaster )
    {
        DeleteRaster( prevRaster );

        this->texRaster = NULL;
    }

    if ( texRaster )
    {
        // We get a new reference to the raster.
        this->texRaster = AcquireRaster( texRaster );

        if ( Raster *ourRaster = this->texRaster )
        {
            // Set the virtual version of this texture.
            this->objVersion = ourRaster->GetEngineVersion();
        }
    }
}

void TextureBase::AddToDictionary( TexDictionary *dict )
{
    this->RemoveFromDictionary();

    // Note: original RenderWare performs an insert, not an append.
    // I switched this around, so that textures stay in the correct order.
    LIST_APPEND( dict->textures.root, texDictNode );

    dict->numTextures++;

    this->texDict = dict;
}

void TextureBase::RemoveFromDictionary( void )
{
    TexDictionary *belongingTXD = this->texDict;

    if ( belongingTXD != NULL )
    {
        LIST_REMOVE( this->texDictNode );

        belongingTXD->numTextures--;

        this->texDict = NULL;
    }
}

TexDictionary* TextureBase::GetTexDictionary( void ) const
{
    return this->texDict;
}

TextureBase* CreateTexture( Interface *intf, Raster *texRaster )
{
    EngineInterface *engineInterface = (EngineInterface*)intf;

    TextureBase *textureOut = NULL;

    if ( RwTypeSystem::typeInfoBase *textureTypeInfo = engineInterface->textureTypeInfo )
    {
        GenericRTTI *rtObj = engineInterface->typeSystem.Construct( engineInterface, textureTypeInfo, NULL );

        if ( rtObj )
        {
            textureOut = (TextureBase*)RwTypeSystem::GetObjectFromTypeStruct( rtObj );

            // Set the raster into the texture.
            textureOut->SetRaster( texRaster );
        }
    }

    return textureOut;
}

TextureBase* ToTexture( Interface *intf, RwObject *rwObj )
{
    EngineInterface *engineInterface = (EngineInterface*)intf;

    if ( isRwObjectInheritingFrom( engineInterface, rwObj, engineInterface->textureTypeInfo ) )
    {
        return (TextureBase*)rwObj;
    }

    return NULL;
}

const TextureBase* ToConstTexture( Interface *intf, const RwObject *rwObj )
{
    EngineInterface *engineInterface = (EngineInterface*)intf;

    if ( isRwObjectInheritingFrom( engineInterface, rwObj, engineInterface->textureTypeInfo ) )
    {
        return (const TextureBase*)rwObj;
    }

    return NULL;
}

// Filtering helper API.
void TextureBase::improveFiltering(void)
{
    // This routine scaled up the filtering settings of this texture.
    // When rendered, this texture will appear smoother.

    // Handle stuff depending on our current settings.
    eRasterStageFilterMode currentFilterMode = this->filterMode;

    eRasterStageFilterMode newFilterMode = currentFilterMode;

    if ( currentFilterMode == RWFILTER_POINT )
    {
        newFilterMode = RWFILTER_LINEAR;
    }
    else if ( currentFilterMode == RWFILTER_POINT_POINT )
    {
        newFilterMode = RWFILTER_POINT_LINEAR;
    }

    // Update our texture fields.
    if ( currentFilterMode != newFilterMode )
    {
        this->filterMode = newFilterMode;
    }
}

void TextureBase::fixFiltering(void)
{
    // Only do things if we have a raster.
    if ( Raster *texRaster = this->texRaster )
    {
        // Adjust filtering mode.
        eRasterStageFilterMode currentFilterMode = this->GetFilterMode();

        uint32 actualNewMipmapCount = texRaster->getMipmapCount();

        // We need to represent a correct filter state, depending on the mipmap count
        // of the native texture. This is required to enable mipmap rendering, when required!
        if ( actualNewMipmapCount > 1 )
        {
            if ( currentFilterMode == RWFILTER_POINT )
            {
                this->SetFilterMode( RWFILTER_POINT_POINT );
            }
            else if ( currentFilterMode == RWFILTER_LINEAR )
            {
                this->SetFilterMode( RWFILTER_LINEAR_LINEAR );
            }
        }
        else
        {
            if ( currentFilterMode == RWFILTER_POINT_POINT ||
                 currentFilterMode == RWFILTER_POINT_LINEAR )
            {
                this->SetFilterMode( RWFILTER_POINT );
            }
            else if ( currentFilterMode == RWFILTER_LINEAR_POINT ||
                      currentFilterMode == RWFILTER_LINEAR_LINEAR )
            {
                this->SetFilterMode( RWFILTER_LINEAR );
            }
        }

        // TODO: if there is a filtering mode that does not make sense at all,
        // how should we handle it?
    }
}

inline bool isValidFilterMode( uint32 binaryFilterMode )
{
    if ( binaryFilterMode == RWFILTER_POINT ||
         binaryFilterMode == RWFILTER_LINEAR ||
         binaryFilterMode == RWFILTER_POINT_POINT ||
         binaryFilterMode == RWFILTER_LINEAR_POINT ||
         binaryFilterMode == RWFILTER_POINT_LINEAR ||
         binaryFilterMode == RWFILTER_LINEAR_LINEAR )
    {
        return true;
    }

    return false;
}

inline bool isValidTexAddressingMode( uint32 binary_addressing )
{
    if ( binary_addressing == RWTEXADDRESS_WRAP ||
         binary_addressing == RWTEXADDRESS_MIRROR ||
         binary_addressing == RWTEXADDRESS_CLAMP ||
         binary_addressing == RWTEXADDRESS_BORDER )
    {
        return true;
    }

    return false;
}

inline void SetFormatInfoToTexture(
    TextureBase& outTex,
    uint32 binaryFilterMode, uint32 binary_uAddressing, uint32 binary_vAddressing,
    bool isLikelyToFail
)
{
    eRasterStageFilterMode rwFilterMode = RWFILTER_LINEAR;

    eRasterStageAddressMode rw_uAddressing = RWTEXADDRESS_WRAP;
    eRasterStageAddressMode rw_vAddressing = RWTEXADDRESS_WRAP;

    // If we are likely to fail, we should check if we should even output warnings.
    bool doOutputWarnings = true;

    if ( isLikelyToFail )
    {
        // If we are likely to fail, we do not want to print as many warnings to the user.
        doOutputWarnings = false;

        const uint32 reqWarningLevel = 4;

        if ( outTex.engineInterface->GetWarningLevel() >= reqWarningLevel )
        {
            doOutputWarnings = true;
        }
    }

    // Make sure they are valid.
    if ( isValidFilterMode( binaryFilterMode ) )
    {
        rwFilterMode = (eRasterStageFilterMode)binaryFilterMode;
    }
    else
    {
        if ( doOutputWarnings )
        {
            outTex.engineInterface->PushObjWarningVerb( &outTex, "has an invalid filter mode; defaulting to linear" );
        }
    }

    if ( isValidTexAddressingMode( binary_uAddressing ) )
    {
        rw_uAddressing = (eRasterStageAddressMode)binary_uAddressing;
    }
    else
    {
        if ( doOutputWarnings )
        {
            outTex.engineInterface->PushObjWarningVerb( &outTex, "has an invalid uAddressing mode; defaulting to wrap" );
        }
    }

    if ( isValidTexAddressingMode( binary_vAddressing ) )
    {
        rw_vAddressing = (eRasterStageAddressMode)binary_vAddressing;
    }
    else
    {
        if ( doOutputWarnings )
        {
            outTex.engineInterface->PushObjWarningVerb( &outTex, "has an invalid vAddressing mode; defaulting to wrap" );
        }
    }

    // Put the fields.
    outTex.SetFilterMode( rwFilterMode );
    outTex.SetUAddressing( rw_uAddressing );
    outTex.SetVAddressing( rw_vAddressing );
}

void texFormatInfo::parse(TextureBase& outTex, bool isLikelyToFail) const
{
    // Read our fields, which are from a binary stream.
    uint32 binaryFilterMode = this->filterMode;

    uint32 binary_uAddressing = this->uAddressing;
    uint32 binary_vAddressing = this->vAddressing;

    SetFormatInfoToTexture(
        outTex,
        binaryFilterMode, binary_uAddressing, binary_vAddressing,
        isLikelyToFail
    );
}

void texFormatInfo::set(const TextureBase& inTex)
{
    this->filterMode = (uint32)inTex.GetFilterMode();
    this->uAddressing = (uint32)inTex.GetUAddressing();
    this->vAddressing = (uint32)inTex.GetVAddressing();
    this->pad1 = 0;
}

void texFormatInfo::writeToBlock(BlockProvider& outputProvider) const
{
    texFormatInfo_serialized <endian::little_endian <uint32>> serStruct;
    serStruct.info = *(uint32*)this;

    outputProvider.writeStruct( serStruct );
}

void texFormatInfo::readFromBlock(BlockProvider& inputProvider)
{
    texFormatInfo_serialized <endian::little_endian <uint32>> serStruct;
    
    inputProvider.readStruct( serStruct );

    *(uint32*)this = serStruct.info;
}

void wardrumFormatInfo::parse(TextureBase& outTex) const
{
    // Read our fields, which are from a binary stream.
    uint32 binaryFilterMode = this->filterMode;

    uint32 binary_uAddressing = this->uAddressing;
    uint32 binary_vAddressing = this->vAddressing;

    SetFormatInfoToTexture(
        outTex,
        binaryFilterMode, binary_uAddressing, binary_vAddressing,
        false
    );
}

void wardrumFormatInfo::set(const TextureBase& inTex)
{
    this->filterMode = (uint32)inTex.GetFilterMode();
    this->uAddressing = (uint32)inTex.GetUAddressing();
    this->vAddressing = (uint32)inTex.GetVAddressing();
}

// Main modules.
void registerNativeTexturePlugins( void );

// Sub modules.
void registerResizeFilteringEnvironment( void );

void registerTXDPlugins( void )
{
    // First register the main serialization plugins.
    // Those are responsible for detecting texture dictionaries and native textures in RW streams.
    // The sub module plugins depend on those.
    texDictionaryStreamStore.RegisterPlugin( engineFactory );
    registerNativeTexturePlugins();

    // Register pure sub modules.
    registerResizeFilteringEnvironment();
}

}
