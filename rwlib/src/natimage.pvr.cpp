// PowerVR file format support for RenderWare, because mobile games tend to use it.
// This file format started out as an inspiration over DDS while adding support for
// many Imagination Technologies formats (PVR 2bpp, PVR 4bpp, ETC, ....).

#include "StdInc.h"

#include "natimage.hxx"

#include "txdread.pvr.hxx"
#include "txdread.d3d8.hxx"
#include "txdread.d3d9.hxx"

#include "streamutil.hxx"

namespace rw
{

// A trend of Linux-invented file formats is that they come in dynamic endianness.
// Instead of standardizing the endianness to a specific value they allow you to write it
// in whatever way and the guys writing the parser gotta make a smart enough implementation
// to detect any case.
// First they refuse to ship static libraries and then they also suck at file formats?

// I gotta give ImgTec credit for the pretty thorough documentation of their formats.

// We implement the legacy formats first.
enum class ePVRLegacyPixelFormat
{
    ARGB_4444,
    ARGB_1555,
    RGB_565,
    RGB_555,
    RGB_888,
    ARGB_8888,
    ARGB_8332,
    I8,
    AI88,
    MONOCHROME,
    V_Y1_U_Y0,      // 2x2 block format, 8bit depth
    Y1_V_Y0_U,      // same as above, but reordered.
    PVRTC2,
    PVRTC4,

    // Secondary formats, appears to be clones?
    ARGB_4444_SEC = 0x10,
    ARGB_1555_SEC,
    ARGB_8888_SEC,
    RGB_565_SEC,
    RGB_555_SEC,
    RGB_888_SEC,
    I8_SEC,
    AI88_SEC,
    PVRTC2_SEC,
    PVRTC4_SEC,
    BGRA_8888,  // I guess some lobbyist wanted easy convertability to PVR from DDS?

    // Special types.
    DXT1 = 0x20,
    DXT2,       // it is nice to see that PVR decided to support this format!
    DXT3,
    DXT4,       // this one aswell.
    DXT5,
    RGB332,
    AL_44,
    LVU_655,
    XLVU_8888,
    QWVU_8888,
    ABGR_2101010,
    ARGB_2101010,
    AWVU_2101010,
    GR_1616,
    VU_1616,
    ABGR_16161616,
    R_16F,
    GR_1616F,
    ABGR_16161616F,
    R_32F,
    GR_3232F,
    ABGR_32323232F,
    ETC,                // 4x4 block format, 4bit depth

    // I guess late additions.
    A8 = 0x40,
    VU_88,
    L16,
    L8,
    AL_88,
    UYVY,               // 2x2 block format, 8bit depth (V_Y1_U_Y0, another reordering)
    YUY2                // 2x2 block format, 8bit depth
};  // 55

// This is quite a gamble that I take, especially since PVR is just an advanced inspiration from DDS anyway.
inline uint32 getPVRNativeImageRowAlignment( void )
{
    // Just like DDS.
    return 1;
}

inline uint32 getPVRNativeImageRasterDataRowSize( uint32 surfWidth, uint32 depth )
{
    return getRasterDataRowSize( surfWidth, depth, getPVRNativeImageRowAlignment() );
}

// We need to classify raster formats in a way to process them properly.
enum class ePVRLegacyPixelFormatType
{
    UNKNOWN,
    RGBA,
    LUMINANCE,
    COMPRESSED
};

inline ePVRLegacyPixelFormatType getPVRLegacyPixelFormatType( ePVRLegacyPixelFormat format )
{
    switch( format )
    {
    case ePVRLegacyPixelFormat::ARGB_4444:
    case ePVRLegacyPixelFormat::ARGB_1555:
    case ePVRLegacyPixelFormat::RGB_565:
    case ePVRLegacyPixelFormat::RGB_555:
    case ePVRLegacyPixelFormat::RGB_888:
    case ePVRLegacyPixelFormat::ARGB_8888:
    case ePVRLegacyPixelFormat::ARGB_8332:
    case ePVRLegacyPixelFormat::ARGB_4444_SEC:
    case ePVRLegacyPixelFormat::ARGB_1555_SEC:
    case ePVRLegacyPixelFormat::ARGB_8888_SEC:
    case ePVRLegacyPixelFormat::RGB_565_SEC:
    case ePVRLegacyPixelFormat::RGB_555_SEC:
    case ePVRLegacyPixelFormat::RGB_888_SEC:
    case ePVRLegacyPixelFormat::BGRA_8888:
    case ePVRLegacyPixelFormat::RGB332:
    case ePVRLegacyPixelFormat::ABGR_2101010:
    case ePVRLegacyPixelFormat::ARGB_2101010:
    case ePVRLegacyPixelFormat::GR_1616:
    case ePVRLegacyPixelFormat::ABGR_16161616:
    case ePVRLegacyPixelFormat::R_16F:
    case ePVRLegacyPixelFormat::GR_1616F:
    case ePVRLegacyPixelFormat::ABGR_16161616F:
    case ePVRLegacyPixelFormat::R_32F:
    case ePVRLegacyPixelFormat::GR_3232F:
    case ePVRLegacyPixelFormat::ABGR_32323232F:
        // Those formats are RGBA samples.
        return ePVRLegacyPixelFormatType::RGBA;
    case ePVRLegacyPixelFormat::I8:
    case ePVRLegacyPixelFormat::AI88:
    case ePVRLegacyPixelFormat::MONOCHROME:
    case ePVRLegacyPixelFormat::I8_SEC:
    case ePVRLegacyPixelFormat::AI88_SEC:
    case ePVRLegacyPixelFormat::AL_44:
    case ePVRLegacyPixelFormat::L16:
    case ePVRLegacyPixelFormat::L8:
    case ePVRLegacyPixelFormat::AL_88:
        return ePVRLegacyPixelFormatType::LUMINANCE;
    case ePVRLegacyPixelFormat::V_Y1_U_Y0:
    case ePVRLegacyPixelFormat::Y1_V_Y0_U:
    case ePVRLegacyPixelFormat::PVRTC2:
    case ePVRLegacyPixelFormat::PVRTC4:
    case ePVRLegacyPixelFormat::PVRTC2_SEC:
    case ePVRLegacyPixelFormat::PVRTC4_SEC:
    case ePVRLegacyPixelFormat::DXT1:
    case ePVRLegacyPixelFormat::DXT2:
    case ePVRLegacyPixelFormat::DXT3:
    case ePVRLegacyPixelFormat::DXT4:
    case ePVRLegacyPixelFormat::DXT5:
    case ePVRLegacyPixelFormat::ETC:
    case ePVRLegacyPixelFormat::UYVY:
    case ePVRLegacyPixelFormat::YUY2:
        return ePVRLegacyPixelFormatType::COMPRESSED;
    }

    // We do not care about anything else.
    return ePVRLegacyPixelFormatType::UNKNOWN;
}

// We can read and write samples for RGBA and LUMINANCE based samples.
struct pvrColorDispatcher
{
    AINLINE pvrColorDispatcher(
        ePVRLegacyPixelFormat pixelFormat, ePVRLegacyPixelFormatType formatType
    )
    {
        this->pixelFormat = pixelFormat;
        this->colorModel = colorModel;
    }

private:
    static AINLINE void browsetexelrgba(
        const void *srcTexels, uint32 colorIndex, ePVRLegacyPixelFormat pixelFormat,
        uint8& red, uint8& green, uint8& blue, uint8& alpha
    )
    {
        if ( pixelFormat == ePVRLegacyPixelFormat::BGRA_8888 )
        {

        }
    }

private:
    ePVRLegacyPixelFormat pixelFormat;
    ePVRLegacyPixelFormatType colorModel;
};

// Under some conditions, we can directly acquire certain pixel formats into RW sample types.
static inline bool getPVRRasterFormatMapping(
    ePVRLegacyPixelFormat format, bool isLittleEndian,
    eRasterFormat& rasterFormatOut, uint32& depthOut, eColorOrdering& colorOrderingOut, eCompressionType& compressionTypeOut
)
{
    // We currently only support little endian types.
    if ( !isLittleEndian )
        return false;

    // We do have to experiment with things for now.
    if ( format == ePVRLegacyPixelFormat::BGRA_8888 )
    {
        rasterFormatOut = RASTER_8888;
        depthOut = 32;
        colorOrderingOut = COLOR_BGRA;
        compressionTypeOut = RWCOMPRESS_NONE;
        return true;
    }

    // No idea.
    return false;
}

// I guess each format has to have a fixed depth.
static inline uint32 getPVRLegacyFormatDepth( ePVRLegacyPixelFormat format )
{
    switch( format )
    {
    case ePVRLegacyPixelFormat::ARGB_4444:
    case ePVRLegacyPixelFormat::ARGB_1555:
    case ePVRLegacyPixelFormat::RGB_565:
    case ePVRLegacyPixelFormat::RGB_555:
    case ePVRLegacyPixelFormat::ARGB_8332:
    case ePVRLegacyPixelFormat::AI88:
    case ePVRLegacyPixelFormat::ARGB_4444_SEC:
    case ePVRLegacyPixelFormat::ARGB_1555_SEC:
    case ePVRLegacyPixelFormat::RGB_565_SEC:
    case ePVRLegacyPixelFormat::RGB_555_SEC:
    case ePVRLegacyPixelFormat::AI88_SEC:
    case ePVRLegacyPixelFormat::LVU_655:
    case ePVRLegacyPixelFormat::R_16F:
    case ePVRLegacyPixelFormat::VU_88:
    case ePVRLegacyPixelFormat::L16:
    case ePVRLegacyPixelFormat::AL_88:
        return 16;
    case ePVRLegacyPixelFormat::RGB_888:
    case ePVRLegacyPixelFormat::ARGB_8888:
    case ePVRLegacyPixelFormat::ARGB_8888_SEC:
    case ePVRLegacyPixelFormat::RGB_888_SEC:
    case ePVRLegacyPixelFormat::BGRA_8888:
    case ePVRLegacyPixelFormat::XLVU_8888:
    case ePVRLegacyPixelFormat::QWVU_8888:
    case ePVRLegacyPixelFormat::ABGR_2101010:
    case ePVRLegacyPixelFormat::ARGB_2101010:
    case ePVRLegacyPixelFormat::AWVU_2101010:
    case ePVRLegacyPixelFormat::GR_1616:
    case ePVRLegacyPixelFormat::VU_1616:
    case ePVRLegacyPixelFormat::GR_1616F:
    case ePVRLegacyPixelFormat::R_32F:
        return 32;
    case ePVRLegacyPixelFormat::I8:
    case ePVRLegacyPixelFormat::V_Y1_U_Y0:
    case ePVRLegacyPixelFormat::Y1_V_Y0_U:
    case ePVRLegacyPixelFormat::UYVY:
    case ePVRLegacyPixelFormat::YUY2:
    case ePVRLegacyPixelFormat::I8_SEC:
    case ePVRLegacyPixelFormat::DXT2:
    case ePVRLegacyPixelFormat::DXT3:
    case ePVRLegacyPixelFormat::DXT4:
    case ePVRLegacyPixelFormat::DXT5:
    case ePVRLegacyPixelFormat::RGB332:
    case ePVRLegacyPixelFormat::AL_44:
    case ePVRLegacyPixelFormat::A8:
    case ePVRLegacyPixelFormat::L8:
        return 8;
    case ePVRLegacyPixelFormat::MONOCHROME:
        return 1;
    case ePVRLegacyPixelFormat::PVRTC2:
    case ePVRLegacyPixelFormat::PVRTC2_SEC:
        return 2;
    case ePVRLegacyPixelFormat::PVRTC4:
    case ePVRLegacyPixelFormat::PVRTC4_SEC:
    case ePVRLegacyPixelFormat::DXT1:
    case ePVRLegacyPixelFormat::ETC:
        return 4;
    case ePVRLegacyPixelFormat::ABGR_16161616:
    case ePVRLegacyPixelFormat::ABGR_16161616F:
    case ePVRLegacyPixelFormat::GR_3232F:
        return 64;
    case ePVRLegacyPixelFormat::ABGR_32323232F:
        return 128;
    }

    assert( 0 );

    // Doesnt really happen, if the format is valid.
    return 0;
}

static inline void getPVRLegacyFormatSurfaceDimensions(
    ePVRLegacyPixelFormat format,
    uint32 layerWidth, uint32 layerHeight,
    uint32& surfWidthOut, uint32& surfHeightOut
)
{
    switch( format )
    {
    case ePVRLegacyPixelFormat::V_Y1_U_Y0:
    case ePVRLegacyPixelFormat::Y1_V_Y0_U:
    case ePVRLegacyPixelFormat::UYVY:
    case ePVRLegacyPixelFormat::YUY2:
        // 2x2 block format.
        surfWidthOut = ALIGN_SIZE( layerWidth, 2u );
        surfHeightOut = ALIGN_SIZE( layerHeight, 2u );
        return;
    case ePVRLegacyPixelFormat::DXT1:
    case ePVRLegacyPixelFormat::DXT2:
    case ePVRLegacyPixelFormat::DXT3:
    case ePVRLegacyPixelFormat::DXT4:
    case ePVRLegacyPixelFormat::DXT5:
    case ePVRLegacyPixelFormat::ETC:
        // 4x4 block compression.
        surfWidthOut = ALIGN_SIZE( layerWidth, 4u );
        surfHeightOut = ALIGN_SIZE( layerHeight, 4u );
        return;
    case ePVRLegacyPixelFormat::PVRTC2:
    case ePVRLegacyPixelFormat::PVRTC4:
    case ePVRLegacyPixelFormat::PVRTC2_SEC:
    case ePVRLegacyPixelFormat::PVRTC4_SEC:
    {
        // 16x8 or 8x8 block compresion.
        uint32 comprBlockWidth, comprBlockHeight;
        getPVRCompressionBlockDimensions( getPVRLegacyFormatDepth( format ), comprBlockWidth, comprBlockHeight );

        surfWidthOut = ALIGN_SIZE( layerWidth, comprBlockWidth );
        surfHeightOut = ALIGN_SIZE( layerHeight, comprBlockHeight );
        return;
    }
    }

    // Everything else is considered raw sample, so layer dimms == surf dimms.
    surfWidthOut = layerWidth;
    surfHeightOut = layerHeight;
}

// Some PVR formats actually have a bitmask.
// For those we kind of want to write a valid one.
// TODO: investigate first, tho.

struct pvr_legacy_formatField
{
    uint32 pixelFormat : 8;
    uint32 mipmapsPresent : 1;
    uint32 dataIsTwiddled : 1;
    uint32 containsNormalData : 1;
    uint32 hasBorder : 1;
    uint32 isCubeMap : 1;
    uint32 mipmapsHaveDebugColoring : 1;
    uint32 isVolumeTexture : 1;
    uint32 hasAlphaChannel_pvrtc : 1;
    uint32 isVerticallyFlipped : 1;
    uint32 pad : 15;
};

template <template <typename numberType> class endianness>
struct pvr_header_ver1
{
    endianness <uint32>     height;
    endianness <uint32>     width;
    endianness <uint32>     mipmapCount;

    endianness <pvr_legacy_formatField>     flags;

    endianness <uint32>     surfaceSize;
    endianness <uint32>     bitsPerPixel;
    endianness <uint32>     redMask;
    endianness <uint32>     greenMask;
    endianness <uint32>     blueMask;
    endianness <uint32>     alphaMask;
};
static_assert( ( sizeof( pvr_header_ver1 <endian::little_endian> ) + sizeof( uint32 ) ) == 44, "invalid byte-size for PVR version 1 header" );

template <template <typename numberType> class endianness>
struct pvr_header_ver2
{
    endianness <uint32>     height;
    endianness <uint32>     width;
    endianness <uint32>     mipmapCount;

    endianness <pvr_legacy_formatField>     flags;

    endianness <uint32>     surfaceSize;
    endianness <uint32>     bitsPerPixel;
    endianness <uint32>     redMask;
    endianness <uint32>     greenMask;
    endianness <uint32>     blueMask;
    endianness <uint32>     alphaMask;
    
    endianness <uint32>     pvr_id;
    endianness <uint32>     numberOfSurfaces;
};
static_assert( ( sizeof( pvr_header_ver2 <endian::little_endian> ) + sizeof( uint32 ) ) == 52, "invalid byte-size for PVR version 2 header" );

// Meta-information about the PVR format.
static const natimg_supported_native_desc pvr_natimg_suppnattex[] =
{
    { "Direct3D8" },
    { "Direct3D9" },
    { "PowerVR" }
};

static const imaging_filename_ext pvr_natimg_fileExt[] =
{
    { "PVR", true }
};

struct pvrNativeImageTypeManager : public nativeImageTypeManager
{
    struct pvrNativeImage
    {
        inline void resetFormat( void )
        {
            this->pixelFormat = ePVRLegacyPixelFormat::ARGB_4444;
            this->dataIsTwiddled = false;
            this->containsNormalData = false;
            this->hasBorder = false;
            this->isCubeMap = false;
            this->mipmapsHaveDebugColoring = false;
            this->isVolumeTexture = false;
            this->hasAlphaChannel_pvrtc = false;
            this->isVerticallyFlipped = false;

            // Reset cached properties.
            this->bitDepth = 0;

            // We really like the little-endian format.
            this->isLittleEndian = true;
        }

        inline pvrNativeImage( Interface *engineInterface )
        {
            this->engineInterface = engineInterface;

            this->resetFormat();
        }

        // We do not have to make special destructors or copy constructors.
        // The default ones are perfectly fine.
        // Remember that deallocation of data is done by the framework itself!

        Interface *engineInterface;

        // Those fields are specialized for the legacy PVR format for now.
        ePVRLegacyPixelFormat pixelFormat;
        bool dataIsTwiddled;
        bool containsNormalData;
        bool hasBorder;
        bool isCubeMap;
        bool mipmapsHaveDebugColoring;
        bool isVolumeTexture;
        bool hasAlphaChannel_pvrtc;
        bool isVerticallyFlipped;

        // Properties that we cache.
        uint32 bitDepth;

        // Now for the color data itself.
        typedef genmip::mipmapLayer mipmap_t;

        typedef std::vector <mipmap_t> mipmaps_t;

        mipmaps_t mipmaps;

        // Meta-data.
        bool isLittleEndian;
    };

    void ConstructImage( Interface *engineInterface, void *imageMem ) const override
    {
        new (imageMem) pvrNativeImage( engineInterface );
    }

    void CopyConstructImage( Interface *engineInterface, void *imageMem, const void *srcImageMem ) const override
    {
        new (imageMem) pvrNativeImage( *(const pvrNativeImage*)srcImageMem );
    }

    void DestroyImage( Interface *engineInterface, void *imageMem ) const override
    {
        ( (pvrNativeImage*)imageMem )->~pvrNativeImage();
    }

    const char* GetBestSupportedNativeTexture( Interface *engineInterface, const void *imageMem ) const override
    {
        // TODO. It kinda depends on the properties.
        return "PowerVR";
    }

    void ClearImageData( Interface *engineInterface, void *imageMem, bool deallocate ) const override
    {
        pvrNativeImage *natImg = (pvrNativeImage*)imageMem;

        // In this routine we clear mipmap and palette data, basically everything from this image.
        if ( deallocate )
        {
            genmip::deleteMipmapLayers( engineInterface, natImg->mipmaps );
        }

        // Clear all color data references.
        natImg->mipmaps.clear();

        // Reset the image.
        natImg->resetFormat();
    }

    void ClearPaletteData( Interface *engineInterface, void *imageMem, bool deallocate ) const override
    {
        // PVR native images do not support palette.
    }

    void ReadFromNativeTexture( Interface *engineInterface, void *imageMem, const char *nativeTexName, void *nativeTexMem, acquireFeedback_t& feedbackOut ) const override
    {
        throw RwException( "PVR texel acquisition from native texture not implemented yet" );
    }

    void WriteToNativeTexture( Interface *engineInterface, void *imageMem, const char *nativeTexName, void *nativeTexMem, acquireFeedback_t& feedbackOut ) const override
    {
        pvrNativeImage *natImg = (pvrNativeImage*)imageMem;

        // Let's first try putting PVR stuff into native textures.
        ePVRLegacyPixelFormat pixelFormat = natImg->pixelFormat;

        bool isLittleEndian = natImg->isLittleEndian;

        // We first want to see if we can just directly acquire the color data.
        eRasterFormat pvrRasterFormat;
        uint32 pvrDepth;
        uint32 pvrRowAlignment = getPVRNativeImageRowAlignment();
        eColorOrdering pvrColorOrder;

        eCompressionType pvrCompressionType;

        bool hasDirectMapping = getPVRRasterFormatMapping( pixelFormat, isLittleEndian, pvrRasterFormat, pvrDepth, pvrColorOrder, pvrCompressionType );

        // FOR NOW TO DEBUG SOME THINGS.
        assert( hasDirectMapping );

        // Determine the target capabilities.
        bool isDirect3D8 = false;
        bool isDirect3D9 = false;
        bool isPowerVR = false;

        if ( strcmp( nativeTexName, "Direct3D8" ) == 0 )
        {
            isDirect3D8 = true;
        }
        else if ( strcmp( nativeTexName, "Direct3D9" ) == 0 )
        {
            isDirect3D9 = true;
        }
        else if ( strcmp( nativeTexName, "PowerVR" ) == 0 )
        {
            isPowerVR = true;
        }
        else
        {
            throw RwException( "unsupported native texture type in PVR native image write-to-raster" );
        }

        // Since we really need to debug some things, we just like directly push color data for now.
        if ( isDirect3D9 )
        {

        }

        throw RwException( "PVR native image color data to native texture not implemented yet" );
    }

    template <typename structType>
    static inline bool readStreamStruct( Stream *stream, structType& structOut )
    {
        size_t readCount = stream->read( &structOut, sizeof( structType ) );

        return ( readCount == sizeof( structType ) );
    }

    static inline bool readLegacyVersionHeader(
        Stream *inputStream,
        uint32& widthOut, uint32& heightOut,
        uint32& mipmapCountOut,
        pvr_legacy_formatField& formatFieldOut,
        uint32& surfaceSizeOut,
        uint32& bitsPerPixelOut,
        uint32& redMaskOut, uint32& greenMaskOut, uint32& blueMaskOut, uint32& alphaMaskOut,
        bool& isLittleEndianOut
    )
    {
        union header_data_union
        {
            inline header_data_union( void ) : header_size_data()
            {}

            char header_size_data[4];
            endian::little_endian <uint32> le_header_size;
            endian::little_endian <uint32> be_header_size;
        };

        header_data_union header_data;

        if ( !readStreamStruct( inputStream, header_data.header_size_data ) )
        {
            return false;
        }

        // Try little endian first.
        {
            uint32 header_size = header_data.le_header_size;

            if ( header_size == 44 )
            {
                pvr_header_ver1 <endian::little_endian> header;

                if ( !readStreamStruct( inputStream, header ) )
                {
                    return false;
                }

                widthOut = header.width;
                heightOut = header.height;
                mipmapCountOut = header.mipmapCount;
                formatFieldOut = header.flags;
                surfaceSizeOut = header.surfaceSize;
                bitsPerPixelOut = header.bitsPerPixel;
                redMaskOut = header.redMask;
                greenMaskOut = header.greenMask;
                blueMaskOut = header.blueMask;
                alphaMaskOut = header.alphaMask;

                isLittleEndianOut = true;
                return true;
            }
            else if ( header_size == 52 )
            {
                pvr_header_ver2 <endian::little_endian> header;

                if ( !readStreamStruct( inputStream, header ) )
                {
                    return false;
                }

                // Verify PVR id.
                if ( header.pvr_id != 0x21525650 )
                {
                    return false;
                }

                widthOut = header.width;
                heightOut = header.height;
                mipmapCountOut = header.mipmapCount;
                formatFieldOut = header.flags;
                surfaceSizeOut = header.surfaceSize;
                bitsPerPixelOut = header.bitsPerPixel;
                redMaskOut = header.redMask;
                greenMaskOut = header.greenMask;
                blueMaskOut = header.blueMask;
                alphaMaskOut = header.alphaMask;

                // TODO: verify PVR ID

                isLittleEndianOut = true;
                return true;
            }
        }

        // Now do big endian.
        {
            uint32 header_size = header_data.be_header_size;

            if ( header_size == 44 )
            {
                pvr_header_ver1 <endian::big_endian> header;

                if ( !readStreamStruct( inputStream, header ) )
                {
                    return false;
                }

                widthOut = header.width;
                heightOut = header.height;
                mipmapCountOut = header.mipmapCount;
                formatFieldOut = header.flags;
                surfaceSizeOut = header.surfaceSize;
                bitsPerPixelOut = header.bitsPerPixel;
                redMaskOut = header.redMask;
                greenMaskOut = header.greenMask;
                blueMaskOut = header.blueMask;
                alphaMaskOut = header.alphaMask;

                isLittleEndianOut = false;
                return true;
            }
            else if ( header_size == 52 )
            {
                pvr_header_ver2 <endian::big_endian> header;

                if ( !readStreamStruct( inputStream, header ) )
                {
                    return false;
                }

                // Verify PVR id.
                if ( header.pvr_id != 0x21525650 )
                {
                    return false;
                }

                widthOut = header.width;
                heightOut = header.height;
                mipmapCountOut = header.mipmapCount;
                formatFieldOut = header.flags;
                surfaceSizeOut = header.surfaceSize;
                bitsPerPixelOut = header.bitsPerPixel;
                redMaskOut = header.redMask;
                greenMaskOut = header.greenMask;
                blueMaskOut = header.blueMask;
                alphaMaskOut = header.alphaMask;

                // TODO: verify PVR ID

                isLittleEndianOut = false;
                return true;
            }
        }

        // Could not find a proper header (legacy).
        return false;
    }

    static inline bool isValidPVRLegacyPixelFormat( ePVRLegacyPixelFormat pixelFormat )
    {
        if ( pixelFormat != ePVRLegacyPixelFormat::ARGB_4444 &&
             pixelFormat != ePVRLegacyPixelFormat::ARGB_1555 &&
             pixelFormat != ePVRLegacyPixelFormat::RGB_565 &&
             pixelFormat != ePVRLegacyPixelFormat::RGB_555 &&
             pixelFormat != ePVRLegacyPixelFormat::ARGB_8888 &&
             pixelFormat != ePVRLegacyPixelFormat::ARGB_8332 &&
             pixelFormat != ePVRLegacyPixelFormat::I8 &&
             pixelFormat != ePVRLegacyPixelFormat::AI88 &&
             pixelFormat != ePVRLegacyPixelFormat::MONOCHROME &&
             pixelFormat != ePVRLegacyPixelFormat::V_Y1_U_Y0 &&
             pixelFormat != ePVRLegacyPixelFormat::Y1_V_Y0_U &&
             pixelFormat != ePVRLegacyPixelFormat::PVRTC2 &&
             pixelFormat != ePVRLegacyPixelFormat::PVRTC4 &&
             pixelFormat != ePVRLegacyPixelFormat::ARGB_4444_SEC &&
             pixelFormat != ePVRLegacyPixelFormat::ARGB_1555_SEC &&
             pixelFormat != ePVRLegacyPixelFormat::ARGB_8888_SEC &&
             pixelFormat != ePVRLegacyPixelFormat::RGB_565_SEC &&
             pixelFormat != ePVRLegacyPixelFormat::RGB_555_SEC &&
             pixelFormat != ePVRLegacyPixelFormat::RGB_888_SEC &&
             pixelFormat != ePVRLegacyPixelFormat::I8_SEC &&
             pixelFormat != ePVRLegacyPixelFormat::AI88_SEC &&
             pixelFormat != ePVRLegacyPixelFormat::PVRTC2_SEC &&
             pixelFormat != ePVRLegacyPixelFormat::PVRTC4_SEC &&
             pixelFormat != ePVRLegacyPixelFormat::BGRA_8888 &&
             pixelFormat != ePVRLegacyPixelFormat::DXT1 &&
             pixelFormat != ePVRLegacyPixelFormat::DXT2 &&
             pixelFormat != ePVRLegacyPixelFormat::DXT3 &&
             pixelFormat != ePVRLegacyPixelFormat::DXT4 &&
             pixelFormat != ePVRLegacyPixelFormat::DXT5 &&
             pixelFormat != ePVRLegacyPixelFormat::RGB332 &&
             pixelFormat != ePVRLegacyPixelFormat::AL_44 &&
             pixelFormat != ePVRLegacyPixelFormat::LVU_655 &&
             pixelFormat != ePVRLegacyPixelFormat::XLVU_8888 &&
             pixelFormat != ePVRLegacyPixelFormat::QWVU_8888 &&
             pixelFormat != ePVRLegacyPixelFormat::ABGR_2101010 &&
             pixelFormat != ePVRLegacyPixelFormat::ARGB_2101010 &&
             pixelFormat != ePVRLegacyPixelFormat::AWVU_2101010 &&
             pixelFormat != ePVRLegacyPixelFormat::GR_1616 &&
             pixelFormat != ePVRLegacyPixelFormat::VU_1616 &&
             pixelFormat != ePVRLegacyPixelFormat::ABGR_16161616 &&
             pixelFormat != ePVRLegacyPixelFormat::R_16F &&
             pixelFormat != ePVRLegacyPixelFormat::GR_1616F &&
             pixelFormat != ePVRLegacyPixelFormat::ABGR_16161616F &&
             pixelFormat != ePVRLegacyPixelFormat::R_32F &&
             pixelFormat != ePVRLegacyPixelFormat::GR_3232F &&
             pixelFormat != ePVRLegacyPixelFormat::ABGR_32323232F &&
             pixelFormat != ePVRLegacyPixelFormat::ETC &&
             pixelFormat != ePVRLegacyPixelFormat::A8 &&
             pixelFormat != ePVRLegacyPixelFormat::VU_88 &&
             pixelFormat != ePVRLegacyPixelFormat::L16 &&
             pixelFormat != ePVRLegacyPixelFormat::L8 &&
             pixelFormat != ePVRLegacyPixelFormat::AL_88 &&
             pixelFormat != ePVRLegacyPixelFormat::UYVY &&
             pixelFormat != ePVRLegacyPixelFormat::YUY2 )
        {
            return false;
        }

        return true;
    }

    bool IsStreamNativeImage( Interface *engineInterface, Stream *inputStream ) const override
    {
        // Try to read some shitty PVR files.
        // We need to support both ver1 and ver2.

        uint32 width, height;
        uint32 mipmapCount;
        pvr_legacy_formatField formatField;
        uint32 surfaceSize;
        uint32 bitsPerPixel;
        uint32 redMask;
        uint32 blueMask;
        uint32 greenMask;
        uint32 alphaMask;

        bool isLittleEndian;

        bool hasLegacyFormatHeader =
            readLegacyVersionHeader(
                inputStream,
                width, height,
                mipmapCount,
                formatField,
                surfaceSize,
                bitsPerPixel,
                redMask, blueMask, greenMask, alphaMask,
                isLittleEndian
            );

        if ( !hasLegacyFormatHeader )
        {
            // We now have no support for non-legacy formats.
            return false;
        }

        // In the legacy format, the mipmapCount excludes the main surface.
        mipmapCount++;

        // Make sure we got a valid format.
        // There cannot be more formats than were specified.
        ePVRLegacyPixelFormat pixelFormat = (ePVRLegacyPixelFormat)formatField.pixelFormat;

        if ( !isValidPVRLegacyPixelFormat( pixelFormat ) )
        {
            return false;
        }

        // Determine the pixel format and what the properties mean to us.
        // Unfortunately, we are not going to be able to support each pixel format thrown at us.
        // This is because things like UVWA require special interpretation, quite frankly
        // cannot be mapped to general color data if there is not a perfect match.

        uint32 format_bitDepth = getPVRLegacyFormatDepth( pixelFormat );

        assert( format_bitDepth != 0 );

        // Verify that all color layers are present.
        mipGenLevelGenerator mipGen( width, height );

        if ( mipGen.isValidLevel() == false )
        {
            return false;
        }

        uint32 mip_index = 0;

        while ( mip_index < mipmapCount )
        {
            bool didEstablishLevel = true;

            if ( mip_index != 0 )
            {
                didEstablishLevel = mipGen.incrementLevel();
            }

            if ( !didEstablishLevel )
            {
                break;
            }

            // Get the data linear size, since we always can.
            uint32 mipLayerWidth = mipGen.getLevelWidth();
            uint32 mipLayerHeight = mipGen.getLevelHeight();

            // For we need the surface dimensions.
            uint32 mipSurfWidth, mipSurfHeight;

            getPVRLegacyFormatSurfaceDimensions( pixelFormat, mipLayerWidth, mipLayerHeight, mipSurfWidth, mipSurfHeight );

            // So now for the calculation part.
            uint32 texRowSize = getPVRNativeImageRasterDataRowSize( mipSurfWidth, format_bitDepth );

            uint32 texDataSize = getRasterDataSizeByRowSize( texRowSize, mipSurfHeight );

            skipAvailable( inputStream, texDataSize );

            // Next level.
            mip_index++;
        }

        // We are a valid PVR!
        return true;
    }

    void ReadNativeImage( Interface *engineInterface, void *imageMem, Stream *inputStream ) const override
    {
        // Let's read those suckers.
        
        uint32 width, height;
        uint32 mipmapCount;
        pvr_legacy_formatField formatField;
        uint32 surfaceSize;
        uint32 bitsPerPixel;
        uint32 redMask;
        uint32 blueMask;
        uint32 greenMask;
        uint32 alphaMask;

        bool isLittleEndian;

        bool hasLegacyFormatHeader =
            readLegacyVersionHeader(
                inputStream,
                width, height,
                mipmapCount,
                formatField,
                surfaceSize,
                bitsPerPixel,
                redMask, blueMask, greenMask, alphaMask,
                isLittleEndian
            );

        if ( !hasLegacyFormatHeader )
        {
            // We now have no support for non-legacy formats.
            throw RwException( "invalid PVR native image" );
        }

        // In the legacy format, the mipmapCount excludes the main surface.
        mipmapCount++;

        // Verify properties of the image file.
        // Make sure we got a valid format.
        // There cannot be more formats than were specified.
        ePVRLegacyPixelFormat pixelFormat = (ePVRLegacyPixelFormat)formatField.pixelFormat;

        if ( !isValidPVRLegacyPixelFormat( pixelFormat ) )
        {
            throw RwException( "invalid PVR native image (legacy) pixel format" );
        }

        uint32 format_bitDepth = getPVRLegacyFormatDepth( pixelFormat );

        // Verify bit depth.
        if ( bitsPerPixel != format_bitDepth )
        {
            engineInterface->PushWarning( "PVR native texture has an invalid bitsPerPixel value" );
        }

        // We do not support certain image files for now.
        if ( formatField.isCubeMap )
        {
            throw RwException( "cubemap PVR native images not supported yet" );
        }

        if ( formatField.isVolumeTexture )
        {
            throw RwException( "volume texture PVR native images not supported yet" );
        }

        // Time to store some properties.
        pvrNativeImage *natImg = (pvrNativeImage*)imageMem;

        natImg->pixelFormat = pixelFormat;
        natImg->dataIsTwiddled = formatField.dataIsTwiddled;
        natImg->containsNormalData = formatField.containsNormalData;
        natImg->hasBorder = formatField.hasBorder;
        natImg->isCubeMap = false;              // TODO
        natImg->mipmapsHaveDebugColoring = formatField.mipmapsHaveDebugColoring;
        natImg->isVolumeTexture = false;        // TODO
        natImg->hasAlphaChannel_pvrtc = formatField.hasAlphaChannel_pvrtc;
        natImg->isVerticallyFlipped = formatField.isVerticallyFlipped;

        // Store cached properties.
        natImg->bitDepth = format_bitDepth;

        // And meta-properties.
        natImg->isLittleEndian = isLittleEndian;

        // Turns out the guys at Imagination do not care about the color bitmasks.
        // So we do not care either.

        // Read the color data now.
        mipGenLevelGenerator mipGen( width, height );

        if ( !mipGen.isValidLevel() )
        {
            throw RwException( "invalid image dimensions in PVR native image" );
        }

        // We want to read only as much surface data as the image tells us is available.
        uint32 remaining_surfDataSize = surfaceSize;

        uint32 mip_index = 0;

        while ( mip_index < mipmapCount )
        {
            bool didEstablishLevel = true;

            if ( mip_index != 0 )
            {
                didEstablishLevel = mipGen.incrementLevel();
            }

            if ( !didEstablishLevel )
            {
                // We are prematurely finished.
                break;
            }

            // Actually get the mipmap properties and store the data now.
            uint32 mipLayerWidth = mipGen.getLevelWidth();
            uint32 mipLayerHeight = mipGen.getLevelHeight();

            uint32 mipSurfWidth, mipSurfHeight;
            getPVRLegacyFormatSurfaceDimensions( pixelFormat, mipLayerWidth, mipLayerHeight, mipSurfWidth, mipSurfHeight );

            // NOTE: even though there is no row-size for each PVR native image pixel format (e.g. compressed), this style
            // of calculating the linear size if perfectly compatible.
            uint32 texRowSize = getPVRNativeImageRasterDataRowSize( mipSurfWidth, format_bitDepth );

            uint32 texDataSize = getRasterDataSizeByRowSize( texRowSize, mipSurfHeight );

            // Check if we can read this layer even.
            if ( remaining_surfDataSize < texDataSize )
            {
                throw RwException( "too little surface data in PVR native image" );
            }

            remaining_surfDataSize -= texDataSize;

            // Check if we even have the data in the stream.
            checkAhead( inputStream, texDataSize );

            void *mipTexels = engineInterface->PixelAllocate( texDataSize );

            if ( !mipTexels )
            {
                throw RwException( "failed to allocate mipmap surface in PVR native image deserialization" );
            }

            try
            {
                // Read the stuff.
                size_t readCount = inputStream->read( mipTexels, texDataSize );

                if ( readCount != texDataSize )
                {
                    throw RwException( "impartial mipmap surface read exception in PVR native image deserialization" );
                }

                // Store our surface.
                pvrNativeImage::mipmap_t newLayer;
                newLayer.width = mipSurfWidth;
                newLayer.height = mipSurfHeight;

                newLayer.layerWidth = mipLayerWidth;
                newLayer.layerHeight = mipLayerHeight;

                newLayer.texels = mipTexels;
                newLayer.dataSize = texDataSize;

                natImg->mipmaps.push_back( std::move( newLayer ) );
            }
            catch( ... )
            {
                // We kinda failed, so clear data.
                engineInterface->PixelFree( mipTexels );

                throw;
            }

            // Next level.
            mip_index++;
        }

        if ( mip_index != mipmapCount )
        {
            engineInterface->PushWarning( "PVR native image specified more mipmap layers than could be read" );
        }

        // Check that we read all surface data.
        if ( remaining_surfDataSize != 0 )
        {
            engineInterface->PushWarning( "PVR native image has surface meta-data" );

            // Skip those bytes.
            inputStream->skip( remaining_surfDataSize );
        }

        // Finito. :)
    }

    void WriteNativeImage( Interface *engineInterface, const void *imageMem, Stream *outputStream ) const override
    {
        // What we have read, and verified, we can easily write back.
        // PVR is a really weird format anyway.

        pvrNativeImage *natImg = (pvrNativeImage*)imageMem;

        // We actually want to support writing either, little endian and big endian.
        bool isLittleEndian = natImg->isLittleEndian;

        size_t mipmapCount = natImg->mipmaps.size();

        if ( mipmapCount == 0 )
        {
            throw RwException( "attempt to write empty PVR native image file" );
        }

        // Prepare the format field.
        pvr_legacy_formatField formatField;
        formatField.pixelFormat = (uint8)natImg->pixelFormat;
        formatField.mipmapsPresent = ( mipmapCount > 1 );
        formatField.dataIsTwiddled = natImg->dataIsTwiddled;
        formatField.containsNormalData = natImg->containsNormalData;
        formatField.hasBorder = natImg->hasBorder;
        formatField.isCubeMap = natImg->isCubeMap;
        formatField.mipmapsHaveDebugColoring = natImg->mipmapsHaveDebugColoring;
        formatField.isVolumeTexture = natImg->isVolumeTexture;
        formatField.hasAlphaChannel_pvrtc = natImg->hasAlphaChannel_pvrtc;
        formatField.isVerticallyFlipped = natImg->isVerticallyFlipped;
        formatField.pad = 0;

        // Calculate the accumulated surface size.
        uint32 totalSurfaceSize = 0;

        for ( size_t n = 0; n < mipmapCount; n++ )
        {
            const pvrNativeImage::mipmap_t& srcLayer = natImg->mipmaps[ n ];

            totalSurfaceSize += srcLayer.dataSize;
        }

        // I guess we should always be writing version two legacy files, if on point.
        // Those file formats are considered legacy already, geez...
        uint32 ver2_headerSize = 52;

        // Need the base layer.
        const pvrNativeImage::mipmap_t& baseLayer = natImg->mipmaps[ 0 ];

        if ( isLittleEndian )
        {
            // First write the header size.
            endian::little_endian <uint32> header_size = ver2_headerSize;

            outputStream->write( &header_size, sizeof( header_size ) );

            pvr_header_ver2 <endian::little_endian> header;
            header.height = baseLayer.layerHeight;
            header.width = baseLayer.layerWidth;
            header.mipmapCount = (uint32)( mipmapCount - 1 );
            header.flags = formatField;
            header.surfaceSize = totalSurfaceSize;
            header.bitsPerPixel = natImg->bitDepth;
            header.redMask = 0; // nobody cares, even ImgTec doesnt.
            header.greenMask = 0;
            header.blueMask = 0;
            header.alphaMask = 0;
            header.pvr_id = 0x21525650;
            header.numberOfSurfaces = 1;

            outputStream->write( &header, sizeof( header ) );
        }
        else
        {
            // First write the header size.
            endian::big_endian <uint32> header_size = ver2_headerSize;

            outputStream->write( &header_size, sizeof( header_size ) );

            pvr_header_ver2 <endian::big_endian> header;
            header.height = baseLayer.layerHeight;
            header.width = baseLayer.layerWidth;
            header.mipmapCount = (uint32)( mipmapCount - 1 );
            header.flags = formatField;
            header.surfaceSize = totalSurfaceSize;
            header.bitsPerPixel = natImg->bitDepth;
            header.redMask = 0; // nobody cares, even ImgTec doesnt.
            header.greenMask = 0;
            header.blueMask = 0;
            header.alphaMask = 0;
            header.pvr_id = 0x21525650;
            header.numberOfSurfaces = 1;

            outputStream->write( &header, sizeof( header ) );
        }

        // Now write the image data.
        // As you may have noticed the PVR native image has no palette support.
        for ( size_t n = 0; n < mipmapCount; n++ )
        {
            const pvrNativeImage::mipmap_t& mipLayer = natImg->mipmaps[ n ];

            uint32 mipDataSize = mipLayer.dataSize;

            const void *mipTexels = mipLayer.texels;

            outputStream->write( mipTexels, mipDataSize );
        }

        // Done.
    }

    inline void Initialize( EngineInterface *engineInterface )
    {
        RegisterNativeImageType(
            engineInterface,
            this,
            "PVR", sizeof( pvrNativeImage ), "PowerVR Image",
            pvr_natimg_fileExt, _countof( pvr_natimg_fileExt ),
            pvr_natimg_suppnattex, _countof( pvr_natimg_suppnattex )
        );
    }

    inline void Shutdown( EngineInterface *engineInterface )
    {
        UnregisterNativeImageType( engineInterface, "PVR" );
    }
};

static PluginDependantStructRegister <pvrNativeImageTypeManager, RwInterfaceFactory_t> pvrNativeImageTypeManagerRegister;

void registerPVRNativeImageTypeEnv( void )
{
    pvrNativeImageTypeManagerRegister.RegisterPlugin( engineFactory );
}

};