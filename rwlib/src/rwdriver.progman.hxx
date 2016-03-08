// RenderWare driver program manager.
// Programs are code that run on the GPU to perform a certain action of the pipeline.
// There are multiple program types supported: vertex, fragment, hull

#ifndef _RENDERWARE_GPU_PROGRAM_MANAGER_
#define _RENDERWARE_GPU_PROGRAM_MANAGER_

#include "pluginutil.hxx"

namespace rw
{

enum eDriverProgType
{
    DRIVER_PROG_VERTEX,
    DRIVER_PROG_FRAGMENT,
    DRIVER_PROG_HULL
};

struct driverProgramHandle
{
    inline driverProgramHandle( EngineInterface *engineInterface, eDriverProgType progType, void *nativeHandle )
    {
        this->engineInterface = engineInterface;
        this->programType = progType;
        this->nativeHandle = nativeHandle;
        
        // Rememeber to add ourselves to the list.
    }

    EngineInterface *engineInterface;

    eDriverProgType programType;

    void *nativeHandle;

    RwListEntry <driverProgramHandle> node;
};

struct driverNativeProgramCParams
{
    eDriverProgType progType;
};

struct driverNativeProgramManager abstract
{
    inline driverNativeProgramManager( void )
    {
        this->nativeManData.isRegistered = false;
        this->nativeManData.nativeType = NULL;
    }

    inline ~driverNativeProgramManager( void )
    {
        assert( this->nativeManData.isRegistered == false );
    }

    virtual void ConstructProgram( EngineInterface *engineInterface, void *progMem, const driverNativeProgramCParams& params ) = 0;
    virtual void CopyConstructProgram( void *progMem, const void *srcProgMem ) = 0;
    virtual void DestructProgram( void *progMem ) = 0;

    virtual void CompileProgram( void *progMem, eDriverProgType progType, const char *sourceCode, size_t bufSize ) const = 0;

    // DO NOT ACCESS THOSE FIELD YOURSELF.
    // These fields are subject to internal change.
    struct
    {
        bool isRegistered;

        RwTypeSystem::typeInfoBase *nativeType;
        RwListEntry <driverNativeProgramManager> node;
    } nativeManData;
};

struct driverProgramManager
{
    void Initialize( EngineInterface *engineInterface );
    void Shutdown( EngineInterface *engineInterface );

    inline driverNativeProgramManager* FindNativeManager( const char *nativeName ) const
    {
        LIST_FOREACH_BEGIN( driverNativeProgramManager, this->nativeManagers.root, nativeManData.node )

            if ( strcmp( item->nativeManData.nativeType->name, nativeName ) == 0 )
            {
                return item;
            }

        LIST_FOREACH_END

        return NULL;
    }

    // We want to keep track of our programs.
    RwList <driverProgramHandle> programs;

    // Native managers are registered in the driverProgramManager.
    RwList <driverNativeProgramManager> nativeManagers;

    // GPUProgram type.
    RwTypeSystem::typeInfoBase *gpuProgTypeInfo;
};

extern PluginDependantStructRegister <driverProgramManager, RwInterfaceFactory_t> driverProgramManagerReg;

// Native program manager registration.
bool RegisterNativeProgramManager( EngineInterface *engineInterface, const char *nativeName, driverNativeProgramManager *manager, size_t programSize );
bool UnregisterNativeProgramManager( EngineInterface *engineInterface, const char *nativeName );

driverProgramHandle* CompileNativeProgram( Interface *engineInterface, const char *nativeName, eDriverProgType progType, const char *shaderSource, size_t shaderSize );

void DeleteDriverProgram( driverProgramHandle *handle );

};

#endif //_RENDERWARE_GPU_PROGRAM_MANAGER_