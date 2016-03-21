// RenderWare private global include file containing misc utilities.
// Those should eventually be included in more specialized headers, provided they are worth it.

#ifndef _RENDERWARE_PRIVATE_INTERNAL_UTILITIES_
#define _RENDERWARE_PRIVATE_INTERNAL_UTILITIES_

template <typename keyType, typename valueType>
struct uniqueMap_t
{
    struct pair
    {
        keyType key;
        valueType value;
    };

    typedef std::vector <pair> list_t;

    list_t pairs;

    inline valueType& operator [] ( const keyType& checkKey )
    {
        pair *targetIter = NULL;

        for ( list_t::iterator iter = pairs.begin(); iter != pairs.end(); iter++ )
        {
            if ( iter->key == checkKey )
            {
                targetIter = &*iter;
            }
        }

        if ( targetIter == NULL )
        {
            pair pair;
            pair.key = checkKey;

            pairs.push_back( pair );

            targetIter = &pairs.back();
        }

        return targetIter->value;
    }
};

AINLINE void setDataByDepth( void *dstArrayData, rw::uint32 depth, rw::uint32 targetArrayIndex, rw::uint32 value )
{
    using namespace rw;

    // Perform the texel set.
    if (depth == 4)
    {
        // Get the src item.
        PixelFormat::palette4bit::trav_t travItem = (PixelFormat::palette4bit::trav_t)value;

        // Put the dst item.
        PixelFormat::palette4bit *dstData = (PixelFormat::palette4bit*)dstArrayData;

        dstData->setvalue(targetArrayIndex, travItem);
    }
    else if (depth == 8)
    {
        // Get the src item.
        PixelFormat::palette8bit::trav_t travItem = (PixelFormat::palette8bit::trav_t)value;

        // Put the dst item.
        PixelFormat::palette8bit *dstData = (PixelFormat::palette8bit*)dstArrayData;

        dstData->setvalue(targetArrayIndex, travItem);
    }
    else if (depth == 16)
    {
        typedef PixelFormat::typedcolor <uint16> theColor;

        // Get the src item.
        theColor::trav_t travItem = (theColor::trav_t)value;

        // Put the dst item.
        theColor *dstData = (theColor*)dstArrayData;

        dstData->setvalue(targetArrayIndex, travItem);
    }
    else if (depth == 24)
    {
        struct colorStruct
        {
            inline colorStruct( uint32 val )
            {
                x = ( val & 0xFF );
                y = ( val & 0xFF00 ) << 8;
                z = ( val & 0xFF0000 ) << 16;
            }

            uint8 x, y, z;
        };

        typedef PixelFormat::typedcolor <colorStruct> theColor;

        // Get the src item.
        theColor::trav_t travItem = (theColor::trav_t)value;

        // Put the dst item.
        theColor *dstData = (theColor*)dstArrayData;

        dstData->setvalue(targetArrayIndex, travItem);
    }
    else if (depth == 32)
    {
        typedef PixelFormat::typedcolor <uint32> theColor;

        // Get the src item.
        theColor::trav_t travItem = (theColor::trav_t)value;

        // Put the dst item.
        theColor *dstData = (theColor*)dstArrayData;

        dstData->setvalue(targetArrayIndex, travItem);
    }
    else
    {
        throw RwException( "unknown bit depth for setting" );
    }
}

AINLINE void moveDataByDepth( void *dstArrayData, const void *srcArrayData, rw::uint32 depth, rw::uint32 targetArrayIndex, rw::uint32 srcArrayIndex )
{
    using namespace rw;

    // Perform the texel movement.
    if (depth == 4)
    {
        PixelFormat::palette4bit *srcData = (PixelFormat::palette4bit*)srcArrayData;

        // Get the src item.
        PixelFormat::palette4bit::trav_t travItem;

        srcData->getvalue(srcArrayIndex, travItem);

        // Put the dst item.
        PixelFormat::palette4bit *dstData = (PixelFormat::palette4bit*)dstArrayData;

        dstData->setvalue(targetArrayIndex, travItem);
    }
    else if (depth == 8)
    {
        // Get the src item.
        PixelFormat::palette8bit *srcData = (PixelFormat::palette8bit*)srcArrayData;

        PixelFormat::palette8bit::trav_t travItem;

        srcData->getvalue(srcArrayIndex, travItem);

        // Put the dst item.
        PixelFormat::palette8bit *dstData = (PixelFormat::palette8bit*)dstArrayData;

        dstData->setvalue(targetArrayIndex, travItem);
    }
    else if (depth == 16)
    {
        typedef PixelFormat::typedcolor <uint16> theColor;

        // Get the src item.
        theColor *srcData = (theColor*)srcArrayData;

        theColor::trav_t travItem;

        srcData->getvalue(srcArrayIndex, travItem);

        // Put the dst item.
        theColor *dstData = (theColor*)dstArrayData;

        dstData->setvalue(targetArrayIndex, travItem);
    }
    else if (depth == 24)
    {
        struct colorStruct
        {
            uint8 x, y, z;
        };

        typedef PixelFormat::typedcolor <colorStruct> theColor;

        // Get the src item.
        theColor *srcData = (theColor*)srcArrayData;

        theColor::trav_t travItem;

        srcData->getvalue(srcArrayIndex, travItem);

        // Put the dst item.
        theColor *dstData = (theColor*)dstArrayData;

        dstData->setvalue(targetArrayIndex, travItem);
    }
    else if (depth == 32)
    {
        typedef PixelFormat::typedcolor <uint32> theColor;

        // Get the src item.
        theColor *srcData = (theColor*)srcArrayData;

        theColor::trav_t travItem;

        srcData->getvalue(srcArrayIndex, travItem);

        // Put the dst item.
        theColor *dstData = (theColor*)dstArrayData;

        dstData->setvalue(targetArrayIndex, travItem);
    }
    else if (depth == 64)
    {
        typedef PixelFormat::typedcolor <uint64> theColor;

        // Get the src item.
        theColor *srcData = (theColor*)srcArrayData;

        theColor::trav_t travItem;

        srcData->getvalue(srcArrayIndex, travItem);

        // Put the dst item.
        theColor *dstData = (theColor*)dstArrayData;

        dstData->setvalue(targetArrayIndex, travItem);
    }
    else if (depth == 128)
    {
        typedef PixelFormat::typedcolor <__m128> theColor;

        // Get the src item.
        theColor *srcData = (theColor*)srcArrayData;

        theColor::trav_t travItem;

        srcData->getvalue(srcArrayIndex, travItem);

        // Put the dst item.
        theColor *dstData = (theColor*)dstArrayData;

        dstData->setvalue(targetArrayIndex, travItem);
    }
    else
    {
        throw RwException( "unknown bit depth for movement" );
    }
}

#endif //_RENDERWARE_PRIVATE_INTERNAL_UTILITIES_