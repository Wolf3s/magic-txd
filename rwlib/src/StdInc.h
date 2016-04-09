#ifndef AINLINE
#ifdef _MSC_VER
#define AINLINE __forceinline
#elif __linux__
#define AINLINE __attribute__((always_inline))
#else
#define AINLINE
#endif
#endif

#include <MemoryUtils.h>

#define RWCORE
#include "renderware.h"

// Include the RenderWare configuration file.
// This one should be private to the rwtools project, hence we reject including it in "renderware.h"
#include "../rwconf.h"

#include <DynamicTypeSystem.h>

#ifdef DEBUG
	#define READ_HEADER(x)\
	header.read(rw);\
	if (header.type != (x)) {\
		cerr << filename << " ";\
		ChunkNotFound((x), rw.tellg());\
	}
#else
	#define READ_HEADER(x)\
	header.read(rw);
#endif

namespace rw
{

// Type system declaration for type abstraction.
// This is where atomics, frames, geometries register to.
struct EngineInterface : public Interface
{
    friend struct Interface;

    EngineInterface( void );
    ~EngineInterface( void );

    // DO NOT ACCESS THE FIELDS DIRECTLY.
    // THEY MUST BE ACCESSED UNDER MUTUAL EXCLUSION/CONTEXT LOCKING.

    // General type system.
    RwMemoryAllocator memAlloc;

    struct typeSystemLockProvider
    {
        typedef rw::rwlock rwlock;

        inline rwlock* CreateLock( void )
        {
            return CreateReadWriteLock( engineInterface );
        }

        inline void CloseLock( rwlock *theLock )
        {
            CloseReadWriteLock( engineInterface, theLock );
        }

        inline void LockEnterRead( rwlock *theLock ) const
        {
            theLock->enter_read();
        }

        inline void LockLeaveRead( rwlock *theLock ) const
        {
            theLock->leave_read();
        }

        inline void LockEnterWrite( rwlock *theLock ) const
        {
            theLock->enter_write();
        }

        inline void LockLeaveWrite( rwlock *theLock ) const
        {
            theLock->leave_write();
        }

        EngineInterface *engineInterface;
    };

    typedef DynamicTypeSystem <RwMemoryAllocator, EngineInterface, typeSystemLockProvider> RwTypeSystem;

    RwTypeSystem typeSystem;

    // Types that should be registered by all RenderWare implementations.
    // These can be NULL, tho.
    RwTypeSystem::typeInfoBase *streamTypeInfo;
    RwTypeSystem::typeInfoBase *rwobjTypeInfo;
    RwTypeSystem::typeInfoBase *textureTypeInfo;

    // Information about the running application.
    std::string applicationName;
    std::string applicationVersion;
    std::string applicationDescription;
};

typedef EngineInterface::RwTypeSystem RwTypeSystem;

// Use this function if you need a string that describes the currently running RenderWare environment.
// It uses the application variables of EngineInterface.
std::string GetRunningSoftwareInformation( EngineInterface *engineInterface, bool outputShort = false );

// Factory for global RenderWare interfaces.
typedef StaticPluginClassFactory <EngineInterface> RwInterfaceFactory_t;

typedef RwInterfaceFactory_t::pluginOffset_t RwInterfacePluginOffset_t;

extern RwInterfaceFactory_t engineFactory;

#include "rwprivate.bmp.h"
#include "rwprivate.txd.h"
#include "rwprivate.imaging.h"
#include "rwprivate.warnings.h"

}

#pragma warning(push)
#pragma warning(disable: 4290)

// Global allocator
extern void* operator new( size_t memSize ) throw(std::bad_alloc);
extern void* operator new( size_t memSize, const std::nothrow_t nothrow ) throw();
extern void* operator new[]( size_t memSize ) throw(std::bad_alloc);
extern void* operator new[]( size_t memSize, const std::nothrow_t nothrow ) throw();
extern void operator delete( void *ptr ) throw();
extern void operator delete[]( void *ptr ) throw();

#pragma warning(pop)

#pragma warning(disable: 4996)

#include "rwprivate.utils.h"