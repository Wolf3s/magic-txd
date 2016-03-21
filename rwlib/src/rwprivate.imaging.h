// Framework-private global include header file about imaging extennsions.

#ifndef _RENDERWARE_PRIVATE_IMAGING_
#define _RENDERWARE_PRIVATE_IMAGING_

// Special mipmap pushing algorithms.
// This was once a public export, but it seems to dangerous to do that.
bool DeserializeMipmapLayer( Stream *inputStream, rawMipmapLayer& rawLayer );
bool SerializeMipmapLayer( Stream *outputStream, const char *formatDescriptor, const rawMipmapLayer& rawLayer );

#endif //_RENDERWARE_PRIVATE_IMAGING_