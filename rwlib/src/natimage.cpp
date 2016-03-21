#include "StdInc.h"

#include "natimage.hxx"

namespace rw
{

NativeImage::NativeImage( Interface *engineInterface )
{
    this->engineInterface = engineInterface;
    this->isPixelDataNewlyAllocated = false;
    this->pixelOwner = NULL;

    this->nativeData = NULL;
}

NativeImage::NativeImage( const NativeImage& right )
{
    Interface *engineInterface = right.engineInterface;

    this->engineInterface = engineInterface;

    // Since we just take the pixel data pointers without copying, we increment the raster ref-count.
    this->isPixelDataNewlyAllocated = right.isPixelDataNewlyAllocated;

    this->pixelOwner = AcquireRaster( right.pixelOwner );

    // TODO: clone native data.
}

NativeImage::~NativeImage( void )
{
    // Release pixel data if required.
    bool isPixelDataNewlyAllocated = this->isPixelDataNewlyAllocated;

    // TODO: release native data appropriately.

    // Release the link to the raster.
    if ( Raster *pixelOwner = this->pixelOwner )
    {
        DeleteRaster( pixelOwner );

        this->pixelOwner = NULL;
    }
}

//

};