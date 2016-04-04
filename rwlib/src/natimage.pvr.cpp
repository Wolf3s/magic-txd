// PowerVR file format support for RenderWare, because mobile games tend to use it.
// This file format started out as an inspiration over DDS while adding support for
// many Imagination Technologies formats (PVR 2bpp, PVR 4bpp, ETC, ....).

#include "StdInc.h"

#include "natimage.hxx"

namespace rw
{

// A trend of Linux-invented file formats is that they come in dynamic endianness.
// Instead of standardizing the endianness to a specific value they allow you to write it
// in whatever way and the guys writing the parser gotta make a smart enough implementation
// to detect any case.
// First they refuse to ship static libraries and then they also suck at file formats?

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
    V_Y1_U_Y0,
    Y1_V_Y0_U,
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
    ETC,

    // I guess late additions.
    A8 = 0x40,
    VU_88,
    L16,
    L8,
    AL_88,
    UYVY,
    YUY2
};

// We need to classify raster formats in a way to process them properly.
enum class ePVRLegacyPixelFormatType
{
    RGBA,
    LUMINANCE,
    PALETTE,
    COMPRESSED
};

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
    uint32 isVericallyFlipped : 1;
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
        inline pvrNativeImage( Interface *engineInterface )
        {

        }

        inline pvrNativeImage( const pvrNativeImage& right )
        {

        }

        inline ~pvrNativeImage( void )
        {

        }
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
        // TODO.
        return "PowerVR";
    }

    void ClearImageData( Interface *engineInterface, void *imageMem, bool deallocate ) const override
    {
        // TODO.
    }

    void ClearPaletteData( Interface *engineInterface, void *imageMem, bool deallocate ) const override
    {
        // TODO.
    }

    void ReadFromNativeTexture( Interface *engineInterface, void *imageMem, const char *nativeTexName, void *nativeTexMem, acquireFeedback_t& feedbackOut ) const override
    {
        throw RwException( "PVR texel acquisition from native texture not implemented yet" );
    }

    void WriteToNativeTexture( Interface *engineInterface, void *imageMem, const char *nativeTexName, void *nativeTexMem, acquireFeedback_t& feedbackOut ) const override
    {
        throw RwException( "PVR texel put into native texture not implemented yet" );
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

            if ( header_size == 52 )
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
            else if ( header_size == 44 )
            {
                pvr_header_ver2 <endian::big_endian> header;

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

        // Make sure we got a valid format.
        // There cannot be more formats than were specified.
        ePVRLegacyPixelFormat pixelFormat = (ePVRLegacyPixelFormat)formatField.pixelFormat;

        if ( !isValidPVRLegacyPixelFormat( pixelFormat ) )
        {
            throw RwException( "invalid PVR legacy pixel format" );
        }

        // Determine the pixel format and what the properties mean to us.
        // Unfortunately, we are not going to be able to support each pixel format thrown at us.
        // This is because things like UVWA require special interpretation, quite frankly
        // cannot be mapped to general color data if there is not a perfect match.

        // TODO.
        return false;
    }

    void ReadNativeImage( Interface *engineInterface, void *imageMem, Stream *inputStream ) const override
    {
        throw RwException( "reading PVR images not supported yet" );
    }

    void WriteNativeImage( Interface *engineInterface, const void *imageMem, Stream *outputStream ) const override
    {
        throw RwException( "writing PVR images not supported yet" );
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