// Main include file of the PlayStation Portable native texture.

#ifdef RWLIB_INCLUDE_NATIVETEX_PSP

#include "txdread.nativetex.hxx"

#include "txdread.d3d.hxx"

#include "txdread.ps2shared.hxx"

#include "txdread.common.hxx"

#define PSP_FOURCC  0x00505350      // 'PSP\0'

namespace rw
{

static inline uint32 getPSPTextureDataRowAlignment( void )
{
    // For compatibility reasons, we say that swizzled mipmap data has a row alignment of 1.
    // It should not matter for any of the operations we do.
    return 1;
}

static inline uint32 getPSPExportTextureDataRowAlignment( void )
{
    // This row alignment should be a framework friendly size.
    // To make things most-compatible with Direct3D, a size of 4 is recommended.
    return 4;
}

inline uint32 getPSPRasterDataRowSize( uint32 mipWidth, uint32 depth )
{
    return getRasterDataRowSize( mipWidth, depth, getPSPTextureDataRowAlignment() );
}

static inline eRasterFormat decodeDepthRasterFormat( uint32 depth, eColorOrdering& colorOrderOut, ePaletteType& paletteTypeOut )
{
    if ( depth == 4 || depth == 8 )
    {
        colorOrderOut = COLOR_RGBA;
        
        if ( depth == 4 )
        {
            paletteTypeOut = PALETTE_4BIT;
        }
        else if ( depth == 8 )
        {
            paletteTypeOut = PALETTE_8BIT;
        }
        
        return RASTER_8888;
    }
    else if ( depth == 16 )
    {
        colorOrderOut = COLOR_RGBA;
        paletteTypeOut = PALETTE_NONE;
        return RASTER_1555;
    }
    else if ( depth == 32 )
    {
        colorOrderOut = COLOR_RGBA;
        paletteTypeOut = PALETTE_NONE;
        return RASTER_8888;
    }

    colorOrderOut = COLOR_RGBA;
    paletteTypeOut = PALETTE_NONE;
    return RASTER_DEFAULT;
}

static inline bool isPSPSwizzlingRequired( uint32 baseWidth, uint32 baseHeight, uint32 depth )
{
    // TODO: not sure about this at all.

    if ( baseWidth > 256 || baseHeight > 256 )
    {
        return false;
    }

    if ( depth == 4 )
    {
        if ( baseWidth < 32 )
        {
            return false;
        }
    }
    else if ( depth == 8 )
    {
        if ( baseWidth < 16 )
        {
            return false;
        }
    }
    else if ( depth == 32 )
    {
        if ( baseWidth < 8 || baseHeight < 8 )
        {
            return false;
        }
    }

    // The PSP native texture uses a very narrow swizzling convention.
    // It is the first native texture I encountered that uses per-level swizzling flags.
    if ( baseWidth > baseHeight * 2 )
    {
        return false;
    }

    if ( baseWidth < 256 && baseHeight < 256 )
    {
        if ( baseHeight > baseWidth )
        {
            return false;
        }
    }

    return true;
}

static inline eFormatEncodingType getPSPHardwareColorBufferFormat( uint32 depth )
{
    if ( depth == 4 || depth == 8 )
    {
        return FORMAT_TEX32;
    }
    else if ( depth == 16 )
    {
        return FORMAT_TEX16;
    }
    else if ( depth == 32 )
    {
        return FORMAT_TEX32;
    }

    return FORMAT_UNKNOWN;
}

static inline bool getPSPBrokenPackedFormatDimensions(
    eFormatEncodingType rawFormat, eFormatEncodingType packedFormat,
    uint32 layerWidth, uint32 layerHeight,
    uint32& packedWidthOut, uint32& packedHeightOut
)
{
    // This routine is broken because it does not honor the memory sheme of the GPU.
    // The PSP GPU stores data in memory columns.

    bool gotProposedDimms = false;

    uint32 prop_packedWidth, prop_packedHeight;

    if ( packedFormat == FORMAT_TEX32 )
    {
        if ( rawFormat == FORMAT_IDTEX8 )
        {
            prop_packedWidth = ( layerWidth / 2 );
            prop_packedHeight = ( layerHeight / 2 );
            
            gotProposedDimms = true;
        }
        else if ( rawFormat == FORMAT_IDTEX8_COMPRESSED )
        {
            prop_packedWidth = ( layerWidth / 4 );
            prop_packedHeight = ( layerHeight / 2 );
            
            gotProposedDimms = true;
        }
    }

    if ( gotProposedDimms )
    {
        if ( prop_packedWidth != 0 && prop_packedHeight != 0 )
        {
            packedWidthOut = prop_packedWidth;
            packedHeightOut = prop_packedHeight;
            return true;
        }
    }

    return false;
}

struct NativeTexturePSP
{
    Interface *engineInterface;

    LibraryVersion texVersion;

    inline NativeTexturePSP( Interface *engineInterface )
    {
        this->engineInterface = engineInterface;
        this->texVersion = engineInterface->GetVersion();
        this->depth = 0;
        this->colorBufferFormat = FORMAT_UNKNOWN;
        this->palette = NULL;
        this->unk = 0;
    }

    inline NativeTexturePSP( const NativeTexturePSP& right ) : mipmaps()
    {
        Interface *engineInterface = right.engineInterface;

        uint32 depth = right.depth;

        this->engineInterface = engineInterface;
        this->texVersion = right.texVersion;
        this->depth = depth;
        this->colorBufferFormat = right.colorBufferFormat;

        // Clone color buffers.
        size_t mipmapCount = right.mipmaps.size();

        this->mipmaps.resize( mipmapCount );

        try
        {
            for ( size_t n = 0; n < mipmapCount; n++ )
            {
                const GETexture& srcLayer = right.mipmaps[ n ];

                GETexture& dstLayer = this->mipmaps[ n ];

                uint32 dataSize = srcLayer.dataSize;

                void *newbuf = engineInterface->PixelAllocate( dataSize );

                if ( !newbuf )
                {
                    throw RwException( "failed to allocate memory buffer for PSP native texture color buffer clone" );
                }

                memcpy( newbuf, srcLayer.texels, dataSize );

                dstLayer.width = srcLayer.width;
                dstLayer.height = srcLayer.height;
                dstLayer.texels = newbuf;
                dstLayer.dataSize = dataSize;

                dstLayer.isSwizzled = srcLayer.isSwizzled;
            }

            ePaletteType srcPaletteType;
            eColorOrdering colorOrder;

            eRasterFormat rasterFormat = decodeDepthRasterFormat( depth, colorOrder, srcPaletteType );

            void *dstPalette = NULL;

            if ( void *srcPalette = right.palette )
            {
                uint32 srcPaletteSize = getPaletteItemCount( srcPaletteType );

                uint32 palRasterDepth = Bitmap::getRasterFormatDepth( rasterFormat );

                uint32 palDataSize = getPaletteDataSize( srcPaletteSize, palRasterDepth );

                dstPalette = engineInterface->PixelAllocate( palDataSize );

                if ( !dstPalette )
                {
                    throw RwException( "failed to allocate palette buffer for PSP native texture cloning" );
                }

                memcpy( dstPalette, srcPalette, palDataSize );
            }

            this->palette = dstPalette;

            this->unk = right.unk;
        }
        catch( ... )
        {
            for ( size_t n = 0; n < mipmapCount; n++ )
            {
                this->mipmaps[ n ].Deallocate( engineInterface );
            }

            throw;
        }
    }

    inline ~NativeTexturePSP( void )
    {
        // Free all color buffer info.
        Interface *engineInterface = this->engineInterface;

        size_t mipmapCount = this->mipmaps.size();

        for ( size_t n = 0; n < mipmapCount; n++ )
        {
            this->mipmaps[n].Deallocate( engineInterface );
        }

        if ( void *palData = this->palette )
        {
            engineInterface->PixelFree( palData );

            this->palette = NULL;
        }
    }

    void UpdateStructure( Interface *engineInterface )
    {
        // todo.
        // I don't think we will ever have to do anything over here.
    }

    struct GETexture
    {
        inline GETexture( void )
        {
            this->width = 0;
            this->height = 0;
            this->texels = NULL;
            this->dataSize = 0;

            this->isSwizzled = false;
        }

        inline GETexture( const GETexture& right )
        {
            throw RwException( "cannot clone NativeTexturePSP::GETexture unit with standard copy constructor" );
        }

        inline GETexture( GETexture&& right )
        {
            this->width = right.width;
            this->height = right.height;
            this->texels = right.texels;
            this->dataSize = right.dataSize;

            this->isSwizzled = right.isSwizzled;

            right.texels = NULL;
        }

        inline ~GETexture( void )
        {
            assert( this->texels == NULL );
        }

        inline void Deallocate( Interface *engineInterface )
        {
            if ( void *texels = this->texels )
            {
                engineInterface->PixelFree( texels );

                this->texels = NULL;
                this->dataSize = 0;
            }
        }

        uint32 width;
        uint32 height;
        void *texels;
        uint32 dataSize;

        bool isSwizzled;
    };

    // Data members.
    uint32 depth;       // this field determines the raster format.

    eFormatEncodingType colorBufferFormat;

    std::vector <GETexture> mipmaps;

    // Color Look Up Table (CLUT).
    // Could be swizzled, but not a GSTexture like on the PS2.
    void *palette;

    // Unknowns.
    uint32 unk;
};

static inline void getPSPNativeTextureSizeRules( nativeTextureSizeRules& rulesOut )
{
    // Sort of the same as in the PS2 native texture.
    rulesOut.powerOfTwo = true;
    rulesOut.squared = false;
    rulesOut.maximum = true;
    rulesOut.maxVal = 1024;
}

struct pspNativeTextureTypeProvider : public texNativeTypeProvider
{
    void ConstructTexture( Interface *engineInterface, void *objMem, size_t memSize ) override
    {
        new (objMem) NativeTexturePSP( engineInterface );
    }

    void CopyConstructTexture( Interface *engineInterface, void *objMem, const void *srcObjMem, size_t memSize ) override
    {
        new (objMem) NativeTexturePSP( *(const NativeTexturePSP*)srcObjMem );
    }
    
    void DestroyTexture( Interface *engineInterface, void *objMem, size_t memSize ) override
    {
        ( *(NativeTexturePSP*)objMem ).~NativeTexturePSP();
    }

    eTexNativeCompatibility IsCompatibleTextureBlock( BlockProvider& inputProvider ) const override;

    void SerializeTexture( TextureBase *theTexture, PlatformTexture *nativeTex, BlockProvider& outputProvider ) const override;
    void DeserializeTexture( TextureBase *theTexture, PlatformTexture *nativeTex, BlockProvider& inputProvider ) const override;

    void GetPixelCapabilities( pixelCapabilities& capsOut ) const override
    {
        // No idea whether it actually does support DXT.
        // There is still one unknown field, but we may never know!
        capsOut.supportsDXT1 = false;
        capsOut.supportsDXT2 = false;
        capsOut.supportsDXT3 = false;
        capsOut.supportsDXT4 = false;
        capsOut.supportsDXT5 = false;
        capsOut.supportsPalette = true;
    }

    void GetStorageCapabilities( storageCapabilities& storeCaps ) const override
    {
        storeCaps.pixelCaps.supportsDXT1 = false;
        storeCaps.pixelCaps.supportsDXT2 = false;
        storeCaps.pixelCaps.supportsDXT3 = false;
        storeCaps.pixelCaps.supportsDXT4 = false;
        storeCaps.pixelCaps.supportsDXT5 = false;
        storeCaps.pixelCaps.supportsPalette = true;

        storeCaps.isCompressedFormat = false;
    }

    void GetPixelDataFromTexture( Interface *engineInterface, void *objMem, pixelDataTraversal& pixelsOut ) override;
    void SetPixelDataToTexture( Interface *engineInterface, void *objMem, const pixelDataTraversal& pixelsIn, acquireFeedback_t& feedbackOut ) override;
    void UnsetPixelDataFromTexture( Interface *engineInterface, void *objMem, bool deallocate ) override;

    void SetTextureVersion( Interface *engineInterface, void *objMem, LibraryVersion version ) override
    {
        NativeTexturePSP *nativeTex = (NativeTexturePSP*)objMem;

        nativeTex->texVersion = version;

        nativeTex->UpdateStructure( engineInterface );
    }

    LibraryVersion GetTextureVersion( const void *objMem ) override
    {
        const NativeTexturePSP *nativeTex = (const NativeTexturePSP*)objMem;

        return nativeTex->texVersion;
    }

    bool GetMipmapLayer( Interface *engineInterface, void *objMem, uint32 mipIndex, rawMipmapLayer& layerOut ) override;
    bool AddMipmapLayer( Interface *engineInterface, void *objMem, const rawMipmapLayer& layerIn, acquireFeedback_t& feedbackOut ) override;
    void ClearMipmaps( Interface *engineInterface, void *objMem ) override;

    void GetTextureInfo( Interface *engineInterface, void *objMem, nativeTextureBatchedInfo& infoOut ) override
    {
        const NativeTexturePSP *pspTex = (NativeTexturePSP*)objMem;

        uint32 baseWidth = 0;
        uint32 baseHeight = 0;

        uint32 mipmapCount = (uint32)pspTex->mipmaps.size();

        if ( mipmapCount > 0 )
        {
            const NativeTexturePSP::GETexture& baseLayer = pspTex->mipmaps[ 0 ];

            baseWidth = baseLayer.width;
            baseHeight = baseLayer.height;
        }

        infoOut.baseWidth = baseWidth;
        infoOut.baseHeight = baseHeight;
        infoOut.mipmapCount = mipmapCount;
    }

    void GetTextureFormatString( Interface *engineInterface, void *objMem, char *buf, size_t bufLen, size_t& lengthOut ) const override
    {
        NativeTexturePSP *nativeTex = (NativeTexturePSP*)objMem;

        // Very similar procedure to the PS2 native texture here.
        std::string formatString = "PSP ";

        uint32 depth = nativeTex->depth;

        eColorOrdering colorOrder;
        ePaletteType paletteType;

        eRasterFormat rasterFormat = decodeDepthRasterFormat( depth, colorOrder, paletteType );

        getDefaultRasterFormatString( rasterFormat, depth, paletteType, colorOrder, formatString );

        if ( buf )
        {
            strncpy( buf, formatString.c_str(), bufLen );
        }

        lengthOut = formatString.length();
    }

    eRasterFormat GetTextureRasterFormat( const void *objMem ) override
    {
        const NativeTexturePSP *nativeTex = (const NativeTexturePSP*)objMem;

        eColorOrdering colorOrdering;
        ePaletteType paletteType;

        return decodeDepthRasterFormat( nativeTex->depth, colorOrdering, paletteType );
    }

    ePaletteType GetTexturePaletteType( const void *objMem ) override
    {
        const NativeTexturePSP *nativeTex = (const NativeTexturePSP*)objMem;

        eColorOrdering colorOrdering;
        ePaletteType paletteType;

        decodeDepthRasterFormat( nativeTex->depth, colorOrdering, paletteType );

        return paletteType;
    }

    bool IsTextureCompressed( const void *objMem ) override
    {
        // TODO: I heard PSP actually supports DXTn?
        return false;
    }

    eCompressionType GetTextureCompressionFormat( const void *objMem ) override
    {
        // TODO: maybe there is DXT support after all?
        // Rockstar Leeds, pls.
        return RWCOMPRESS_NONE;
    }

    bool DoesTextureHaveAlpha( const void *objMem ) override;

    uint32 GetTextureDataRowAlignment( void ) const override
    {
        return getPSPTextureDataRowAlignment();
    }

    void GetFormatSizeRules( const pixelFormat& format, nativeTextureSizeRules& rulesOut ) const override
    {
        getPSPNativeTextureSizeRules( rulesOut );
    }

    void GetTextureSizeRules( const void *objMem, nativeTextureSizeRules& rulesOut ) const override
    {
        // TODO: I have no idea what formats are supported yet.
        // The PSP does support DXT after all, but there is no trace of this.
        getPSPNativeTextureSizeRules( rulesOut );
    }

    uint32 GetDriverIdentifier( void *objMem ) const override
    {
        // There has been no driver id assigned to the PSP native texture.
        // Because the format never advanced beyond RW 3.1.
        return 0;
    }

    inline void Initialize( EngineInterface *engineInterface )
    {
        RegisterNativeTextureType( engineInterface, "PSP", this, sizeof( NativeTexturePSP ) );
    }

    inline void Shutdown( EngineInterface *engineInterface )
    {
        UnregisterNativeTextureType( engineInterface, "PSP" );
    }
};

namespace psp
{

struct textureMetaDataHeader
{
    // This is the master meta data header.
    // You can see how poorly optimized this format is.
    // Doesnt look like R*Leeds had the top minds of the to-the-metal programming squad.
    endian::little_endian <uint32> width;
    endian::little_endian <uint32> height;
    endian::little_endian <uint32> depth;
    endian::little_endian <uint32> mipmapCount;
    endian::little_endian <uint32> unknown;     // ???, always zero? reserved?
};

}

};

#endif //RWLIB_INCLUDE_NATIVETEX_PSP