// Mipmap transformation between framework-friendly format and native format.

#ifndef _GAMECUBE_NATIVE_MIPMAP_TRANSFORMATION_
#define _GAMECUBE_NATIVE_MIPMAP_TRANSFORMATION_

#ifdef RWLIB_INCLUDE_NATIVETEX_GAMECUBE

#include "txdread.d3d.dxt.hxx"

#include "txdread.miputil.hxx"

#include "txdread.memcodec.hxx"

namespace rw
{

template <typename callbackType>
AINLINE void GCProcessRandomAccessTileSurface(
    uint32 mipWidth, uint32 mipHeight,
    uint32 clusterWidth, uint32 clusterHeight,
    uint32 clusterCount,
    bool isSwizzled,
    callbackType& cb
)
{
    if ( isSwizzled )
    {
        memcodec::permutationUtilities::ProcessTextureLayerTiles(
            mipWidth, mipHeight,
            clusterWidth, clusterHeight,
            clusterCount,
            cb
        );
    }
    else
    {
        uint32 proc_width = ALIGN_SIZE( mipWidth, clusterWidth );
        uint32 proc_height = ALIGN_SIZE( mipHeight, clusterHeight );

        // Process a linear surface.
        for ( uint32 proc_y = 0; proc_y < proc_height; proc_y++ )
        {
            uint32 packed_x = 0;

            for ( uint32 proc_x = 0; proc_x < proc_width; proc_x++ )
            {
                for ( uint32 cluster_index = 0; cluster_index < clusterCount; cluster_index++ )
                {
                    cb( proc_x, proc_y, packed_x++, proc_y, cluster_index );
                }
            }
        }
    }
}

inline void DXTIndexListInverseCopy( uint32& dstIndexList, uint32 srcIndexList, uint32 blockPixelWidth, uint32 blockPixelHeight )
{
    dstIndexList = 0;

    for ( uint32 local_y = 0; local_y < blockPixelHeight; local_y++ )
    {
        for ( uint32 local_x = 0; local_x < blockPixelWidth; local_x++ )
        {
            uint32 item = fetchDXTIndexList( srcIndexList, local_x, local_y );

            uint32 real_localDstX = ( blockPixelWidth - local_x - 1 );
            uint32 real_localDstY = ( blockPixelHeight - local_y - 1 );

            putDXTIndexList( dstIndexList, real_localDstX, real_localDstY, item );
        }
    }
}

// Gamecube DXT1 compression struct.
struct gc_dxt1_block
{
    endian::big_endian <rgb565> rgb0;
    endian::big_endian <rgb565> rgb1;

    endian::big_endian <uint32> indexList;
};

inline void ConvertGCMipmapToRasterFormat(
    Interface *engineInterface,
    uint32 mipWidth, uint32 mipHeight, uint32 layerWidth, uint32 layerHeight, void *texelSource, uint32 dataSize,
    eGCNativeTextureFormat internalFormat, eGCPixelFormat palettePixelFormat,
    ePaletteType paletteType, const void *paletteData, uint32 paletteSize,
    eRasterFormat dstRasterFormat, uint32 dstDepth, uint32 dstRowAlignment, eColorOrdering dstColorOrder,
    eCompressionType dstCompressionType,
    uint32& dstSurfWidthOut, uint32& dstSurfHeightOut,
    void*& dstTexelDataOut, uint32& dstDataSizeOut
)
{
    // Check if we are a raw format that can be converted on a per-sample basis.
    if ( isGVRNativeFormatRawSample( internalFormat ) )
    {
        uint32 srcDepth = getGCInternalFormatDepth( internalFormat );

        // Get swizzling properties of the format we want to decode from.
        uint32 clusterWidth = 1;
        uint32 clusterHeight = 1;

        uint32 clusterCount = 1;

        bool isClusterFormat = 
            getGVRNativeFormatClusterDimensions(
                srcDepth,
                clusterWidth, clusterHeight,
                clusterCount
            );

        if ( !isClusterFormat )
        {
            throw RwException( "fatal error: could not determine cluster format for GC native texture layer" );
        }

        // Allocate the destination buffer.
        uint32 dstRowSize = getRasterDataRowSize( layerWidth, dstDepth, dstRowAlignment );

        uint32 dstDataSize = getRasterDataSizeByRowSize( dstRowSize, layerHeight );

        void *dstTexels = engineInterface->PixelAllocate( dstDataSize );

        if ( !dstTexels )
        {
            throw RwException( "failed to allocate texel buffer for Gamecube mipmap decoding" );
        }

        try
        {
            uint32 srcRowSize = getGCRasterDataRowSize( mipWidth, srcDepth );

            if ( internalFormat == GVRFMT_PAL_4BIT || internalFormat == GVRFMT_PAL_8BIT )
            {
                assert( paletteType != PALETTE_NONE );

                // Swizzle/unswizzle the palette items.
                // This is pretty simple, anyway.
                GCProcessRandomAccessTileSurface(
                    mipWidth, mipHeight,
                    clusterWidth, clusterHeight, 1,
                    true,
                    [&]( uint32 dst_pos_x, uint32 dst_pos_y, uint32 src_pos_x, uint32 src_pos_y, uint32 cluster_index )
                {
                    // We do this to not overcomplicate the code.
                    if ( dst_pos_x < layerWidth && dst_pos_y < layerHeight )
                    {
                        void *dstRow = getTexelDataRow( dstTexels, dstRowSize, dst_pos_y );

                        if ( src_pos_x < mipWidth && src_pos_y < mipHeight )
                        {
                            const void *srcRow = getConstTexelDataRow( texelSource, srcRowSize, src_pos_y );

                            copyPaletteItemGeneric(
                                srcRow, dstRow,
                                src_pos_x, srcDepth, paletteType,
                                dst_pos_x, dstDepth, paletteType,
                                paletteSize
                            );
                        }
                        else
                        {
                            // We can just clear this.
                            setpaletteindex( dstRow, dst_pos_x, dstDepth, paletteType, 0 );
                        }
                    }
                });
            }
            else
            {
                // Decide whether we have to swizzle the format.
                bool isFormatSwizzled = isGVRNativeFormatSwizzled( internalFormat );

                // Set up the GC color dispatcher.
                gcColorDispatch srcDispatch(
                    internalFormat, palettePixelFormat,
                    COLOR_RGBA,
                    0, PALETTE_NONE, NULL, 0
                );

                // We need a destination dispatcher.
                colorModelDispatcher <void> dstDispatch(
                    dstRasterFormat, dstColorOrder, dstDepth,
                    NULL, 0, PALETTE_NONE
                );

                // TODO: thoroughly test 32bit textures!
                
                // If we store things in multi-clustered format, we promise the runtime
                // that we can store the same amount of data in a more spread-out way, hence
                // the buffer size if supposed to stay the same (IMPORTANT).
                uint32 clusterGCItemWidth = mipWidth * clusterCount;
                uint32 clusterGCItemHeight = mipHeight;

                GCProcessRandomAccessTileSurface(
                    mipWidth, mipHeight,
                    clusterWidth, clusterHeight, clusterCount,
                    isFormatSwizzled,
                    [&]( uint32 dst_pos_x, uint32 dst_pos_y, uint32 src_pos_x, uint32 src_pos_y, uint32 cluster_index )
                {
                    // We are unswizzling.
                    if ( dst_pos_x < layerWidth && dst_pos_y < layerHeight )
                    {
                        // Just do a naive movement for now.
                        void *dstRow = getTexelDataRow( dstTexels, dstRowSize, dst_pos_y );

                        abstractColorItem colorItem;

                        bool hasColor = false;

                        if ( src_pos_x < clusterGCItemWidth && src_pos_y < clusterGCItemHeight )
                        {
                            const void *srcRow = getConstTexelDataRow( texelSource, srcRowSize, src_pos_y );

                            if ( internalFormat == GVRFMT_RGBA8888 )
                            {
                                // This is actually a more complicated fetch operation.

                                // Get alpha and red components.
                                if ( cluster_index == 0 )
                                {
                                    struct alpha_red
                                    {
                                        unsigned char alpha;
                                        unsigned char red;
                                    };

                                    const PixelFormat::typedcolor <alpha_red> *colorInfo = (const PixelFormat::typedcolor <alpha_red>*)srcRow;

                                    alpha_red ar;

                                    colorInfo->getvalue( src_pos_x, ar );

                                    colorItem.rgbaColor.a = ar.alpha;
                                    colorItem.rgbaColor.r = ar.red;
                                    colorItem.rgbaColor.g = 0;
                                    colorItem.rgbaColor.b = 0;

                                    colorItem.model = COLORMODEL_RGBA;

                                    hasColor = true;
                                }
                                // Get the green and blue components.
                                else if ( cluster_index == 1 )
                                {
                                    struct green_blue
                                    {
                                        unsigned char green;
                                        unsigned char blue;
                                    };

                                    const PixelFormat::typedcolor <green_blue> *colorInfo = (const PixelFormat::typedcolor <green_blue>*)srcRow;

                                    green_blue gb;

                                    colorInfo->getvalue( src_pos_x, gb );

                                    // We want to update the color with green and blue.
                                    dstDispatch.getColor( dstRow, dst_pos_x, colorItem );

                                    assert( colorItem.model == COLORMODEL_RGBA );

                                    colorItem.rgbaColor.g = gb.green;
                                    colorItem.rgbaColor.b = gb.blue;

                                    hasColor = true;
                                }
                                else
                                {
                                    assert( 0 );
                                }
                            }
                            else
                            {
                                srcDispatch.getColor( srcRow, src_pos_x, colorItem );

                                hasColor = true;
                            }
                        }
                    
                        if ( !hasColor )
                        {
                            // If we could not get a valid color, we set it to cleared state.
                            dstDispatch.setClearedColor( colorItem );
                        }

                        // Put the destination color.
                        dstDispatch.setColor( dstRow, dst_pos_x, colorItem );
                    }
                });
            }
        }
        catch( ... )
        {
            engineInterface->PixelFree( dstTexels );

            throw;
        }

        // Return the buffer.
        // Since we return raw texels, we must return layer dimensions.
        dstSurfWidthOut = layerWidth;
        dstSurfHeightOut = layerHeight;
        dstTexelDataOut = dstTexels;
        dstDataSizeOut = dstDataSize;
    }
    else if ( internalFormat == GVRFMT_CMP )
    {
        // Convert the GC DXT1 layer directly into a framework-friendly DXT1 layer!

        // gc_dxt1_block is Gamecube DXT1 struct.
        // dxt1_block is framework-friendly DXT1 struct.

        const uint32 dxt_blockPixelWidth = 4u;
        const uint32 dxt_blockPixelHeight = 4u;

        // Calculate the DXT dimensions that are actually used by us.
        uint32 plainSurfWidth = ALIGN_SIZE( layerWidth, dxt_blockPixelWidth );
        uint32 plainSurfHeight = ALIGN_SIZE( layerHeight, dxt_blockPixelHeight );

        // Allocate a destination buffer.
        uint32 dxtBlocksWidth = ( plainSurfWidth / dxt_blockPixelWidth );
        uint32 dxtBlocksHeight = ( plainSurfHeight / dxt_blockPixelHeight );

        uint32 dxtBlockCount = ( dxtBlocksWidth * dxtBlocksHeight );

        uint32 dxtDataSize = ( dxtBlockCount * sizeof( dxt1_block ) );

        void *dstTexels = engineInterface->PixelAllocate( dxtDataSize );

        if ( !dstTexels )
        {
            throw RwException( "failed to allocate DXT1 destination buffer for GC to framework conversion" );
        }

        try
        {
            // We want to iterate through all allocated GC DXT1 blocks
            // and process the valid ones.
            uint32 gcBlocksWidth = ( mipWidth / dxt_blockPixelWidth );
            uint32 gcBlocksHeight = ( mipHeight / dxt_blockPixelHeight );

            const gc_dxt1_block *gcBlocks = (const gc_dxt1_block*)texelSource;

            dxt1_block *dstBlocks = (dxt1_block*)dstTexels;

            memcodec::permutationUtilities::ProcessTextureLayerTiles(
                gcBlocksWidth, gcBlocksHeight,
                2, 2,
                1,
                [&]( uint32 perm_x_off, uint32 perm_y_off, uint32 packedData_xOff, uint32 packedData_yOff, uint32 cluster_index )
            {
                // Do the usual coordinate dispatch.
                bool unswizzle = true;

                uint32 src_pos_x;
                uint32 src_pos_y;

                uint32 dst_pos_x;
                uint32 dst_pos_y;

                if ( unswizzle )
                {
                    src_pos_x = packedData_xOff;
                    src_pos_y = packedData_yOff;

                    dst_pos_x = perm_x_off;
                    dst_pos_y = perm_y_off;
                }
                else
                {
                    src_pos_x = perm_x_off;
                    src_pos_y = perm_y_off;

                    dst_pos_x = packedData_xOff;
                    dst_pos_y = packedData_yOff;
                }

                if ( dst_pos_x < dxtBlocksWidth && dst_pos_y < dxtBlocksHeight )
                {
                    // Get the destination block ptr.
                    uint32 dstBlockIndex = ( dst_pos_x + dst_pos_y * dxtBlocksWidth );

                    dxt1_block *dstBlock = ( dstBlocks + dstBlockIndex );

                    // Attempt to get the source block.
                    if ( src_pos_x < gcBlocksWidth && src_pos_y < gcBlocksHeight )
                    {
                        uint32 srcBlockIndex = ( src_pos_x + src_pos_y * gcBlocksWidth );

                        const gc_dxt1_block *srcBlock = ( gcBlocks + srcBlockIndex );

                        // Copy things over properly.
                        dstBlock->col0 = srcBlock->rgb0;
                        dstBlock->col1 = srcBlock->rgb1;

                        // The index list needs special handling.
                        uint32 dstIndexList = 0;

                        DXTIndexListInverseCopy( dstIndexList, srcBlock->indexList, dxt_blockPixelWidth, dxt_blockPixelHeight );

                        dstBlock->indexList = dstIndexList;
                    }
                    else
                    {
                        // We sort of just clear our block.
                        dstBlock->col0.val = 0;
                        dstBlock->col1.val = 0;
                        dstBlock->indexList = 0;
                    }
                }
            });

            // We successfully converted!
        }
        catch( ... )
        {
            engineInterface->PixelFree( dstTexels );

            throw;
        }

        // Return the stuff.
        dstSurfWidthOut = plainSurfWidth;
        dstSurfHeightOut = plainSurfHeight;
        dstTexelDataOut = dstTexels;
        dstDataSizeOut = dxtDataSize;
    }
    else
    {
        throw RwException( "unknown GC native texture mipmap internalFormat" );
    }
}

inline void TranscodeIntoNativeGCLayer(
    Interface *engineInterface,
    uint32 mipWidth, uint32 mipHeight, uint32 layerWidth, uint32 layerHeight, const void *srcTexels, uint32 srcDataSize,
    eRasterFormat srcRasterFormat, uint32 srcDepth, uint32 srcRowAlignment, eColorOrdering srcColorOrder,
    ePaletteType srcPaletteType, uint32 srcPaletteSize, eCompressionType srcCompressionType,
    eGCNativeTextureFormat internalFormat, eGCPixelFormat palettePixelFormat,
    uint32& dstSurfWidthOut, uint32& dstSurfHeightOut,
    void*& dstTexelsOut, uint32& dstDataSizeOut
)
{
    // We once again decide logic by the native format.
    // One could decide by source format, but I think that the native format is what we care most
    // about. Also, we are most flexible about the framework formats.
    // So going by native format is really good.
    if ( isGVRNativeFormatRawSample( internalFormat ) )
    {
        assert( srcCompressionType == RWCOMPRESS_NONE );

        // Get properties about the native surface for allocation
        // and processing.
        uint32 nativeDepth = getGCInternalFormatDepth( internalFormat );

        uint32 clusterWidth, clusterHeight;
        uint32 clusterCount;

        bool gotClusterProps =
            getGVRNativeFormatClusterDimensions(
                nativeDepth,
                clusterWidth, clusterHeight,
                clusterCount
           );
           
        if ( !gotClusterProps )
        {
            throw RwException( "failed to get GC native format cluster-props for swizzling" );
        }

        // Now that we have all the properties we need, we can allocate
        // the native surface.
        uint32 gcSurfWidth = ALIGN_SIZE( layerWidth, clusterWidth );
        uint32 gcSurfHeight = ALIGN_SIZE( layerHeight, clusterHeight );

        uint32 gcRowSize = getGCRasterDataRowSize( gcSurfWidth, nativeDepth );

        uint32 gcDataSize = getRasterDataSizeByRowSize( gcRowSize, gcSurfHeight );

        void *gcTexels = engineInterface->PixelAllocate( gcDataSize );

        if ( !gcTexels )
        {
            throw RwException( "failed to allocate native GC texture layer for swizzling" );
        }

        try
        {
            // Process the layer.
            uint32 srcRowSize = getRasterDataRowSize( mipWidth, srcDepth, srcRowAlignment );

            if ( internalFormat == GVRFMT_PAL_4BIT || internalFormat == GVRFMT_PAL_8BIT )
            {
                assert( srcPaletteType != PALETTE_NONE );

                ePaletteType dstPaletteType = getPaletteTypeFromGCNativeFormat( internalFormat, nativeDepth );

                assert( dstPaletteType != PALETTE_NONE );

                // Transform palette indice.
                GCProcessRandomAccessTileSurface(
                    gcSurfWidth, gcSurfHeight,
                    clusterWidth, clusterHeight,
                    1,
                    true,
                    [&]( uint32 src_pos_x, uint32 src_pos_y, uint32 dst_pos_x, uint32 dst_pos_y, uint32 cluster_index )
                {
                    // We are swizzling.
                    if ( dst_pos_x < gcSurfWidth && dst_pos_y < gcSurfHeight )
                    {
                        void *dstRow = getTexelDataRow( gcTexels, gcRowSize, dst_pos_y );

                        if ( src_pos_x < layerWidth && src_pos_y < layerHeight )
                        {
                            // We can copy the palette index.
                            const void *srcRow = getConstTexelDataRow( srcTexels, srcRowSize, src_pos_y );

                            copyPaletteItemGeneric(
                                srcRow, dstRow,
                                src_pos_x, srcDepth, srcPaletteType,
                                dst_pos_x, nativeDepth, dstPaletteType,
                                srcPaletteSize
                            );
                        }
                        else
                        {
                            // Clear if we could not get a source index.
                            setpaletteindex( dstRow, dst_pos_x, nativeDepth, dstPaletteType, 0 );
                        }
                    }
                });
            }
            else
            {
                assert( srcPaletteType == PALETTE_NONE );

                bool isSurfaceSwizzled = isGVRNativeFormatSwizzled( internalFormat );

                // Create the source and destination color model dispatchers.
                colorModelDispatcher <const void> srcDispatch(
                    srcRasterFormat, srcColorOrder, srcDepth,
                    NULL, 0, PALETTE_NONE
                );

                gcColorDispatch dstDispatch(
                    internalFormat, palettePixelFormat,
                    COLOR_RGBA,
                    0, PALETTE_NONE, NULL, 0
                );

                // Set up valid clustered dimensions.
                uint32 clusterGCItemWidth = gcSurfWidth * clusterCount;
                uint32 clusterGCItemHeight = gcSurfHeight;

                // Process the color data.
                GCProcessRandomAccessTileSurface(
                    gcSurfWidth, gcSurfHeight,
                    clusterWidth, clusterHeight,
                    clusterCount,
                    isSurfaceSwizzled,
                    [&]( uint32 src_pos_x, uint32 src_pos_y, uint32 dst_pos_x, uint32 dst_pos_y, uint32 cluster_index )
                {
                    // We are swizzling.
                    if ( dst_pos_x < clusterGCItemWidth && dst_pos_y < clusterGCItemHeight )
                    {
                        // Pretty dangerous to intermix cluster dimensions with regular dimensions.
                        // But it works here because the y dimension is left untouched.
                        void *dstRow = getTexelDataRow( gcTexels, gcRowSize, dst_pos_y );

                        // Get the color to put into the row.
                        abstractColorItem colorItem;
                        bool gotColor = false;

                        if ( src_pos_x < layerWidth && src_pos_y < layerHeight )
                        {
                            const void *srcRow = getConstTexelDataRow( srcTexels, srcRowSize, src_pos_y );

                            srcDispatch.getColor( srcRow, src_pos_x, colorItem );

                            gotColor = true;
                        }

                        // If we could not get a color, we just put a cleared one.
                        if ( !gotColor )
                        {
                            srcDispatch.setClearedColor( colorItem );
                        }

                        // Write properly into the destination.
                        // The important tidbit is that we need to double-cluster the RGBA8888 format.
                        if ( internalFormat == GVRFMT_RGBA8888 )
                        {
                            // TODO: add generic abstractColorItem RGBA fetch.
                            assert( colorItem.model == COLORMODEL_RGBA );

                            if ( cluster_index == 0 )
                            {
                                struct alpha_red
                                {
                                    uint8 alpha, red;
                                };

                                alpha_red ar;
                                ar.alpha = colorItem.rgbaColor.a;
                                ar.red = colorItem.rgbaColor.r;

                                *( (alpha_red*)dstRow + dst_pos_x ) = ar;
                            }
                            else if ( cluster_index == 1 )
                            {
                                struct green_blue
                                {
                                    uint8 green, blue;
                                };

                                green_blue gb;
                                gb.green = colorItem.rgbaColor.g;
                                gb.blue = colorItem.rgbaColor.b;

                                *( (green_blue*)dstRow + dst_pos_x ) = gb;
                            }
                            else
                            {
                                assert( 0 );
                            }
                        }
                        else
                        {
                            // We are a simple color, so no problem.
                            dstDispatch.setColor( dstRow, dst_pos_x, colorItem );
                        }
                    }
                });
            }
        }
        catch( ... )
        {
            engineInterface->PixelFree( gcTexels );

            throw;
        }

        // Give the data to the runtime.
        dstSurfWidthOut = gcSurfWidth;
        dstSurfHeightOut = gcSurfHeight;
        dstTexelsOut = gcTexels;
        dstDataSizeOut = gcDataSize;
    }
    else if ( internalFormat == GVRFMT_CMP )
    {
        // We can only receive DXT1 compressed surfaces anyway.
        assert( srcCompressionType == RWCOMPRESS_DXT1 );

        const uint32 dxt_blockPixelWidth = 4u;
        const uint32 dxt_blockPixelHeight = 4u;

        // Allocate the destination native DXT1 surface.
        uint32 gcSurfWidth = ALIGN_SIZE( layerWidth, dxt_blockPixelWidth * 2 );
        uint32 gcSurfHeight = ALIGN_SIZE( layerHeight, dxt_blockPixelHeight * 2 );

        uint32 gcDXTBlocksWidth = ( gcSurfWidth / dxt_blockPixelWidth );
        uint32 gcDXTBlocksHeight = ( gcSurfHeight / dxt_blockPixelHeight );

        uint32 gcDXTBlockCount = ( gcDXTBlocksWidth * gcDXTBlocksHeight );

        uint32 gcDataSize = ( gcDXTBlockCount * sizeof( gc_dxt1_block ) );

        void *gcTexels = engineInterface->PixelAllocate( gcDataSize );

        if ( !gcTexels )
        {
            throw RwException( "failed to allocate GC native DXT1 surface buffer for swizzling" );
        }

        try
        {
            // Get properties about the source DXT1 layer.
            uint32 srcDXTBlocksWidth = ( mipWidth / dxt_blockPixelWidth );
            uint32 srcDXTBlocksHeight = ( mipHeight / dxt_blockPixelHeight );

            const dxt1_block *srcBlocks = (const dxt1_block*)srcTexels;
            gc_dxt1_block *dstBlocks = (gc_dxt1_block*)gcTexels;

            // Process the framework DXT1 structs into native GC DXT1 structs.
            memcodec::permutationUtilities::ProcessTextureLayerTiles(
                gcDXTBlocksWidth, gcDXTBlocksHeight,
                2, 2,
                1,
                [&]( uint32 src_pos_x, uint32 src_pos_y, uint32 dst_pos_x, uint32 dst_pos_y, uint32 cluster_index )
            {
                // We are swizzling.
                if ( dst_pos_x < gcDXTBlocksWidth && dst_pos_y < gcDXTBlocksHeight )
                {
                    // Get the destination block ptr.
                    uint32 dstBlockIndex = ( dst_pos_x + gcDXTBlocksWidth * dst_pos_y );

                    gc_dxt1_block *dstBlock = ( dstBlocks + dstBlockIndex );

                    if ( src_pos_x < srcDXTBlocksWidth && src_pos_y < srcDXTBlocksHeight )
                    {
                        uint32 srcBlockIndex = ( src_pos_x + srcDXTBlocksWidth * src_pos_y );

                        const dxt1_block *srcBlock = ( (const dxt1_block*)srcTexels + srcBlockIndex );

                        dstBlock->rgb0 = srcBlock->col0;
                        dstBlock->rgb1 = srcBlock->col1;
                        
                        // Indexlist needs special handling.
                        uint32 dstIndexList = 0;

                        DXTIndexListInverseCopy( dstIndexList, srcBlock->indexList, dxt_blockPixelWidth, dxt_blockPixelHeight );

                        dstBlock->indexList = dstIndexList;
                    }
                    else
                    {
                        // Just clear the block.
                        rgb565 clearColor;
                        clearColor.val = 0;

                        dstBlock->rgb0 = clearColor;
                        dstBlock->rgb1 = clearColor;
                        dstBlock->indexList = 0;
                    }
                }
            });
        }
        catch( ... )
        {
            engineInterface->PixelFree( gcTexels );

            throw;
        }

        // Return stuff to the runtime.
        dstSurfWidthOut = gcSurfWidth;
        dstSurfHeightOut = gcSurfHeight;
        dstTexelsOut = gcTexels;
        dstDataSizeOut = gcDataSize;
    }
    else
    {
        throw RwException( "attempt to swizzle unknown GC native texture format" );
    }
}

inline void* GetGCPaletteData(
    Interface *engineInterface,
    const void *paletteData, uint32 paletteSize,
    eGCPixelFormat palettePixelFormat,
    eRasterFormat dstRasterFormat, eColorOrdering dstColorOrder
)
{
    assert( palettePixelFormat != GVRPIX_NO_PALETTE );

    // We have to convert the colors from GC format to framework format.
    // In the future we might think about directly acquiring those colors, but that is for later.
    // After all, it is important to define endian-ness even for colors.

    uint32 dstPalRasterDepth = Bitmap::getRasterFormatDepth( dstRasterFormat );

    uint32 dstPalDataSize = getPaletteDataSize( paletteSize, dstPalRasterDepth );

    void *dstPaletteData = engineInterface->PixelAllocate( dstPalDataSize );

    if ( !dstPaletteData )
    {
        throw RwException( "failed to allocate palette data for GC native texture" );
    }

    try
    {
        // TODO: make a distinction between serialized gc format identifiers and framework
        // ones to have a more secure code-base.

        eGCNativeTextureFormat internalFormat = GVRFMT_RGB5A3;
        eColorOrdering colorOrder = COLOR_RGBA;

        bool couldConvertFormat = getGVRNativeFormatFromPaletteFormat( palettePixelFormat, colorOrder, internalFormat );

        assert( couldConvertFormat == true );

        // Convert the palette entries.
        gcColorDispatch srcDispatch(
            internalFormat, GVRPIX_NO_PALETTE,  // we put raw colors, so no palette.
            colorOrder,
            0, PALETTE_NONE, NULL, 0    // those parameters are only required if you want to read palette indices.
        );

        colorModelDispatcher <void> dstDispatch(
            dstRasterFormat, dstColorOrder, dstPalRasterDepth,
            NULL, 0, PALETTE_NONE
        );
        
        for ( uint32 index = 0; index < paletteSize; index++ )
        {
            abstractColorItem colorItem;

            srcDispatch.getColor( paletteData, index, colorItem );

            dstDispatch.setColor( dstPaletteData, index, colorItem );
        }

        // Success!
    }
    catch( ... )
    {
        engineInterface->PixelFree( dstPaletteData );

        throw;
    }

    // Return the newly generated palette.
    return dstPaletteData;
}

inline void* TranscodeGCPaletteData(
    Interface *engineInterface,
    const void *paletteData, uint32 paletteSize,
    eRasterFormat srcRasterFormat, eColorOrdering srcColorOrder,
    eGCPixelFormat palettePixelFormat
)
{
    assert( palettePixelFormat != GVRPIX_NO_PALETTE );
    assert( paletteSize != 0 );

    uint32 srcPalRasterDepth = Bitmap::getRasterFormatDepth( srcRasterFormat );

    // Prepare the color dispatchers.
    colorModelDispatcher <const void> srcDispatch(
        srcRasterFormat, srcColorOrder, srcPalRasterDepth,
        NULL, 0, PALETTE_NONE
    );

    eGCNativeTextureFormat palInternalFormat;
    eColorOrdering colorOrder;
    
    bool couldGetFormat = getGVRNativeFormatFromPaletteFormat( palettePixelFormat, colorOrder, palInternalFormat );

    if ( !couldGetFormat )
    {
        throw RwException( "failed to get internalFormat from palette format for GC palette encoding" );
    }

    gcColorDispatch dstDispatch(
        palInternalFormat, GVRPIX_NO_PALETTE,   // we put raw colors, so no palette.
        colorOrder,
        0, PALETTE_NONE, NULL, 0
    );
        
    // We want to put our framework color format into the Gamecube color format.
    uint32 dstPalRasterDepth = getGVRPixelFormatDepth( palettePixelFormat );

    uint32 dstPalDataSize = getPaletteDataSize( paletteSize, dstPalRasterDepth );

    void *dstPaletteData = engineInterface->PixelAllocate( dstPalDataSize );

    if ( !dstPaletteData )
    {
        throw RwException( "failed to allocate palette buffer for GC native texture encoding" );
    }

    try
    {
        // Process the palette colors.
        for ( uint32 n = 0; n < paletteSize; n++ )
        {
            abstractColorItem colorItem;

            srcDispatch.getColor( paletteData, n, colorItem );

            dstDispatch.setColor( dstPaletteData, n, colorItem );
        }

        // Yay!
    }
    catch( ... )
    {
        engineInterface->PixelFree( dstPaletteData );

        throw;
    }

    // Return the stuff.
    return dstPaletteData;
}

};

#endif //RWLIB_INCLUDE_NATIVETEX_GAMECUBE

#endif //_GAMECUBE_NATIVE_MIPMAP_TRANSFORMATION_