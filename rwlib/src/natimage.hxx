// RenderWare native image format support (.DDS, .PVR, .TM2, .GVR, etc).
// Native image formats are extended basic image formats that additionally come with mipmaps and, possibly, rendering properties.
// While native imaging formats are more complicated they map stronger to native textures.

namespace rw
{

// Virtual interface for native image types.
struct nativeImageTypeManager abstract
{
    // Basic object management.
    virtual void ConstructImage( Interface *engineInterface, void *texMem, size_t memSize ) const = 0;
    virtual void CopyConstructImage( Interface *engineInterface, void *texMem, size_t memSize ) const = 0;
    virtual void DestroyImage( Interface *engineInterface, void *texMem, size_t memSize ) const = 0;

    // Pixel movement.
    //todo.
};



};