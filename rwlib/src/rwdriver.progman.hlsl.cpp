#include "StdInc.h"

#include "rwdriver.progman.hxx"

#include <d3dcompiler.h>

namespace rw
{

struct hlslProgram
{
    inline hlslProgram( EngineInterface *engineInterface )
    {
        this->engineInterface = engineInterface;
    }

    inline hlslProgram( const hlslProgram& right )
    {
        this->engineInterface = right.engineInterface;
    }

    inline ~hlslProgram( void )
    {
        // todo...
    }

    EngineInterface *engineInterface;
};

struct hlslDriverProgramManager : public driverNativeProgramManager
{
    void ConstructProgram( EngineInterface *engineInterface, void *progMem, const driverNativeProgramCParams& params ) override
    {
        new (progMem) hlslProgram( engineInterface );
    }

    void CopyConstructProgram( void *progMem, const void *srcProgMem ) override
    {
        new (progMem) hlslProgram( *(const hlslProgram*)srcProgMem );
    }

    void DestructProgram( void *progMem ) override
    {
        ((hlslProgram*)progMem)->~hlslProgram();
    }

    void CompileProgram( void *progMem, eDriverProgType progType, const char *sourceCode, size_t dataSize ) const override
    {
        throw RwException( "not supported yet" );
    }

    inline void Initialize( EngineInterface *engineInterface )
    {
        bool hasRegistered = false;

        // Attempt to initialize the shader compiler.
        this->compilerModule = LoadLibraryA( "D3DCompiler_47.dll" );
        this->procCompileFromFile = NULL;

        if ( HMODULE compModule = this->compilerModule )
        {
            this->procCompileFromFile = (decltype(&D3DCompileFromFile))GetProcAddress( compModule, "D3DCompileFromFile" );
        }

        if ( this->procCompileFromFile )
        {
            hasRegistered = RegisterNativeProgramManager( engineInterface, "HLSL", this, sizeof( hlslProgram ) );
        }

        this->hasRegistered = hasRegistered;
    }

    inline void Shutdown( EngineInterface *engineInterface )
    {
        if ( HMODULE compModule = this->compilerModule )
        {
            FreeLibrary( compModule );

            this->compilerModule = NULL;
        }

        if ( this->hasRegistered )
        {
            UnregisterNativeProgramManager( engineInterface, "HLSL" );
        }
    }

    bool hasRegistered;

    // Need to store information about the shader compiler we will use.
    HMODULE compilerModule;

    decltype( &D3DCompileFromFile ) procCompileFromFile;
};

static PluginDependantStructRegister <hlslDriverProgramManager, RwInterfaceFactory_t> hlslDriverProgramManagerReg;

void registerHLSLDriverProgramManager( void )
{
    hlslDriverProgramManagerReg.RegisterPlugin( engineFactory );
}

};