#include "StdInc.h"

#include "rwdriver.progman.hxx"

namespace rw
{

void driverProgramManager::Initialize( EngineInterface *engineInterface )
{
    LIST_CLEAR( this->programs.root );
    LIST_CLEAR( this->nativeManagers.root );

    // We need a type for GPU programs.
    this->gpuProgTypeInfo = engineInterface->typeSystem.RegisterAbstractType <void*> ( "GPUProgram" );
}

void driverProgramManager::Shutdown( EngineInterface *engineInterface )
{
    // Make sure all programs have deleted themselves.
    assert( LIST_EMPTY( this->programs.root ) == true );
    assert( LIST_EMPTY( this->nativeManagers.root ) == true );

    // Delete the GPU program type.
    if ( RwTypeSystem::typeInfoBase *typeInfo = this->gpuProgTypeInfo )
    {
        engineInterface->typeSystem.DeleteType( typeInfo );

        this->gpuProgTypeInfo = NULL;
    }
}

PluginDependantStructRegister <driverProgramManager, RwInterfaceFactory_t> driverProgramManagerReg;

// Sub modules.
extern void registerHLSLDriverProgramManager( void );

void registerDriverProgramManagerEnv( void )
{
    driverProgramManagerReg.RegisterPlugin( engineFactory );

    // And now for sub-modules.
    registerHLSLDriverProgramManager();
}

struct customNativeProgramTypeInterface : public RwTypeSystem::typeInterface
{
    void Construct( void *mem, EngineInterface *engineInterface, void *construct_params ) const override
    {
        const driverNativeProgramCParams *progParams = (const driverNativeProgramCParams*)construct_params;

        nativeMan->ConstructProgram( engineInterface, mem, *progParams );
    }

    void CopyConstruct( void *mem, const void *srcMem ) const override
    {
        nativeMan->CopyConstructProgram( mem, srcMem );
    }

    void Destruct( void *mem ) const override
    {
        nativeMan->DestructProgram( mem );
    }

    size_t GetTypeSize( EngineInterface *engineInterface, void *construct_params ) const override
    {
        return this->programSize;
    }

    size_t GetTypeSizeByObject( EngineInterface *engineInterface, const void *mem ) const override
    {
        return this->programSize;
    }

    size_t programSize;
    driverNativeProgramManager *nativeMan;
};

// Native manager registration API.
bool RegisterNativeProgramManager( EngineInterface *engineInterface, const char *nativeName, driverNativeProgramManager *manager, size_t programSize )
{
    bool success = false;

    if ( driverProgramManager *progMan = driverProgramManagerReg.GetPluginStruct( engineInterface ) )
    {
        if ( RwTypeSystem::typeInfoBase *gpuProgTypeInfo = progMan->gpuProgTypeInfo )
        {
            // Only register if the native name is not taken already.
            bool isAlreadyTaken = ( progMan->FindNativeManager( nativeName ) != NULL );

            if ( !isAlreadyTaken )
            {
                if ( manager->nativeManData.isRegistered == false )
                {
                    // We need to create a type for our native program.
                    customNativeProgramTypeInterface *nativeTypeInfo = new customNativeProgramTypeInterface();
                    
                    if ( nativeTypeInfo )
                    {
                        // Set things up.
                        nativeTypeInfo->programSize = programSize;
                        nativeTypeInfo->nativeMan = manager;

                        try
                        {
                            // Attempt to create the native program type.
                            RwTypeSystem::typeInfoBase *nativeProgType = engineInterface->typeSystem.RegisterCommonTypeInterface( nativeName, nativeTypeInfo, progMan->gpuProgTypeInfo );

                            if ( nativeProgType )
                            {
                                // Time to put us into position :)
                                manager->nativeManData.nativeType = nativeProgType;
                                LIST_INSERT( progMan->nativeManagers.root, manager->nativeManData.node );

                                manager->nativeManData.isRegistered = true;

                                success = true;
                            }
                        }
                        catch( ... )
                        {
                            delete nativeTypeInfo;

                            throw;
                        }

                        if ( !success )
                        {
                            delete nativeTypeInfo;

                            throw;
                        }
                    }
                }
            }
        }
    }

    return success;
}

bool UnregisterNativeProgramManager( EngineInterface *engineInterface, const char *nativeName )
{
    bool success = false;

    if ( driverProgramManager *progMan = driverProgramManagerReg.GetPluginStruct( engineInterface ) )
    {
        driverNativeProgramManager *nativeMan = progMan->FindNativeManager( nativeName );

        if ( nativeMan )
        {
            // Delete the type associated with this native program manager.
            engineInterface->typeSystem.DeleteType( nativeMan->nativeManData.nativeType );

            // Well, unregister the thing that the runtime requested us to.
            LIST_REMOVE( nativeMan->nativeManData.node );

            nativeMan->nativeManData.isRegistered = false;

            success = true;
        }
    }

    return success;
}

// Program API :)
driverProgramHandle* CompileNativeProgram( Interface *intf, const char *nativeName, eDriverProgType progType, const char *shaderSrc, size_t shaderSize )
{
    EngineInterface *engineInterface = (EngineInterface*)intf;

    driverProgramHandle *handle = NULL;

    if ( driverProgramManager *progMan = driverProgramManagerReg.GetPluginStruct( engineInterface ) )
    {
        // Find the native compiler for this shader code.
    }

    return handle;
}

};